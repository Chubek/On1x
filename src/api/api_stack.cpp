#include "api/api_common.hpp"

extern "C" {
int on1x_top(const On1x_State* state) {
    return static_cast<int>(on1x::visible_stack_size(state));
}

int on1x_pop(On1x_State* state, int count) {
    if (!state || count < 0 || static_cast<std::size_t>(count) > on1x::visible_stack_size(state)) return 0;
    state->top -= static_cast<std::size_t>(count);
    return 1;
}

On1x_Status on1x_dup(On1x_State* state, int index) {
    on1x::Value value;
    return on1x::stack_at(state, index, value) && on1x::stack_push(state, value) ? ON1X_OK : ON1X_ERR;
}

On1x_Type on1x_type(const On1x_State* state, int index) {
    on1x::Value value;
    return on1x::stack_at(state, index, value) ? on1x::api_type(value) : ON1X_INVALID;
}
}
