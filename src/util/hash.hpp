#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace on1x {

inline constexpr std::uint64_t fnv1a_offset_basis = 14695981039346656037ULL;
inline constexpr std::uint64_t fnv1a_prime = 1099511628211ULL;

[[nodiscard]] std::uint64_t hash_bytes(
    const void* data,
    std::size_t size,
    std::uint64_t seed = fnv1a_offset_basis) noexcept;

[[nodiscard]] std::uint64_t hash_string(
    std::string_view text,
    std::uint64_t seed = fnv1a_offset_basis) noexcept;

[[nodiscard]] constexpr std::uint64_t hash_combine(
    std::uint64_t left,
    std::uint64_t right) noexcept {
    left ^= right + 0x9e3779b97f4a7c15ULL + (left << 6U) + (left >> 2U);
    return left;
}

}
