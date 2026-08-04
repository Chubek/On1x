#include "api/api_common.hpp"

#include "core/result.hpp"

namespace {

On1x_Status push_result(On1x_State* state, bool success) {
    if (!state || on1x::visible_stack_size(state) == 0) {
        return on1x::push_api_error(state, "Result constructor expects a payload");
    }
    on1x::Value payload;
    if (!on1x::stack_at(state, -1, payload)) {
        return on1x::push_api_error(state, "Result constructor expects a payload");
    }
    try {
        const on1x::Value result = success
            ? on1x::make_success(&state->gc, state->reserved, payload)
            : on1x::make_error(&state->gc, state->reserved, payload);
        return on1x::stack_replace(state, -1, result)
            ? ON1X_OK
            : on1x::push_api_error(state, "unable to create Result");
    } catch (...) {
        return on1x::push_api_error(state, "unable to create Result");
    }
}

}  // namespace

extern "C" {

On1x_Status on1x_push_success(On1x_State* state) {
    return push_result(state, true);
}

On1x_Status on1x_push_error_result(On1x_State* state) {
    return push_result(state, false);
}

int on1x_is_success(const On1x_State* state, int index) {
    on1x::Value value;
    return on1x::stack_at(state, index, value) && on1x::is_success(value, state->reserved);
}

int on1x_is_error(const On1x_State* state, int index) {
    on1x::Value value;
    return on1x::stack_at(state, index, value) && on1x::is_error(value, state->reserved);
}

}  // extern "C"
