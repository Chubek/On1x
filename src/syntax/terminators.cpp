#include "syntax/terminators.hpp"

namespace on1x::syntax {

bool has_valid_terminators(std::string_view source) noexcept {
    int brackets = 0;
    bool in_string = false;
    bool escaped = false;
    for (const char character : source) {
        if (in_string) {
            if (!escaped && character == '"') in_string = false;
            escaped = !escaped && character == '\\';
            if (character != '\\') escaped = false;
            continue;
        }
        if (character == '"') { in_string = true; continue; }
        if (character == '(' || character == '[' || character == '{') ++brackets;
        if (character == ')' || character == ']' || character == '}') {
            if (--brackets < 0) return false;
        }
    }
    return !in_string && brackets == 0;
}

}  // namespace on1x::syntax
