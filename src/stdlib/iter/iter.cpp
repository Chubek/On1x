#include "api/api_common.hpp"
#include "core/hashing.hpp"
#include "core/list.hpp"
#include "core/optional.hpp"
#include "core/table.hpp"
#include "core/tagged_list.hpp"
#include "core/value.hpp"
#include "gc/alloc.hpp"
#include "gc/roots.hpp"
#include "on1x/on1x_capability.h"
#include "on1x/on1x_module.h"
#include "runtime/closure.hpp"
#include "runtime/state.hpp"
#include "stdlib/args.hpp"
#include "stdlib/iter/iter.hpp"
#include "vm/interpreter.hpp"

#include <cstdint>
#include <cstring>
#include <cmath>

namespace on1x::stdlib {
namespace {

static On1x_Status bad(On1x_State* s, const char* m) noexcept {
    return push_api_error(s, m);
}

// ---- Iterator helpers ----

// An iterator is a Tagged List: :Iter[state]
// state is a List: [source_data/value, position, ...adapter_context]

// Tag for :Iter
static const char iter_tag_name[] = "Iter";

static TagObject* iter_tag(On1x_State* s) {
    return s->tags.intern(&s->gc, iter_tag_name);
}

// Make an iterator value from a state list
static Value make_iter(On1x_State* s, ListObject* state) {
    auto* tag = iter_tag(s);
    auto* tl = new_tagged_list(&s->gc, tag, 1);
    if (!tl) return Value();
    GcRoot root(tl);
    list_push(&s->gc, tl, value_from_object(state));
    return value_from_object(tl);
}

// Check if value is an :Iter tagged list
static bool is_iter_value(On1x_State* s, Value v) {
    if (v.kind() != Value::Kind::List) return false;
    const auto* list = as_list_const(v);
    if (!list || !list->constructor) return false;
    auto* tag = iter_tag(s);
    return list->constructor == tag;
}

// Get the state list from an iterator value
static ListObject* iter_state(On1x_State* s, Value v) {
    if (!is_iter_value(s, v)) return nullptr;
    const auto* list = as_list_const(v);
    Value state_val;
    if (!list_get(list, 0, state_val) || state_val.kind() != Value::Kind::List) return nullptr;
    return as_list(state_val);
}

// ---- Call helper ----

static bool call_fn(On1x_State* s, FunctionObject* fn, const Value* args, std::size_t count, Value& result) noexcept {
    const char* error = nullptr;
    return vm::invoke_function(s, fn, args, count, result, error);
}

// ---- FromList ----

On1x_Status iter_from_list(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 1, "Iter.FromList")) return ON1X_ERR;
    Value v;
    if (!read_argument(s, 1, v) || v.kind() != Value::Kind::List)
        return bad(s, "Iter.FromList expects a List");
    // State: [source_list, position(0)]
    auto* state = new_list(&s->gc, 2);
    if (!state) return bad(s, "Iter.FromList");
    GcRoot state_root(state);
    list_push(&s->gc, state, v);  // source list
    list_push(&s->gc, state, Value::integer(&s->gc, 0));  // position = 0
    Value it = make_iter(s, state);
    return stack_push(s, it) ? ON1X_OK : ON1X_ERR;
}

// ---- Next ----

