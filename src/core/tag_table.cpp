#include "core/tag_table.hpp"

#include "gc/alloc.hpp"
#include "util/utf8.hpp"

#include <stdexcept>

namespace on1x {

TagObject* TagTable::intern(GcState* gc, std::string_view text) {
    if (!utf8::validate(text)) throw std::invalid_argument("Tag requires valid UTF-8");
    for (TagObject* current = head_; current; current = current->next) {
        if (tag_text(current) == text) return current;
    }
    auto* tag = gc_alloc<TagObject>(gc);
    tag->text = new_string(gc, text);
    tag->next = head_;
    head_ = tag;
    return tag;
}

std::string_view tag_text(const TagObject* tag) noexcept {
    return tag ? string_view(tag->text) : std::string_view();
}

TagObject* as_tag(Value value) noexcept {
    return value.kind() == Value::Kind::Tag ? static_cast<TagObject*>(value.as_object()) : nullptr;
}

const TagObject* as_tag_const(Value value) noexcept { return as_tag(value); }

}  // namespace on1x
