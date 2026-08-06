#include "prelude/prelude.hpp"

#include "core/tagged_list.hpp"
#include "core/type.hpp"

namespace on1x::prelude {

On1x_Status type_of(On1x_State* state, int argc) {
    if (!has_arity(state, argc, 1, "TypeOf expects one argument")) return ON1X_ERR;
    Value value;
    if (!argument(state, 1, value)) return raise(state, "TypeOf received an invalid argument");
    return push_result(state, on1x::type_of(value, state->reserved));
}

On1x_Status tag_of(On1x_State* state, int argc) {
    if (!has_arity(state, argc, 1, "TagOf expects one argument")) return ON1X_ERR;
    Value value;
    if (!argument(state, 1, value)) return raise(state, "TagOf received an invalid argument");
    try {
        return push_result(state, on1x::tag_of(&state->gc, state->reserved, value));
    } catch (...) {
        return raise(state, "unable to inspect List tag");
    }
}

On1x_Status payload_of(On1x_State* state, int argc) {
    if (!has_arity(state, argc, 1, "PayloadOf expects one argument")) return ON1X_ERR;
    Value value;
    if (!argument(state, 1, value)) return raise(state, "PayloadOf received an invalid argument");
    try {
        return push_result(state, on1x::payload_of(&state->gc, state->reserved, value));
    } catch (...) {
        return raise(state, "unable to inspect List payload");
    }
}

}  // namespace on1x::prelude
