#pragma once

#include "runtime/state.hpp"

namespace on1x::runtime {

[[nodiscard]] FunctionObject* new_native_function(GcState* gc, On1x_CFn native);
[[nodiscard]] FunctionObject* new_closure(
    GcState* gc,
    vm::Chunk* chunk,
    const Value* captures,
    std::size_t capture_count);

}  // namespace on1x::runtime
