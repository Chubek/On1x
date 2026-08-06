#pragma once

#include <on1x/on1x_capability.h>

struct On1x_State;

namespace on1x::stdlib {

[[nodiscard]] bool has_capability(
    const On1x_State* state,
    On1x_Capability capability) noexcept;

}  // namespace on1x::stdlib
