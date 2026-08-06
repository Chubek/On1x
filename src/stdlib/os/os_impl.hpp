#pragma once

#include <on1x/on1x_types.h>

struct On1x_State;

namespace on1x::stdlib::os_detail {

On1x_Status os_getenv(On1x_State* s, int argc) noexcept;
On1x_Status os_setenv(On1x_State* s, int argc) noexcept;
On1x_Status os_unsetenv(On1x_State* s, int argc) noexcept;
On1x_Status os_envtable(On1x_State* s, int argc) noexcept;

}  // namespace on1x::stdlib::os_detail
