// spec §10: Res stdlib — combinators over :Success / :Error beyond '~'.

#include "api/api_common.hpp"
#include "core/list.hpp"
#include "core/optional.hpp"
#include "core/result.hpp"
#include "core/value.hpp"
#include "gc/roots.hpp"
#include "on1x/on1x_capability.h"
#include "on1x/on1x_module.h"
#include "runtime/state.hpp"
#include "stdlib/args.hpp"
#include "vm/interpreter.hpp"

#include <cstdint>
#include <cstdio>

namespace on1x::stdlib {
namespace {

[[nodiscard]] static On1x_Status bad(On1x_State* s, const char* m) noexcept {
    return push_api_error(s, m);
}

[[nodiscard]] static bool is_fn(Value v) noexcept {
    return v.kind() == Value::Kind::Function;
}

// Extract the payload from a :Success or :Error tagged list,
// unwrapping both tagged list layers: :Success[:Some[value]] -> value.
[[nodiscard]] static bool unwrap_result_payload(Value rv, const ReservedTags& tags, Value& payload) noexcept {
    auto* tl = as_list(rv);
    if (!tl || tl->length == 0) return false;
    Value inner = tl->items[0];  // :Some[value] or :None
    if (on1x::is_some(inner, tags)) {
        return on1x::unwrap_some(inner, tags, payload);
    }
    // In the None case (error with no payload), return unit
    payload = Value::unit();
    return true;
}

// Unwrap :Success[:Some[value]] -> :Some[value] (the direct inner tagged list)
[[nodiscard]] static Value unwrap_success_inner(Value rv) noexcept {
    auto* tl = as_list(rv);
    return (tl && tl->length > 0) ? tl->items[0] : Value::unit();
}

// Unwrap :Error[:Some[value]] -> :Some[value] (the direct inner)
[[nodiscard]] static Value unwrap_error_inner(Value rv) noexcept {
    auto* tl = as_list(rv);
    return (tl && tl->length > 0) ? tl->items[0] : Value::unit();
}

[[nodiscard]] static bool call1(On1x_State* s, FunctionObject* func, Value arg,
                                 Value& result, const char*& error) noexcept {
    GcRoot func_root(func);
    const Value args[] = {arg};
    return vm::invoke_function(s, func, args, 1, result, error);
}

// Res.Map(r, f) -> :Success | :Error
On1x_Status res_map_fn(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 2, "Res.Map")) return ON1X_ERR;
    Value rv, fn;
    if (!read_argument(s, 1, rv) || !read_argument(s, 2, fn) || !is_fn(fn))
        return bad(s, "Res.Map");
    if (!on1x::is_success(rv, s->reserved)) return stack_push(s, rv) ? ON1X_OK : ON1X_ERR;
    auto* func = static_cast<FunctionObject*>(fn.as_object());
    Value inner;
    if (!unwrap_result_payload(rv, s->reserved, inner))
        return bad(s, "Res.Map");
    Value call_result;
    const char* error = nullptr;
    if (!call1(s, func, inner, call_result, error))
        return bad(s, error ? error : "Res.Map callback failed");
    return stack_push(s, on1x::make_success(&s->gc, s->reserved, call_result)) ? ON1X_OK : ON1X_ERR;
}

// Res.MapError(r, f) -> :Success | :Error
On1x_Status res_map_error_fn(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 2, "Res.MapError")) return ON1X_ERR;
    Value rv, fn;
    if (!read_argument(s, 1, rv) || !read_argument(s, 2, fn) || !is_fn(fn))
        return bad(s, "Res.MapError");
    if (!on1x::is_error(rv, s->reserved)) return stack_push(s, rv) ? ON1X_OK : ON1X_ERR;
    auto* func = static_cast<FunctionObject*>(fn.as_object());
    Value inner;
    if (!unwrap_result_payload(rv, s->reserved, inner))
        return bad(s, "Res.MapError");
    Value call_result;
    const char* error = nullptr;
    if (!call1(s, func, inner, call_result, error))
        return bad(s, error ? error : "Res.MapError callback failed");
    return stack_push(s, on1x::make_error(&s->gc, s->reserved, call_result)) ? ON1X_OK : ON1X_ERR;
}

// Res.AndThen(r, f) -> :Success | :Error
On1x_Status res_and_then_fn(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 2, "Res.AndThen")) return ON1X_ERR;
    Value rv, fn;
    if (!read_argument(s, 1, rv) || !read_argument(s, 2, fn) || !is_fn(fn)) {
        return bad(s, "Res.AndThen");
    }
    if (!on1x::is_success(rv, s->reserved)) {
        return stack_push(s, rv) ? ON1X_OK : ON1X_ERR;
    }
    auto* func = static_cast<FunctionObject*>(fn.as_object());
    Value inner;
    if (!unwrap_result_payload(rv, s->reserved, inner)) {
        return bad(s, "Res.AndThen");
    }
    Value call_result;
    const char* error = nullptr;
    if (!call1(s, func, inner, call_result, error)) {
        return bad(s, error ? error : "Res.AndThen callback failed");
    }
    return stack_push(s, call_result) ? ON1X_OK : ON1X_ERR;
}

