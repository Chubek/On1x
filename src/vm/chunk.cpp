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

void Chunk::grow_patterns() {
    const std::size_t capacity = pattern_capacity_ == 0 ? 8 : pattern_capacity_ * 2U;
    auto** patterns = gc_alloc_array<runtime::Pattern*>(gc_, capacity);
    for (std::size_t index = 0; index < pattern_count_; ++index) patterns[index] = patterns_[index];
    patterns_ = patterns;
    pattern_capacity_ = capacity;
}

bool Chunk::add_constant(Value value, std::uint32_t& index) {
    if (constant_count_ == constant_capacity_) grow_constants();
    constants_[constant_count_] = value;
    index = static_cast<std::uint32_t>(constant_count_++);
    return true;
}

bool Chunk::add_pattern(runtime::Pattern* pattern, std::uint32_t& index) {
    if (pattern_count_ == pattern_capacity_) grow_patterns();
    patterns_[pattern_count_] = pattern;
    index = static_cast<std::uint32_t>(pattern_count_++);
    return true;
}

bool Chunk::emit(Opcode opcode, std::uint32_t operand) {
    if (instruction_count_ == instruction_capacity_) grow_instructions();
    instructions_[instruction_count_++] = {opcode, operand};
    return true;
}

void Chunk::patch_operand(std::size_t index, std::uint32_t operand) noexcept {
    instructions_[index].operand = operand;
}

void Chunk::set_signature(
    const std::uint32_t* parameter_bindings,
    std::size_t parameter_count,
    bool variadic,
    std::size_t capture_count) noexcept {
    parameter_bindings_ = parameter_count == 0
        ? nullptr
        : gc_alloc_array<std::uint32_t>(gc_, parameter_count);
    for (std::size_t index = 0; index < parameter_count; ++index) {
        parameter_bindings_[index] = parameter_bindings[index];
    }
    parameter_count_ = parameter_count;
    variadic_ = variadic;
    capture_count_ = capture_count;
}

const Value& Chunk::constant(std::uint32_t index) const noexcept { return constants_[index]; }
const Instruction& Chunk::instruction(std::size_t index) const noexcept { return instructions_[index]; }

}  // namespace on1x::vm
