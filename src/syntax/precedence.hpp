#pragma once

#include <array>
#include <string_view>

namespace on1x::syntax {

enum class Associativity { Left, Right, None };

struct OperatorPrecedence {
    std::string_view text;
    int level;
    Associativity associativity;
};

inline constexpr std::array binary_precedence{
    OperatorPrecedence{"or", 1, Associativity::Left},
    OperatorPrecedence{"and", 2, Associativity::Left},
    OperatorPrecedence{"==", 3, Associativity::Left},
    OperatorPrecedence{"!=", 3, Associativity::Left},
    OperatorPrecedence{"<", 4, Associativity::Left},
    OperatorPrecedence{"<=", 4, Associativity::Left},
    OperatorPrecedence{">", 4, Associativity::Left},
    OperatorPrecedence{">=", 4, Associativity::Left},
    OperatorPrecedence{"..", 5, Associativity::None},
    OperatorPrecedence{"+", 6, Associativity::Left},
    OperatorPrecedence{"-", 6, Associativity::Left},
    OperatorPrecedence{"*", 7, Associativity::Left},
    OperatorPrecedence{"/", 7, Associativity::Left},
    OperatorPrecedence{"%", 7, Associativity::Left},
};

[[nodiscard]] inline const OperatorPrecedence* find_binary_operator(
    std::string_view text) noexcept {
    for (const auto& entry : binary_precedence) {
        if (entry.text == text) return &entry;
    }
    return nullptr;
}

}  // namespace on1x::syntax