On1x_Status iter_next(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 1, "Iter.Next")) return ON1X_ERR;
    Value v;
    if (!read_argument(s, 1, v)) return bad(s, "Iter.Next");
    if (!is_iter_value(s, v)) return bad(s, "Iter.Next expects an iterator");
    ListObject* state = iter_state(s, v);
    if (!state || state->length < 2) return bad(s, "Iter.Next: invalid iterator");

    // state[0] = source, state[1] = position
    Value source;
    Value pos_val;
    list_get(state, 0, source);
    list_get(state, 1, pos_val);
    if (!pos_val.is_int()) return bad(s, "Iter.Next: invalid position");

    std::int64_t pos = pos_val.as_int();

    if (source.kind() == Value::Kind::List) {
        const auto* src = as_list_const(source);
        if (static_cast<std::size_t>(pos) >= src->length) {
            return stack_push(s, make_none(&s->gc, s->reserved)) ? ON1X_OK : ON1X_ERR;
        }
        Value item;
        list_get(src, static_cast<std::size_t>(pos), item);
        Value new_pos = Value::integer(&s->gc, pos + 1);
        list_set(state, 1, new_pos);
        GcRoot item_root(item.is_object() ? item.as_object() : nullptr);
        return stack_push(s, make_some(&s->gc, s->reserved, item)) ? ON1X_OK : ON1X_ERR;
    }
    // Source is a function (FromFn): state = [fn, pos]
    if (source.kind() == Value::Kind::Function) {
        auto* fn = static_cast<FunctionObject*>(source.as_object());
        GcRoot fn_root(fn);
        Value result;
        if (!call_fn(s, fn, nullptr, 0, result)) {
            return stack_push(s, make_none(&s->gc, s->reserved)) ? ON1X_OK : ON1X_ERR;
        }
        if (!is_some(result, s->reserved)) {
            return stack_push(s, make_none(&s->gc, s->reserved)) ? ON1X_OK : ON1X_ERR;
        }
        Value payload;
        unwrap_some(result, s->reserved, payload);
        Value new_pos = Value::integer(&s->gc, pos + 1);
        list_set(state, 1, new_pos);
        return stack_push(s, make_some(&s->gc, s->reserved, payload)) ? ON1X_OK : ON1X_ERR;
    }
    // Source is a table (FromTable): state = [table, keys_list, pos]
    if (source.kind() == Value::Kind::Table && state->length >= 3) {
        Value keys_val, pos_val2;
        list_get(state, 1, keys_val);
        list_get(state, 2, pos_val2);
        if (keys_val.kind() != Value::Kind::List || !pos_val2.is_int())
            return stack_push(s, make_none(&s->gc, s->reserved)) ? ON1X_OK : ON1X_ERR;
        const auto* keys = as_list_const(keys_val);
        std::int64_t table_pos = pos_val2.as_int();
        if (static_cast<std::size_t>(table_pos) >= keys->length)
            return stack_push(s, make_none(&s->gc, s->reserved)) ? ON1X_OK : ON1X_ERR;
        Value key;
        list_get(keys, static_cast<std::size_t>(table_pos), key);
        // For table iteration, return a [key, value] pair
        const auto* table = as_table_const(source);
        Value val;
        if (!table_get(table, key, val))
            return stack_push(s, make_none(&s->gc, s->reserved)) ? ON1X_OK : ON1X_ERR;
        auto* pair = new_list(&s->gc, 2);
        if (!pair) return bad(s, "Iter.Next");
        GcRoot pair_root(pair);
        list_push(&s->gc, pair, key);
        list_push(&s->gc, pair, val);
        Value new_pos = Value::integer(&s->gc, table_pos + 1);
        list_set(state, 2, new_pos);
        return stack_push(s, make_some(&s->gc, s->reserved, value_from_object(pair))) ? ON1X_OK : ON1X_ERR;
    }
    if (source.kind() == Value::Kind::Table) {
        return stack_push(s, make_none(&s->gc, s->reserved)) ? ON1X_OK : ON1X_ERR;
    }

    return stack_push(s, make_none(&s->gc, s->reserved)) ? ON1X_OK : ON1X_ERR;
}

// ---- Map ----

On1x_Status iter_map(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 2, "Iter.Map")) return ON1X_ERR;
    Value it_v, fn_v;
    if (!read_argument(s, 1, it_v) || !read_argument(s, 2, fn_v))
        return bad(s, "Iter.Map");
    if (!is_iter_value(s, it_v) || fn_v.kind() != Value::Kind::Function)
        return bad(s, "Iter.Map expects an iterator and an Fn");
    // State: [source_iterator, type_tag("map"), fn]
    auto* state = new_list(&s->gc, 3);
    if (!state) return bad(s, "Iter.Map");
    GcRoot state_root(state);
    list_push(&s->gc, state, it_v);  // source iterator
    auto* map_tag = s->tags.intern(&s->gc, "map");
    list_push(&s->gc, state, value_from_object(map_tag));  // adapter type
    list_push(&s->gc, state, fn_v);  // transform function
    Value it = make_iter(s, state);
    return stack_push(s, it) ? ON1X_OK : ON1X_ERR;
}

// ---- Filter ----

