#pragma once

#include "syntax/ast.hpp"
#include "syntax/source_map.hpp"
#include "util/arena.hpp"

#include <cstddef>
#include <string_view>

namespace on1x::syntax {

class AstBuilder {
public:
    AstBuilder(std::string_view source, Arena& arena) : source_map_(source), arena_(arena) {}

    [[nodiscard]] AstNode* make(
        AstKind kind,
        std::size_t offset,
        std::string_view text = {});
    static void append_child(AstNode*& head, AstNode* child) noexcept;

private:
    SourceMap source_map_;
    Arena& arena_;
};

}  // namespace on1x::syntax
