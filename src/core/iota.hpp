#pragma once

#include "core/optional.hpp"

#include <initializer_list>

namespace on1x {

[[nodiscard]] Value make_iota(GcState* gc, const ReservedTags& tags, std::initializer_list<Value> arguments);

}  // namespace on1x
