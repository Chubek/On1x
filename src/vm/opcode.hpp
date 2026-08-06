#pragma once

#include <cstdint>

namespace on1x::vm {

enum class Opcode : std::uint8_t {
    Constant,
    LoadGlobal,
    StoreGlobal,
    Add,
    Subtract,
    Multiply,
    Divide,
    Modulo,
    Equal,
    NotEqual,
    Less,
    LessEqual,
    Greater,
    GreaterEqual,
    Not,
    Negate,
    LoadLocal,
    StoreLocal,
    LoadUpvalue,
    MakeClosure,
    Call,
    MakeList,
    MakeTable,
    MakeTaggedList,
    MakeSome,
    MakeNone,
    BeginCapture,
    EndCapture,
    EndEffectScope,
    LoadEffectResult,
    Index,
    Field,
    SetIndex,
    SetField,
    Iota,
    IterInit,
    IterNext,
    IterClose,
    AssertBool,
    MatchPattern,
    MatchFailure,
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
