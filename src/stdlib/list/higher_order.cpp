
// spec §17: List higher-order functions — Map, Filter, Reduce, ForEach, Any, All,
// Find, FindIndex, Partition, IndexOf, Count.

#include "api/api_common.hpp"
#include "core/equality.hpp"
#include "core/list.hpp"
#include "core/optional.hpp"
#include "core/value.hpp"
#include "gc/roots.hpp"
#include "runtime/state.hpp"
#include "stdlib/args.hpp"
#include "stdlib/list/list_fns.hpp"
#include "vm/interpreter.hpp"

#include <cstdint>

namespace on1x::stdlib {

namespace {

static On1x_Status bad(On1x_State* s, const char* m) noexcept {
    return push_api_error(s, m);
}

// Call an On1x function value (native or bytecode) with one argument.
// The function must be a :Fn value. Pushes the result onto the stack and
// returns true, or returns false with error set.
bool call_fn1(On1x_State* s, Value fn, Value arg, Value& result,
              const char*& error) noexcept {
    if (fn.kind() != Value::Kind::Function) {
        error = "callback must be :Fn";
        return false;
    }
    auto* func = static_cast<FunctionObject*>(fn.as_object());
    GcRoot func_root(func);
    GcRoot arg_root(arg.is_object() ? arg.as_object() : nullptr);
    const Value args[] = {arg};
    return vm::invoke_function(s, func, args, 1, result, error);
}

// Call an On1x function value with two arguments.
bool call_fn2(On1x_State* s, Value fn, Value a1, Value a2, Value& result,
              const char*& error) noexcept {
    if (fn.kind() != Value::Kind::Function) {
        error = "callback must be :Fn";
        return false;
    }
    auto* func = static_cast<FunctionObject*>(fn.as_object());
    GcRoot func_root(func);
    GcRoot r1(a1.is_object() ? a1.as_object() : nullptr);
    GcRoot r2(a2.is_object() ? a2.as_object() : nullptr);
    const Value args[] = {a1, a2};
    return vm::invoke_function(s, func, args, 2, result, error);
}

bool is_fn(Value v) noexcept { return v.kind() == Value::Kind::Function; }

}  // namespace

On1x_Status list_map_fn(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 2, "List.Map")) return ON1X_ERR;
    Value lv, fn;
    if (!read_argument(s, 1, lv) || !read_argument(s, 2, fn) ||
        lv.kind() != Value::Kind::List || !is_fn(fn))
        return bad(s, "List.Map");
    const auto* src = as_list_const(lv);
    auto* dst = new_list(&s->gc, src->length);
    GcRoot dst_root(dst);
    auto* func = static_cast<FunctionObject*>(fn.as_object());
    GcRoot func_root(func);
    for (std::size_t i = 0; i < src->length; ++i) {
        Value item;
        list_get(src, i, item);
        Value call_result;
        const char* error = nullptr;
        const Value args[] = {item};
        if (!vm::invoke_function(s, func, args, 1, call_result, error))
            return bad(s, error ? error : "List.Map callback failed");
        if (!list_push(&s->gc, dst, call_result))
            return bad(s, "List.Map");
    }
    return stack_push(s, value_from_object(dst)) ? ON1X_OK : ON1X_ERR;
}

On1x_Status list_filter_fn(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 2, "List.Filter")) return ON1X_ERR;
    Value lv, fn;
    if (!read_argument(s, 1, lv) || !read_argument(s, 2, fn) ||
        lv.kind() != Value::Kind::List || !is_fn(fn))
        return bad(s, "List.Filter");
    const auto* src = as_list_const(lv);
    auto* dst = new_list(&s->gc, 0);
    GcRoot dst_root(dst);
    auto* func = static_cast<FunctionObject*>(fn.as_object());
    GcRoot func_root(func);
    for (std::size_t i = 0; i < src->length; ++i) {
        Value item;
        list_get(src, i, item);
        Value call_result;
        const char* error = nullptr;
        const Value args[] = {item};
        if (!vm::invoke_function(s, func, args, 1, call_result, error))
            return bad(s, error ? error : "List.Filter callback failed");
        if (!call_result.is_bool())
            return bad(s, "List.Filter predicate must return :Bool");
        if (call_result.as_bool()) {
            if (!list_push(&s->gc, dst, item))
                return bad(s, "List.Filter");
        }
    }
    return stack_push(s, value_from_object(dst)) ? ON1X_OK : ON1X_ERR;
}

