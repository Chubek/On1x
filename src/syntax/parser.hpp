#pragma once

#include "syntax/ast.hpp"
#include "syntax/ast_builder.hpp"
#include "syntax/diagnostics.hpp"
#include "util/arena.hpp"

#include <cstddef>
#include <string_view>

namespace on1x::syntax {

class Parser {
public:
    Parser(std::string_view source, Arena& arena, Diagnostics& diagnostics);
    [[nodiscard]] AstNode* parse_program();

private:
    [[nodiscard]] AstNode* parse_statement();
    [[nodiscard]] AstNode* parse_terminated_statement();
    [[nodiscard]] AstNode* parse_expression(int minimum_precedence = 1);
    [[nodiscard]] AstNode* parse_unary();
    [[nodiscard]] AstNode* parse_postfix();
    [[nodiscard]] AstNode* parse_primary();
    [[nodiscard]] AstNode* parse_block(std::size_t begin);
    [[nodiscard]] AstNode* parse_if(std::size_t begin);
    [[nodiscard]] AstNode* parse_function(std::size_t begin);
    [[nodiscard]] AstNode* parse_return(std::size_t begin);
    [[nodiscard]] AstNode* parse_list(std::size_t begin);
    [[nodiscard]] AstNode* parse_table(std::size_t begin);
    [[nodiscard]] AstNode* parse_tag(std::size_t begin);
    [[nodiscard]] AstNode* parse_delimited_expression(char closing, std::string_view context);
    [[nodiscard]] std::string_view peek_binary_operator();
    [[nodiscard]] std::string_view scan_identifier();
    [[nodiscard]] std::string_view scan_number();
    [[nodiscard]] std::string_view scan_string();
    void skip_trivia(bool allow_newlines);
    void consume_terminators();
    [[nodiscard]] bool consume(char character, bool allow_newlines = false);
    [[nodiscard]] bool consume(std::string_view text, bool allow_newlines = false);
    [[nodiscard]] bool at_statement_end() const noexcept;
    [[nodiscard]] bool at_word(std::string_view word) const noexcept;
    [[nodiscard]] static bool is_lvalue(const AstNode* node) noexcept;
    void error(std::size_t offset, std::string message);

    std::string_view source_;
    Arena& arena_;
    Diagnostics& diagnostics_;
    AstBuilder builder_;
    std::size_t offset_ = 0;
    std::size_t nesting_depth_ = 0;
    bool logical_newline_ = false;
    bool failed_ = false;
};

}  // namespace on1x::syntax
