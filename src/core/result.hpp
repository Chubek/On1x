#pragma once

#include "core/optional.hpp"

namespace on1x {

[[nodiscard]] Value make_success(GcState* gc, const ReservedTags& tags, Value payload);
[[nodiscard]] Value make_error(GcState* gc, const ReservedTags& tags, Value payload);
[[nodiscard]] bool is_success(Value value, const ReservedTags& tags) noexcept;
[[nodiscard]] bool is_error(Value value, const ReservedTags& tags) noexcept;

}  // namespace on1x
