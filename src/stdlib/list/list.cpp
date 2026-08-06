#include "api/api_common.hpp"
#include "core/list.hpp"
#include "core/optional.hpp"
#include "core/value.hpp"
#include "gc/roots.hpp"
#include "on1x/on1x_capability.h"
#include "on1x/on1x_module.h"
#include "runtime/state.hpp"
#include "stdlib/args.hpp"
#include "stdlib/list/list_fns.hpp"

#include <cstdint>
#include <cstring>
 
namespace on1x::stdlib {
namespace {

static On1x_Status bad(On1x_State* s, const char* m) noexcept { return push_api_error(s, m); }

On1x_Status list_new_fn(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 2, "List.New")) return ON1X_ERR;
    Value n, fill;
     if (!read_argument(s, 1, n) || !read_argument(s, 2, fill) || !n.is_int()) return bad(s, "List.New");
     std::int64_t count = n.as_int();
     if (count < 0) return stack_push(s, make_none(&s->gc, s->reserved)) ? ON1X_OK : ON1X_ERR;
     auto* list = new_list(&s->gc, static_cast<std::size_t>(count));
     GcRoot root(list);
     for (std::int64_t i = 0; i < count; ++i) {
         if (!list_push(&s->gc, list, fill)) return bad(s, "List.New");
     }
     return stack_push(s, make_some(&s->gc, s->reserved, value_from_object(list))) ? ON1X_OK : ON1X_ERR;
}

On1x_Status list_copy_fn(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 1, "List.Copy")) return ON1X_ERR;
    Value v;
     if (!read_argument(s, 1, v) || v.kind() != Value::Kind::List) return bad(s, "List.Copy");
     const auto* src = as_list_const(v);
     auto* dst = new_list(&s->gc, src->length);
     GcRoot root(dst);
     dst->constructor = src->constructor;
     for (std::size_t i = 0; i < src->length; ++i) {
         Value item;
         list_get(src, i, item);
         list_push(&s->gc, dst, item);
     }
     return stack_push(s, value_from_object(dst)) ? ON1X_OK : ON1X_ERR;
}

On1x_Status list_concat_fn(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 2, "List.Concat")) return ON1X_ERR;
    Value a, b;
     if (!read_argument(s, 1, a) || !read_argument(s, 2, b) ||
         a.kind() != Value::Kind::List || b.kind() != Value::Kind::List) return bad(s, "List.Concat");
     const auto* la = as_list_const(a);
     const auto* lb = as_list_const(b);
     auto* dst = new_list(&s->gc, la->length + lb->length);
     GcRoot root(dst);
     for (std::size_t i = 0; i < la->length; ++i) {
         Value item;
         list_get(la, i, item);
         list_push(&s->gc, dst, item);
     }
     for (std::size_t i = 0; i < lb->length; ++i) {
         Value item;
         list_get(lb, i, item);
         list_push(&s->gc, dst, item);
     }
     return stack_push(s, value_from_object(dst)) ? ON1X_OK : ON1X_ERR;
}

On1x_Status list_append_fn(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 2, "List.Append")) return ON1X_ERR;
    Value a, b;
     if (!read_argument(s, 1, a) || !read_argument(s, 2, b) ||
         a.kind() != Value::Kind::List || b.kind() != Value::Kind::List) return bad(s, "List.Append");
     const auto* la = as_list_const(a);
     const auto* lb = as_list_const(b);
     auto* dst = new_list(&s->gc, la->length + lb->length);
     GcRoot root(dst);
     for (std::size_t i = 0; i < la->length; ++i) {
         Value item;
         list_get(la, i, item);
         list_push(&s->gc, dst, item);
     }
     for (std::size_t i = 0; i < lb->length; ++i) {
         Value item;
         list_get(lb, i, item);
         list_push(&s->gc, dst, item);
     }
     return stack_push(s, value_from_object(dst)) ? ON1X_OK : ON1X_ERR;
}

On1x_Status list_slice_fn(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 3, "List.Slice")) return ON1X_ERR;
    Value lv, sv, ev;
     if (!read_argument(s, 1, lv) || !read_argument(s, 2, sv) || !read_argument(s, 3, ev) ||
         lv.kind() != Value::Kind::List || !sv.is_int() || !ev.is_int()) return bad(s, "List.Slice");
     const auto* src = as_list_const(lv);
     std::int64_t start = sv.as_int();
     std::int64_t end = ev.as_int();
     if (start < 0) start += static_cast<std::int64_t>(src->length);
     if (end < 0) end += static_cast<std::int64_t>(src->length);
     if (start < 0 || end < 0 || start > static_cast<std::int64_t>(src->length) ||
         end > static_cast<std::int64_t>(src->length) || start > end)
         return stack_push(s, make_none(&s->gc, s->reserved)) ? ON1X_OK : ON1X_ERR;
     std::size_t len = static_cast<std::size_t>(end - start);
     auto* dst = new_list(&s->gc, len);
     GcRoot root(dst);
     for (std::size_t i = 0; i < len; ++i) {
         Value item;
         list_get(src, static_cast<std::size_t>(start) + i, item);
         list_push(&s->gc, dst, item);
     }
     return stack_push(s, make_some(&s->gc, s->reserved, value_from_object(dst))) ? ON1X_OK : ON1X_ERR;
}

const On1x_FnDesc fns[] = {
    {"New", list_new_fn},
    {"Copy", list_copy_fn},
    {"Concat", list_concat_fn},
    {"Append", list_append_fn},
    {"Slice", list_slice_fn},
    {"First", list_first_fn},
    {"Last", list_last_fn},
    {"Take", list_take_fn},
    {"Drop", list_drop_fn},
    {"Insert", list_insert_fn},
    {"Remove", list_remove_fn},
    {"Reverse", list_reverse_fn},
    {"ReverseInPlace", list_reverse_in_place_fn},
    {"Fill", list_fill_fn},
    {"Map", list_map_fn},
    {"Filter", list_filter_fn},
    {"Reduce", list_reduce_fn},
    {"ForEach", list_for_each_fn},
    {"Any", list_any_fn},
    {"All", list_all_fn},
    {"Find", list_find_fn},
    {"FindIndex", list_find_index_fn},
    {"Partition", list_partition_fn},
    {"IndexOf", list_index_of_fn},
    {"Count", list_count_fn},
    {"Sort", list_sort_fn},
    {"SortBy", list_sort_by_fn},
    {"IsSorted", list_is_sorted_fn},
    {"Unique", list_unique_fn},
    {"Zip", list_zip_fn},
    {"Unzip", list_unzip_fn},
    {"Enumerate", list_enumerate_fn},
    {"Flatten", list_flatten_fn},
    {"Chunk", list_chunk_fn},
    {"Window", list_window_fn},
};

const On1x_ModuleDesc desc{"List", ON1X_CAP_NONE, fns, sizeof(fns) / sizeof(*fns)};
 
 }  // namespace
 
 const On1x_ModuleDesc* list_module() noexcept { return &desc; }
 
 }  // namespace on1x::stdlib
