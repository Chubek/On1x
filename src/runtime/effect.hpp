#pragma once

#include "core/reserved_tags.hpp"
#include "core/value.hpp"

namespace on1x {
struct GcState;
}

namespace on1x::runtime {

[[nodiscard]] Value capture_success(
    GcState* gc,
    const ReservedTags& tags,
    Value value);

}  // namespace on1x::runtime
