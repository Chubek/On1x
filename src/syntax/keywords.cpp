#include "syntax/keywords.hpp"

#include <array>

namespace on1x::syntax {

bool is_keyword(std::string_view text) noexcept {
    constexpr std::array keywords{
        "let", "fn", "if", "else", "while", "for", "in", "match", "return",
        "break", "continue", "enum", "and", "or", "not", "true", "false",
    };
    for (const auto keyword : keywords) {
        if (text == keyword) return true;
    }
    return false;
}

bool is_identifier(std::string_view text) noexcept {
    if (text.empty() || is_keyword(text)) return false;
    const auto letter_or_underscore = [](char character) {
        return character == '_' || (character >= 'a' && character <= 'z') ||
            (character >= 'A' && character <= 'Z');
    };
    if (!letter_or_underscore(text.front())) return false;
    for (const char character : text.substr(1)) {
        if (!letter_or_underscore(character) && !(character >= '0' && character <= '9')) return false;
    }
    return true;
}

}  // namespace on1x::syntax
