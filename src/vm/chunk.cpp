#include "vm/chunk.hpp"

#include "gc/alloc.hpp"

namespace on1x::vm {

void Chunk::grow_constants() {
    const std::size_t capacity = constant_capacity_ == 0 ? 8 : constant_capacity_ * 2U;
    Value* values = gc_alloc_array<Value>(gc_, capacity);
    for (std::size_t index = 0; index < constant_count_; ++index) values[index] = constants_[index];
    constants_ = values;
    constant_capacity_ = capacity;
}

void Chunk::grow_instructions() {
    const std::size_t capacity = instruction_capacity_ == 0 ? 16 : instruction_capacity_ * 2U;
    Instruction* instructions = gc_alloc_array<Instruction>(gc_, capacity);
    for (std::size_t index = 0; index < instruction_count_; ++index) instructions[index] = instructions_[index];
    instructions_ = instructions;
    instruction_capacity_ = capacity;
}

bool Chunk::add_constant(Value value, std::uint32_t& index) {
    if (constant_count_ == constant_capacity_) grow_constants();
    constants_[constant_count_] = value;
    index = static_cast<std::uint32_t>(constant_count_++);
    return true;
}

bool Chunk::emit(Opcode opcode, std::uint32_t operand) {
    if (instruction_count_ == instruction_capacity_) grow_instructions();
    instructions_[instruction_count_++] = {opcode, operand};
    return true;
}

const Value& Chunk::constant(std::uint32_t index) const noexcept { return constants_[index]; }
const Instruction& Chunk::instruction(std::size_t index) const noexcept { return instructions_[index]; }

}  // namespace on1x::vm