On1x_Status iter_filter(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 2, "Iter.Filter")) return ON1X_ERR;
    Value it_v, fn_v;
    if (!read_argument(s, 1, it_v) || !read_argument(s, 2, fn_v))
        return bad(s, "Iter.Filter");
    if (!is_iter_value(s, it_v) || fn_v.kind() != Value::Kind::Function)
        return bad(s, "Iter.Filter expects an iterator and an Fn");
    auto* state = new_list(&s->gc, 3);
    if (!state) return bad(s, "Iter.Filter");
    GcRoot state_root(state);
    list_push(&s->gc, state, it_v);
    auto* filter_tag = s->tags.intern(&s->gc, "filter");
    list_push(&s->gc, state, value_from_object(filter_tag));
    list_push(&s->gc, state, fn_v);
    Value it = make_iter(s, state);
    return stack_push(s, it) ? ON1X_OK : ON1X_ERR;
}

// ---- Take ----

On1x_Status iter_take(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 2, "Iter.Take")) return ON1X_ERR;
    Value it_v, n_v;
    if (!read_argument(s, 1, it_v) || !read_argument(s, 2, n_v))
        return bad(s, "Iter.Take");
    if (!is_iter_value(s, it_v) || !n_v.is_int())
        return bad(s, "Iter.Take expects an iterator and an Int");
    auto* state = new_list(&s->gc, 3);
    if (!state) return bad(s, "Iter.Take");
    GcRoot state_root(state);
    list_push(&s->gc, state, it_v);
    auto* take_tag = s->tags.intern(&s->gc, "take");
    list_push(&s->gc, state, value_from_object(take_tag));
    list_push(&s->gc, state, n_v);
    Value it = make_iter(s, state);
    return stack_push(s, it) ? ON1X_OK : ON1X_ERR;
}

// ---- Drop ----

On1x_Status iter_drop(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 2, "Iter.Drop")) return ON1X_ERR;
    Value it_v, n_v;
    if (!read_argument(s, 1, it_v) || !read_argument(s, 2, n_v))
        return bad(s, "Iter.Drop");
    if (!is_iter_value(s, it_v) || !n_v.is_int())
        return bad(s, "Iter.Drop expects an iterator and an Int");
    auto* state = new_list(&s->gc, 3);
    if (!state) return bad(s, "Iter.Drop");
    GcRoot state_root(state);
    list_push(&s->gc, state, it_v);
    auto* drop_tag = s->tags.intern(&s->gc, "drop");
    list_push(&s->gc, state, value_from_object(drop_tag));
    list_push(&s->gc, state, n_v);
    Value it = make_iter(s, state);
    return stack_push(s, it) ? ON1X_OK : ON1X_ERR;
}

// ---- Collect ----

On1x_Status iter_collect(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 1, "Iter.Collect")) return ON1X_ERR;
    Value it_v;
    if (!read_argument(s, 1, it_v)) return bad(s, "Iter.Collect");
    if (!is_iter_value(s, it_v)) return bad(s, "Iter.Collect expects an iterator");
    auto* result = new_list(&s->gc, 8);
    if (!result) return bad(s, "Iter.Collect");
    GcRoot result_root(result);

    // Consume iterator by repeatedly calling Next
    // We call the Next function from the module itself
    while (true) {
        // Simulate calling Next(it_v) by implementing inline
        ListObject* state = iter_state(s, it_v);
        if (!state || state->length < 2) break;

        Value source;
        list_get(state, 0, source);
        Value pos_val;
        list_get(state, 1, pos_val);
        if (!pos_val.is_int()) break;
        std::int64_t pos = pos_val.as_int();

        if (source.kind() == Value::Kind::List) {
            const auto* src = as_list_const(source);
            if (static_cast<std::size_t>(pos) >= src->length) break;
            Value item;
            list_get(src, static_cast<std::size_t>(pos), item);
            list_push(&s->gc, result, item);
            Value new_pos = Value::integer(&s->gc, pos + 1);
            list_set(state, 1, new_pos);
        } else {
            break;
        }
    }

    return stack_push(s, value_from_object(result)) ? ON1X_OK : ON1X_ERR;
}

// ---- Fold ----

