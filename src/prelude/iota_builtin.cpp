#include "prelude/prelude.hpp"

#include "core/iota.hpp"

namespace on1x::prelude {

On1x_Status iota(On1x_State* state, int argc) {
    if (argc < 1 || argc > 3) return raise(state, "Iota expects one to three arguments");
    Value first;
    Value second;
    Value third;
    if (!argument(state, 1, first) ||
        (argc >= 2 && !argument(state, 2, second)) ||
        (argc >= 3 && !argument(state, 3, third))) {
        return raise(state, "Iota received an invalid argument");
    }
    try {
        switch (argc) {
        case 1: return push_result(state, make_iota(&state->gc, state->reserved, {first}));
        case 2: return push_result(state, make_iota(&state->gc, state->reserved, {first, second}));
        case 3: return push_result(state, make_iota(&state->gc, state->reserved, {first, second, third}));
        default: return raise(state, "Iota expects one to three arguments");
        }
    } catch (...) {
        return raise(state, "unable to create Iota range");
    }
}

}  // namespace on1x::prelude
