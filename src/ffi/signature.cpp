#include "ffi/signature.hpp"

namespace on1x::ffi {

namespace {

bool decode_type(char code, SignatureType& type) noexcept {
    switch (code) {
    case 'v': type = SignatureType::Void; return true;
    case 'B': type = SignatureType::Bool; return true;
    case 'l': type = SignatureType::Int; return true;
    case 'd': type = SignatureType::Float; return true;
    case 'p': type = SignatureType::Pointer; return true;
    default: return false;
    }
}

}  // namespace

bool parse_signature(std::string_view text, Signature& signature) noexcept {
    const std::size_t separator = text.find(')');
    if (separator == std::string_view::npos || separator + 2U != text.size()) return false;
    SignatureType result;
    if (!decode_type(text.back(), result)) return false;
    if (result == SignatureType::Pointer) return false;
    for (std::size_t index = 0; index < separator; ++index) {
        SignatureType argument;
        if (!decode_type(text[index], argument) || argument == SignatureType::Void) return false;
    }
    signature = Signature{text.data(), separator, result};
    return true;
}

}  // namespace on1x::ffi
