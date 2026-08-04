#include "api/api_common.hpp"

#include "core/optional.hpp"

extern "C" {

On1x_Status on1x_push_some(On1x_State* state) {
    if (!state || on1x::visible_stack_size(state) == 0) {
        return on1x::push_api_error(state, "Some expects a value");
    }
    on1x::Value value;
    if (!on1x::stack_at(state, -1, value)) return on1x::push_api_error(state, "Some expects a value");
    try {
        return on1x::stack_replace(state, -1, on1x::make_some(&state->gc, state->reserved, value))
            ? ON1X_OK
            : on1x::push_api_error(state, "unable to create Some");
    } catch (...) {
        return on1x::push_api_error(state, "unable to create Some");
    }
}

void on1x_push_none(On1x_State* state) {
    if (!state) return;
    try {
        (void)on1x::stack_push(state, on1x::make_none(&state->gc, state->reserved));
    } catch (...) {
        (void)on1x::push_api_error(state, "unable to create None");
    }
}

int on1x_is_some(const On1x_State* state, int index) {
    on1x::Value value;
    return on1x::stack_at(state, index, value) && on1x::is_some(value, state->reserved);
}

int on1x_is_none(const On1x_State* state, int index) {
    on1x::Value value;
    return on1x::stack_at(state, index, value) && on1x::is_none(value, state->reserved);
}

}  // extern "C"
