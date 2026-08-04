#include "api/api_common.hpp"

#include "core/tagged_list.hpp"

extern "C" {

On1x_Status on1x_tag_list(On1x_State* state, const char* tag, size_t length, int list_index) {
    if (!state || !tag) return on1x::push_api_error(state, "Tag.List expects a tag name and List");

    on1x::Value value;
    if (!on1x::stack_at(state, list_index, value)) {
        return on1x::push_api_error(state, "Tag.List received an invalid stack index");
    }
    auto* list = on1x::as_list(value);
    if (!list) return on1x::push_api_error(state, "Tag.List expects a List");

    try {
        list->constructor = state->tags.intern(&state->gc, {tag, length});
        return ON1X_OK;
    } catch (...) {
        return on1x::push_api_error(state, "Tag.List requires a valid UTF-8 tag");
    }
}

int on1x_tag_of(On1x_State* state, int index) {
    if (!state) return 0;
    on1x::Value value;
    if (!on1x::stack_at(state, index, value)) {
        (void)on1x::push_api_error(state, "Tag.Of received an invalid stack index");
        return 0;
    }
    try {
        return on1x::stack_push(state, on1x::tag_of(&state->gc, state->reserved, value)) ? 1 : 0;
    } catch (...) {
        (void)on1x::push_api_error(state, "unable to inspect List tag");
        return 0;
    }
}

int on1x_payload_of(On1x_State* state, int index) {
    if (!state) return 0;
    on1x::Value value;
    if (!on1x::stack_at(state, index, value)) {
        (void)on1x::push_api_error(state, "Payload.Of received an invalid stack index");
        return 0;
    }
    try {
        return on1x::stack_push(state, on1x::payload_of(&state->gc, state->reserved, value)) ? 1 : 0;
    } catch (...) {
        (void)on1x::push_api_error(state, "unable to inspect List payload");
        return 0;
    }
}

}  // extern "C"
