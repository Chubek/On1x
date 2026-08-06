#pragma once

#include "on1x_stdlib.h"
#include "state.hpp"

namespace on1x {

inline On1x_Status open_std(State& state) {
    return on1x_open_std(state.get());
}

}  // namespace on1x
