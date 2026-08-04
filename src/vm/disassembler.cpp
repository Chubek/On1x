#include "vm/chunk.hpp"

namespace on1x::vm {
static_assert(sizeof(Instruction) >= sizeof(std::uint32_t));
}  // namespace on1x::vm
