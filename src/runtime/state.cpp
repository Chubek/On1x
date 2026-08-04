#include "runtime/state.hpp"

#include "gc/alloc.hpp"

namespace on1x {

bool stack_push(On1x_State* state, Value value) {
    if (!state) return false;
    if (state->top == state->capacity) {
        const std::size_t capacity = state->capacity == 0 ? 32 : state->capacity * 2;
        auto* stack = gc_alloc_array<Value>(&state->gc, capacity);
        for (std::size_t index = 0; index < state->top; ++index) stack[index] = state->stack[index];
        state->stack = stack;
        state->capacity = capacity;
    }
    state->stack[state->top++] = value;
    return true;
}

int normalize_stack_index(const On1x_State* state, int index) noexcept {
    if (!state || index == 0) return -1;
    const std::size_t base = state->api_frame_active ? state->api_frame_base : 0;
    const int count = static_cast<int>(state->top - base);
    const int normalized = index > 0 ? index - 1 : count + index;
    return normalized >= 0 && normalized < count
        ? static_cast<int>(base) + normalized
        : -1;
}

bool stack_at(const On1x_State* state, int index, Value& value) noexcept {
    const int normalized = normalize_stack_index(state, index);
    if (normalized < 0) return false;
    value = state->stack[static_cast<std::size_t>(normalized)];
    return true;
}

bool stack_replace(On1x_State* state, int index, Value value) noexcept {
    const int normalized = normalize_stack_index(state, index);
    if (normalized < 0) return false;
    state->stack[static_cast<std::size_t>(normalized)] = value;
    return true;
}

std::size_t visible_stack_size(const On1x_State* state) noexcept {
    if (!state) return 0;
    return state->top - (state->api_frame_active ? state->api_frame_base : 0);
}

void release_api_references(On1x_State* state) noexcept {
    if (!state) return;
    while (state->references) {
        ApiReference* reference = state->references;
        state->references = reference->next;
        if (reference->handle) state->handles.release(reference->handle);
        delete reference;
    }
}

}  // namespace on1x
