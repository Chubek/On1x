#pragma once

#include "core/string.hpp"

#include <string_view>

namespace on1x {

struct TagObject {
    ObjectHeader header{ObjectKind::Tag};
    StringObject* text = nullptr;
    TagObject* next = nullptr;
};

class TagTable {
public:
    TagTable() = default;
    [[nodiscard]] TagObject* intern(GcState* gc, std::string_view text);
    void root() noexcept;
    void unroot() noexcept;

private:
    TagObject* head_ = nullptr;
    bool rooted_ = false;
};

[[nodiscard]] std::string_view tag_text(const TagObject* tag) noexcept;
[[nodiscard]] TagObject* as_tag(Value value) noexcept;
[[nodiscard]] const TagObject* as_tag_const(Value value) noexcept;

}  // namespace on1x
