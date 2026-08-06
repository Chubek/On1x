#pragma once

#include <on1x/on1x_module.h>

namespace on1x::stdlib {

[[nodiscard]] const On1x_ModuleDesc* fs_module() noexcept;
[[nodiscard]] const On1x_ModuleDesc* path_module() noexcept;

}  // namespace on1x::stdlib