On1x_Status iter_fold(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 3, "Iter.Fold")) return ON1X_ERR;
    Value it_v, fn_v, init_v;
    if (!read_argument(s, 1, it_v) || !read_argument(s, 2, fn_v) || !read_argument(s, 3, init_v))
        return bad(s, "Iter.Fold");
    if (!is_iter_value(s, it_v) || fn_v.kind() != Value::Kind::Function)
        return bad(s, "Iter.Fold expects an iterator, an Fn, and an init value");
    auto* fn = static_cast<FunctionObject*>(fn_v.as_object());
    GcRoot fn_root(fn);

    Value acc = init_v;
    GcRoot acc_root(acc.is_object() ? acc.as_object() : nullptr);

    while (true) {
        ListObject* state = iter_state(s, it_v);
        if (!state || state->length < 2) break;
        Value source;
        list_get(state, 0, source);
        Value pos_val;
        list_get(state, 1, pos_val);
        if (!pos_val.is_int()) break;
        std::int64_t pos = pos_val.as_int();

        if (source.kind() == Value::Kind::List) {
            const auto* src = as_list_const(source);
            if (static_cast<std::size_t>(pos) >= src->length) break;
            Value item;
            list_get(src, static_cast<std::size_t>(pos), item);
            Value args[2] = {acc, item};
            if (!call_fn(s, fn, args, 2, acc)) return push_api_error(s, "Iter.Fold: call failed");
            acc_root.set(acc.is_object() ? acc.as_object() : nullptr);
            Value new_pos = Value::integer(&s->gc, pos + 1);
            list_set(state, 1, new_pos);
        } else {
            break;
        }
    }
    return stack_push(s, acc) ? ON1X_OK : ON1X_ERR;
}

// ---- Count ----

On1x_Status iter_count(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 1, "Iter.Count")) return ON1X_ERR;
    Value it_v;
    if (!read_argument(s, 1, it_v)) return bad(s, "Iter.Count");
    if (!is_iter_value(s, it_v)) return bad(s, "Iter.Count expects an iterator");

    std::int64_t count = 0;
    while (true) {
        ListObject* state = iter_state(s, it_v);
        if (!state || state->length < 2) break;
        Value source;
        list_get(state, 0, source);
        Value pos_val;
        list_get(state, 1, pos_val);
        if (!pos_val.is_int()) break;
        std::int64_t pos = pos_val.as_int();

        if (source.kind() == Value::Kind::List) {
            const auto* src = as_list_const(source);
            if (static_cast<std::size_t>(pos) >= src->length) break;
            ++count;
            Value new_pos = Value::integer(&s->gc, pos + 1);
            list_set(state, 1, new_pos);
        } else {
            break;
        }
    }
    return stack_push(s, Value::integer(&s->gc, count)) ? ON1X_OK : ON1X_ERR;
}

// ---- ForEach ----

On1x_Status iter_for_each(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 2, "Iter.ForEach")) return ON1X_ERR;
    Value it_v, fn_v;
    if (!read_argument(s, 1, it_v) || !read_argument(s, 2, fn_v))
        return bad(s, "Iter.ForEach");
    if (!is_iter_value(s, it_v) || fn_v.kind() != Value::Kind::Function)
        return bad(s, "Iter.ForEach expects an iterator and an Fn");
    auto* fn = static_cast<FunctionObject*>(fn_v.as_object());
    GcRoot fn_root(fn);

    while (true) {
        ListObject* state = iter_state(s, it_v);
        if (!state || state->length < 2) break;
        Value source;
        list_get(state, 0, source);
        Value pos_val;
        list_get(state, 1, pos_val);
        if (!pos_val.is_int()) break;
        std::int64_t pos = pos_val.as_int();

        if (source.kind() == Value::Kind::List) {
            const auto* src = as_list_const(source);
            if (static_cast<std::size_t>(pos) >= src->length) break;
            Value item;
            list_get(src, static_cast<std::size_t>(pos), item);
            Value args[1] = {item};
            Value discard;
            if (!call_fn(s, fn, args, 1, discard)) return push_api_error(s, "Iter.ForEach: call failed");
            Value new_pos = Value::integer(&s->gc, pos + 1);
            list_set(state, 1, new_pos);
        } else {
            break;
        }
    }
    return stack_push(s, Value::boolean(false)) ? ON1X_OK : ON1X_ERR;
}

// ---- FromTable ----

