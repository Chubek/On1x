// spec §17: List sort functions — Sort, SortBy, IsSorted, Unique.
// Stable merge sort over GC storage.

#include "api/api_common.hpp"
#include "core/equality.hpp"
#include "core/list.hpp"
#include "core/optional.hpp"
#include "core/string.hpp"
#include "core/tag_table.hpp"
#include "core/value.hpp"
#include "gc/alloc.hpp"
#include "gc/roots.hpp"
#include "runtime/state.hpp"
#include "stdlib/args.hpp"
#include "stdlib/list/list_fns.hpp"
#include "vm/interpreter.hpp"

#include <cmath>
#include <cstdint>
#include <string_view>

namespace on1x::stdlib {

namespace {

static On1x_Status bad(On1x_State* s, const char* m) noexcept {
    return push_api_error(s, m);
}

// Compare two values using runtime ordering (Int, Float, String, Tag).
// Returns true with ordering set to -1, 0, or 1 if comparable.
bool comparable(Value left, Value right, int& ordering) noexcept {
    if ((left.is_int() || left.is_float()) && (right.is_int() || right.is_float())) {
        double lhs = left.is_float() ? left.as_float() : static_cast<double>(left.as_int());
        double rhs = right.is_float() ? right.as_float() : static_cast<double>(right.as_int());
        if (std::isnan(lhs) || std::isnan(rhs)) return false;
        ordering = lhs < rhs ? -1 : lhs > rhs ? 1 : 0;
        return true;
    }
    if (left.kind() == Value::Kind::String && right.kind() == Value::Kind::String) {
        std::string_view ls = string_view(as_string_const(left));
        std::string_view rs = string_view(as_string_const(right));
        if (ls < rs) ordering = -1;
        else if (ls > rs) ordering = 1;
        else ordering = 0;
        return true;
    }
    if (left.kind() == Value::Kind::Tag && right.kind() == Value::Kind::Tag) {
        std::string_view lt = tag_text(as_tag_const(left));
        std::string_view rt = tag_text(as_tag_const(right));
        if (lt < rt) ordering = -1;
        else if (lt > rt) ordering = 1;
        else ordering = 0;
        return true;
    }
    return false;
}

bool all_comparable(const ListObject* list) noexcept {
    for (std::size_t i = 0; i < list->length; ++i) {
        Value a;
        list_get(list, i, a);
        for (std::size_t j = i + 1; j < list->length; ++j) {
            Value b;
            list_get(list, j, b);
            int ordering = 0;
            if (!comparable(a, b, ordering)) return false;
        }
    }
    return true;
}

void merge_sort(
    GcState* gc,
    Value* items,
    std::size_t length,
    Value* temp) {
    if (length <= 1) return;
    std::size_t mid = length / 2;
    merge_sort(gc, items, mid, temp);
    merge_sort(gc, items + mid, length - mid, temp);
    std::size_t left = 0;
    std::size_t right = mid;
    std::size_t out = 0;
    while (left < mid && right < length) {
        int ordering = 0;
        comparable(items[left], items[right], ordering);
        if (ordering <= 0) {
            temp[out++] = items[left++];
        } else {
            temp[out++] = items[right++];
        }
    }
    while (left < mid) temp[out++] = items[left++];
    while (right < length) temp[out++] = items[right++];
    for (std::size_t i = 0; i < length; ++i) items[i] = temp[i];
}

bool merge_sort_by(
    On1x_State* s,
    Value* items,
    std::size_t length,
    Value* temp,
    FunctionObject* cmp_fn,
    const char*& error) {
    if (length <= 1) return true;
    std::size_t mid = length / 2;
    if (!merge_sort_by(s, items, mid, temp, cmp_fn, error)) return false;
    if (!merge_sort_by(s, items + mid, length - mid, temp, cmp_fn, error)) return false;
    std::size_t left = 0;
    std::size_t right = mid;
    std::size_t out = 0;
    while (left < mid && right < length) {
        Value call_result;
        const Value args[] = {items[left], items[right]};
        if (!vm::invoke_function(s, cmp_fn, args, 2, call_result, error)) return false;
        if (!call_result.is_int()) {
            error = "SortBy comparator must return :Int";
            return false;
        }
        if (call_result.as_int() <= 0) {
            temp[out++] = items[left++];
        } else {
            temp[out++] = items[right++];
        }
    }
    while (left < mid) temp[out++] = items[left++];
    while (right < length) temp[out++] = items[right++];
    for (std::size_t i = 0; i < length; ++i) items[i] = temp[i];
    return true;
}

}  // namespace

On1x_Status list_sort_fn(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 1, "List.Sort")) return ON1X_ERR;
    Value lv;
    if (!read_argument(s, 1, lv) || lv.kind() != Value::Kind::List)
        return bad(s, "List.Sort");
    const auto* src = as_list_const(lv);
    if (!src) return bad(s, "List.Sort");
    if (src->length == 0) {
        auto* empty = new_list(&s->gc, 0);
        return stack_push(s, make_some(&s->gc, s->reserved, value_from_object(empty)))
            ? ON1X_OK : ON1X_ERR;
    }
    if (!all_comparable(src))
        return stack_push(s, make_none(&s->gc, s->reserved)) ? ON1X_OK : ON1X_ERR;
    auto* temp = gc_alloc_array<Value>(&s->gc, src->length);
    auto* items = gc_alloc_array<Value>(&s->gc, src->length);
    GcRoot temp_root(temp);
    GcRoot items_root(items);
    for (std::size_t i = 0; i < src->length; ++i) list_get(src, i, items[i]);
    merge_sort(&s->gc, items, src->length, temp);
    auto* result = new_list(&s->gc, src->length);
    GcRoot result_root(result);
    for (std::size_t i = 0; i < src->length; ++i)
        if (!list_push(&s->gc, result, items[i])) return bad(s, "List.Sort");
    return stack_push(s, make_some(&s->gc, s->reserved, value_from_object(result)))
        ? ON1X_OK : ON1X_ERR;
}

