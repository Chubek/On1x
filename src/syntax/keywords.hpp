#pragma once

#include <string_view>

namespace on1x::syntax {

[[nodiscard]] bool is_keyword(std::string_view text) noexcept;
[[nodiscard]] bool is_identifier(std::string_view text) noexcept;

}  // namespace on1x::syntax
