#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace on1x::utf8 {

struct DecodeResult {
    char32_t code_point = 0;
    std::size_t width = 0;
};

[[nodiscard]] bool is_scalar_value(char32_t code_point) noexcept;
[[nodiscard]] bool validate(std::string_view text) noexcept;
[[nodiscard]] bool decode_next(
    std::string_view text,
    std::size_t offset,
    DecodeResult& result) noexcept;
[[nodiscard]] bool append(std::string& output, char32_t code_point);

}
