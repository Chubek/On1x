#pragma once

#include "syntax/ast.hpp"
#include "syntax/diagnostics.hpp"
#include "util/small_vector.hpp"

#include <cstdint>
#include <deque>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace on1x::ir {

using Register = std::uint32_t;
using BlockId = std::uint32_t;
inline constexpr Register no_register = UINT32_MAX;

enum class PatternKind {
    Literal,
    Wildcard,
    Binding,
    List,
    TaggedList,
};

struct Pattern {
    PatternKind kind{};
    syntax::AstKind literal_kind{};
    std::string text;
    std::vector<Pattern> children;
    std::uint32_t binding = 0;
    bool has_tail = false;
    std::uint32_t tail_binding = 0;
};

enum class Opcode {
    Unit,
    Bool,
    Int,
    Float,
    String,
    Tag,
    LoadGlobal,
    StoreGlobal,
    Unary,
    Binary,
    MakeList,
    MakeTable,
    MakeTaggedList,
    Call,
    Index,
    Field,
    SetIndex,
    SetField,
    Some,
    None,
    EffectResult,
    BeginCapture,
    EndCapture,
    EndEffectScope,
    Discard,
    LoadLocal,
    StoreLocal,
    BranchIfFalse,
    Jump,
    Phi,
    LoadUpvalue,
    MakeFunction,
    IterInit,
    IterNext,
    IterClose,
    RequireBool,
    MatchPattern,
    MatchFailure,
    Return,
};

struct Instruction {
    Opcode opcode{};
    Register result = no_register;
    SmallVector<Register, 3> operands;
    std::string_view text;
    syntax::SourcePosition position{};
    BlockId target = 0;
    std::uint32_t binding = 0;
    std::int64_t integer_value = 0;
    bool has_integer_value = false;
    std::shared_ptr<Pattern> pattern;

    [[nodiscard]] bool has_result() const noexcept { return result != no_register; }
    [[nodiscard]] bool has_side_effects() const noexcept;
};

struct BasicBlock {
    BlockId id = 0;
    std::vector<Instruction> instructions;
};

struct Function {
    std::string name;
    std::vector<BasicBlock> blocks;
    Register register_count = 0;
    std::vector<std::uint32_t> parameter_bindings;
    std::vector<std::uint32_t> capture_bindings;
    bool variadic = false;
};

struct Module {
    Function entry = [] {
        Function function;
        function.name = "<chunk>";
        function.blocks = {{0, {}}};
        return function;
    }();
    std::deque<Function> functions;

    [[nodiscard]] Function& function(BlockId id) {
        return id == 0 ? entry : functions.at(static_cast<std::size_t>(id - 1U));
    }
    [[nodiscard]] const Function& function(BlockId id) const {
        return id == 0 ? entry : functions.at(static_cast<std::size_t>(id - 1U));
    }
};

[[nodiscard]] std::string print(const Module& module);

}  // namespace on1x::ir
