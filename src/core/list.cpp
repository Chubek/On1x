#include "core/list.hpp"

#include "gc/alloc.hpp"

namespace on1x {

ListObject* new_list(GcState* gc, std::size_t capacity) {
    auto* list = gc_alloc<ListObject>(gc);
    if (capacity != 0) {
        list->items = gc_alloc_array<Value>(gc, capacity);
        list->capacity = capacity;
    }
    return list;
}

ListObject* as_list(Value value) noexcept {
    return value.kind() == Value::Kind::List ? static_cast<ListObject*>(value.as_object()) : nullptr;
}

const ListObject* as_list_const(Value value) noexcept { return as_list(value); }

bool list_push(GcState* gc, ListObject* list, Value value) {
    if (!list) return false;
    if (list->length == list->capacity) {
        const std::size_t next_capacity = list->capacity == 0 ? 4 : list->capacity * 2;
        Value* items = gc_alloc_array<Value>(gc, next_capacity);
        for (std::size_t index = 0; index < list->length; ++index) items[index] = list->items[index];
        list->items = items;
        list->capacity = next_capacity;
    }
    list->items[list->length++] = value;
    return true;
}

bool list_set(ListObject* list, std::size_t index, Value value) noexcept {
    if (!list || index >= list->length) return false;
    list->items[index] = value;
    return true;
}

bool list_get(const ListObject* list, std::size_t index, Value& result) noexcept {
    if (!list || index >= list->length) return false;
    result = list->items[index];
    return true;
}

Value list_pop(ListObject* list) noexcept {
    return (!list || list->length == 0) ? Value::unit() : list->items[--list->length];
}

}  // namespace on1x
