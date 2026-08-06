#pragma once

#include <on1x/on1x_types.h>

struct On1x_State;

namespace on1x::stdlib::time_detail {

On1x_Status time_breakdown(On1x_State* s, int argc) noexcept;
On1x_Status time_format(On1x_State* s, int argc) noexcept;
On1x_Status time_parse(On1x_State* s, int argc) noexcept;

}  // namespace on1x::stdlib::time_detail
