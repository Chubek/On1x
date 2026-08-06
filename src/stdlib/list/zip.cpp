// spec §17: List zip functions — Zip, Unzip, Enumerate, Flatten, Chunk, Window.

#include "api/api_common.hpp"
#include "core/list.hpp"
#include "core/optional.hpp"
#include "core/value.hpp"
#include "gc/roots.hpp"
#include "runtime/state.hpp"
#include "stdlib/args.hpp"
#include "stdlib/list/list_fns.hpp"

#include <cstdint>

namespace on1x::stdlib {

namespace {

static On1x_Status bad(On1x_State* s, const char* m) noexcept {
    return push_api_error(s, m);
}

}  // namespace

On1x_Status list_zip_fn(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 2, "List.Zip")) return ON1X_ERR;
    Value a, b;
    if (!read_argument(s, 1, a) || !read_argument(s, 2, b) ||
        a.kind() != Value::Kind::List || b.kind() != Value::Kind::List)
        return bad(s, "List.Zip");
    const auto* la = as_list_const(a);
    const auto* lb = as_list_const(b);
    std::size_t len = la->length < lb->length ? la->length : lb->length;
    auto* result = new_list(&s->gc, len);
    GcRoot result_root(result);
    for (std::size_t i = 0; i < len; ++i) {
        Value va, vb;
        list_get(la, i, va);
        list_get(lb, i, vb);
        auto* pair = new_list(&s->gc, 2);
        GcRoot pair_root(pair);
        if (!list_push(&s->gc, pair, va) || !list_push(&s->gc, pair, vb))
            return bad(s, "List.Zip");
        if (!list_push(&s->gc, result, value_from_object(pair)))
            return bad(s, "List.Zip");
    }
    return stack_push(s, value_from_object(result)) ? ON1X_OK : ON1X_ERR;
}

On1x_Status list_unzip_fn(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 1, "List.Unzip")) return ON1X_ERR;
    Value lv;
    if (!read_argument(s, 1, lv) || lv.kind() != Value::Kind::List)
        return bad(s, "List.Unzip");
    const auto* src = as_list_const(lv);
    auto* left = new_list(&s->gc, src->length);
    auto* right = new_list(&s->gc, src->length);
    GcRoot left_root(left);
    GcRoot right_root(right);
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
        Value first, second;
        list_get(p, 0, first);
        list_get(p, 1, second);
        if (!list_push(&s->gc, left, first) || !list_push(&s->gc, right, second))
            return bad(s, "List.Unzip");
    }
    auto* result = new_list(&s->gc, 2);
    GcRoot result_root(result);
    if (!list_push(&s->gc, result, value_from_object(left)) ||
        !list_push(&s->gc, result, value_from_object(right)))
        return bad(s, "List.Unzip");
    return stack_push(s, make_some(&s->gc, s->reserved, value_from_object(result)))
        ? ON1X_OK : ON1X_ERR;
}

On1x_Status list_enumerate_fn(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 1, "List.Enumerate")) return ON1X_ERR;
    Value lv;
    if (!read_argument(s, 1, lv) || lv.kind() != Value::Kind::List)
        return bad(s, "List.Enumerate");
    const auto* src = as_list_const(lv);
    auto* result = new_list(&s->gc, src->length);
    GcRoot result_root(result);
    for (std::size_t i = 0; i < src->length; ++i) {
        Value item;
        list_get(src, i, item);
        auto* pair = new_list(&s->gc, 2);
        GcRoot pair_root(pair);
        Value idx = Value::integer(&s->gc, static_cast<std::int64_t>(i));
        if (!list_push(&s->gc, pair, idx) || !list_push(&s->gc, pair, item))
            return bad(s, "List.Enumerate");
        if (!list_push(&s->gc, result, value_from_object(pair)))
            return bad(s, "List.Enumerate");
    }
    return stack_push(s, value_from_object(result)) ? ON1X_OK : ON1X_ERR;
}

