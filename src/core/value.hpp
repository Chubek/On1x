#pragma once

#include "gc/object_header.hpp"

#include <bit>
#include <cstdint>

namespace on1x {

struct GcState;

class Value {
public:
    enum class Kind : std::uint8_t {
        Unit,
        Bool,
        Int,
        Float,
        String,
        Tag,
        List,
        Table,
        Function,
        Iota,
    };

    constexpr Value() noexcept : bits_(unit_bits) {}

    [[nodiscard]] static constexpr Value unit() noexcept { return Value(unit_bits); }
    [[nodiscard]] static constexpr Value boolean(bool value) noexcept {
        return Value(value ? true_bits : false_bits);
    }
    [[nodiscard]] static Value integer(GcState* gc, std::int64_t value);
    [[nodiscard]] static Value floating(double value) noexcept;
    [[nodiscard]] static constexpr Value iota() noexcept { return Value(iota_bits); }
    [[nodiscard]] static constexpr Value object(void* value, ObjectKind kind) noexcept {
        return Value(tagged_bits |
                     (reinterpret_cast<std::uintptr_t>(value) & payload_mask) |
                     (static_cast<std::uint64_t>(object_tag(kind)) << tag_shift));
    }

    [[nodiscard]] Kind kind() const noexcept;
    [[nodiscard]] bool is_unit() const noexcept { return bits_ == unit_bits; }
    [[nodiscard]] bool is_bool() const noexcept { return tag() == Tag::Bool; }
    [[nodiscard]] bool is_int() const noexcept;
    [[nodiscard]] bool is_float() const noexcept { return !is_tagged(); }
    [[nodiscard]] bool is_iota() const noexcept { return bits_ == iota_bits; }
    [[nodiscard]] bool is_object() const noexcept;
    [[nodiscard]] bool as_bool() const noexcept;
    [[nodiscard]] std::int64_t as_int() const noexcept;
    [[nodiscard]] double as_float() const noexcept;
    [[nodiscard]] void* as_object() const noexcept;
    [[nodiscard]] std::uint64_t bits() const noexcept { return bits_; }

    friend constexpr bool operator==(Value left, Value right) noexcept {
        return left.bits_ == right.bits_;
    }

private:
    enum class Tag : std::uint8_t {
        Unit = 0,
        Bool = 1,
        SmallInt = 2,
        Pointer = 3,
        Iota = 4,
    };

    static constexpr std::uint64_t tagged_bits = 0x7ff8000000000000ULL;
    static constexpr std::uint64_t tag_shift = 48;
    static constexpr std::uint64_t payload_mask = 0x0000ffffffffffffULL;
    static constexpr std::uint64_t tag_mask = 0x0007000000000000ULL;
    static constexpr std::uint64_t unit_bits = tagged_bits;
    static constexpr std::uint64_t false_bits =
        tagged_bits | (static_cast<std::uint64_t>(Tag::Bool) << tag_shift);
    static constexpr std::uint64_t true_bits = false_bits | 1U;
    static constexpr std::uint64_t iota_bits = tagged_bits | (static_cast<std::uint64_t>(Tag::Iota) << tag_shift);

    explicit constexpr Value(std::uint64_t bits) noexcept : bits_(bits) {}
    [[nodiscard]] bool is_tagged() const noexcept {
        return (bits_ & 0x7ff8000000000000ULL) == tagged_bits;
    }
    [[nodiscard]] Tag tag() const noexcept {
        return static_cast<Tag>((bits_ & tag_mask) >> tag_shift);
    }
    [[nodiscard]] static constexpr Tag object_tag(ObjectKind) noexcept {
        return Tag::Pointer;
    }

    std::uint64_t bits_;
};

struct StringObject;
struct TagObject;
struct BigIntObject;
struct ListObject;
struct TableObject;
struct FunctionObject;

[[nodiscard]] inline Value value_from_object(StringObject* value) noexcept {
    return Value::object(value, ObjectKind::String);
}
[[nodiscard]] inline Value value_from_object(TagObject* value) noexcept {
    return Value::object(value, ObjectKind::Tag);
}
[[nodiscard]] inline Value value_from_object(BigIntObject* value) noexcept {
    return Value::object(value, ObjectKind::BigInt);
}
[[nodiscard]] inline Value value_from_object(ListObject* value) noexcept {
    return Value::object(value, ObjectKind::List);
}
[[nodiscard]] inline Value value_from_object(TableObject* value) noexcept {
    return Value::object(value, ObjectKind::Table);
}
[[nodiscard]] inline Value value_from_object(FunctionObject* value) noexcept {
    return Value::object(value, ObjectKind::Function);
}

}  // namespace on1x
