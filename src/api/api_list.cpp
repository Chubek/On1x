#include "api/api_common.hpp"

#include "core/list.hpp"
#include "core/string.hpp"
#include "core/table.hpp"
#include "gc/roots.hpp"

#include <climits>

extern "C" {

void on1x_new_list(On1x_State* state) {
    if (!state) return;
    try {
        auto* list = on1x::new_list(&state->gc);
        on1x::GcRoot root(list);
        if (!on1x::stack_push(state, on1x::value_from_object(list))) {
            (void)on1x::push_api_error(state, "unable to push List");
        }
    } catch (...) {
        (void)on1x::push_api_error(state, "unable to create List");
    }
}

On1x_Status on1x_list_push(On1x_State* state, int list_index) {
    if (!state || on1x::visible_stack_size(state) == 0) {
        return on1x::push_api_error(state, "List.Push expects a List and a value");
    }

    on1x::Value list_value;
    on1x::Value item;
    if (!on1x::stack_at(state, list_index, list_value) ||
        !on1x::stack_at(state, -1, item)) {
        return on1x::push_api_error(state, "List.Push received an invalid stack index");
    }
    auto* list = on1x::as_list(list_value);
    if (!list) return on1x::push_api_error(state, "List.Push expects a List");

    try {
        on1x::GcRoot root(list);
        if (!on1x::list_push(&state->gc, list, item)) {
            return on1x::push_api_error(state, "unable to append to List");
        }
        --state->top;
        return ON1X_OK;
    } catch (...) {
        return on1x::push_api_error(state, "unable to append to List");
    }
}

int on1x_list_get(On1x_State* state, int list_index, int element_index) {
    on1x::Value value;
    on1x::Value result;
    if (!on1x::stack_at(state, list_index, value)) {
        (void)on1x::push_api_error(state, "List.Get received an invalid stack index");
        return 0;
    }
    const auto* list = on1x::as_list_const(value);
    if (!list) {
        (void)on1x::push_api_error(state, "List.Get expects a List");
        return 0;
    }
    if (element_index < 0 ||
        !on1x::list_get(list, static_cast<std::size_t>(element_index), result)) {
        return 0;
    }
    return on1x::stack_push(state, result) ? 1 : 0;
}

int on1x_len(const On1x_State* state, int index) {
    on1x::Value value;
    if (!on1x::stack_at(state, index, value)) return -1;
    if (const auto* list = on1x::as_list_const(value)) {
        return list->length <= static_cast<std::size_t>(INT_MAX)
            ? static_cast<int>(list->length)
            : -1;
    }
    if (const auto* table = on1x::as_table_const(value)) {
        return table->length <= static_cast<std::size_t>(INT_MAX)
            ? static_cast<int>(table->length)
            : -1;
    }
    if (const auto* string = on1x::as_string_const(value)) {
        return string->bytes <= static_cast<std::size_t>(INT_MAX)
            ? static_cast<int>(string->bytes)
            : -1;
    }
    return -1;
}

}  // extern "C"
