#pragma once

#include "core/value.hpp"

struct On1x_State;

namespace on1x::stdlib {

[[nodiscard]] bool require_arity(
    On1x_State* state,
    int argc,
    int expected,
    const char* member) noexcept;
[[nodiscard]] bool read_argument(
    const On1x_State* state,
    int index,
    Value& value) noexcept;

}  // namespace on1x::stdlib
