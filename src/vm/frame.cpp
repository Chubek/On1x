#include "vm/frame.hpp"

namespace on1x::vm {
static_assert(sizeof(Frame) >= sizeof(std::size_t) * 2U);
}  // namespace on1x::vm
