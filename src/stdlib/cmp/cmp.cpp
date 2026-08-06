#include "api/api_common.hpp"
#include "core/equality.hpp"
#include "core/hashing.hpp"
#include "core/list.hpp"
#include "core/optional.hpp"
#include "core/string.hpp"
#include "core/table.hpp"
#include "core/tag_table.hpp"
#include "gc/roots.hpp"
#include "on1x/on1x_capability.h"
#include "on1x/on1x_module.h"
#include "runtime/state.hpp"
#include "stdlib/args.hpp"

#include <cmath>
#include <cstdint>
#include <array>
#include <string_view>
#include <utility>

namespace on1x::stdlib {
namespace {

On1x_Status type_error(On1x_State* state, const char* member) noexcept {
    return push_api_error(state, member);
}

bool comparable(Value left, Value right, int& result) noexcept {
    if ((left.is_int() || left.is_float()) && (right.is_int() || right.is_float())) {
        const double lhs = left.is_float() ? left.as_float() : static_cast<double>(left.as_int());
        const double rhs = right.is_float() ? right.as_float() : static_cast<double>(right.as_int());
        if (std::isnan(lhs) || std::isnan(rhs)) return false;
        result = lhs < rhs ? -1 : lhs > rhs ? 1 : 0;
        return true;
    }
    if (left.is_object() && right.is_object() &&
        left.kind() == Value::Kind::String && right.kind() == Value::Kind::String) {
        const std::string_view lhs = string_view(as_string_const(left));
        const std::string_view rhs = string_view(as_string_const(right));
        result = lhs < rhs ? -1 : lhs > rhs ? 1 : 0;
        return true;
    }
    if (left.is_object() && right.is_object() &&
        left.kind() == Value::Kind::Tag && right.kind() == Value::Kind::Tag) {
        const std::string_view lhs = tag_text(as_tag_const(left));
        const std::string_view rhs = tag_text(as_tag_const(right));
        result = lhs < rhs ? -1 : lhs > rhs ? 1 : 0;
        return true;
    }
    return false;
}

bool deep_equals(
    Value left,
    Value right,
    std::array<std::pair<Value, Value>, 128>& seen,
    std::size_t& seen_count) noexcept {
    if (left == right) return true;
    if (left.is_int() && right.is_float()) return static_cast<double>(left.as_int()) == right.as_float();
    if (left.is_float() && right.is_int()) return left.as_float() == static_cast<double>(right.as_int());
    if (left.kind() != right.kind()) return false;
    if (left.kind() != Value::Kind::List && left.kind() != Value::Kind::Table) {
        return value_equals(left, right);
    }
    for (std::size_t index = 0; index < seen_count; ++index) {
        if (seen[index].first == left && seen[index].second == right) return true;
    }
    if (seen_count == seen.size()) return false;
    seen[seen_count++] = {left, right};
    if (left.kind() == Value::Kind::List) {
        const auto* lhs = as_list_const(left);
        const auto* rhs = as_list_const(right);
        if (lhs->constructor != rhs->constructor || lhs->length != rhs->length) return false;
        for (std::size_t index = 0; index < lhs->length; ++index) {
            Value lhs_item;
            Value rhs_item;
            if (!list_get(lhs, index, lhs_item) || !list_get(rhs, index, rhs_item) ||
                !deep_equals(lhs_item, rhs_item, seen, seen_count)) return false;
        }
        return true;
    }
    const auto* lhs = as_table_const(left);
    const auto* rhs = as_table_const(right);
    if (lhs->length != rhs->length) return false;
    for (const TableEntry* entry = lhs->entries; entry; entry = entry->next) {
        Value candidate;
        if (!table_get(rhs, entry->key, candidate) ||
            !deep_equals(entry->value, candidate, seen, seen_count)) return false;
    }
    return true;
}

On1x_Status cmp_compare(On1x_State* state, int argc) noexcept {
    if (!require_arity(state, argc, 2, "Cmp.Compare")) return ON1X_ERR;
    Value left;
    Value right;
    if (!read_argument(state, 1, left) || !read_argument(state, 2, right)) {
        return type_error(state, "Cmp.Compare expects two values");
    }
    int ordering = 0;
    if (!comparable(left, right, ordering)) {
        try {
            return stack_push(state, make_none(&state->gc, state->reserved))
                ? ON1X_OK
                : ON1X_ERR;
        } catch (...) {
            return type_error(state, "Cmp.Compare failed");
        }
    }
    try {
        const Value ordering_value = Value::integer(&state->gc, ordering);
        GcRoot ordering_root(
            ordering_value.is_object() ? ordering_value.as_object() : nullptr);
        const Value result = make_some(&state->gc, state->reserved, ordering_value);
        return stack_push(state, result) ? ON1X_OK : ON1X_ERR;
    } catch (...) {
        return type_error(state, "Cmp.Compare failed");
    }
}

On1x_Status cmp_eq(On1x_State* state, int argc) noexcept {
    if (!require_arity(state, argc, 2, "Cmp.Eq")) return ON1X_ERR;
    Value left;
    Value right;
    if (!read_argument(state, 1, left) || !read_argument(state, 2, right)) {
        return type_error(state, "Cmp.Eq expects two values");
    }
    return stack_push(state, Value::boolean(value_equals(left, right))) ? ON1X_OK : ON1X_ERR;
}

On1x_Status cmp_deep_eq(On1x_State* state, int argc) noexcept {
    if (!require_arity(state, argc, 2, "Cmp.DeepEq")) return ON1X_ERR;
    Value left;
    Value right;
    if (!read_argument(state, 1, left) || !read_argument(state, 2, right)) {
        return type_error(state, "Cmp.DeepEq expects two values");
    }
    std::array<std::pair<Value, Value>, 128> seen{};
    std::size_t seen_count = 0;
    return stack_push(state, Value::boolean(deep_equals(left, right, seen, seen_count)))
        ? ON1X_OK
        : ON1X_ERR;
}

On1x_Status cmp_hash(On1x_State* state, int argc) noexcept {
    if (!require_arity(state, argc, 1, "Cmp.Hash")) return ON1X_ERR;
    Value value;
    if (!read_argument(state, 1, value)) return type_error(state, "Cmp.Hash expects a value");
    if (!is_hashable(value)) {
        try {
            return stack_push(state, make_none(&state->gc, state->reserved))
                ? ON1X_OK
                : ON1X_ERR;
        } catch (...) {
            return type_error(state, "Cmp.Hash failed");
        }
    }
    try {
        const Value hash_value = Value::integer(
            &state->gc, static_cast<std::int64_t>(value_hash(value)));
        GcRoot hash_root(hash_value.is_object() ? hash_value.as_object() : nullptr);
        const Value result = make_some(&state->gc, state->reserved, hash_value);
        return stack_push(state, result) ? ON1X_OK : ON1X_ERR;
    } catch (...) {
        return type_error(state, "Cmp.Hash failed");
    }
}

On1x_Status cmp_min_max(On1x_State* state, int argc, bool minimum) noexcept {
    const char* member = minimum ? "Cmp.MinOf" : "Cmp.MaxOf";
    if (!require_arity(state, argc, 1, member)) return ON1X_ERR;
    Value list_value;
    if (!read_argument(state, 1, list_value) || list_value.kind() != Value::Kind::List) {
        return type_error(state, member);
    }
    const auto* list = as_list_const(list_value);
    if (!list || list->length == 0U) {
        try {
            return stack_push(state, make_none(&state->gc, state->reserved))
                ? ON1X_OK
                : ON1X_ERR;
        } catch (...) {
            return type_error(state, member);
        }
    }
    Value best;
    if (!list_get(list, 0, best)) return type_error(state, member);
    for (std::size_t index = 1; index < list->length; ++index) {
        Value candidate;
        int ordering = 0;
        if (!list_get(list, index, candidate) || !comparable(best, candidate, ordering)) {
            return stack_push(state, make_none(&state->gc, state->reserved))
                ? ON1X_OK
                : ON1X_ERR;
        }
        if ((minimum && ordering > 0) || (!minimum && ordering < 0)) best = candidate;
    }
    try {
        GcRoot best_root(best.is_object() ? best.as_object() : nullptr);
        return stack_push(state, make_some(&state->gc, state->reserved, best))
            ? ON1X_OK
            : ON1X_ERR;
    } catch (...) {
        return type_error(state, member);
    }
}

On1x_Status cmp_min(On1x_State* state, int argc) noexcept {
    return cmp_min_max(state, argc, true);
}

On1x_Status cmp_max(On1x_State* state, int argc) noexcept {
    return cmp_min_max(state, argc, false);
}

const On1x_FnDesc functions[] = {
    {"Compare", cmp_compare},
    {"Eq", cmp_eq},
    {"DeepEq", cmp_deep_eq},
    {"Hash", cmp_hash},
    {"MinOf", cmp_min},
    {"MaxOf", cmp_max},
};

const On1x_ModuleDesc descriptor{
    "Cmp",
    ON1X_CAP_NONE,
    functions,
    sizeof(functions) / sizeof(functions[0]),
};

}  // namespace

const On1x_ModuleDesc* cmp_module() noexcept {
    return &descriptor;
}

}  // namespace on1x::stdlib
