#include "util/utf8.hpp"

#include <utf8proc.h>

#include <limits>

namespace on1x::utf8 {
namespace {

[[nodiscard]] bool can_pass_to_utf8proc(std::size_t size) noexcept {
    return size <= static_cast<std::size_t>(
                       std::numeric_limits<utf8proc_ssize_t>::max());
}

}

bool is_scalar_value(char32_t code_point) noexcept {
    return code_point <= static_cast<char32_t>(std::numeric_limits<utf8proc_int32_t>::max()) &&
           utf8proc_codepoint_valid(static_cast<utf8proc_int32_t>(code_point)) != 0;
}

bool decode_next(
    std::string_view text,
    std::size_t offset,
    DecodeResult& result) noexcept {
    result = {};
    if (offset >= text.size() || !can_pass_to_utf8proc(text.size() - offset)) {
        return false;
    }

    utf8proc_int32_t code_point = 0;
    const auto width = utf8proc_iterate(
        reinterpret_cast<const utf8proc_uint8_t*>(text.data() + offset),
        static_cast<utf8proc_ssize_t>(text.size() - offset),
        &code_point);
    if (width <= 0) {
        return false;
    }
    result = {
        static_cast<char32_t>(code_point),
        static_cast<std::size_t>(width),
    };
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

bool codepoint_count(std::string_view text, std::size_t& count) noexcept {
    count = 0;
    std::size_t offset = 0;
    while (offset < text.size()) {
        DecodeResult result;
        if (!decode_next(text, offset, result)) {
            count = 0;
            return false;
        }
        offset += result.width;
        ++count;
    }
    return true;
}

bool byte_offset_for_codepoint(
    std::string_view text,
    std::size_t index,
    std::size_t& offset) noexcept {
    offset = 0;
    for (std::size_t current = 0; current < index; ++current) {
        DecodeResult result;
        if (!decode_next(text, offset, result)) {
            offset = 0;
            return false;
        }
        offset += result.width;
    }
    if (offset < text.size()) {
        DecodeResult result;
        if (!decode_next(text, offset, result)) {
            offset = 0;
            return false;
        }
    }
    return true;
}

bool decode_codepoint_at(
    std::string_view text,
    std::size_t index,
    DecodeResult& result) noexcept {
    std::size_t offset = 0;
    if (!byte_offset_for_codepoint(text, index, offset) || offset == text.size()) {
        result = {};
        return false;
    }
    return decode_next(text, offset, result);
}

bool append(std::string& output, char32_t code_point) {
    if (!is_scalar_value(code_point)) {
        return false;
    }
    utf8proc_uint8_t encoded[4] = {};
    const auto width = utf8proc_encode_char(
        static_cast<utf8proc_int32_t>(code_point),
        encoded);
    if (width <= 0) {
        return false;
    }
    output.append(
        reinterpret_cast<const char*>(encoded),
        static_cast<std::size_t>(width));
    return true;
}

}
