#pragma once

#include "core/value.hpp"
#include "gc/gc.hpp"

#include <string_view>

namespace on1x {

struct StringObject {
    ObjectHeader header{ObjectKind::String};
    std::size_t bytes = 0;
    char data[1]{};
};

[[nodiscard]] StringObject* new_string(GcState* gc, std::string_view text);
[[nodiscard]] std::string_view string_view(const StringObject* value) noexcept;
[[nodiscard]] StringObject* as_string(Value value) noexcept;
[[nodiscard]] const StringObject* as_string_const(Value value) noexcept;
[[nodiscard]] StringObject* string_concat(GcState* gc, Value left, Value right);
[[nodiscard]] bool string_byte_at(const StringObject* value, std::int64_t index, std::uint8_t& result) noexcept;
[[nodiscard]] StringObject* string_codepoint_at(GcState* gc, const StringObject* value, std::int64_t index);

}  // namespace on1x
