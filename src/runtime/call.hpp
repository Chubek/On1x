#pragma once

#include "runtime/state.hpp"

namespace on1x::runtime {

[[nodiscard]] bool invoke_native(
    On1x_State* state,
    FunctionObject* function,
    const Value* arguments,
    std::size_t argument_count,
    Value& result,
    const char*& error) noexcept;

}  // namespace on1x::runtime
