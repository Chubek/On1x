#pragma once

#include "core/reserved_tags.hpp"
#include "core/tagged_list.hpp"

namespace on1x {

[[nodiscard]] Value make_some(GcState* gc, const ReservedTags& tags, Value value);
[[nodiscard]] Value make_none(GcState* gc, const ReservedTags& tags);
[[nodiscard]] bool is_some(Value value, const ReservedTags& tags) noexcept;
[[nodiscard]] bool is_none(Value value, const ReservedTags& tags) noexcept;
[[nodiscard]] bool unwrap_some(Value value, const ReservedTags& tags, Value& result) noexcept;

}  // namespace on1x