On1x_Status iter_from_table(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 1, "Iter.FromTable")) return ON1X_ERR;
    Value v;
    if (!read_argument(s, 1, v) || v.kind() != Value::Kind::Table)
        return bad(s, "Iter.FromTable expects a Table");
    // Build a list of keys for iteration order
    const auto* table = as_table_const(v);
    auto* keys = new_list(&s->gc, table->length);
    if (!keys) return bad(s, "Iter.FromTable");
    GcRoot keys_root(keys);
    for (const TableEntry* e = table->entries; e; e = e->next) {
        list_push(&s->gc, keys, e->key);
    }
    // State: [table, keys_list, position(0)]
    auto* state = new_list(&s->gc, 3);
    if (!state) return bad(s, "Iter.FromTable");
    GcRoot state_root(state);
    list_push(&s->gc, state, v);
    list_push(&s->gc, state, value_from_object(keys));
    list_push(&s->gc, state, Value::integer(&s->gc, 0));
    Value it = make_iter(s, state);
    return stack_push(s, it) ? ON1X_OK : ON1X_ERR;
}

// ---- FromIota ----

On1x_Status iter_from_iota(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 1, "Iter.FromIota")) return ON1X_ERR;
    Value v;
    if (!read_argument(s, 1, v)) return bad(s, "Iter.FromIota expects a List (Iota result)");
    if (v.kind() != Value::Kind::List)
        return bad(s, "Iter.FromIota expects a List");
    return iter_from_list(s, argc);
}

// ---- FromFn ----

On1x_Status iter_from_fn(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 1, "Iter.FromFn")) return ON1X_ERR;
    Value v;
    if (!read_argument(s, 1, v) || v.kind() != Value::Kind::Function)
        return bad(s, "Iter.FromFn expects an Fn");
    // State: [fn, done_flag(0)]
    auto* state = new_list(&s->gc, 2);
    if (!state) return bad(s, "Iter.FromFn");
    GcRoot state_root(state);
    list_push(&s->gc, state, v);
    list_push(&s->gc, state, Value::integer(&s->gc, 0));
    Value it = make_iter(s, state);
    return stack_push(s, it) ? ON1X_OK : ON1X_ERR;
}

// ---- TakeWhile ----

On1x_Status iter_take_while(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 2, "Iter.TakeWhile")) return ON1X_ERR;
    Value it_v, pred_v;
    if (!read_argument(s, 1, it_v) || !read_argument(s, 2, pred_v))
        return bad(s, "Iter.TakeWhile");
    if (!is_iter_value(s, it_v) || pred_v.kind() != Value::Kind::Function)
        return bad(s, "Iter.TakeWhile expects an iterator and an Fn");
    auto* state = new_list(&s->gc, 2);
    if (!state) return bad(s, "Iter.TakeWhile");
    GcRoot state_root(state);
    list_push(&s->gc, state, it_v);
    list_push(&s->gc, state, pred_v);
    Value it = make_iter(s, state);
    return stack_push(s, it) ? ON1X_OK : ON1X_ERR;
}

// ---- DropWhile ----

On1x_Status iter_drop_while(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 2, "Iter.DropWhile")) return ON1X_ERR;
    Value it_v, pred_v;
    if (!read_argument(s, 1, it_v) || !read_argument(s, 2, pred_v))
        return bad(s, "Iter.DropWhile");
    if (!is_iter_value(s, it_v) || pred_v.kind() != Value::Kind::Function)
        return bad(s, "Iter.DropWhile expects an iterator and an Fn");
    // State: [source_iter, pred_fn, dropped_flag(0)]
    auto* state = new_list(&s->gc, 3);
    if (!state) return bad(s, "Iter.DropWhile");
    GcRoot state_root(state);
    list_push(&s->gc, state, it_v);
    list_push(&s->gc, state, pred_v);
    list_push(&s->gc, state, Value::integer(&s->gc, 0));
    Value it = make_iter(s, state);
    return stack_push(s, it) ? ON1X_OK : ON1X_ERR;
}

// ---- Chain ----

