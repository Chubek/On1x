#pragma once

#include <cstdint>

namespace on1x::vm {

enum class Opcode : std::uint8_t {
    Constant,
    LoadGlobal,
    StoreGlobal,
    Add,
    Subtract,
    LoadLocal,
    StoreLocal,
    LoadUpvalue,
    MakeClosure,
    Call,
    JumpIfFalse,
    Jump,
    Pop,
    Return,
};

struct Instruction {
    Opcode opcode{};
    std::uint32_t operand = 0;
};

}  // namespace on1x::vm
