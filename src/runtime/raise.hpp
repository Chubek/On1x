#pragma once

#include "core/reserved_tags.hpp"
#include "core/value.hpp"

#include <string_view>

namespace on1x {
struct GcState;
}

namespace on1x::runtime {

[[nodiscard]] Value capture_error(
    GcState* gc,
    const ReservedTags& tags,
    std::string_view message);

}  // namespace on1x::runtime
