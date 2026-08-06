#pragma once

#include <on1x/on1x_module.h>

#include <cstddef>

struct On1x_State;

namespace on1x::stdlib {

struct ModuleEntry {
    const On1x_ModuleDesc* descriptor = nullptr;
};

[[nodiscard]] const ModuleEntry* modules(std::size_t& count) noexcept;
[[nodiscard]] bool install_pure_modules(On1x_State* state) noexcept;

}  // namespace on1x::stdlib