On1x_Status list_sort_by_fn(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 2, "List.SortBy")) return ON1X_ERR;
    Value lv, fn;
    if (!read_argument(s, 1, lv) || !read_argument(s, 2, fn) ||
        lv.kind() != Value::Kind::List || fn.kind() != Value::Kind::Function)
        return bad(s, "List.SortBy");
    const auto* src = as_list_const(lv);
    if (!src) return bad(s, "List.SortBy");
    auto* cmp_fn = static_cast<FunctionObject*>(fn.as_object());
    GcRoot cmp_root(cmp_fn);
    if (src->length == 0) {
        auto* empty = new_list(&s->gc, 0);
        return stack_push(s, value_from_object(empty)) ? ON1X_OK : ON1X_ERR;
    }
    auto* temp = gc_alloc_array<Value>(&s->gc, src->length);
    auto* items = gc_alloc_array<Value>(&s->gc, src->length);
    GcRoot temp_root(temp);
    GcRoot items_root(items);
    for (std::size_t i = 0; i < src->length; ++i) list_get(src, i, items[i]);
    const char* error = nullptr;
    if (!merge_sort_by(s, items, src->length, temp, cmp_fn, error))
        return bad(s, error ? error : "List.SortBy");
    auto* result = new_list(&s->gc, src->length);
    GcRoot result_root(result);
    for (std::size_t i = 0; i < src->length; ++i)
        if (!list_push(&s->gc, result, items[i])) return bad(s, "List.SortBy");
    return stack_push(s, value_from_object(result)) ? ON1X_OK : ON1X_ERR;
}

On1x_Status list_is_sorted_fn(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 1, "List.IsSorted")) return ON1X_ERR;
    Value lv;
    if (!read_argument(s, 1, lv) || lv.kind() != Value::Kind::List)
        return bad(s, "List.IsSorted");
    const auto* src = as_list_const(lv);
    if (!src) return bad(s, "List.IsSorted");
    for (std::size_t i = 1; i < src->length; ++i) {
        Value prev, curr;
        list_get(src, i - 1, prev);
        list_get(src, i, curr);
        int ordering = 0;
        if (!comparable(prev, curr, ordering) || ordering > 0)
            return stack_push(s, Value::boolean(false)) ? ON1X_OK : ON1X_ERR;
    }
    return stack_push(s, Value::boolean(true)) ? ON1X_OK : ON1X_ERR;
}

On1x_Status list_unique_fn(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 1, "List.Unique")) return ON1X_ERR;
    Value lv;
    if (!read_argument(s, 1, lv) || lv.kind() != Value::Kind::List)
        return bad(s, "List.Unique");
    const auto* src = as_list_const(lv);
    if (!src) return bad(s, "List.Unique");
    auto* result = new_list(&s->gc, 0);
    GcRoot result_root(result);
    for (std::size_t i = 0; i < src->length; ++i) {
        Value item;
        list_get(src, i, item);
        bool found = false;
        for (std::size_t j = 0; j < result->length; ++j) {
            Value existing;
            list_get(result, j, existing);
            if (value_equals(item, existing)) { found = true; break; }
        }
        if (!found && !list_push(&s->gc, result, item)) return bad(s, "List.Unique");
    }
    return stack_push(s, value_from_object(result)) ? ON1X_OK : ON1X_ERR;
}

}  // namespace on1x::stdlib
