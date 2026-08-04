#pragma once

#include "core/value.hpp"
#include "vm/opcode.hpp"

#include <cstddef>

namespace on1x {
struct GcState;
}

namespace on1x::vm {

class Chunk {
public:
    explicit Chunk(GcState* gc) : gc_(gc) {}

    [[nodiscard]] bool add_constant(Value value, std::uint32_t& index);
    [[nodiscard]] bool emit(Opcode opcode, std::uint32_t operand = 0);
    [[nodiscard]] const Value& constant(std::uint32_t index) const noexcept;
    [[nodiscard]] const Instruction& instruction(std::size_t index) const noexcept;
    [[nodiscard]] std::size_t instruction_count() const noexcept { return instruction_count_; }

private:
    void grow_constants();
    void grow_instructions();

    GcState* gc_;
    Value* constants_ = nullptr;
    std::size_t constant_count_ = 0;
    std::size_t constant_capacity_ = 0;
    Instruction* instructions_ = nullptr;
    std::size_t instruction_count_ = 0;
    std::size_t instruction_capacity_ = 0;
};

}  // namespace on1x::vm
