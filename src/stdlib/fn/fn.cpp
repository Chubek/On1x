#include "api/api_common.hpp"
#include "core/hashing.hpp"
#include "core/list.hpp"
#include "core/optional.hpp"
#include "core/table.hpp"
#include "core/value.hpp"
#include "gc/alloc.hpp"
#include "gc/roots.hpp"
#include "on1x/on1x_capability.h"
#include "on1x/on1x_module.h"
#include "runtime/closure.hpp"
#include "runtime/state.hpp"
#include "stdlib/args.hpp"
#include "stdlib/fn/fn.hpp"
#include "vm/interpreter.hpp"

#include <cstdint>
#include <cstring>

namespace on1x::stdlib {
namespace {

static On1x_Status bad(On1x_State* s, const char* m) noexcept {
    return push_api_error(s, m);
}

// ---- helpers ----

// Call a function with arguments stored on the stack at position (state->top - argc)
// Helper: call an arbitrary On1x function with explicit argument array
static bool call_fn(
    On1x_State* s,
    FunctionObject* fn,
    const Value* args,
    std::size_t arg_count,
    Value& result) noexcept {
    const char* error = nullptr;
    return vm::invoke_function(s, fn, args, arg_count, result, error);
}

// ---- Apply ----

On1x_Status fn_apply(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 2, "Fn.Apply")) return ON1X_ERR;
    Value fv, lv;
    if (!read_argument(s, 1, fv) || !read_argument(s, 2, lv)) return bad(s, "Fn.Apply");
    if (fv.kind() != Value::Kind::Function) return bad(s, "Fn.Apply expects an Fn");
    if (lv.kind() != Value::Kind::List) return bad(s, "Fn.Apply expects a List as second arg");
    auto* func = static_cast<FunctionObject*>(fv.as_object());
    const auto* list = as_list_const(lv);
    if (!func || !list) return bad(s, "Fn.Apply");
    GcRoot func_root(func);

    const std::size_t arg_count = list->length;
    // Extract arguments from the list
    Value* args = nullptr;
    if (arg_count > 0) {
        args = gc_alloc_array<Value>(&s->gc, arg_count);
        if (!args) return bad(s, "Fn.Apply: allocation failed");
        for (std::size_t i = 0; i < arg_count; ++i) {
            list_get(list, i, args[i]);
        }
    }
    Value result;
    if (!call_fn(s, func, args, arg_count, result)) {
        return push_api_error(s, "Fn.Apply: call failed");
    }
    return stack_push(s, result) ? ON1X_OK : ON1X_ERR;
}

// ---- Arity ----

On1x_Status fn_arity(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 1, "Fn.Arity")) return ON1X_ERR;
    Value v;
    if (!read_argument(s, 1, v) || v.kind() != Value::Kind::Function)
        return bad(s, "Fn.Arity expects an Fn");
    auto* func = static_cast<FunctionObject*>(v.as_object());
    if (!func) return bad(s, "Fn.Arity");
    if (func->native) {
        // Native functions don't expose arity in 1.0.0
        return stack_push(s, Value::integer(&s->gc, -1)) ? ON1X_OK : ON1X_ERR;
    }
    if (!func->chunk) return bad(s, "Fn.Arity: invalid function");
    std::int64_t arity = static_cast<std::int64_t>(func->chunk->parameter_count());
    return stack_push(s, Value::integer(&s->gc, arity)) ? ON1X_OK : ON1X_ERR;
}

// ---- IsVariadic ----

On1x_Status fn_is_variadic(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 1, "Fn.IsVariadic")) return ON1X_ERR;
    Value v;
    if (!read_argument(s, 1, v) || v.kind() != Value::Kind::Function)
        return bad(s, "Fn.IsVariadic expects an Fn");
    auto* func = static_cast<FunctionObject*>(v.as_object());
    if (!func) return bad(s, "Fn.IsVariadic");
    if (func->native) {
        return stack_push(s, Value::boolean(true)) ? ON1X_OK : ON1X_ERR;
    }
    if (!func->chunk) return bad(s, "Fn.IsVariadic: invalid function");
    return stack_push(s, Value::boolean(func->chunk->variadic())) ? ON1X_OK : ON1X_ERR;
}

