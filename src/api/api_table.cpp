#include "api/api_common.hpp"

#include "core/optional.hpp"
#include "core/table.hpp"
#include "gc/roots.hpp"

extern "C" {

void on1x_new_table(On1x_State* state) {
    if (!state) return;
    try {
        auto* table = on1x::new_table(&state->gc);
        on1x::GcRoot root(table);
        if (!on1x::stack_push(state, on1x::value_from_object(table))) {
            (void)on1x::push_api_error(state, "unable to push Table");
        }
    } catch (...) {
        (void)on1x::push_api_error(state, "unable to create Table");
    }
}

On1x_Status on1x_table_set(On1x_State* state, int table_index) {
    if (!state || on1x::visible_stack_size(state) < 2) {
        return on1x::push_api_error(state, "Table.Set expects a Table, key, and value");
    }

    on1x::Value table_value;
    on1x::Value key;
    on1x::Value value;
    if (!on1x::stack_at(state, table_index, table_value) ||
        !on1x::stack_at(state, -2, key) ||
        !on1x::stack_at(state, -1, value)) {
        return on1x::push_api_error(state, "Table.Set received an invalid stack index");
    }
    auto* table = on1x::as_table(table_value);
    if (!table) return on1x::push_api_error(state, "Table.Set expects a Table");

    --state->top;
    --state->top;
    try {
        on1x::GcRoot root(table);
        if (!on1x::table_set(&state->gc, table, key, value)) {
            return on1x::push_api_error(state, "Table keys must be scalar values or Tags");
        }
        return ON1X_OK;
    } catch (...) {
        return on1x::push_api_error(state, "unable to set Table entry");
    }
}

int on1x_table_get(On1x_State* state, int table_index) {
    if (!state || on1x::visible_stack_size(state) == 0) {
        (void)on1x::push_api_error(state, "Table.Get expects a Table and key");
        return 0;
    }

    on1x::Value table_value;
    on1x::Value key;
    if (!on1x::stack_at(state, table_index, table_value) ||
        !on1x::stack_at(state, -1, key)) {
        (void)on1x::push_api_error(state, "Table.Get received an invalid stack index");
        return 0;
    }
    const auto* table = on1x::as_table_const(table_value);
    if (!table) {
        (void)on1x::push_api_error(state, "Table.Get expects a Table");
        return 0;
    }

    --state->top;
    try {
        on1x::Value value;
        const on1x::Value result = on1x::table_get(table, key, value)
            ? on1x::make_some(&state->gc, state->reserved, value)
            : on1x::make_none(&state->gc, state->reserved);
        return on1x::stack_push(state, result) ? 1 : 0;
    } catch (...) {
        (void)on1x::push_api_error(state, "unable to read Table entry");
        return 0;
    }
}

}  // extern "C"
