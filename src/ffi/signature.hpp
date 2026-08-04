#pragma once

#include <string_view>
#include <cstddef>

namespace on1x::ffi {

enum class SignatureType {
    Void,
    Bool,
    Int,
    Float,
    Pointer,
};

struct Signature {
    const char* arguments = nullptr;
    std::size_t argument_count = 0;
    SignatureType result = SignatureType::Void;
};

// Validates a restricted dyncall signature: scalar arguments, one ')' separator,
// and a scalar return type. Structs, callbacks, and calling-convention prefixes
// are deliberately rejected until their ownership and lifetime rules are exposed.
[[nodiscard]] bool parse_signature(std::string_view text, Signature& signature) noexcept;

}  // namespace on1x::ffi