On1x_Status iter_chain(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 2, "Iter.Chain")) return ON1X_ERR;
    Value a, b;
    if (!read_argument(s, 1, a) || !read_argument(s, 2, b))
        return bad(s, "Iter.Chain");
    if (!is_iter_value(s, a) || !is_iter_value(s, b))
        return bad(s, "Iter.Chain expects two iterators");
    // State: [first_iter, second_iter, phase(0=first, 1=second)]
    auto* state = new_list(&s->gc, 3);
    if (!state) return bad(s, "Iter.Chain");
    GcRoot state_root(state);
    list_push(&s->gc, state, a);
    list_push(&s->gc, state, b);
    list_push(&s->gc, state, Value::integer(&s->gc, 0));
    Value it = make_iter(s, state);
    return stack_push(s, it) ? ON1X_OK : ON1X_ERR;
}

// ---- Zip ----

On1x_Status iter_zip(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 2, "Iter.Zip")) return ON1X_ERR;
    Value a, b;
    if (!read_argument(s, 1, a) || !read_argument(s, 2, b))
        return bad(s, "Iter.Zip");
    if (!is_iter_value(s, a) || !is_iter_value(s, b))
        return bad(s, "Iter.Zip expects two iterators");
    // State: [iter_a, iter_b]
    auto* state = new_list(&s->gc, 2);
    if (!state) return bad(s, "Iter.Zip");
    GcRoot state_root(state);
    list_push(&s->gc, state, a);
    list_push(&s->gc, state, b);
    Value it = make_iter(s, state);
    return stack_push(s, it) ? ON1X_OK : ON1X_ERR;
}

// ---- Sum ----

On1x_Status iter_sum(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 1, "Iter.Sum")) return ON1X_ERR;
    Value it_v;
    if (!read_argument(s, 1, it_v)) return bad(s, "Iter.Sum");
    if (!is_iter_value(s, it_v)) return bad(s, "Iter.Sum expects an iterator");

    bool has_int = false, has_float = false, empty = true;
    std::int64_t int_sum = 0;
    double float_sum = 0.0;

    while (true) {
        ListObject* state = iter_state(s, it_v);
        if (!state || state->length < 2) break;
        Value source;
        list_get(state, 0, source);
        Value pos_val;
        list_get(state, 1, pos_val);
        if (!pos_val.is_int()) break;
        std::int64_t pos = pos_val.as_int();

        if (source.kind() == Value::Kind::List) {
            const auto* src = as_list_const(source);
            if (static_cast<std::size_t>(pos) >= src->length) break;
            Value item;
            list_get(src, static_cast<std::size_t>(pos), item);
            empty = false;
            if (item.is_int()) {
                has_int = true;
                int_sum += item.as_int();
            } else if (item.is_float()) {
                has_float = true;
                float_sum += item.as_float();
            }
            Value new_pos = Value::integer(&s->gc, pos + 1);
            list_set(state, 1, new_pos);
        } else {
            break;
        }
    }
    if (empty) return stack_push(s, make_none(&s->gc, s->reserved)) ? ON1X_OK : ON1X_ERR;
    Value result = has_float ? Value::floating(static_cast<double>(int_sum) + float_sum)
                             : Value::integer(&s->gc, int_sum);
    return stack_push(s, make_some(&s->gc, s->reserved, result)) ? ON1X_OK : ON1X_ERR;
}

// ---- Product ----

On1x_Status iter_product(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 1, "Iter.Product")) return ON1X_ERR;
    Value it_v;
    if (!read_argument(s, 1, it_v)) return bad(s, "Iter.Product");
    if (!is_iter_value(s, it_v)) return bad(s, "Iter.Product expects an iterator");

    bool has_int = false, has_float = false, empty = true;
    std::int64_t int_prod = 1;
    double float_prod = 1.0;

    while (true) {
        ListObject* state = iter_state(s, it_v);
        if (!state || state->length < 2) break;
        Value source;
        list_get(state, 0, source);
        Value pos_val;
        list_get(state, 1, pos_val);
        if (!pos_val.is_int()) break;
        std::int64_t pos = pos_val.as_int();

        if (source.kind() == Value::Kind::List) {
            const auto* src = as_list_const(source);
            if (static_cast<std::size_t>(pos) >= src->length) break;
            Value item;
            list_get(src, static_cast<std::size_t>(pos), item);
            empty = false;
            if (item.is_int()) {
                has_int = true;
                int_prod *= item.as_int();
            } else if (item.is_float()) {
                has_float = true;
                float_prod *= item.as_float();
            }
            Value new_pos = Value::integer(&s->gc, pos + 1);
            list_set(state, 1, new_pos);
        } else {
            break;
        }
    }
    if (empty) return stack_push(s, make_none(&s->gc, s->reserved)) ? ON1X_OK : ON1X_ERR;
    Value result = has_float ? Value::floating(static_cast<double>(int_prod) * float_prod)
                             : Value::integer(&s->gc, int_prod);
    return stack_push(s, make_some(&s->gc, s->reserved, result)) ? ON1X_OK : ON1X_ERR;
}