// ---- Identity: just returns its argument ----

On1x_Status fn_identity(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 1, "Fn.Identity")) return ON1X_ERR;
    Value v;
    if (!read_argument(s, 1, v)) return bad(s, "Fn.Identity expects one value");
    return stack_push(s, v) ? ON1X_OK : ON1X_ERR;
}

// ---- Const: returns a function that ignores its args and returns the captured value ----

On1x_Status fn_const_call(On1x_State* s, int /*argc*/) noexcept {
    // Captures: [0] = the constant value
    if (!s || !s->invocation_captures || s->invocation_capture_count < 1) {
        return push_api_error(s, "Fn.Const: missing capture");
    }
    return stack_push(s, s->invocation_captures[0]) ? ON1X_OK : ON1X_ERR;
}

On1x_Status fn_const(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 1, "Fn.Const")) return ON1X_ERR;
    Value v;
    if (!read_argument(s, 1, v)) return bad(s, "Fn.Const expects one value");
    auto* native_fn = runtime::new_native_function(&s->gc, fn_const_call);
    if (!native_fn) return bad(s, "Fn.Const: allocation failed");
    GcRoot root(native_fn);
    Value* caps = gc_alloc_array<Value>(&s->gc, 1);
    if (!caps) return bad(s, "Fn.Const: allocation failed");
    caps[0] = v;
    native_fn->captures = caps;
    native_fn->capture_count = 1;
    s->persistent_roots.push(native_fn);
    return stack_push(s, value_from_object(native_fn)) ? ON1X_OK : ON1X_ERR;
}

// ---- Compose: f(g(x)) ----

On1x_Status fn_compose_call(On1x_State* s, int argc) noexcept {
    // Captures: [0] = f, [1] = g
    if (!s || !s->invocation_captures || s->invocation_capture_count < 2) {
        return push_api_error(s, "Fn.Compose: missing captures");
    }
    Value fv = s->invocation_captures[0];
    Value gv = s->invocation_captures[1];
    if (fv.kind() != Value::Kind::Function || gv.kind() != Value::Kind::Function) {
        return push_api_error(s, "Fn.Compose: corrupted captures");
    }
    auto* g = static_cast<FunctionObject*>(gv.as_object());
    auto* f = static_cast<FunctionObject*>(fv.as_object());

    // Read the arg(s) passed to the composed function
    // They are on the stack at positions 1..argc
    Value* g_args = nullptr;
    if (argc > 0) {
        g_args = gc_alloc_array<Value>(&s->gc, static_cast<std::size_t>(argc));
        if (!g_args) return bad(s, "Fn.Compose");
        for (int i = 0; i < argc; ++i) {
            read_argument(s, i + 1, g_args[i]);
        }
    }
    Value g_result;
    if (!call_fn(s, g, g_args, static_cast<std::size_t>(argc), g_result)) {
        return push_api_error(s, "Fn.Compose: g call failed");
    }
    // Now call f(g_result)
    Value f_args[1] = {g_result};
    Value f_result;
    if (!call_fn(s, f, f_args, 1, f_result)) {
        return push_api_error(s, "Fn.Compose: f call failed");
    }
    return stack_push(s, f_result) ? ON1X_OK : ON1X_ERR;
}

On1x_Status fn_compose(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 2, "Fn.Compose")) return ON1X_ERR;
    Value fv, gv;
    if (!read_argument(s, 1, fv) || !read_argument(s, 2, gv))
        return bad(s, "Fn.Compose");
    if (fv.kind() != Value::Kind::Function || gv.kind() != Value::Kind::Function)
        return bad(s, "Fn.Compose expects two Fns");
    auto* native_fn = runtime::new_native_function(&s->gc, fn_compose_call);
    if (!native_fn) return bad(s, "Fn.Compose: allocation failed");
    GcRoot root(native_fn);
    Value* caps = gc_alloc_array<Value>(&s->gc, 2);
    if (!caps) return bad(s, "Fn.Compose: allocation failed");
    caps[0] = fv;
    caps[1] = gv;
    native_fn->captures = caps;
    native_fn->capture_count = 2;
    s->persistent_roots.push(native_fn);
    return stack_push(s, value_from_object(native_fn)) ? ON1X_OK : ON1X_ERR;
}