// Res.OrElse(r, f) -> :Success | :Error
On1x_Status res_or_else_fn(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 2, "Res.OrElse")) return ON1X_ERR;
    Value rv, fn;
    if (!read_argument(s, 1, rv) || !read_argument(s, 2, fn) || !is_fn(fn))
        return bad(s, "Res.OrElse");
    if (on1x::is_success(rv, s->reserved)) return stack_push(s, rv) ? ON1X_OK : ON1X_ERR;
    auto* func = static_cast<FunctionObject*>(fn.as_object());
    Value inner;
    if (!unwrap_result_payload(rv, s->reserved, inner))
        return bad(s, "Res.OrElse");
    Value call_result;
    const char* error = nullptr;
    if (!call1(s, func, inner, call_result, error))
        return bad(s, error ? error : "Res.OrElse callback failed");
    return stack_push(s, call_result) ? ON1X_OK : ON1X_ERR;
}

// Res.Payload(r) -> :Some[v] | :None
On1x_Status res_payload_fn(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 1, "Res.Payload")) return ON1X_ERR;
    Value rv;
    if (!read_argument(s, 1, rv)) return bad(s, "Res.Payload");
    // Payload returns :Some[payload] for both :Success and :Error
    Value inner = unwrap_success_inner(rv);
    return stack_push(s, inner) ? ON1X_OK : ON1X_ERR;
}

// Res.ToOpt(r) -> :Some[v] | :None
On1x_Status res_to_opt_fn(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 1, "Res.ToOpt")) return ON1X_ERR;
    Value rv;
    if (!read_argument(s, 1, rv)) return bad(s, "Res.ToOpt");
    if (on1x::is_success(rv, s->reserved)) {
        return stack_push(s, unwrap_success_inner(rv)) ? ON1X_OK : ON1X_ERR;
    }
    // :Error maps to :None
    return stack_push(s, on1x::make_none(&s->gc, s->reserved)) ? ON1X_OK : ON1X_ERR;
}

// Res.UnwrapOr(r, default) -> value
On1x_Status res_unwrap_or_fn(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 2, "Res.UnwrapOr")) return ON1X_ERR;
    Value rv, def;
    if (!read_argument(s, 1, rv) || !read_argument(s, 2, def))
        return bad(s, "Res.UnwrapOr");
    if (on1x::is_success(rv, s->reserved)) {
        Value payload;
        if (!unwrap_result_payload(rv, s->reserved, payload))
            return bad(s, "Res.UnwrapOr");
        return stack_push(s, payload) ? ON1X_OK : ON1X_ERR;
    }
    // :Error -> return default
    return stack_push(s, def) ? ON1X_OK : ON1X_ERR;
}

// Res.Collect(l: :List of results) -> :Success[:List] | :Error[first]
On1x_Status res_collect_fn(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 1, "Res.Collect")) return ON1X_ERR;
    Value lv;
    if (!read_argument(s, 1, lv) || lv.kind() != Value::Kind::List)
        return bad(s, "Res.Collect");
    const auto* src = as_list_const(lv);
    if (!src) return bad(s, "Res.Collect");
    auto* dst = new_list(&s->gc);
    GcRoot dst_root(dst);
    for (std::size_t i = 0; i < src->length; ++i) {
        Value item = src->items[i];
        if (on1x::is_error(item, s->reserved))
            return stack_push(s, item) ? ON1X_OK : ON1X_ERR;
        Value payload;
        if (!unwrap_result_payload(item, s->reserved, payload)) {
            payload = Value::unit();
        }
        if (!list_push(&s->gc, dst, payload))
            return bad(s, "Res.Collect");
    }
    return stack_push(s, on1x::make_success(&s->gc, s->reserved, value_from_object(dst))) ? ON1X_OK : ON1X_ERR;
}

const On1x_FnDesc fns[] = {
    {"Map", res_map_fn},
    {"MapError", res_map_error_fn},
    {"AndThen", res_and_then_fn},
    {"OrElse", res_or_else_fn},
    {"Payload", res_payload_fn},
    {"ToOpt", res_to_opt_fn},
    {"UnwrapOr", res_unwrap_or_fn},
    {"Collect", res_collect_fn},
};

const On1x_ModuleDesc desc{"Res", ON1X_CAP_NONE, fns, sizeof(fns) / sizeof(*fns)};

}  // namespace

const On1x_ModuleDesc* res_module() noexcept { return &desc; }

}  // namespace on1x::stdlib
