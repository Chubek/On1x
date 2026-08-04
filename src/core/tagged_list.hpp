#pragma once

#include "core/list.hpp"

namespace on1x {

struct ReservedTags;

[[nodiscard]] ListObject* new_tagged_list(GcState* gc, TagObject* constructor, std::size_t capacity = 0);
[[nodiscard]] Value tag_of(GcState* gc, const ReservedTags& tags, Value value);
[[nodiscard]] Value payload_of(GcState* gc, const ReservedTags& tags, Value value);

}  // namespace on1x
