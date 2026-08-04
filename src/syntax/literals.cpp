#include "syntax/literals.hpp"

#include "util/utf8.hpp"

#include <charconv>
#include <cctype>
#include <limits>

namespace on1x::syntax {

namespace {

bool remove_separators(std::string_view source, std::string& destination) {
    destination.clear();
    if (source.empty() || source.front() == '_' || source.back() == '_') return false;
    bool previous_separator = false;
    for (char character : source) {
        if (character == '_') {
            if (previous_separator) return false;
            previous_separator = true;
            continue;
        }
        previous_separator = false;
        destination.push_back(character);
    }
    return true;
}

int digit_value(char character) noexcept {
    if (character >= '0' && character <= '9') return character - '0';
    if (character >= 'a' && character <= 'f') return 10 + character - 'a';
    if (character >= 'A' && character <= 'F') return 10 + character - 'A';
    return -1;
}

}

bool decode_integer(std::string_view text, std::int64_t& value) noexcept {
    std::string compact;
    if (!remove_separators(text, compact)) return false;
    bool negative = false;
    std::size_t offset = 0;
    if (compact[offset] == '+' || compact[offset] == '-') {
        negative = compact[offset] == '-';
        if (++offset == compact.size()) return false;
    }
    int base = 10;
    if (compact.size() - offset >= 2 && compact[offset] == '0') {
        const char prefix = compact[offset + 1];
        if (prefix == 'x' || prefix == 'X') base = 16;
        else if (prefix == 'b' || prefix == 'B') base = 2;
        else if (prefix == 'o' || prefix == 'O') base = 8;
        if (base != 10) offset += 2;
    }
    if (offset == compact.size()) return false;
    std::uint64_t magnitude = 0;
    const std::uint64_t limit = negative
        ? (std::uint64_t{1} << 63)
        : static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max());
    for (; offset < compact.size(); ++offset) {
        const int digit = digit_value(compact[offset]);
        if (digit < 0 || digit >= base) return false;
        if (magnitude > (limit - static_cast<std::uint64_t>(digit)) / static_cast<std::uint64_t>(base)) {
            return false;
        }
        magnitude = magnitude * static_cast<std::uint64_t>(base) + static_cast<std::uint64_t>(digit);
    }
    if (!negative) {
        value = static_cast<std::int64_t>(magnitude);
    } else if (magnitude == (std::uint64_t{1} << 63)) {
        value = std::numeric_limits<std::int64_t>::min();
    } else {
        value = -static_cast<std::int64_t>(magnitude);
    }
    return true;
}

bool decode_float(std::string_view text, double& value) noexcept {
    std::string compact;
    if (!remove_separators(text, compact)) return false;
    const char* begin = compact.data();
    const char* end = begin + compact.size();
    const auto result = std::from_chars(begin, end, value, std::chars_format::general);
    return result.ec == std::errc{} && result.ptr == end;
}

bool decode_string(std::string_view text, std::string& value) {
    if (text.size() < 2 || text.front() != '"' || text.back() != '"') return false;
    value.clear();
    for (std::size_t index = 1; index + 1 < text.size();) {
        const char character = text[index++];
        if (character != '\\') {
            value.push_back(character);
            continue;
        }
        if (index + 1 > text.size()) return false;
        const char escape = text[index++];
        switch (escape) {
        case 'n': value.push_back('\n'); break;
        case 't': value.push_back('\t'); break;
        case 'r': value.push_back('\r'); break;
        case '\\': value.push_back('\\'); break;
        case '"': value.push_back('"'); break;
        case '0': value.push_back('\0'); break;
        case 'u': {
            if (index >= text.size() || text[index++] != '{') return false;
            const std::size_t begin = index;
            while (index + 1 < text.size() && text[index] != '}') ++index;
            if (index == begin || index + 1 >= text.size() || index - begin > 6) return false;
            char32_t code_point = 0;
            for (std::size_t position = begin; position < index; ++position) {
                const int digit = digit_value(text[position]);
                if (digit < 0) return false;
                code_point = static_cast<char32_t>(code_point * 16U + static_cast<char32_t>(digit));
            }
            if (!utf8::append(value, code_point)) return false;
            ++index;
            break;
        }
        default: return false;
        }
    }
    return utf8::validate(value);
}

}  // namespace on1x::syntax
