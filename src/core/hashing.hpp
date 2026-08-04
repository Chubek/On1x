#pragma once

#include "core/value.hpp"

#include <cstdint>

namespace on1x {

[[nodiscard]] bool is_hashable(Value value) noexcept;
[[nodiscard]] std::uint64_t value_hash(Value value);

}  // namespace on1x
