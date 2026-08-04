#include "util/utf8.hpp"

namespace on1x::utf8 {
namespace {

[[nodiscard]] bool is_continuation(unsigned char byte) noexcept {
    return (byte & 0xc0U) == 0x80U;
}

}

bool is_scalar_value(char32_t code_point) noexcept {
    return code_point <= 0x10ffffU &&
           !(code_point >= 0xd800U && code_point <= 0xdfffU);
}

bool decode_next(
    std::string_view text,
    std::size_t offset,
    DecodeResult& result) noexcept {
    result = {};
    if (offset >= text.size()) {
        return false;
    }

    const auto first = static_cast<unsigned char>(text[offset]);
    if (first <= 0x7fU) {
        result = {static_cast<char32_t>(first), 1};
        return true;
    }

    std::size_t width = 0;
    char32_t code_point = 0;
    char32_t minimum = 0;
    if ((first & 0xe0U) == 0xc0U) {
        width = 2;
        code_point = static_cast<char32_t>(first & 0x1fU);
        minimum = 0x80U;
    } else if ((first & 0xf0U) == 0xe0U) {
        width = 3;
        code_point = static_cast<char32_t>(first & 0x0fU);
        minimum = 0x800U;
    } else if ((first & 0xf8U) == 0xf0U) {
        width = 4;
        code_point = static_cast<char32_t>(first & 0x07U);
        minimum = 0x10000U;
    } else {
        return false;
    }

    if (width > text.size() - offset) {
        return false;
    }
    for (std::size_t index = 1; index < width; ++index) {
        const auto byte = static_cast<unsigned char>(text[offset + index]);
        if (!is_continuation(byte)) {
            return false;
        }
        code_point = (code_point << 6U) | static_cast<char32_t>(byte & 0x3fU);
    }

    if (code_point < minimum || !is_scalar_value(code_point)) {
        return false;
    }
    result = {code_point, width};
    return true;
}

bool validate(std::string_view text) noexcept {
    std::size_t offset = 0;
    while (offset < text.size()) {
        DecodeResult result;
        if (!decode_next(text, offset, result)) {
            return false;
        }
        offset += result.width;
    }
    return true;
}

bool append(std::string& output, char32_t code_point) {
    if (!is_scalar_value(code_point)) {
        return false;
    }
    if (code_point <= 0x7fU) {
        output.push_back(static_cast<char>(code_point));
    } else if (code_point <= 0x7ffU) {
        output.push_back(static_cast<char>(0xc0U | (code_point >> 6U)));
        output.push_back(static_cast<char>(0x80U | (code_point & 0x3fU)));
    } else if (code_point <= 0xffffU) {
        output.push_back(static_cast<char>(0xe0U | (code_point >> 12U)));
        output.push_back(static_cast<char>(0x80U | ((code_point >> 6U) & 0x3fU)));
        output.push_back(static_cast<char>(0x80U | (code_point & 0x3fU)));
    } else {
        output.push_back(static_cast<char>(0xf0U | (code_point >> 18U)));
        output.push_back(static_cast<char>(0x80U | ((code_point >> 12U) & 0x3fU)));
        output.push_back(static_cast<char>(0x80U | ((code_point >> 6U) & 0x3fU)));
        output.push_back(static_cast<char>(0x80U | (code_point & 0x3fU)));
    }
    return true;
}

}
