#include "core/hashing.hpp"
#include "core/list.hpp"
#include "core/optional.hpp"
#include "core/string.hpp"
#include "core/table.hpp"

#include <cstdint>
#include <string_view>

namespace on1x::runtime {

bool index_value(
    GcState* gc,
    const ReservedTags& tags,
    Value container,
    Value key,
    Value& result,
    const char*& error) noexcept {
    try {
        if (const ListObject* list = as_list_const(container)) {
            if (!key.is_int()) {
                error = "List index requires an Int";
                return false;
            }
            Value value;
            const std::int64_t index = key.as_int();
            result = index >= 0 && list_get(list, static_cast<std::size_t>(index), value)
                ? make_some(gc, tags, value)
                : make_none(gc, tags);
            return true;
        }
        if (const TableObject* table = as_table_const(container)) {
            if (!is_hashable(key)) {
                error = "Table keys must be scalar values or Tags";
                return false;
            }
            Value value;
            result = table_get(table, key, value) ? make_some(gc, tags, value) : make_none(gc, tags);
            return true;
        }
        if (const StringObject* string = as_string_const(container)) {
            if (!key.is_int()) {
                error = "String index requires an Int";
                return false;
            }
            std::uint8_t byte = 0;
            const std::int64_t index = key.as_int();
            result = index >= 0 && string_byte_at(string, index, byte)
                ? make_some(gc, tags, Value::integer(gc, byte))
                : make_none(gc, tags);
            return true;
        }
        error = "index expects a List, Table, or String";
        return false;
    } catch (...) {
        error = "unable to index value";
        return false;
    }
}

bool field_value(
    Value container,
    Value key,
    Value& result,
    const char*& error) noexcept {
    const TableObject* table = as_table_const(container);
    if (!table) {
        error = "field access expects a Table";
        return false;
    }
    if (!table_get(table, key, result)) {
        error = "Table field is missing";
        return false;
    }
    return true;
}

bool set_index_value(
    GcState* gc,
    Value container,
    Value key,
    Value value,
    const char*& error) noexcept {
    try {
        if (ListObject* list = as_list(container)) {
            if (!key.is_int()) {
                error = "List index requires an Int";
                return false;
            }
            const std::int64_t index = key.as_int();
            if (index < 0 || !list_set(list, static_cast<std::size_t>(index), value)) {
                error = "List assignment index is out of range";
                return false;
            }
            return true;
        }
        if (TableObject* table = as_table(container)) {
            if (!is_hashable(key)) {
                error = "Table keys must be scalar values or Tags";
                return false;
            }
            if (!table_set(gc, table, key, value)) {
                error = "unable to set Table entry";
                return false;
            }
            return true;
        }
        error = "indexed assignment expects a List or Table";
        return false;
    } catch (...) {
        error = "unable to assign through index";
        return false;
    }
}

bool set_field_value(
    GcState* gc,
    Value container,
    Value key,
    Value value,
    const char*& error) noexcept {
    TableObject* table = as_table(container);
    if (!table) {
        error = "field assignment expects a Table";
        return false;
    }
    try {
        if (!table_set(gc, table, key, value)) {
            error = "unable to set Table field";
            return false;
        }
        return true;
    } catch (...) {
        error = "unable to set Table field";
        return false;
    }
}

}  // namespace on1x::runtime
