#pragma once
#include <on1x/on1x_types.h>
#include "runtime/state.hpp"

namespace on1x {
[[nodiscard]] On1x_Type api_type(Value value) noexcept;
[[nodiscard]] On1x_Status push_api_error(On1x_State* state, const char* message);
}
