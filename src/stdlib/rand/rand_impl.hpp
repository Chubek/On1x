#pragma once

#include <on1x/on1x_types.h>

struct On1x_State;

namespace on1x::stdlib::rand_detail {

On1x_Status rand_seed_from_system(On1x_State* s, int argc) noexcept;

}  // namespace on1x::stdlib::rand_detail
