#pragma once

#include "syntax/diagnostics.hpp"

#include <cstdint>
#include <string>
#include <string_view>

namespace on1x::syntax {

enum class AstKind {
    Unit,
    Bool,
    Int,
    Float,
    String,
    Tag,
    Identifier,
    List,
    Table,
    TableEntry,
    TaggedList,
    Unary,
    Binary,
    Call,
    Index,
    Field,
    Optional,
    EffectResult,
    Let,
    Assign,
    Effect,
    Block,
    If,
    Function,
    Parameter,
    Return,
    Program,
};

enum class ResolutionKind {
    Unresolved,
    Global,
    Local,
    Upvalue,
};

struct AstNode {
    AstKind kind{};
    SourcePosition position{};
    std::string_view text{};
    AstNode* first = nullptr;
    AstNode* next = nullptr;
    AstNode* sibling = nullptr;
    ResolutionKind resolution = ResolutionKind::Unresolved;
    std::uint32_t binding_index = 0;
    std::uint32_t lexical_depth = 0;
    std::uint32_t function_index = 0;
    std::uint32_t source_binding_index = 0;
    AstNode* captures = nullptr;
    bool variadic = false;
};

[[nodiscard]] std::string print_ast(const AstNode* node);

}  // namespace on1x::syntax
