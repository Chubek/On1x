#include "api/api_common.hpp"
#include "core/list.hpp"
#include "core/optional.hpp"
#include "core/value.hpp"
#include "gc/roots.hpp"
#include "runtime/state.hpp"
#include "stdlib/args.hpp"

#include <cstdint>

namespace on1x::stdlib {

namespace {
static On1x_Status bad(On1x_State* s, const char* m) noexcept { return push_api_error(s, m); }
}  // namespace

On1x_Status list_first_fn(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 1, "List.First")) return ON1X_ERR;
     Value v;
     if (!read_argument(s, 1, v) || v.kind() != Value::Kind::List) return bad(s, "List.First");
     const auto* list = as_list_const(v);
     if (list->length == 0) return stack_push(s, make_none(&s->gc, s->reserved)) ? ON1X_OK : ON1X_ERR;
     Value first;
     list_get(list, 0, first);
     return stack_push(s, make_some(&s->gc, s->reserved, first)) ? ON1X_OK : ON1X_ERR;
}

On1x_Status list_last_fn(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 1, "List.Last")) return ON1X_ERR;
     Value v;
     if (!read_argument(s, 1, v) || v.kind() != Value::Kind::List) return bad(s, "List.Last");
     const auto* list = as_list_const(v);
     if (list->length == 0) return stack_push(s, make_none(&s->gc, s->reserved)) ? ON1X_OK : ON1X_ERR;
     Value last;
     list_get(list, list->length - 1, last);
     return stack_push(s, make_some(&s->gc, s->reserved, last)) ? ON1X_OK : ON1X_ERR;
}

On1x_Status list_take_fn(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 2, "List.Take")) return ON1X_ERR;
     Value lv, nv;
     if (!read_argument(s, 1, lv) || !read_argument(s, 2, nv) ||
         lv.kind() != Value::Kind::List || !nv.is_int()) return bad(s, "List.Take");
     const auto* src = as_list_const(lv);
     std::int64_t n = nv.as_int();
     if (n < 0) n = 0;
     if (n > static_cast<std::int64_t>(src->length)) n = static_cast<std::int64_t>(src->length);
     auto* dst = new_list(&s->gc, static_cast<std::size_t>(n));
     GcRoot root(dst);
     for (std::size_t i = 0; i < static_cast<std::size_t>(n); ++i) {
         Value item;
         list_get(src, i, item);
         list_push(&s->gc, dst, item);
     }
     return stack_push(s, value_from_object(dst)) ? ON1X_OK : ON1X_ERR;
}

On1x_Status list_drop_fn(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 2, "List.Drop")) return ON1X_ERR;
     Value lv, nv;
     if (!read_argument(s, 1, lv) || !read_argument(s, 2, nv) ||
         lv.kind() != Value::Kind::List || !nv.is_int()) return bad(s, "List.Drop");
     const auto* src = as_list_const(lv);
     std::int64_t n = nv.as_int();
     if (n < 0) n = 0;
     if (n > static_cast<std::int64_t>(src->length)) n = static_cast<std::int64_t>(src->length);
     std::size_t remaining = src->length - static_cast<std::size_t>(n);
     auto* dst = new_list(&s->gc, remaining);
     GcRoot root(dst);
     for (std::size_t i = static_cast<std::size_t>(n); i < src->length; ++i) {
         Value item;
         list_get(src, i, item);
         list_push(&s->gc, dst, item);
     }
     return stack_push(s, value_from_object(dst)) ? ON1X_OK : ON1X_ERR;
}

