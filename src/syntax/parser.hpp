#pragma once

#include "syntax/ast.hpp"
#include "syntax/diagnostics.hpp"
#include "util/arena.hpp"

#include <string_view>

namespace on1x::syntax {

class Parser {
public:
    Parser(std::string_view source, Arena& arena, Diagnostics& diagnostics);
    [[nodiscard]] AstNode* parse_program();

private:
    [[nodiscard]] AstNode* parse_expression();
    [[nodiscard]] AstNode* parse_atom();
    void skip_trivia();
    [[nodiscard]] bool consume(char character);
    [[nodiscard]] AstNode* node(AstKind kind, std::size_t offset, std::string_view text = {});

    std::string_view source_;
    Arena& arena_;
    Diagnostics& diagnostics_;
    std::size_t offset_ = 0;
};

}  // namespace on1x::syntax