// ---- Pipe: chains a list of functions ----

On1x_Status fn_pipe_call(On1x_State* s, int argc) noexcept {
    // Captures: [0] = list of functions
    if (!s || !s->invocation_captures || s->invocation_capture_count < 1) {
        return push_api_error(s, "Fn.Pipe: missing captures");
    }
    Value lv = s->invocation_captures[0];
    if (lv.kind() != Value::Kind::List) {
        return push_api_error(s, "Fn.Pipe: corrupted captures");
    }
    const auto* list = as_list_const(lv);

    // Get the initial argument
    if (argc != 1) return bad(s, "Fn.Pipe: piped function expects 1 argument");
    Value current;
    if (!read_argument(s, 1, current)) return bad(s, "Fn.Pipe");

    for (std::size_t i = 0; i < list->length; ++i) {
        Value fn_v;
        list_get(list, i, fn_v);
        if (fn_v.kind() != Value::Kind::Function) {
            return push_api_error(s, "Fn.Pipe: non-function in pipe");
        }
        auto* fn = static_cast<FunctionObject*>(fn_v.as_object());
        Value args[1] = {current};
        if (!call_fn(s, fn, args, 1, current)) {
            return push_api_error(s, "Fn.Pipe: function call failed");
        }
    }
    return stack_push(s, current) ? ON1X_OK : ON1X_ERR;
}

On1x_Status fn_pipe(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 1, "Fn.Pipe")) return ON1X_ERR;
    Value lv;
    if (!read_argument(s, 1, lv) || lv.kind() != Value::Kind::List)
        return bad(s, "Fn.Pipe expects a List of Fns");
    const auto* list = as_list_const(lv);
    if (!list || list->length == 0)
        return stack_push(s, make_none(&s->gc, s->reserved)) ? ON1X_OK : ON1X_ERR;
    for (std::size_t i = 0; i < list->length; ++i) {
        Value item;
        list_get(list, i, item);
        if (item.kind() != Value::Kind::Function)
            return bad(s, "Fn.Pipe expects all elements to be Fns");
    }
    auto* native_fn = runtime::new_native_function(&s->gc, fn_pipe_call);
    if (!native_fn) return bad(s, "Fn.Pipe: allocation failed");
    GcRoot root(native_fn);
    Value* caps = gc_alloc_array<Value>(&s->gc, 1);
    if (!caps) return bad(s, "Fn.Pipe: allocation failed");
    caps[0] = lv;
    native_fn->captures = caps;
    native_fn->capture_count = 1;
    s->persistent_roots.push(native_fn);
    return stack_push(s, value_from_object(native_fn)) ? ON1X_OK : ON1X_ERR;
}

// ---- Partial: binds some arguments ----

On1x_Status fn_partial_call(On1x_State* s, int argc) noexcept {
    // Captures: [0] = f, [1] = bound args list
    if (!s || !s->invocation_captures || s->invocation_capture_count < 2) {
        return push_api_error(s, "Fn.Partial: missing captures");
    }
    Value fv = s->invocation_captures[0];
    Value bound_v = s->invocation_captures[1];
    if (fv.kind() != Value::Kind::Function || bound_v.kind() != Value::Kind::List) {
        return push_api_error(s, "Fn.Partial: corrupted captures");
    }
    auto* fn = static_cast<FunctionObject*>(fv.as_object());
    const auto* bound = as_list_const(bound_v);

    // Combine bound args with new args: bound first, then new args
    const std::size_t total_count = bound->length + static_cast<std::size_t>(argc);
    Value* all_args = gc_alloc_array<Value>(&s->gc, total_count);
    if (!all_args) return bad(s, "Fn.Partial");
    for (std::size_t i = 0; i < bound->length; ++i) {
        list_get(bound, i, all_args[i]);
    }
    for (int i = 0; i < argc; ++i) {
        read_argument(s, i + 1, all_args[bound->length + static_cast<std::size_t>(i)]);
    }
    Value result;
    if (!call_fn(s, fn, all_args, total_count, result)) {
        return push_api_error(s, "Fn.Partial: call failed");
    }
    return stack_push(s, result) ? ON1X_OK : ON1X_ERR;
}

