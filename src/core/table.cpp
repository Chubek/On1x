#include "core/table.hpp"

#include "core/equality.hpp"
#include "core/hashing.hpp"
#include "gc/alloc.hpp"

namespace on1x {

TableObject* new_table(GcState* gc) { return gc_alloc<TableObject>(gc); }

TableObject* as_table(Value value) noexcept {
    return value.kind() == Value::Kind::Table ? static_cast<TableObject*>(value.as_object()) : nullptr;
}

const TableObject* as_table_const(Value value) noexcept { return as_table(value); }

bool table_set(GcState* gc, TableObject* table, Value key, Value value) {
    if (!table || !is_hashable(key)) return false;
    for (TableEntry* entry = table->entries; entry; entry = entry->next) {
        if (value_equals(entry->key, key)) {
            entry->value = value;
            return true;
        }
    }
    auto* entry = gc_alloc<TableEntry>(gc);
    entry->key = key;
    entry->value = value;
    entry->next = table->entries;
    table->entries = entry;
    ++table->length;
    return true;
}

bool table_get(const TableObject* table, Value key, Value& result) {
    if (!table || !is_hashable(key)) return false;
    for (const TableEntry* entry = table->entries; entry; entry = entry->next) {
        if (value_equals(entry->key, key)) {
            result = entry->value;
            return true;
        }
    }
    return false;
}

bool table_remove(TableObject* table, Value key, Value& result) {
    if (!table || !is_hashable(key)) return false;
    TableEntry** current = &table->entries;
    while (*current) {
        if (value_equals((*current)->key, key)) {
            TableEntry* removed = *current;
            *current = removed->next;
            result = removed->value;
            --table->length;
            return true;
        }
        current = &(*current)->next;
    }
    return false;
}

}  // namespace on1x
