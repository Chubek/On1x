#include "core/optional.hpp"

#include <exception>

namespace on1x {

Value make_some(GcState* gc, const ReservedTags& tags, Value value) {
    auto* result = new_tagged_list(gc, tags.some, 1);
    if (!list_push(gc, result, value)) std::terminate();
    return value_from_object(result);
}

Value make_none(GcState* gc, const ReservedTags& tags) {
    return value_from_object(new_tagged_list(gc, tags.none));
}

bool is_some(Value value, const ReservedTags& tags) noexcept {
    const auto* list = as_list_const(value);
    return list && list->constructor == tags.some && list->length == 1;
}

bool is_none(Value value, const ReservedTags& tags) noexcept {
    const auto* list = as_list_const(value);
    return list && list->constructor == tags.none && list->length == 0;
}

bool unwrap_some(Value value, const ReservedTags& tags, Value& result) noexcept {
    if (!is_some(value, tags)) return false;
    result = as_list(value)->items[0];
    return true;
}

}  // namespace on1x
