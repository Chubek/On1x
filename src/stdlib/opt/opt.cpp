// spec §9: Opt stdlib — combinators over :Some / :None beyond the prelude four.

#include "api/api_common.hpp"
#include "core/list.hpp"
#include "core/optional.hpp"
#include "core/value.hpp"
#include "gc/roots.hpp"
#include "on1x/on1x_capability.h"
#include "on1x/on1x_module.h"
#include "runtime/state.hpp"
#include "stdlib/args.hpp"
#include "vm/interpreter.hpp"

#include <cstdint>

namespace on1x::stdlib {
namespace {

[[nodiscard]] static On1x_Status bad(On1x_State* s, const char* m) noexcept {
    return push_api_error(s, m);
}

[[nodiscard]] static bool is_fn(Value v) noexcept {
    return v.kind() == Value::Kind::Function;
}

// Helper: invoke a callback with one argument, return the result (or error).
[[nodiscard]] static bool call1(On1x_State* s, FunctionObject* func, Value arg,
                                 Value& result, const char*& error) noexcept {
    GcRoot func_root(func);
    const Value args[] = {arg};
    return vm::invoke_function(s, func, args, 1, result, error);
}

// Helper: invoke a callback with zero arguments.
[[nodiscard]] static bool call0(On1x_State* s, FunctionObject* func,
                                 Value& result, const char*& error) noexcept {
    GcRoot func_root(func);
    return vm::invoke_function(s, func, nullptr, 0, result, error);
}

// Opt.Map(o, f) -> :Some | :None
On1x_Status opt_map_fn(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 2, "Opt.Map")) return ON1X_ERR;
    Value ov, fn;
    if (!read_argument(s, 1, ov) || !read_argument(s, 2, fn) || !is_fn(fn))
        return bad(s, "Opt.Map");
    if (!on1x::is_some(ov, s->reserved)) return stack_push(s, ov) ? ON1X_OK : ON1X_ERR;
    auto* func = static_cast<FunctionObject*>(fn.as_object());
    Value inner;
    if (!on1x::unwrap_some(ov, s->reserved, inner))
        return bad(s, "Opt.Map");
    Value call_result;
    const char* error = nullptr;
    if (!call1(s, func, inner, call_result, error))
        return bad(s, error ? error : "Opt.Map callback failed");
    return stack_push(s, on1x::make_some(&s->gc, s->reserved, call_result)) ? ON1X_OK : ON1X_ERR;
}

// Opt.AndThen(o, f) -> :Some | :None — f returns an optional
On1x_Status opt_and_then_fn(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 2, "Opt.AndThen")) return ON1X_ERR;
    Value ov, fn;
    if (!read_argument(s, 1, ov) || !read_argument(s, 2, fn) || !is_fn(fn))
        return bad(s, "Opt.AndThen");
    if (!on1x::is_some(ov, s->reserved))
        return stack_push(s, on1x::make_none(&s->gc, s->reserved)) ? ON1X_OK : ON1X_ERR;
    auto* func = static_cast<FunctionObject*>(fn.as_object());
    Value inner;
    if (!on1x::unwrap_some(ov, s->reserved, inner))
        return bad(s, "Opt.AndThen");
    Value call_result;
    const char* error = nullptr;
    if (!call1(s, func, inner, call_result, error))
        return bad(s, error ? error : "Opt.AndThen callback failed");
    return stack_push(s, call_result) ? ON1X_OK : ON1X_ERR;
}

// Opt.OrElse(o, f) -> :Some | :None
On1x_Status opt_or_else_fn(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 2, "Opt.OrElse")) return ON1X_ERR;
    Value ov, fn;
    if (!read_argument(s, 1, ov) || !read_argument(s, 2, fn) || !is_fn(fn))
        return bad(s, "Opt.OrElse");
    if (on1x::is_some(ov, s->reserved)) return stack_push(s, ov) ? ON1X_OK : ON1X_ERR;
    auto* func = static_cast<FunctionObject*>(fn.as_object());
    Value call_result;
    const char* error = nullptr;
    if (!call0(s, func, call_result, error))
        return bad(s, error ? error : "Opt.OrElse callback failed");
    return stack_push(s, call_result) ? ON1X_OK : ON1X_ERR;
}

