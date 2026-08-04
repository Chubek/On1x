#include "core/tagged_list.hpp"

#include "core/optional.hpp"

#include <exception>

namespace on1x {

ListObject* new_tagged_list(GcState* gc, TagObject* constructor, std::size_t capacity) {
    auto* list = new_list(gc, capacity);
    list->constructor = constructor;
    return list;
}

Value tag_of(GcState* gc, const ReservedTags& tags, Value value) {
    const auto* list = as_list_const(value);
    return list && list->constructor ? make_some(gc, tags, value_from_object(list->constructor))
                                     : make_none(gc, tags);
}

Value payload_of(GcState* gc, const ReservedTags& tags, Value value) {
    const auto* source = as_list_const(value);
    if (!source || !source->constructor) return make_none(gc, tags);
    auto* payload = new_list(gc, source->length);
    for (std::size_t index = 0; index < source->length; ++index) {
        if (!list_push(gc, payload, source->items[index])) std::terminate();
    }
    return value_from_object(payload);
}

}  // namespace on1x
