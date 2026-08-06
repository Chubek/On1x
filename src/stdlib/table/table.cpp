// spec §6: Table stdlib — construction, copy, merge, registration.

#include "api/api_common.hpp"
#include "core/list.hpp"
#include "core/optional.hpp"
#include "core/table.hpp"
#include "core/value.hpp"
#include "gc/roots.hpp"
#include "on1x/on1x_capability.h"
#include "on1x/on1x_module.h"
#include "runtime/state.hpp"
#include "stdlib/args.hpp"

namespace on1x::stdlib {

// Forward declarations — defined in query.cpp and higher_order.cpp
On1x_Status table_has_key_fn(On1x_State* s, int argc) noexcept;
On1x_Status table_get_or_fn(On1x_State* s, int argc) noexcept;
On1x_Status table_remove_fn(On1x_State* s, int argc) noexcept;
On1x_Status table_entries_fn(On1x_State* s, int argc) noexcept;
On1x_Status table_from_entries_fn(On1x_State* s, int argc) noexcept;
On1x_Status table_invert_fn(On1x_State* s, int argc) noexcept;
On1x_Status table_pick_fn(On1x_State* s, int argc) noexcept;
On1x_Status table_omit_fn(On1x_State* s, int argc) noexcept;
On1x_Status table_is_hashable_fn(On1x_State* s, int argc) noexcept;
On1x_Status table_map_values_fn(On1x_State* s, int argc) noexcept;
On1x_Status table_filter_keys_fn(On1x_State* s, int argc) noexcept;
On1x_Status table_reduce_entries_fn(On1x_State* s, int argc) noexcept;
On1x_Status table_for_each_entry_fn(On1x_State* s, int argc) noexcept;

namespace {

static On1x_Status bad(On1x_State* s, const char* m) noexcept { return push_api_error(s, m); }

On1x_Status table_new_fn(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 0, "Table.New")) return ON1X_ERR;
    return stack_push(s, value_from_object(new_table(&s->gc))) ? ON1X_OK : ON1X_ERR;
}

On1x_Status table_copy_fn(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 1, "Table.Copy")) return ON1X_ERR;
    Value tv;
    if (!read_argument(s, 1, tv) || tv.kind() != Value::Kind::Table) return bad(s, "Table.Copy");
    const auto* src = as_table_const(tv);
    auto* dst = new_table(&s->gc);
    GcRoot dst_root(dst);
    const auto* entry = src->entries;
    while (entry) {
        if (!table_set(&s->gc, dst, entry->key, entry->value)) return bad(s, "Table.Copy");
        entry = entry->next;
    }
    return stack_push(s, value_from_object(dst)) ? ON1X_OK : ON1X_ERR;
}

On1x_Status table_merge_fn(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 2, "Table.Merge")) return ON1X_ERR;
    Value a, b;
    if (!read_argument(s, 1, a) || !read_argument(s, 2, b) ||
        a.kind() != Value::Kind::Table || b.kind() != Value::Kind::Table)
        return bad(s, "Table.Merge");
    const auto* ta = as_table_const(a);
    const auto* tb = as_table_const(b);
    auto* dst = new_table(&s->gc);
    GcRoot dst_root(dst);
    const auto* entry = ta->entries;
    while (entry) {
        if (!table_set(&s->gc, dst, entry->key, entry->value)) return bad(s, "Table.Merge");
        entry = entry->next;
    }
    entry = tb->entries;
    while (entry) {
        if (!table_set(&s->gc, dst, entry->key, entry->value)) return bad(s, "Table.Merge");
        entry = entry->next;
    }
    return stack_push(s, value_from_object(dst)) ? ON1X_OK : ON1X_ERR;
}

const On1x_FnDesc fns[] = {
    {"New", table_new_fn},
    {"Copy", table_copy_fn},
    {"Merge", table_merge_fn},
    {"HasKey", table_has_key_fn},
    {"GetOr", table_get_or_fn},
    {"Remove", table_remove_fn},
    {"Entries", table_entries_fn},
    {"FromEntries", table_from_entries_fn},
    {"Invert", table_invert_fn},
    {"Pick", table_pick_fn},
    {"Omit", table_omit_fn},
    {"IsHashable", table_is_hashable_fn},
    {"MapValues", table_map_values_fn},
    {"FilterKeys", table_filter_keys_fn},
    {"ReduceEntries", table_reduce_entries_fn},
    {"ForEachEntry", table_for_each_entry_fn},
};

const On1x_ModuleDesc desc{"Table", ON1X_CAP_NONE, fns, sizeof(fns) / sizeof(*fns)};

}  // namespace

const On1x_ModuleDesc* table_module() noexcept { return &desc; }

}  // namespace on1x::stdlib
