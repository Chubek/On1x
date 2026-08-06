#include "runtime/pattern_match.hpp"

#include "core/equality.hpp"
#include "core/list.hpp"
#include "gc/alloc.hpp"
#include "gc/roots.hpp"

namespace on1x::runtime {

namespace {

bool match(
    GcState* gc,
    const Pattern& pattern,
    Value value,
    Value* locals,
    std::size_t local_count,
    const char*& error) noexcept {
    switch (pattern.kind) {
    case PatternKind::Literal:
        return value_equals(value, pattern.literal);
    case PatternKind::Wildcard:
        return true;
    case PatternKind::Binding:
        if (pattern.binding >= local_count) {
            error = "invalid match binding";
            return false;
        }
        locals[pattern.binding] = value;
        return true;
    case PatternKind::List:
    case PatternKind::TaggedList:
        break;
    }

    const ListObject* list = as_list_const(value);
    if (!list || (pattern.kind == PatternKind::List && list->constructor) ||
        (pattern.kind == PatternKind::TaggedList && list->constructor != pattern.tag) ||
        list->length < pattern.child_count ||
        (!pattern.has_tail && list->length != pattern.child_count)) {
        return false;
    }
    for (std::size_t index = 0; index < pattern.child_count; ++index) {
        if (!match(gc, pattern.children[index], list->items[index], locals, local_count, error)) {
            return false;
        }
    }
    if (!pattern.has_tail) return true;
    if (pattern.tail_binding >= local_count) {
        error = "invalid match tail binding";
        return false;
    }
    try {
        auto* tail = new_list(gc, list->length - pattern.child_count);
        GcRoot tail_root(tail);
        for (std::size_t index = pattern.child_count; index < list->length; ++index) {
            if (!list_push(gc, tail, list->items[index])) {
                error = "unable to allocate match tail";
                return false;
            }
        }
        locals[pattern.tail_binding] = value_from_object(tail);
        return true;
    } catch (...) {
        error = "unable to allocate match tail";
        return false;
    }
}

}  // namespace

bool match_pattern(
    GcState* gc,
    const Pattern& pattern,
    Value value,
    Value* locals,
    std::size_t local_count,
    const char*& error) noexcept {
    return match(gc, pattern, value, locals, local_count, error);
}

}  // namespace on1x::runtime