On1x_Status list_flatten_fn(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 1, "List.Flatten")) return ON1X_ERR;
    Value lv;
    if (!read_argument(s, 1, lv) || lv.kind() != Value::Kind::List)
        return bad(s, "List.Flatten");
    const auto* src = as_list_const(lv);
    // First pass: compute total length and validate each element is a List
    std::size_t total = 0;
    for (std::size_t i = 0; i < src->length; ++i) {
        Value item;
        list_get(src, i, item);
        if (item.kind() != Value::Kind::List) {
            return stack_push(s, make_none(&s->gc, s->reserved)) ? ON1X_OK : ON1X_ERR;
        }
        total += as_list_const(item)->length;
    }
    auto* result = new_list(&s->gc, total);
    GcRoot result_root(result);
    for (std::size_t i = 0; i < src->length; ++i) {
        Value item;
        list_get(src, i, item);
        const auto* inner = as_list_const(item);
        for (std::size_t j = 0; j < inner->length; ++j) {
            Value inner_item;
            list_get(inner, j, inner_item);
            if (!list_push(&s->gc, result, inner_item))
                return bad(s, "List.Flatten");
        }
    }
    return stack_push(s, make_some(&s->gc, s->reserved, value_from_object(result)))
        ? ON1X_OK : ON1X_ERR;
}

On1x_Status list_chunk_fn(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 2, "List.Chunk")) return ON1X_ERR;
    Value lv, nv;
    if (!read_argument(s, 1, lv) || !read_argument(s, 2, nv) ||
        lv.kind() != Value::Kind::List || !nv.is_int())
        return bad(s, "List.Chunk");
    const auto* src = as_list_const(lv);
    std::int64_t n = nv.as_int();
    if (n <= 0 || src->length == 0) {
        if (n <= 0) {
            return stack_push(s, make_none(&s->gc, s->reserved)) ? ON1X_OK : ON1X_ERR;
        }
        auto* empty = new_list(&s->gc, 0);
        return stack_push(s, make_some(&s->gc, s->reserved, value_from_object(empty)))
            ? ON1X_OK : ON1X_ERR;
    }
    std::size_t chunk_size = static_cast<std::size_t>(n);
    std::size_t num_chunks = (src->length + chunk_size - 1) / chunk_size;
    auto* result = new_list(&s->gc, num_chunks);
    GcRoot result_root(result);
    for (std::size_t i = 0; i < src->length; i += chunk_size) {
        std::size_t end = i + chunk_size;
        if (end > src->length) end = src->length;
        auto* chunk = new_list(&s->gc, end - i);
        GcRoot chunk_root(chunk);
        for (std::size_t j = i; j < end; ++j) {
            Value item;
            list_get(src, j, item);
            if (!list_push(&s->gc, chunk, item)) return bad(s, "List.Chunk");
        }
        if (!list_push(&s->gc, result, value_from_object(chunk)))
            return bad(s, "List.Chunk");
    }
    return stack_push(s, make_some(&s->gc, s->reserved, value_from_object(result)))
        ? ON1X_OK : ON1X_ERR;
}

On1x_Status list_window_fn(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 2, "List.Window")) return ON1X_ERR;
    Value lv, nv;
    if (!read_argument(s, 1, lv) || !read_argument(s, 2, nv) ||
        lv.kind() != Value::Kind::List || !nv.is_int())
        return bad(s, "List.Window");
    const auto* src = as_list_const(lv);
    std::int64_t n = nv.as_int();
    if (n <= 0) {
        return stack_push(s, make_none(&s->gc, s->reserved)) ? ON1X_OK : ON1X_ERR;
    }
    std::size_t win_size = static_cast<std::size_t>(n);
    if (win_size > src->length) {
        return stack_push(s, make_none(&s->gc, s->reserved)) ? ON1X_OK : ON1X_ERR;
    }
    std::size_t num_windows = src->length - win_size + 1;
    auto* result = new_list(&s->gc, num_windows);
    GcRoot result_root(result);
    for (std::size_t i = 0; i < num_windows; ++i) {
        auto* window = new_list(&s->gc, win_size);
        GcRoot window_root(window);
        for (std::size_t j = 0; j < win_size; ++j) {
            Value item;
            list_get(src, i + j, item);
            if (!list_push(&s->gc, window, item)) return bad(s, "List.Window");
        }
        if (!list_push(&s->gc, result, value_from_object(window)))
            return bad(s, "List.Window");
    }
    return stack_push(s, make_some(&s->gc, s->reserved, value_from_object(result)))
        ? ON1X_OK : ON1X_ERR;
}

}  // namespace on1x::stdlib
