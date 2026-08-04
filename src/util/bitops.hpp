#pragma once

#include <bit>
#include <cstdint>
#include <type_traits>

namespace on1x {

template <typename To, typename From>
[[nodiscard]] constexpr To bit_cast(From value) noexcept {
    static_assert(sizeof(To) == sizeof(From));
    static_assert(std::is_trivially_copyable_v<To>);
    static_assert(std::is_trivially_copyable_v<From>);
    return std::bit_cast<To>(value);
}

[[nodiscard]] constexpr bool is_power_of_two(std::size_t value) noexcept {
    return value != 0 && (value & (value - 1)) == 0;
}

[[nodiscard]] constexpr std::uintptr_t align_up(
    std::uintptr_t value,
    std::size_t alignment) noexcept {
    return (value + alignment - 1) & ~(static_cast<std::uintptr_t>(alignment) - 1);
}

}