On1x_Status list_reduce_fn(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 3, "List.Reduce")) return ON1X_ERR;
    Value lv, fn, init;
    if (!read_argument(s, 1, lv) || !read_argument(s, 2, fn) ||
        !read_argument(s, 3, init) || lv.kind() != Value::Kind::List || !is_fn(fn))
        return bad(s, "List.Reduce");
    const auto* src = as_list_const(lv);
    auto* func = static_cast<FunctionObject*>(fn.as_object());
    GcRoot func_root(func);
    Value acc = init;
    for (std::size_t i = 0; i < src->length; ++i) {
        Value item;
        list_get(src, i, item);
        const char* error = nullptr;
        Value call_result;
        GcRoot acc_root(acc.is_object() ? acc.as_object() : nullptr);
        const Value args[] = {acc, item};
        if (!vm::invoke_function(s, func, args, 2, call_result, error))
            return bad(s, error ? error : "List.Reduce callback failed");
        acc = call_result;
    }
    return stack_push(s, acc) ? ON1X_OK : ON1X_ERR;
}

On1x_Status list_for_each_fn(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 2, "List.ForEach")) return ON1X_ERR;
    Value lv, fn;
    if (!read_argument(s, 1, lv) || !read_argument(s, 2, fn) ||
        lv.kind() != Value::Kind::List || !is_fn(fn))
        return bad(s, "List.ForEach");
    const auto* src = as_list_const(lv);
    auto* func = static_cast<FunctionObject*>(fn.as_object());
    GcRoot func_root(func);
    for (std::size_t i = 0; i < src->length; ++i) {
        Value item;
        list_get(src, i, item);
        Value call_result;
        const char* error = nullptr;
        const Value args[] = {item};
        if (!vm::invoke_function(s, func, args, 1, call_result, error))
            return bad(s, error ? error : "List.ForEach callback failed");
    }
    return stack_push(s, Value::unit()) ? ON1X_OK : ON1X_ERR;
}

On1x_Status list_any_fn(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 2, "List.Any")) return ON1X_ERR;
    Value lv, fn;
    if (!read_argument(s, 1, lv) || !read_argument(s, 2, fn) ||
        lv.kind() != Value::Kind::List || !is_fn(fn))
        return bad(s, "List.Any");
    const auto* src = as_list_const(lv);
    auto* func = static_cast<FunctionObject*>(fn.as_object());
    GcRoot func_root(func);
    for (std::size_t i = 0; i < src->length; ++i) {
        Value item;
        list_get(src, i, item);
        Value call_result;
        const char* error = nullptr;
        const Value args[] = {item};
        if (!vm::invoke_function(s, func, args, 1, call_result, error))
            return bad(s, error ? error : "List.Any callback failed");
        if (!call_result.is_bool())
            return bad(s, "List.Any predicate must return :Bool");
        if (call_result.as_bool())
            return stack_push(s, Value::boolean(true)) ? ON1X_OK : ON1X_ERR;
    }
    return stack_push(s, Value::boolean(false)) ? ON1X_OK : ON1X_ERR;
}

On1x_Status list_all_fn(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 2, "List.All")) return ON1X_ERR;
    Value lv, fn;
    if (!read_argument(s, 1, lv) || !read_argument(s, 2, fn) ||
        lv.kind() != Value::Kind::List || !is_fn(fn))
        return bad(s, "List.All");
    const auto* src = as_list_const(lv);
    auto* func = static_cast<FunctionObject*>(fn.as_object());
    GcRoot func_root(func);
    for (std::size_t i = 0; i < src->length; ++i) {
        Value item;
        list_get(src, i, item);
        Value call_result;
        const char* error = nullptr;
        const Value args[] = {item};
        if (!vm::invoke_function(s, func, args, 1, call_result, error))
            return bad(s, error ? error : "List.All callback failed");
        if (!call_result.is_bool())
            return bad(s, "List.All predicate must return :Bool");
        if (!call_result.as_bool())
            return stack_push(s, Value::boolean(false)) ? ON1X_OK : ON1X_ERR;
    }
    return stack_push(s, Value::boolean(true)) ? ON1X_OK : ON1X_ERR;
}

On1x_Status list_find_fn(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 2, "List.Find")) return ON1X_ERR;
    Value lv, fn;
    if (!read_argument(s, 1, lv) || !read_argument(s, 2, fn) ||
        lv.kind() != Value::Kind::List || !is_fn(fn))
        return bad(s, "List.Find");
    const auto* src = as_list_const(lv);
    auto* func = static_cast<FunctionObject*>(fn.as_object());
    GcRoot func_root(func);
    for (std::size_t i = 0; i < src->length; ++i) {
        Value item;
        list_get(src, i, item);
        Value call_result;
        const char* error = nullptr;
        const Value args[] = {item};
        if (!vm::invoke_function(s, func, args, 1, call_result, error))
            return bad(s, error ? error : "List.Find callback failed");
        if (!call_result.is_bool())
            return bad(s, "List.Find predicate must return :Bool");
        if (call_result.as_bool())
            return stack_push(s, make_some(&s->gc, s->reserved, item)) ? ON1X_OK : ON1X_ERR;
    }
    return stack_push(s, make_none(&s->gc, s->reserved)) ? ON1X_OK : ON1X_ERR;
}

