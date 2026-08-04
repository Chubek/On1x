#pragma once

#include <string_view>

namespace on1x::syntax {

[[nodiscard]] bool has_valid_terminators(std::string_view source) noexcept;

}  // namespace on1x::syntax
