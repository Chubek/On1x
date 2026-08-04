#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace on1x::syntax {

[[nodiscard]] bool decode_integer(std::string_view text, std::int64_t& value) noexcept;
[[nodiscard]] bool decode_float(std::string_view text, double& value) noexcept;
[[nodiscard]] bool decode_string(std::string_view text, std::string& value);

}  // namespace on1x::syntax
