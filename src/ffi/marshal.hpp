#pragma once

#include "core/value.hpp"
#include "ffi/signature.hpp"

namespace on1x {
struct GcState;
}

struct DCCallVM_;

namespace on1x::ffi {

[[nodiscard]] bool push_argument(DCCallVM_* vm, SignatureType type, Value value) noexcept;
[[nodiscard]] Value read_result(GcState* gc, DCCallVM_* vm, void* function, SignatureType type);

}  // namespace on1x::ffi
