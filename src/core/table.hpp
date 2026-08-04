#pragma once

#include "core/list.hpp"

#include <cstddef>

namespace on1x {

struct TableEntry {
    Value key{};
    Value value{};
    TableEntry* next = nullptr;
};

struct TableObject {
    ObjectHeader header{ObjectKind::Table};
    std::size_t length = 0;
    TableEntry* entries = nullptr;
};

[[nodiscard]] TableObject* new_table(GcState* gc);
[[nodiscard]] TableObject* as_table(Value value) noexcept;
[[nodiscard]] const TableObject* as_table_const(Value value) noexcept;
[[nodiscard]] bool table_set(GcState* gc, TableObject* table, Value key, Value value);
[[nodiscard]] bool table_get(const TableObject* table, Value key, Value& result);
[[nodiscard]] bool table_remove(TableObject* table, Value key, Value& result);

}  // namespace on1x
