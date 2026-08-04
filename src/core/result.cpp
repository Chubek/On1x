#include "core/result.hpp"

#include <exception>

namespace on1x {

namespace {
Value make_result(GcState* gc, TagObject* tag, Value payload) {
    auto* result = new_tagged_list(gc, tag, 1);
    if (!list_push(gc, result, payload)) std::terminate();
    return value_from_object(result);
}
}

Value make_success(GcState* gc, const ReservedTags& tags, Value payload) {
    return make_result(gc, tags.success, payload);
}

Value make_error(GcState* gc, const ReservedTags& tags, Value payload) {
    return make_result(gc, tags.error, payload);
}

bool is_success(Value value, const ReservedTags& tags) noexcept {
    const auto* list = as_list_const(value);
    return list && list->constructor == tags.success && list->length == 1;
}

bool is_error(Value value, const ReservedTags& tags) noexcept {
    const auto* list = as_list_const(value);
    return list && list->constructor == tags.error && list->length == 1;
}

}  // namespace on1x
