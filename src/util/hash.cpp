#include "util/hash.hpp"

namespace on1x {

std::uint64_t hash_bytes(
    const void* data,
    std::size_t size,
    std::uint64_t seed) noexcept {
    const auto* bytes = static_cast<const unsigned char*>(data);
    std::uint64_t hash = seed;
    for (std::size_t index = 0; index < size; ++index) {
        hash ^= bytes[index];
        hash *= fnv1a_prime;
    }
    return hash;
}

std::uint64_t hash_string(std::string_view text, std::uint64_t seed) noexcept {
    return hash_bytes(text.data(), text.size(), seed);
}

}
