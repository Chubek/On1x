#include "core/string.hpp"

#include "gc/alloc.hpp"
#include "util/utf8.hpp"

#include <cstring>
#include <stdexcept>

namespace on1x {

StringObject* new_string(GcState* gc, std::string_view text) {
    if (!utf8::validate(text)) throw std::invalid_argument("String requires valid UTF-8");
    const std::size_t size = sizeof(StringObject) + text.size();
    auto* result = static_cast<StringObject*>(gc_alloc_raw(gc, size));
    result->header = ObjectHeader{ObjectKind::String};
    result->bytes = text.size();
    std::memcpy(result->data, text.data(), text.size());
    result->data[text.size()] = '\0';
    return result;
}

std::string_view string_view(const StringObject* value) noexcept {
    return value ? std::string_view(value->data, value->bytes) : std::string_view();
}

StringObject* as_string(Value value) noexcept {
    return value.kind() == Value::Kind::String ? static_cast<StringObject*>(value.as_object()) : nullptr;
}

const StringObject* as_string_const(Value value) noexcept { return as_string(value); }

StringObject* string_concat(GcState* gc, Value left, Value right) {
    const auto* left_string = as_string_const(left);
    const auto* right_string = as_string_const(right);
    if (!left_string || !right_string) throw std::invalid_argument("String concatenation requires strings");
    std::string text;
    text.reserve(left_string->bytes + right_string->bytes);
    text.append(string_view(left_string));
    text.append(string_view(right_string));
    return new_string(gc, text);
}

bool string_byte_at(const StringObject* value, std::int64_t index, std::uint8_t& result) noexcept {
    if (!value) return false;
    const std::int64_t length = static_cast<std::int64_t>(value->bytes);
    if (index < 0) index += length;
    if (index < 0 || index >= length) return false;
    result = static_cast<std::uint8_t>(value->data[static_cast<std::size_t>(index)]);
    return true;
}

StringObject* string_codepoint_at(GcState* gc, const StringObject* value, std::int64_t index) {
    if (!value) return nullptr;
    std::size_t count = 0;
    if (!utf8::codepoint_count(string_view(value), count)) return nullptr;
    std::int64_t adjusted = index;
    if (adjusted < 0) adjusted += static_cast<std::int64_t>(count);
    if (adjusted < 0 || adjusted >= static_cast<std::int64_t>(count)) return nullptr;
    utf8::DecodeResult decoded;
    if (!utf8::decode_codepoint_at(
            string_view(value), static_cast<std::size_t>(adjusted), decoded)) {
        return nullptr;
    }
    std::string result;
    if (!utf8::append(result, decoded.code_point)) return nullptr;
    return new_string(gc, result);
}

}  // namespace on1x
