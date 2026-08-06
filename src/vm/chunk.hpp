#pragma once

#include "core/value.hpp"
#include "runtime/pattern_match.hpp"
#include "vm/opcode.hpp"

#include <cstddef>
#include <string>

namespace on1x {
struct GcState;
}

namespace on1x::vm {

class Chunk {
public:
    explicit Chunk(GcState* gc) : gc_(gc) {}

    [[nodiscard]] bool add_constant(Value value, std::uint32_t& index);
    [[nodiscard]] bool add_pattern(runtime::Pattern* pattern, std::uint32_t& index);
    [[nodiscard]] bool emit(Opcode opcode, std::uint32_t operand = 0);
    void patch_operand(std::size_t index, std::uint32_t operand) noexcept;
    [[nodiscard]] const Value& constant(std::uint32_t index) const noexcept;
    [[nodiscard]] std::size_t constant_count() const noexcept { return constant_count_; }
    [[nodiscard]] const Instruction& instruction(std::size_t index) const noexcept;
    [[nodiscard]] std::size_t instruction_count() const noexcept { return instruction_count_; }
    void set_local_count(std::size_t count) noexcept { local_count_ = count; }
    [[nodiscard]] std::size_t local_count() const noexcept { return local_count_; }
    void set_signature(
        const std::uint32_t* parameter_bindings,
        std::size_t parameter_count,
        bool variadic,
        std::size_t capture_count) noexcept;
    [[nodiscard]] std::size_t parameter_count() const noexcept { return parameter_count_; }
    [[nodiscard]] bool variadic() const noexcept { return variadic_; }
    [[nodiscard]] std::uint32_t parameter_binding(std::size_t index) const noexcept {
        return parameter_bindings_[index];
    }
    [[nodiscard]] std::size_t capture_count() const noexcept { return capture_count_; }
    void set_functions(Chunk** functions, std::size_t count) noexcept {
        functions_ = functions;
        function_count_ = count;
    }
    [[nodiscard]] Chunk* function(std::size_t index) const noexcept {
        return index < function_count_ ? functions_[index] : nullptr;
    }
    [[nodiscard]] const runtime::Pattern* pattern(std::uint32_t index) const noexcept {
        return index < pattern_count_ ? patterns_[index] : nullptr;
    }

private:
    void grow_constants();
    void grow_patterns();
    void grow_instructions();

    GcState* gc_;
    Value* constants_ = nullptr;
    std::size_t constant_count_ = 0;
    std::size_t constant_capacity_ = 0;
    runtime::Pattern** patterns_ = nullptr;
    std::size_t pattern_count_ = 0;
    std::size_t pattern_capacity_ = 0;
    Instruction* instructions_ = nullptr;
    std::size_t instruction_count_ = 0;
    std::size_t instruction_capacity_ = 0;
    std::size_t local_count_ = 0;
    std::uint32_t* parameter_bindings_ = nullptr;
    std::size_t parameter_count_ = 0;
    bool variadic_ = false;
    std::size_t capture_count_ = 0;
    Chunk** functions_ = nullptr;
    std::size_t function_count_ = 0;
};

[[nodiscard]] std::string disassemble(const Chunk& chunk);

}  // namespace on1x::vm
