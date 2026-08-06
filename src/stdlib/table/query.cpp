// spec §6: Table stdlib — HasKey, GetOr, Remove, Entries, FromEntries, Invert, Pick, Omit, IsHashable.

#include "api/api_common.hpp"
#include "core/hashing.hpp"
#include "core/list.hpp"
#include "core/optional.hpp"
#include "core/table.hpp"
#include "core/value.hpp"
#include "gc/roots.hpp"
#include "runtime/state.hpp"
#include "stdlib/args.hpp"

#include <cstdint>

namespace on1x::stdlib {
namespace {

static On1x_Status bad(On1x_State* s, const char* m) noexcept { return push_api_error(s, m); }

}  // namespace

On1x_Status table_has_key_fn(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 2, "Table.HasKey")) return ON1X_ERR;
    Value tv, key;
    if (!read_argument(s, 1, tv) || !read_argument(s, 2, key) ||
        tv.kind() != Value::Kind::Table) return bad(s, "Table.HasKey");
    const auto* tab = as_table_const(tv);
    Value dummy;
    return stack_push(s, Value::boolean(table_get(tab, key, dummy))) ? ON1X_OK : ON1X_ERR;
}

On1x_Status table_get_or_fn(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 3, "Table.GetOr")) return ON1X_ERR;
    Value tv, key, def;
    if (!read_argument(s, 1, tv) || !read_argument(s, 2, key) ||
        !read_argument(s, 3, def) || tv.kind() != Value::Kind::Table)
        return bad(s, "Table.GetOr");
    const auto* tab = as_table_const(tv);
    Value result;
    if (table_get(tab, key, result))
        return stack_push(s, result) ? ON1X_OK : ON1X_ERR;
    return stack_push(s, def) ? ON1X_OK : ON1X_ERR;
}

On1x_Status table_remove_fn(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 2, "Table.Remove")) return ON1X_ERR;
    Value tv, key;
    if (!read_argument(s, 1, tv) || !read_argument(s, 2, key) ||
        tv.kind() != Value::Kind::Table) return bad(s, "Table.Remove");
    auto* tab = as_table(tv);
    if (!tab) return bad(s, "Table.Remove");
    Value result;
    if (table_remove(tab, key, result))
        return stack_push(s, make_some(&s->gc, s->reserved, result)) ? ON1X_OK : ON1X_ERR;
    return stack_push(s, make_none(&s->gc, s->reserved)) ? ON1X_OK : ON1X_ERR;
}

On1x_Status table_entries_fn(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 1, "Table.Entries")) return ON1X_ERR;
    Value tv;
    if (!read_argument(s, 1, tv) || tv.kind() != Value::Kind::Table) return bad(s, "Table.Entries");
    const auto* tab = as_table_const(tv);
    auto* result = new_list(&s->gc, tab->length);
    GcRoot result_root(result);
    const auto* entry = tab->entries;
    while (entry) {
        auto* pair = new_list(&s->gc, 2);
        if (!list_push(&s->gc, pair, entry->key) || !list_push(&s->gc, pair, entry->value))
            return bad(s, "Table.Entries");
        if (!list_push(&s->gc, result, value_from_object(pair))) return bad(s, "Table.Entries");
        entry = entry->next;
    }
    return stack_push(s, value_from_object(result)) ? ON1X_OK : ON1X_ERR;
}

