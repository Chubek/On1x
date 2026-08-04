#include "syntax/parser.hpp"

#include "syntax/keywords.hpp"
#include "syntax/literals.hpp"
#include "syntax/source_map.hpp"

#include <cctype>

namespace on1x::syntax {

Parser::Parser(std::string_view source, Arena& arena, Diagnostics& diagnostics)
    : source_(source), arena_(arena), diagnostics_(diagnostics) {}

AstNode* Parser::node(AstKind kind, std::size_t offset, std::string_view text) {
    return arena_.make<AstNode>(AstNode{kind, SourceMap(source_).position(offset), text});
}

void Parser::skip_trivia() {
    while (offset_ < source_.size() && std::isspace(static_cast<unsigned char>(source_[offset_]))) ++offset_;
}

bool Parser::consume(char character) {
    skip_trivia();
    if (offset_ < source_.size() && source_[offset_] == character) { ++offset_; return true; }
    return false;
}

AstNode* Parser::parse_atom() {
    skip_trivia();
    const std::size_t begin = offset_;
    if (consume('(')) {
        if (!consume(')')) {
            diagnostics_.add(SourceMap(source_).position(begin), "expected ')' for Unit literal");
            return nullptr;
        }
        return node(AstKind::Unit, begin, "()");
    }
    if (offset_ >= source_.size()) return nullptr;
    if (source_[offset_] == '"') {
        ++offset_;
        bool escaped = false;
        while (offset_ < source_.size() && (escaped || source_[offset_] != '"')) {
            escaped = !escaped && source_[offset_] == '\\';
            if (source_[offset_] != '\\') escaped = false;
            ++offset_;
        }
        if (offset_ == source_.size()) {
            diagnostics_.add(SourceMap(source_).position(begin), "unterminated string literal");
            return nullptr;
        }
        ++offset_;
        return node(AstKind::String, begin, source_.substr(begin, offset_ - begin));
    }
    if (source_[offset_] == ':') {
        ++offset_;
        const std::size_t tag_begin = offset_;
        while (offset_ < source_.size() && (std::isalnum(static_cast<unsigned char>(source_[offset_])) || source_[offset_] == '_')) ++offset_;
        if (tag_begin == offset_) {
            diagnostics_.add(SourceMap(source_).position(begin), "expected Tag name after ':'");
            return nullptr;
        }
        return node(AstKind::Tag, begin, source_.substr(begin + 1U, offset_ - begin - 1U));
    }
    while (offset_ < source_.size() && !std::isspace(static_cast<unsigned char>(source_[offset_])) &&
           source_[offset_] != ';' && source_[offset_] != ',' && source_[offset_] != ']' &&
           source_[offset_] != '+' && source_[offset_] != '-') ++offset_;
    const std::string_view token = source_.substr(begin, offset_ - begin);
    if (token == "true" || token == "false") return node(AstKind::Bool, begin, token);
    std::int64_t integer = 0;
    if (decode_integer(token, integer)) return node(AstKind::Int, begin, token);
    double floating = 0.0;
    if (decode_float(token, floating)) return node(AstKind::Float, begin, token);
    if (is_identifier(token)) return node(AstKind::Identifier, begin, token);
    diagnostics_.add(SourceMap(source_).position(begin), "expected expression");
    return nullptr;
}

AstNode* Parser::parse_expression() {
    AstNode* left = parse_atom();
    if (!left) return nullptr;
    skip_trivia();
    if (offset_ < source_.size() && (source_[offset_] == '+' || source_[offset_] == '-')) {
        const std::size_t operator_offset = offset_++;
        AstNode* right = parse_expression();
        if (!right) return nullptr;
        AstNode* result = node(AstKind::Binary, operator_offset, source_.substr(operator_offset, 1));
        result->first = left;
        left->next = right;
        return result;
    }
    return left;
}

AstNode* Parser::parse_program() {
    AstNode* program = node(AstKind::Program, 0);
    AstNode** tail = &program->first;
    while (true) {
        skip_trivia();
        if (offset_ == source_.size()) break;
        AstNode* expression = parse_expression();
        if (!expression) return nullptr;
        *tail = expression;
        tail = &expression->sibling;
        skip_trivia();
        if (offset_ < source_.size() && source_[offset_] == ';') ++offset_;
    }
    return program;
}

}  // namespace on1x::syntax
