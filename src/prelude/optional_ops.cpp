#include "prelude/prelude.hpp"

#include "core/optional.hpp"

namespace on1x::prelude {

namespace {

bool is_optional(Value value, const ReservedTags& tags) noexcept {
    return on1x::is_some(value, tags) || on1x::is_none(value, tags);
}

}  // namespace

On1x_Status is_some(On1x_State* state, int argc) {
    if (!has_arity(state, argc, 1, "IsSome expects one argument")) return ON1X_ERR;
    Value value;
    return argument(state, 1, value)
        ? push_result(state, Value::boolean(on1x::is_some(value, state->reserved)))
        : raise(state, "IsSome received an invalid argument");
}

On1x_Status is_none(On1x_State* state, int argc) {
    if (!has_arity(state, argc, 1, "IsNone expects one argument")) return ON1X_ERR;
    Value value;
    return argument(state, 1, value)
        ? push_result(state, Value::boolean(on1x::is_none(value, state->reserved)))
        : raise(state, "IsNone received an invalid argument");
}

On1x_Status unwrap(On1x_State* state, int argc) {
    if (!has_arity(state, argc, 1, "Unwrap expects one argument")) return ON1X_ERR;
    Value value;
    Value result;
    if (!argument(state, 1, value)) return raise(state, "Unwrap received an invalid argument");
    if (!unwrap_some(value, state->reserved, result)) return raise(state, "Unwrap expects Some");
    return push_result(state, result);
}

On1x_Status unwrap_or(On1x_State* state, int argc) {
    if (!has_arity(state, argc, 2, "UnwrapOr expects two arguments")) return ON1X_ERR;
    Value value;
    Value fallback;
    Value result;
    if (!argument(state, 1, value) || !argument(state, 2, fallback)) {
        return raise(state, "UnwrapOr received an invalid argument");
    }
    if (!is_optional(value, state->reserved)) return raise(state, "UnwrapOr expects an optional");
    return push_result(
        state,
        unwrap_some(value, state->reserved, result) ? result : fallback);
}

}  // namespace on1x::prelude
