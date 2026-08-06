// spec §6: Table stdlib — MapValues, FilterKeys, ReduceEntries, ForEachEntry.

#include "api/api_common.hpp"
#include "core/list.hpp"
#include "core/optional.hpp"
#include "core/table.hpp"
#include "core/value.hpp"
#include "gc/roots.hpp"
#include "runtime/state.hpp"
#include "stdlib/args.hpp"
#include "vm/interpreter.hpp"

#include <cstdint>

namespace on1x::stdlib {
namespace {

static On1x_Status bad(On1x_State* s, const char* m) noexcept { return push_api_error(s, m); }

[[nodiscard]] static bool is_fn(Value v) noexcept {
    return v.kind() == Value::Kind::Function;
}

}  // namespace

On1x_Status table_map_values_fn(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 2, "Table.MapValues")) return ON1X_ERR;
    Value tv, fn;
    if (!read_argument(s, 1, tv) || !read_argument(s, 2, fn) ||
        tv.kind() != Value::Kind::Table || !is_fn(fn))
        return bad(s, "Table.MapValues");
    const auto* src = as_table_const(tv);
    auto* func = static_cast<FunctionObject*>(fn.as_object());
    GcRoot func_root(func);
    auto* dst = new_table(&s->gc);
    GcRoot dst_root(dst);
    const auto* entry = src->entries;
    while (entry) {
        Value call_result;
        const char* error = nullptr;
        const Value args[] = {entry->value};
        if (!vm::invoke_function(s, func, args, 1, call_result, error))
            return bad(s, error ? error : "Table.MapValues callback failed");
        if (!table_set(&s->gc, dst, entry->key, call_result))
            return bad(s, "Table.MapValues");
        entry = entry->next;
    }
    return stack_push(s, value_from_object(dst)) ? ON1X_OK : ON1X_ERR;
}

On1x_Status table_filter_keys_fn(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 2, "Table.FilterKeys")) return ON1X_ERR;
    Value tv, fn;
    if (!read_argument(s, 1, tv) || !read_argument(s, 2, fn) ||
        tv.kind() != Value::Kind::Table || !is_fn(fn))
        return bad(s, "Table.FilterKeys");
    const auto* src = as_table_const(tv);
    auto* func = static_cast<FunctionObject*>(fn.as_object());
    GcRoot func_root(func);
    auto* dst = new_table(&s->gc);
    GcRoot dst_root(dst);
    const auto* entry = src->entries;
    while (entry) {
        Value call_result;
        const char* error = nullptr;
        const Value args[] = {entry->key};
        if (!vm::invoke_function(s, func, args, 1, call_result, error))
            return bad(s, error ? error : "Table.FilterKeys callback failed");
        if (!call_result.is_bool())
            return bad(s, "Table.FilterKeys predicate must return :Bool");
        if (call_result.as_bool()) {
            if (!table_set(&s->gc, dst, entry->key, entry->value))
                return bad(s, "Table.FilterKeys");
        }
        entry = entry->next;
    }
    return stack_push(s, value_from_object(dst)) ? ON1X_OK : ON1X_ERR;
}

On1x_Status table_reduce_entries_fn(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 3, "Table.ReduceEntries")) return ON1X_ERR;
    Value tv, fn, init;
    if (!read_argument(s, 1, tv) || !read_argument(s, 2, fn) ||
        !read_argument(s, 3, init) || tv.kind() != Value::Kind::Table || !is_fn(fn))
        return bad(s, "Table.ReduceEntries");
    const auto* src = as_table_const(tv);
    auto* func = static_cast<FunctionObject*>(fn.as_object());
    GcRoot func_root(func);
    Value acc = init;
    const auto* entry = src->entries;
    while (entry) {
        const char* error = nullptr;
        Value call_result;
        GcRoot acc_root(acc.is_object() ? acc.as_object() : nullptr);
        const Value args[] = {acc, entry->key, entry->value};
        if (!vm::invoke_function(s, func, args, 3, call_result, error))
            return bad(s, error ? error : "Table.ReduceEntries callback failed");
        acc = call_result;
        entry = entry->next;
    }
    return stack_push(s, acc) ? ON1X_OK : ON1X_ERR;
}

On1x_Status table_for_each_entry_fn(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 2, "Table.ForEachEntry")) return ON1X_ERR;
    Value tv, fn;
    if (!read_argument(s, 1, tv) || !read_argument(s, 2, fn) ||
        tv.kind() != Value::Kind::Table || !is_fn(fn))
        return bad(s, "Table.ForEachEntry");
    auto* src = as_table(tv);
    if (!src)
        return bad(s, "Table.ForEachEntry");
    GcRoot table_root(src);
    auto* func = static_cast<FunctionObject*>(fn.as_object());
    GcRoot func_root(func);
    const auto* entry = src->entries;
    while (entry) {
        Value call_result;
        const char* error = nullptr;
        const Value args[] = {entry->key, entry->value};
        if (!vm::invoke_function(s, func, args, 2, call_result, error))
            return bad(s, error ? error : "Table.ForEachEntry callback failed");
        entry = entry->next;
    }
    return stack_push(s, Value::unit()) ? ON1X_OK : ON1X_ERR;
}

}  // namespace on1x::stdlib