On1x_Status list_find_index_fn(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 2, "List.FindIndex")) return ON1X_ERR;
    Value lv, fn;
    if (!read_argument(s, 1, lv) || !read_argument(s, 2, fn) ||
        lv.kind() != Value::Kind::List || !is_fn(fn))
        return bad(s, "List.FindIndex");
    const auto* src = as_list_const(lv);
    auto* func = static_cast<FunctionObject*>(fn.as_object());
    GcRoot func_root(func);
    for (std::size_t i = 0; i < src->length; ++i) {
        Value item;
        list_get(src, i, item);
        Value call_result;
        const char* error = nullptr;
        const Value args[] = {item};
        if (!vm::invoke_function(s, func, args, 1, call_result, error))
            return bad(s, error ? error : "List.FindIndex callback failed");
        if (!call_result.is_bool())
            return bad(s, "List.FindIndex predicate must return :Bool");
        if (call_result.as_bool()) {
            Value idx = Value::integer(&s->gc, static_cast<std::int64_t>(i));
            return stack_push(s, make_some(&s->gc, s->reserved, idx)) ? ON1X_OK : ON1X_ERR;
        }
    }
    return stack_push(s, make_none(&s->gc, s->reserved)) ? ON1X_OK : ON1X_ERR;
}

On1x_Status list_partition_fn(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 2, "List.Partition")) return ON1X_ERR;
    Value lv, fn;
    if (!read_argument(s, 1, lv) || !read_argument(s, 2, fn) ||
        lv.kind() != Value::Kind::List || !is_fn(fn))
        return bad(s, "List.Partition");
    const auto* src = as_list_const(lv);
    auto* yes = new_list(&s->gc, 0);
    auto* no = new_list(&s->gc, 0);
    GcRoot yes_root(yes);
    GcRoot no_root(no);
    auto* func = static_cast<FunctionObject*>(fn.as_object());
    GcRoot func_root(func);
    for (std::size_t i = 0; i < src->length; ++i) {
        Value item;
        list_get(src, i, item);
        Value call_result;
        const char* error = nullptr;
        const Value args[] = {item};
        if (!vm::invoke_function(s, func, args, 1, call_result, error))
            return bad(s, error ? error : "List.Partition callback failed");
        if (!call_result.is_bool())
            return bad(s, "List.Partition predicate must return :Bool");
        if (call_result.as_bool()) {
            if (!list_push(&s->gc, yes, item)) return bad(s, "List.Partition");
        } else {
            if (!list_push(&s->gc, no, item)) return bad(s, "List.Partition");
        }
    }
    auto* result = new_list(&s->gc, 2);
    GcRoot result_root(result);
    if (!list_push(&s->gc, result, value_from_object(yes)) ||
        !list_push(&s->gc, result, value_from_object(no)))
        return bad(s, "List.Partition");
    return stack_push(s, value_from_object(result)) ? ON1X_OK : ON1X_ERR;
}

On1x_Status list_index_of_fn(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 2, "List.IndexOf")) return ON1X_ERR;
    Value lv, target;
    if (!read_argument(s, 1, lv) || !read_argument(s, 2, target) ||
        lv.kind() != Value::Kind::List)
        return bad(s, "List.IndexOf");
    const auto* src = as_list_const(lv);
    for (std::size_t i = 0; i < src->length; ++i) {
        Value item;
        list_get(src, i, item);
        if (value_equals(item, target)) {
            Value idx = Value::integer(&s->gc, static_cast<std::int64_t>(i));
            return stack_push(s, make_some(&s->gc, s->reserved, idx)) ? ON1X_OK : ON1X_ERR;
        }
    }
    return stack_push(s, make_none(&s->gc, s->reserved)) ? ON1X_OK : ON1X_ERR;
}

On1x_Status list_count_fn(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 2, "List.Count")) return ON1X_ERR;
    Value lv, fn;
    if (!read_argument(s, 1, lv) || !read_argument(s, 2, fn) ||
        lv.kind() != Value::Kind::List || !is_fn(fn))
        return bad(s, "List.Count");
    const auto* src = as_list_const(lv);
    auto* func = static_cast<FunctionObject*>(fn.as_object());
    GcRoot func_root(func);
    std::int64_t count = 0;
    for (std::size_t i = 0; i < src->length; ++i) {
        Value item;
        list_get(src, i, item);
        Value call_result;
        const char* error = nullptr;
        const Value args[] = {item};
        if (!vm::invoke_function(s, func, args, 1, call_result, error))
            return bad(s, error ? error : "List.Count callback failed");
        if (!call_result.is_bool())
            return bad(s, "List.Count predicate must return :Bool");
        if (call_result.as_bool()) ++count;
    }
    return stack_push(s, Value::integer(&s->gc, count)) ? ON1X_OK : ON1X_ERR;
}

}  // namespace on1x::stdlib
