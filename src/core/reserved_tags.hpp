#pragma once

#include "core/tag_table.hpp"

namespace on1x {

struct ReservedTags {
    TagObject* unit = nullptr;
    TagObject* boolean = nullptr;
    TagObject* integer = nullptr;
    TagObject* floating = nullptr;
    TagObject* string = nullptr;
    TagObject* tag = nullptr;
    TagObject* list = nullptr;
    TagObject* table = nullptr;
    TagObject* function = nullptr;
    TagObject* iota = nullptr;
    TagObject* some = nullptr;
    TagObject* none = nullptr;
    TagObject* success = nullptr;
    TagObject* error = nullptr;
};

[[nodiscard]] ReservedTags make_reserved_tags(GcState* gc, TagTable& tags);

}  // namespace on1x