On1x_Status table_from_entries_fn(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 1, "Table.FromEntries")) return ON1X_ERR;
    Value lv;
    if (!read_argument(s, 1, lv) || lv.kind() != Value::Kind::List) return bad(s, "Table.FromEntries");
    const auto* src = as_list_const(lv);
    auto* tab = new_table(&s->gc);
    GcRoot tab_root(tab);
    for (std::size_t i = 0; i < src->length; ++i) {
        Value pair;
        list_get(src, i, pair);
        if (pair.kind() != Value::Kind::List) {
            return stack_push(s, make_none(&s->gc, s->reserved)) ? ON1X_OK : ON1X_ERR;
        }
        const auto* p = as_list_const(pair);
        if (p->length != 2) {
            return stack_push(s, make_none(&s->gc, s->reserved)) ? ON1X_OK : ON1X_ERR;
        }
        Value k, v;
        list_get(p, 0, k);
        list_get(p, 1, v);
        if (!is_hashable(k)) {
            return stack_push(s, make_none(&s->gc, s->reserved)) ? ON1X_OK : ON1X_ERR;
        }
        try {
            if (!table_set(&s->gc, tab, k, v)) return bad(s, "Table.FromEntries");
        } catch (...) {
            return stack_push(s, make_none(&s->gc, s->reserved)) ? ON1X_OK : ON1X_ERR;
        }
    }
    return stack_push(s, make_some(&s->gc, s->reserved, value_from_object(tab)))
        ? ON1X_OK : ON1X_ERR;
}

On1x_Status table_invert_fn(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 1, "Table.Invert")) return ON1X_ERR;
    Value tv;
    if (!read_argument(s, 1, tv) || tv.kind() != Value::Kind::Table) return bad(s, "Table.Invert");
    const auto* src = as_table_const(tv);
    auto* dst = new_table(&s->gc);
    GcRoot dst_root(dst);
    const auto* entry = src->entries;
    while (entry) {
        if (!is_hashable(entry->value)) {
            return stack_push(s, make_none(&s->gc, s->reserved)) ? ON1X_OK : ON1X_ERR;
        }
        if (!table_set(&s->gc, dst, entry->value, entry->key)) {
            return stack_push(s, make_none(&s->gc, s->reserved)) ? ON1X_OK : ON1X_ERR;
        }
        entry = entry->next;
    }
    return stack_push(s, make_some(&s->gc, s->reserved, value_from_object(dst)))
        ? ON1X_OK : ON1X_ERR;
}

On1x_Status table_pick_fn(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 2, "Table.Pick")) return ON1X_ERR;
    Value tv, keys_lv;
    if (!read_argument(s, 1, tv) || !read_argument(s, 2, keys_lv) ||
        tv.kind() != Value::Kind::Table || keys_lv.kind() != Value::Kind::List)
        return bad(s, "Table.Pick");
    const auto* src = as_table_const(tv);
    const auto* keys = as_list_const(keys_lv);
    auto* dst = new_table(&s->gc);
    GcRoot dst_root(dst);
    for (std::size_t i = 0; i < keys->length; ++i) {
        Value key;
        list_get(keys, i, key);
        Value value;
        if (table_get(src, key, value)) {
            if (!table_set(&s->gc, dst, key, value)) return bad(s, "Table.Pick");
        }
    }
    return stack_push(s, value_from_object(dst)) ? ON1X_OK : ON1X_ERR;
}

On1x_Status table_omit_fn(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 2, "Table.Omit")) return ON1X_ERR;
    Value tv, keys_lv;
    if (!read_argument(s, 1, tv) || !read_argument(s, 2, keys_lv) ||
        tv.kind() != Value::Kind::Table || keys_lv.kind() != Value::Kind::List)
        return bad(s, "Table.Omit");
    const auto* src = as_table_const(tv);
    const auto* keys = as_list_const(keys_lv);
    auto* dst = new_table(&s->gc);
    GcRoot dst_root(dst);
    const auto* entry = src->entries;
    while (entry) {
        if (!table_set(&s->gc, dst, entry->key, entry->value)) return bad(s, "Table.Omit");
        entry = entry->next;
    }
    for (std::size_t i = 0; i < keys->length; ++i) {
        Value key;
        list_get(keys, i, key);
        Value dummy;
        table_remove(dst, key, dummy);
    }
    return stack_push(s, value_from_object(dst)) ? ON1X_OK : ON1X_ERR;
}

On1x_Status table_is_hashable_fn(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 1, "Table.IsHashable")) return ON1X_ERR;
    Value v;
    if (!read_argument(s, 1, v)) return bad(s, "Table.IsHashable");
    return stack_push(s, Value::boolean(is_hashable(v))) ? ON1X_OK : ON1X_ERR;
}

}  // namespace on1x::stdlib
