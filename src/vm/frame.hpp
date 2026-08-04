#pragma once

#include <cstddef>

namespace on1x::vm {

struct Frame {
    std::size_t instruction_pointer = 0;
    std::size_t stack_base = 0;
};

}  // namespace on1x::vm