// Opt.Filter(o, pred) -> :Some | :None
On1x_Status opt_filter_fn(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 2, "Opt.Filter")) return ON1X_ERR;
    Value ov, fn;
    if (!read_argument(s, 1, ov) || !read_argument(s, 2, fn) || !is_fn(fn))
        return bad(s, "Opt.Filter");
    if (!on1x::is_some(ov, s->reserved))
        return stack_push(s, on1x::make_none(&s->gc, s->reserved)) ? ON1X_OK : ON1X_ERR;
    auto* func = static_cast<FunctionObject*>(fn.as_object());
    Value inner;
    if (!on1x::unwrap_some(ov, s->reserved, inner))
        return bad(s, "Opt.Filter");
    Value call_result;
    const char* error = nullptr;
    if (!call1(s, func, inner, call_result, error))
        return bad(s, error ? error : "Opt.Filter callback failed");
    if (call_result.is_bool() && call_result.as_bool())
        return stack_push(s, ov) ? ON1X_OK : ON1X_ERR;
    return stack_push(s, on1x::make_none(&s->gc, s->reserved)) ? ON1X_OK : ON1X_ERR;
}

// Opt.ToList(o) -> :List — [] or [v]
On1x_Status opt_to_list_fn(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 1, "Opt.ToList")) return ON1X_ERR;
    Value ov;
    if (!read_argument(s, 1, ov)) return bad(s, "Opt.ToList");
    auto* lst = new_list(&s->gc);
    GcRoot lst_root(lst);
    if (on1x::is_some(ov, s->reserved)) {
        Value inner;
        if (!on1x::unwrap_some(ov, s->reserved, inner))
            return bad(s, "Opt.ToList");
        if (!list_push(&s->gc, lst, inner)) return bad(s, "Opt.ToList");
    }
    return stack_push(s, value_from_object(lst)) ? ON1X_OK : ON1X_ERR;
}

// Opt.FromList(l) -> :Some[v] | :None — first element, or :None if empty
On1x_Status opt_from_list_fn(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 1, "Opt.FromList")) return ON1X_ERR;
    Value lv;
    if (!read_argument(s, 1, lv) || lv.kind() != Value::Kind::List)
        return bad(s, "Opt.FromList");
    const auto* lst = as_list_const(lv);
    if (!lst || lst->length == 0)
        return stack_push(s, on1x::make_none(&s->gc, s->reserved)) ? ON1X_OK : ON1X_ERR;
    return stack_push(s, on1x::make_some(&s->gc, s->reserved, lst->items[0])) ? ON1X_OK : ON1X_ERR;
}

// Opt.Zip(a, b) -> :Some[[x, y]] | :None
On1x_Status opt_zip_fn(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 2, "Opt.Zip")) return ON1X_ERR;
    Value a, b;
    if (!read_argument(s, 1, a) || !read_argument(s, 2, b))
        return bad(s, "Opt.Zip");
    if (!on1x::is_some(a, s->reserved) || !on1x::is_some(b, s->reserved))
        return stack_push(s, on1x::make_none(&s->gc, s->reserved)) ? ON1X_OK : ON1X_ERR;
    Value inner_a, inner_b;
    if (!on1x::unwrap_some(a, s->reserved, inner_a) ||
        !on1x::unwrap_some(b, s->reserved, inner_b))
        return bad(s, "Opt.Zip");
    auto* pair = new_list(&s->gc);
    GcRoot pair_root(pair);
    if (!list_push(&s->gc, pair, inner_a) || !list_push(&s->gc, pair, inner_b))
        return bad(s, "Opt.Zip");
    return stack_push(s, on1x::make_some(&s->gc, s->reserved, value_from_object(pair))) ? ON1X_OK : ON1X_ERR;
}

// Opt.Flatten(o) -> :Some | :None
On1x_Status opt_flatten_fn(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 1, "Opt.Flatten")) return ON1X_ERR;
    Value ov;
    if (!read_argument(s, 1, ov)) return bad(s, "Opt.Flatten");
    if (!on1x::is_some(ov, s->reserved))
        return stack_push(s, on1x::make_none(&s->gc, s->reserved)) ? ON1X_OK : ON1X_ERR;
    Value inner;
    if (!on1x::unwrap_some(ov, s->reserved, inner))
        return bad(s, "Opt.Flatten");
    // If inner is also an optional (Some or None), return it; otherwise return original
    if (on1x::is_some(inner, s->reserved) || on1x::is_none(inner, s->reserved))
        return stack_push(s, inner) ? ON1X_OK : ON1X_ERR;
    return stack_push(s, ov) ? ON1X_OK : ON1X_ERR;
}

const On1x_FnDesc fns[] = {
    {"Map", opt_map_fn},
    {"AndThen", opt_and_then_fn},
    {"OrElse", opt_or_else_fn},
    {"Filter", opt_filter_fn},
    {"ToList", opt_to_list_fn},
    {"FromList", opt_from_list_fn},
    {"Zip", opt_zip_fn},
    {"Flatten", opt_flatten_fn},
};

const On1x_ModuleDesc desc{"Opt", ON1X_CAP_NONE, fns, sizeof(fns) / sizeof(*fns)};

}  // namespace

const On1x_ModuleDesc* opt_module() noexcept { return &desc; }

}  // namespace on1x::stdlib