On1x_Status fn_partial(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 2, "Fn.Partial")) return ON1X_ERR;
    Value fv, bv;
    if (!read_argument(s, 1, fv) || !read_argument(s, 2, bv))
        return bad(s, "Fn.Partial");
    if (fv.kind() != Value::Kind::Function || bv.kind() != Value::Kind::List)
        return bad(s, "Fn.Partial expects an Fn and a List");
    auto* native_fn = runtime::new_native_function(&s->gc, fn_partial_call);
    if (!native_fn) return bad(s, "Fn.Partial: allocation failed");
    GcRoot root(native_fn);
    Value* caps = gc_alloc_array<Value>(&s->gc, 2);
    if (!caps) return bad(s, "Fn.Partial: allocation failed");
    caps[0] = fv;
    caps[1] = bv;
    native_fn->captures = caps;
    native_fn->capture_count = 2;
    s->persistent_roots.push(native_fn);
    return stack_push(s, value_from_object(native_fn)) ? ON1X_OK : ON1X_ERR;
}

// ---- Flip: swaps first two arguments ----

On1x_Status fn_flip_call(On1x_State* s, int argc) noexcept {
    // Captures: [0] = original function
    if (!s || !s->invocation_captures || s->invocation_capture_count < 1) {
        return push_api_error(s, "Fn.Flip: missing captures");
    }
    Value fv = s->invocation_captures[0];
    if (fv.kind() != Value::Kind::Function) {
        return push_api_error(s, "Fn.Flip: corrupted captures");
    }
    auto* fn = static_cast<FunctionObject*>(fv.as_object());

    // Flip first two arguments
    Value* args = gc_alloc_array<Value>(&s->gc, static_cast<std::size_t>(argc));
    if (!args) return bad(s, "Fn.Flip");
    for (int i = 0; i < argc; ++i) {
        read_argument(s, i + 1, args[i]);
    }
    if (argc >= 2) {
        Value tmp = args[0];
        args[0] = args[1];
        args[1] = tmp;
    }
    Value result;
    if (!call_fn(s, fn, args, static_cast<std::size_t>(argc), result)) {
        return push_api_error(s, "Fn.Flip: call failed");
    }
    return stack_push(s, result) ? ON1X_OK : ON1X_ERR;
}

On1x_Status fn_flip(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 1, "Fn.Flip")) return ON1X_ERR;
    Value fv;
    if (!read_argument(s, 1, fv)) return bad(s, "Fn.Flip");
    if (fv.kind() != Value::Kind::Function)
        return bad(s, "Fn.Flip expects an Fn");
    auto* native_fn = runtime::new_native_function(&s->gc, fn_flip_call);
    if (!native_fn) return bad(s, "Fn.Flip: allocation failed");
    GcRoot root(native_fn);
    Value* caps = gc_alloc_array<Value>(&s->gc, 1);
    if (!caps) return bad(s, "Fn.Flip: allocation failed");
    caps[0] = fv;
    native_fn->captures = caps;
    native_fn->capture_count = 1;
    s->persistent_roots.push(native_fn);
    return stack_push(s, value_from_object(native_fn)) ? ON1X_OK : ON1X_ERR;
}

// ---- Memo: caches results ----

