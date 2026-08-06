#pragma once

#include "core/value.hpp"
#include "vm/chunk.hpp"

struct On1x_State;
struct FunctionObject;

namespace on1x::vm {

[[nodiscard]] bool execute(On1x_State* state, const Chunk& chunk, Value& result, const char*& error) noexcept;

// Call an On1x function (native or bytecode) from native code.
// Returns the result in `result` and sets `error` on failure.
[[nodiscard]] bool invoke_function(
    On1x_State* state,
    FunctionObject* function,
    const Value* arguments,
    std::size_t argument_count,
    Value& result,
    const char*& error) noexcept;

}  // namespace on1x::vm
