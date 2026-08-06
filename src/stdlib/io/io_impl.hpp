#pragma once

#include <on1x/on1x_types.h>

struct On1x_State;

namespace on1x::stdlib::io_detail {

On1x_Status io_readline(On1x_State* s, int argc) noexcept;
On1x_Status io_readall(On1x_State* s, int argc) noexcept;
On1x_Status io_show(On1x_State* s, int argc) noexcept;
On1x_Status io_repr(On1x_State* s, int argc) noexcept;

}  // namespace on1x::stdlib::io_detail
