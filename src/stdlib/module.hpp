#pragma once

#include <on1x/on1x_module.h>

struct On1x_State;

namespace on1x::stdlib {

[[nodiscard]] bool install_module(
    On1x_State* state,
    const On1x_ModuleDesc& module) noexcept;

}  // namespace on1x::stdlib
