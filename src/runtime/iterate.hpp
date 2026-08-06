#pragma once

#include "core/value.hpp"

namespace on1x {
struct ReservedTags;
}

namespace on1x::runtime {

[[nodiscard]] bool initialize_iteration(
    GcState* gc,
    Value iterable,
    Value& normalized,
    const char*& error) noexcept;

}  // namespace on1x::runtime
