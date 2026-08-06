#include "prelude/prelude.hpp"

#include "core/hashing.hpp"
#include "core/list.hpp"
#include "core/optional.hpp"
#include "core/string.hpp"
#include "core/table.hpp"
#include "gc/roots.hpp"

#include <cstdint>

namespace on1x::prelude {

On1x_Status length(On1x_State* state, int argc) {
    if (!has_arity(state, argc, 1, "Len expects one argument")) return ON1X_ERR;
    Value value;
    if (!argument(state, 1, value)) return raise(state, "Len received an invalid argument");
    if (const ListObject* list = as_list_const(value)) {
        return push_result(state, Value::integer(&state->gc, static_cast<std::int64_t>(list->length)));
    }
    if (const TableObject* table = as_table_const(value)) {
        return push_result(state, Value::integer(&state->gc, static_cast<std::int64_t>(table->length)));
    }
    bool valid = false;
    const std::size_t bytes = string_length(value, valid);
    if (valid) return push_result(state, Value::integer(&state->gc, static_cast<std::int64_t>(bytes)));
    return raise(state, "Len expects a List, Table, or String");
}

On1x_Status get(On1x_State* state, int argc) {
    if (!has_arity(state, argc, 2, "Get expects two arguments")) return ON1X_ERR;
    Value container;
    Value key;
    if (!argument(state, 1, container) || !argument(state, 2, key)) {
        return raise(state, "Get received an invalid argument");
    }
    try {
        if (const ListObject* list = as_list_const(container)) {
            if (!key.is_int()) return raise(state, "List index requires an Int");
            Value result;
            const std::int64_t index = key.as_int();
            return push_result(
                state,
                index >= 0 && list_get(list, static_cast<std::size_t>(index), result)
                    ? make_some(&state->gc, state->reserved, result)
                    : make_none(&state->gc, state->reserved));
        }
        if (const TableObject* table = as_table_const(container)) {
            if (!is_hashable(key)) return raise(state, "Table keys must be scalar values or Tags");
            Value result;
            return push_result(
                state,
                table_get(table, key, result)
                    ? make_some(&state->gc, state->reserved, result)
                    : make_none(&state->gc, state->reserved));
        }
        if (const StringObject* string = as_string_const(container)) {
            if (!key.is_int()) return raise(state, "String index requires an Int");
            std::uint8_t byte = 0;
            const std::int64_t index = key.as_int();
            return push_result(
                state,
                index >= 0 && string_byte_at(string, index, byte)
                    ? make_some(&state->gc, state->reserved, Value::integer(&state->gc, byte))
                    : make_none(&state->gc, state->reserved));
        }
    } catch (...) {
        return raise(state, "unable to read value");
    }
    return raise(state, "Get expects a List, Table, or String");
}

On1x_Status set(On1x_State* state, int argc) {
    if (!has_arity(state, argc, 3, "Set expects three arguments")) return ON1X_ERR;
    Value container;
    Value key;
    Value value;
    if (!argument(state, 1, container) || !argument(state, 2, key) || !argument(state, 3, value)) {
        return raise(state, "Set received an invalid argument");
    }
    try {
        if (ListObject* list = as_list(container)) {
            if (!key.is_int()) return raise(state, "List index requires an Int");
            const std::int64_t index = key.as_int();
            if (index < 0 || !list_set(list, static_cast<std::size_t>(index), value)) {
                return raise(state, "List assignment index is out of range");
            }
            return push_result(state, Value::unit());
        }
        if (TableObject* table = as_table(container)) {
            GcRoot table_root(table);
            if (!is_hashable(key)) return raise(state, "Table keys must be scalar values or Tags");
            if (!table_set(&state->gc, table, key, value)) return raise(state, "unable to set Table entry");
            return push_result(state, Value::unit());
        }
    } catch (...) {
        return raise(state, "unable to set value");
    }
    return raise(state, "Set expects a List or Table");
}

On1x_Status push(On1x_State* state, int argc) {
    if (!has_arity(state, argc, 2, "Push expects two arguments")) return ON1X_ERR;
    Value list_value;
    Value value;
    if (!argument(state, 1, list_value) || !argument(state, 2, value)) {
        return raise(state, "Push received an invalid argument");
    }
    ListObject* list = as_list(list_value);
    if (!list) return raise(state, "Push expects a List");
    try {
        GcRoot list_root(list);
        if (!list_push(&state->gc, list, value)) return raise(state, "unable to append to List");
        return push_result(state, Value::unit());
    } catch (...) {
        return raise(state, "unable to append to List");
    }
}

On1x_Status pop(On1x_State* state, int argc) {
    if (!has_arity(state, argc, 1, "Pop expects one argument")) return ON1X_ERR;
    Value list_value;
    if (!argument(state, 1, list_value)) return raise(state, "Pop received an invalid argument");
    ListObject* list = as_list(list_value);
    if (!list) return raise(state, "Pop expects a List");
    try {
        if (list->length == 0) return push_result(state, make_none(&state->gc, state->reserved));
        return push_result(state, make_some(&state->gc, state->reserved, list_pop(list)));
    } catch (...) {
        return raise(state, "unable to pop List");
    }
}

}  // namespace on1x::prelude
