#include "core/iota.hpp"

#include "core/list.hpp"

#include <limits>

namespace on1x {

namespace {
bool append_range(GcState* gc, ListObject* result, std::int64_t start, std::int64_t end, std::int64_t step) {
    if (step == 0 || (step > 0 && start > end) || (step < 0 && start < end)) return false;
    for (std::int64_t current = start; step > 0 ? current < end : current > end;) {
        if (!list_push(gc, result, Value::integer(gc, current))) return false;
        const std::int64_t next = static_cast<std::int64_t>(
            static_cast<std::uint64_t>(current) + static_cast<std::uint64_t>(step));
        if ((step > 0 && next <= current) || (step < 0 && next >= current)) return false;
        current = next;
    }
    return true;
}
}

Value make_iota(GcState* gc, const ReservedTags& tags, std::initializer_list<Value> arguments) {
    if (arguments.size() < 1 || arguments.size() > 3) return make_none(gc, tags);
    for (Value argument : arguments) if (!argument.is_int()) return make_none(gc, tags);
    const auto* values = arguments.begin();
    if (arguments.size() == 1) {
        const std::int64_t end = values[0].as_int();
        if (end < 0) return make_none(gc, tags);
        auto* result = new_list(gc, static_cast<std::size_t>(end));
        return append_range(gc, result, 0, end, 1) ? value_from_object(result) : make_none(gc, tags);
    }
    const std::int64_t start = values[0].as_int();
    const std::int64_t end = values[1].as_int();
    const std::int64_t step = arguments.size() == 3 ? values[2].as_int() : 1;
    auto* result = new_list(gc);
    return append_range(gc, result, start, end, step) ? value_from_object(result) : make_none(gc, tags);
}

}  // namespace on1x
