#pragma once

#include "core/value.hpp"

#include <cstddef>
#include <cstdint>

namespace on1x {
struct GcState;
struct TagObject;
}

namespace on1x::runtime {

enum class PatternKind : std::uint8_t {
    Literal,
    Wildcard,
    Binding,
    List,
    TaggedList,
};

struct Pattern {
    PatternKind kind{};
    Value literal{};
    TagObject* tag = nullptr;
    Pattern* children = nullptr;
    std::size_t child_count = 0;
    std::uint32_t binding = 0;
    bool has_tail = false;
    std::uint32_t tail_binding = 0;
};

[[nodiscard]] bool match_pattern(
    GcState* gc,
    const Pattern& pattern,
    Value value,
    Value* locals,
    std::size_t local_count,
    const char*& error) noexcept;

}  // namespace on1x::runtime
