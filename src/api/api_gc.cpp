#include "api/api_common.hpp"

#include "gc/gc.hpp"

namespace {

on1x::ApiReference* find_reference(On1x_State* state, On1x_Ref reference) noexcept {
    if (!state || reference == 0) return nullptr;
    auto* candidate = reinterpret_cast<on1x::ApiReference*>(reference);
    for (auto* current = state->references; current; current = current->next) {
        if (current == candidate) return current;
    }
    return nullptr;
}

}  // namespace

extern "C" {

On1x_Ref on1x_ref_create(On1x_State* state, int stack_index) {
    on1x::Value value;
    if (!state || !on1x::stack_at(state, stack_index, value)) return 0;
    try {
        auto* reference = new on1x::ApiReference;
        reference->value = value;
        if (value.is_object()) reference->handle = state->handles.create(value.as_object());
        reference->next = state->references;
        state->references = reference;
        return reinterpret_cast<On1x_Ref>(reference);
    } catch (...) {
        return 0;
    }
}

int on1x_ref_release(On1x_State* state, On1x_Ref reference) {
    auto* found = find_reference(state, reference);
    if (!found) return 0;
    auto** current = &state->references;
    while (*current != found) current = &(*current)->next;
    *current = found->next;
    if (found->handle) state->handles.release(found->handle);
    delete found;
    return 1;
}

On1x_Status on1x_ref_push(On1x_State* state, On1x_Ref reference) {
    auto* found = find_reference(state, reference);
    if (!found) return on1x::push_api_error(state, "invalid rooted reference");
    return on1x::stack_push(state, found->value)
        ? ON1X_OK
        : on1x::push_api_error(state, "unable to push rooted reference");
}

void on1x_gc_collect(On1x_State* state) {
    if (state) on1x::gc_collect(&state->gc);
}

size_t on1x_gc_bytes_allocated(const On1x_State* state) {
    return state ? on1x::gc_bytes_allocated(&state->gc) : 0;
}

}  // extern "C"
