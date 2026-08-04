#pragma once

#include "syntax/diagnostics.hpp"

#include <string_view>

namespace on1x::syntax {

enum class AstKind { Unit, Bool, Int, Float, String, Tag, Identifier, List, Table, Binary, Program };

struct AstNode {
    AstKind kind{};
    SourcePosition position{};
    std::string_view text{};
    AstNode* first = nullptr;
    AstNode* next = nullptr;
    AstNode* sibling = nullptr;
};

}  // namespace on1x::syntax