On1x_Status fn_memo_call(On1x_State* s, int argc) noexcept {
    // Captures: [0] = f, [1] = cache table
    if (!s || !s->invocation_captures || s->invocation_capture_count < 2) {
        return push_api_error(s, "Fn.Memo: missing captures");
    }
    Value fv = s->invocation_captures[0];
    Value cache_v = s->invocation_captures[1];
    if (fv.kind() != Value::Kind::Function || cache_v.kind() != Value::Kind::Table) {
        return push_api_error(s, "Fn.Memo: corrupted captures");
    }
    auto* fn = static_cast<FunctionObject*>(fv.as_object());
    auto* cache = as_table(cache_v);

    // Build a key from the arguments
    // For single arg, use hash; for multiple args, create a temp list and hash it
    std::uint64_t key_hash = 0;
    if (argc == 0) {
        key_hash = 0;
    } else if (argc == 1) {
        Value arg;
        read_argument(s, 1, arg);
        if (!is_hashable(arg)) return push_api_error(s, "Fn.Memo: unhashable argument");
        key_hash = value_hash(arg);
    } else {
        // Create a list of args for hashing
        auto* arg_list = new_list(&s->gc, static_cast<std::size_t>(argc));
        if (!arg_list) return bad(s, "Fn.Memo");
        GcRoot list_root(arg_list);
        for (int i = 0; i < argc; ++i) {
            Value arg;
            read_argument(s, i + 1, arg);
            if (!is_hashable(arg)) return push_api_error(s, "Fn.Memo: unhashable argument");
            list_push(&s->gc, arg_list, arg);
        }
        key_hash = value_hash(value_from_object(arg_list));
    }

    Value cache_key = Value::integer(&s->gc, static_cast<std::int64_t>(key_hash));

    // Check cache
    Value cached;
    if (table_get(cache, cache_key, cached)) {
        return stack_push(s, cached) ? ON1X_OK : ON1X_ERR;
    }

    // Compute and cache
    Value* args = gc_alloc_array<Value>(&s->gc, static_cast<std::size_t>(argc));
    if (!args) return bad(s, "Fn.Memo");
    for (int i = 0; i < argc; ++i) {
        read_argument(s, i + 1, args[i]);
    }
    Value result;
    if (!call_fn(s, fn, args, static_cast<std::size_t>(argc), result)) {
        return push_api_error(s, "Fn.Memo: call failed");
    }
    GcRoot result_root(result.is_object() ? result.as_object() : nullptr);
    table_set(&s->gc, cache, cache_key, result);
    return stack_push(s, result) ? ON1X_OK : ON1X_ERR;
}

On1x_Status fn_memo(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 1, "Fn.Memo")) return ON1X_ERR;
    Value fv;
    if (!read_argument(s, 1, fv)) return bad(s, "Fn.Memo");
    if (fv.kind() != Value::Kind::Function)
        return bad(s, "Fn.Memo expects an Fn");
    auto* cache = new_table(&s->gc);
    if (!cache) return bad(s, "Fn.Memo: allocation failed");
    GcRoot cache_root(cache);
    s->persistent_roots.push(cache);
    auto* native_fn = runtime::new_native_function(&s->gc, fn_memo_call);
    if (!native_fn) return bad(s, "Fn.Memo: allocation failed");
    GcRoot root(native_fn);
    Value* caps = gc_alloc_array<Value>(&s->gc, 2);
    if (!caps) return bad(s, "Fn.Memo: allocation failed");
    caps[0] = fv;
    caps[1] = value_from_object(cache);
    native_fn->captures = caps;
    native_fn->capture_count = 2;
    s->persistent_roots.push(native_fn);
    return stack_push(s, value_from_object(native_fn)) ? ON1X_OK : ON1X_ERR;
}

const On1x_FnDesc fns[] = {
    {"Apply", fn_apply},
    {"Arity", fn_arity},
    {"IsVariadic", fn_is_variadic},
    {"Compose", fn_compose},
    {"Pipe", fn_pipe},
    {"Partial", fn_partial},
    {"Flip", fn_flip},
    {"Identity", fn_identity},
    {"Const", fn_const},
    {"Memo", fn_memo},
};

const On1x_ModuleDesc desc{"Fn", ON1X_CAP_NONE, fns, sizeof(fns) / sizeof(*fns)};

}  // namespace

const On1x_ModuleDesc* fn_module() noexcept { return &desc; }

}  // namespace on1x::stdlib
