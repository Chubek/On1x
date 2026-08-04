#include "core/hashing.hpp"

#include "core/string.hpp"
#include "core/tag_table.hpp"
#include "util/hash.hpp"

#include <bit>
#include <stdexcept>

namespace on1x {

bool is_hashable(Value value) noexcept {
    switch (value.kind()) {
    case Value::Kind::Unit:
    case Value::Kind::Bool:
    case Value::Kind::Int:
    case Value::Kind::Float:
    case Value::Kind::String:
    case Value::Kind::Tag:
    case Value::Kind::Iota:
        return true;
    default:
        return false;
    }
}

std::uint64_t value_hash(Value value) {
    if (!is_hashable(value)) throw std::invalid_argument("Lists and Tables are not valid Table keys");
    switch (value.kind()) {
    case Value::Kind::String: return hash_string(string_view(as_string_const(value)));
    case Value::Kind::Tag: return hash_string(tag_text(as_tag_const(value)));
    case Value::Kind::Float: return hash_combine(0x46U, std::bit_cast<std::uint64_t>(value.as_float()));
    default: return hash_combine(static_cast<std::uint64_t>(value.kind()), value.bits());
    }
}

}  // namespace on1x
