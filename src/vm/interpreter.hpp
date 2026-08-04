#pragma once

#include "core/value.hpp"
#include "vm/chunk.hpp"

struct On1x_State;

namespace on1x::vm {

[[nodiscard]] bool execute(On1x_State* state, const Chunk& chunk, Value& result, const char*& error) noexcept;

}  // namespace on1x::vm
