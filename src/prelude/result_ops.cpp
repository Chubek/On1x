#include "prelude/prelude.hpp"

#include "core/result.hpp"

namespace on1x::prelude {

On1x_Status is_success(On1x_State* state, int argc) {
    if (!has_arity(state, argc, 1, "IsSuccess expects one argument")) return ON1X_ERR;
    Value value;
    return argument(state, 1, value)
        ? push_result(state, Value::boolean(on1x::is_success(value, state->reserved)))
        : raise(state, "IsSuccess received an invalid argument");
}

On1x_Status is_error(On1x_State* state, int argc) {
    if (!has_arity(state, argc, 1, "IsError expects one argument")) return ON1X_ERR;
    Value value;
    return argument(state, 1, value)
        ? push_result(state, Value::boolean(on1x::is_error(value, state->reserved)))
        : raise(state, "IsError received an invalid argument");
}

}  // namespace on1x::prelude
