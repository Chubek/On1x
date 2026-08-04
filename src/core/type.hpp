#pragma once

#include "core/reserved_tags.hpp"

namespace on1x {

[[nodiscard]] Value type_of(Value value, const ReservedTags& tags) noexcept;

}  // namespace on1x
