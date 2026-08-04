#include "core/value.hpp"

#include "gc/alloc.hpp"

#include <cmath>

namespace on1x {

struct BigIntObject {
    ObjectHeader header{ObjectKind::BigInt};
    std::int64_t value = 0;
};

Value Value::integer(GcState* gc, std::int64_t value) {
    constexpr std::int64_t small_minimum = -(std::int64_t{1} << 47);
    constexpr std::int64_t small_maximum = (std::int64_t{1} << 47) - 1;
    if (value >= small_minimum && value <= small_maximum) {
        return Value(
            tagged_bits | (static_cast<std::uint64_t>(value) & payload_mask) |
            (static_cast<std::uint64_t>(Tag::SmallInt) << tag_shift));
    }
    auto* boxed = gc_alloc<BigIntObject>(gc);
    boxed->value = value;
    return value_from_object(boxed);
}

Value Value::floating(double value) noexcept {
    if (std::isnan(value)) {
        return Value(0x7ff4000000000000ULL);
    }
    return Value(std::bit_cast<std::uint64_t>(value));
}

Value::Kind Value::kind() const noexcept {
    if (!is_tagged()) return Kind::Float;
    switch (tag()) {
    case Tag::Unit: return Kind::Unit;
    case Tag::Bool: return Kind::Bool;
    case Tag::SmallInt: return Kind::Int;
    case Tag::Iota: return Kind::Iota;
    case Tag::Pointer: {
        const auto* header = static_cast<const ObjectHeader*>(as_object());
        switch (header->kind) {
        case ObjectKind::String: return Kind::String;
        case ObjectKind::BigInt: return Kind::Int;
        case ObjectKind::Tag: return Kind::Tag;
        case ObjectKind::List: return Kind::List;
        case ObjectKind::Table: return Kind::Table;
        case ObjectKind::Function: return Kind::Function;
        default: return Kind::Unit;
        }
    }
    }
    return Kind::Unit;
}

bool Value::is_object() const noexcept {
    return is_tagged() && tag() == Tag::Pointer;
}

bool Value::is_int() const noexcept {
    return is_tagged() &&
           (tag() == Tag::SmallInt ||
            (tag() == Tag::Pointer &&
             static_cast<const ObjectHeader*>(as_object())->kind == ObjectKind::BigInt));
}

bool Value::as_bool() const noexcept { return (bits_ & 1U) != 0; }

std::int64_t Value::as_int() const noexcept {
    if (is_tagged() && tag() == Tag::Pointer) {
        return static_cast<const BigIntObject*>(as_object())->value;
    }
    const std::uint64_t payload = bits_ & payload_mask;
    return static_cast<std::int64_t>(payload << 16U) >> 16U;
}

double Value::as_float() const noexcept { return std::bit_cast<double>(bits_); }

void* Value::as_object() const noexcept {
    return reinterpret_cast<void*>(static_cast<std::uintptr_t>(bits_ & payload_mask));
}

}  // namespace on1x
