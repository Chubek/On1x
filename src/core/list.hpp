#pragma once

#include "core/tag_table.hpp"

#include <cstddef>

namespace on1x {

struct ListObject {
    ObjectHeader header{ObjectKind::List};
    TagObject* constructor = nullptr;
    std::size_t length = 0;
    std::size_t capacity = 0;
    Value* items = nullptr;
};

[[nodiscard]] ListObject* new_list(GcState* gc, std::size_t capacity = 0);
[[nodiscard]] ListObject* as_list(Value value) noexcept;
[[nodiscard]] const ListObject* as_list_const(Value value) noexcept;
[[nodiscard]] bool list_push(GcState* gc, ListObject* list, Value value);
[[nodiscard]] bool list_set(ListObject* list, std::size_t index, Value value) noexcept;
[[nodiscard]] bool list_get(const ListObject* list, std::size_t index, Value& result) noexcept;
[[nodiscard]] Value list_pop(ListObject* list) noexcept;

}  // namespace on1x