// ---- Nth ----

On1x_Status iter_nth(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 2, "Iter.Nth")) return ON1X_ERR;
    Value it_v, n_v;
    if (!read_argument(s, 1, it_v) || !read_argument(s, 2, n_v))
        return bad(s, "Iter.Nth");
    if (!is_iter_value(s, it_v) || !n_v.is_int() || n_v.as_int() < 0)
        return bad(s, "Iter.Nth expects an iterator and a non-negative Int");
    std::int64_t target = n_v.as_int();
    std::int64_t count = 0;
    while (true) {
        ListObject* state = iter_state(s, it_v);
        if (!state || state->length < 2) break;
        Value source;
        list_get(state, 0, source);
        Value pos_val;
        list_get(state, 1, pos_val);
        if (!pos_val.is_int()) break;
        std::int64_t pos = pos_val.as_int();
        if (source.kind() == Value::Kind::List) {
            const auto* src = as_list_const(source);
            if (static_cast<std::size_t>(pos) >= src->length) break;
            if (count == target) {
                Value item;
                list_get(src, static_cast<std::size_t>(pos), item);
                return stack_push(s, make_some(&s->gc, s->reserved, item)) ? ON1X_OK : ON1X_ERR;
            }
            ++count;
            Value new_pos = Value::integer(&s->gc, pos + 1);
            list_set(state, 1, new_pos);
        } else {
            break;
        }
    }
    return stack_push(s, make_none(&s->gc, s->reserved)) ? ON1X_OK : ON1X_ERR;
}

// ---- Last ----

On1x_Status iter_last(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 1, "Iter.Last")) return ON1X_ERR;
    Value it_v;
    if (!read_argument(s, 1, it_v)) return bad(s, "Iter.Last");
    if (!is_iter_value(s, it_v)) return bad(s, "Iter.Last expects an iterator");
    Value last;
    bool has = false;
    while (true) {
        ListObject* state = iter_state(s, it_v);
        if (!state || state->length < 2) break;
        Value source;
        list_get(state, 0, source);
        Value pos_val;
        list_get(state, 1, pos_val);
        if (!pos_val.is_int()) break;
        std::int64_t pos = pos_val.as_int();
        if (source.kind() == Value::Kind::List) {
            const auto* src = as_list_const(source);
            if (static_cast<std::size_t>(pos) >= src->length) break;
            list_get(src, static_cast<std::size_t>(pos), last);
            has = true;
            Value new_pos = Value::integer(&s->gc, pos + 1);
            list_set(state, 1, new_pos);
        } else {
            break;
        }
    }
    if (!has) return stack_push(s, make_none(&s->gc, s->reserved)) ? ON1X_OK : ON1X_ERR;
    return stack_push(s, make_some(&s->gc, s->reserved, last)) ? ON1X_OK : ON1X_ERR;
}

const On1x_FnDesc fns[] = {
    {"FromList", iter_from_list},
    {"FromTable", iter_from_table},
    {"FromIota", iter_from_iota},
    {"FromFn", iter_from_fn},
    {"Next", iter_next},
    {"Map", iter_map},
    {"Filter", iter_filter},
    {"Take", iter_take},
    {"Drop", iter_drop},
    {"TakeWhile", iter_take_while},
    {"DropWhile", iter_drop_while},
    {"Chain", iter_chain},
    {"Zip", iter_zip},
    {"Collect", iter_collect},
    {"Fold", iter_fold},
    {"Count", iter_count},
    {"Sum", iter_sum},
    {"Product", iter_product},
    {"Nth", iter_nth},
    {"Last", iter_last},
    {"ForEach", iter_for_each},
};

const On1x_ModuleDesc desc{"Iter", ON1X_CAP_NONE, fns, sizeof(fns) / sizeof(*fns)};

}  // namespace

const On1x_ModuleDesc* iter_module() noexcept { return &desc; }

}  // namespace on1x::stdlib