On1x_Status list_insert_fn(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 3, "List.Insert")) return ON1X_ERR;
     Value lv, iv, vv;
     if (!read_argument(s, 1, lv) || !read_argument(s, 2, iv) || !read_argument(s, 3, vv) ||
         lv.kind() != Value::Kind::List || !iv.is_int()) return bad(s, "List.Insert");
     auto* list = as_list(lv);
     if (!list) return bad(s, "List.Insert");
     std::int64_t idx = iv.as_int();
     if (idx < 0) idx += static_cast<std::int64_t>(list->length);
     if (idx < 0 || idx > static_cast<std::int64_t>(list->length))
         return stack_push(s, Value::boolean(false)) ? ON1X_OK : ON1X_ERR;
     // Insert in place: shift elements right and set
     GcRoot root(list);
     if (!list_push(&s->gc, list, Value::unit())) return bad(s, "List.Insert");
     for (std::size_t i = list->length - 1; i > static_cast<std::size_t>(idx); --i) {
         list->items[i] = list->items[i - 1];
     }
     list->items[static_cast<std::size_t>(idx)] = vv;
     return stack_push(s, Value::boolean(true)) ? ON1X_OK : ON1X_ERR;
}

On1x_Status list_remove_fn(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 2, "List.Remove")) return ON1X_ERR;
     Value lv, iv;
     if (!read_argument(s, 1, lv) || !read_argument(s, 2, iv) ||
         lv.kind() != Value::Kind::List || !iv.is_int()) return bad(s, "List.Remove");
     auto* list = as_list(lv);
     if (!list) return bad(s, "List.Remove");
     std::int64_t idx = iv.as_int();
     if (idx < 0) idx += static_cast<std::int64_t>(list->length);
     if (idx < 0 || idx >= static_cast<std::int64_t>(list->length))
         return stack_push(s, make_none(&s->gc, s->reserved)) ? ON1X_OK : ON1X_ERR;
     Value removed = list->items[static_cast<std::size_t>(idx)];
     for (std::size_t i = static_cast<std::size_t>(idx); i + 1 < list->length; ++i) {
         list->items[i] = list->items[i + 1];
     }
     --list->length;
     return stack_push(s, make_some(&s->gc, s->reserved, removed)) ? ON1X_OK : ON1X_ERR;
}

On1x_Status list_reverse_fn(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 1, "List.Reverse")) return ON1X_ERR;
     Value v;
     if (!read_argument(s, 1, v) || v.kind() != Value::Kind::List) return bad(s, "List.Reverse");
     const auto* src = as_list_const(v);
     auto* dst = new_list(&s->gc, src->length);
     GcRoot root(dst);
     for (std::size_t i = 0; i < src->length; ++i) {
         Value item;
         list_get(src, src->length - 1 - i, item);
         list_push(&s->gc, dst, item);
     }
     return stack_push(s, value_from_object(dst)) ? ON1X_OK : ON1X_ERR;
}

On1x_Status list_reverse_in_place_fn(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 1, "List.ReverseInPlace")) return ON1X_ERR;
     Value v;
     if (!read_argument(s, 1, v) || v.kind() != Value::Kind::List) return bad(s, "List.ReverseInPlace");
     auto* list = as_list(v);
     if (!list) return bad(s, "List.ReverseInPlace");
     for (std::size_t i = 0; i < list->length / 2; ++i) {
         Value tmp = list->items[i];
         list->items[i] = list->items[list->length - 1 - i];
         list->items[list->length - 1 - i] = tmp;
     }
     return stack_push(s, Value::unit()) ? ON1X_OK : ON1X_ERR;
}

On1x_Status list_fill_fn(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 2, "List.Fill")) return ON1X_ERR;
     Value lv, fv;
     if (!read_argument(s, 1, lv) || !read_argument(s, 2, fv) ||
         lv.kind() != Value::Kind::List) return bad(s, "List.Fill");
     auto* list = as_list(lv);
     if (!list) return bad(s, "List.Fill");
     GcRoot root(list);
     for (std::size_t i = 0; i < list->length; ++i) {
         list->items[i] = fv;
     }
     return stack_push(s, value_from_object(list)) ? ON1X_OK : ON1X_ERR;
}

}  // namespace on1x::stdlib
