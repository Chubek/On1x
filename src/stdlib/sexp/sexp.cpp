#include "api/api_common.hpp"
#include "core/list.hpp"
#include "core/optional.hpp"
#include "core/string.hpp"
#include "core/table.hpp"
#include "core/tag_table.hpp"
#include "core/tostring.hpp"
#include "gc/roots.hpp"
#include "on1x/on1x_capability.h"
#include "on1x/on1x_module.h"
#include "runtime/state.hpp"
#include "stdlib/args.hpp"
#include "stdlib/sexp/sexp.hpp"

#include <algorithm>
#include <cstring>
#include <cstdlib>
#include <string>
#include <vector>

// sfsexp C API
#include <sfsexp/src/sexp.h>

namespace on1x::stdlib {
namespace {

On1x_Status bad(On1x_State* s, const char* m) noexcept {
    return push_api_error(s, m);
}

// Convert an sfsexp s-expression to an On1x Value.
// s-expressions map as:
//   atoms (strings) -> :String (or :Int/:Float if they parse as numbers)
//   lists -> :List
static Value sexp_to_value(On1x_State* s, sexp_t* sx) {
    if (!sx) return Value::unit();
    switch (sx->ty) {
    case SEXP_VALUE: {
        // Atom: treat as string
        const char* val = sx->val;
        if (!val) return Value::unit();
        // Try to parse as integer
        char* end = nullptr;
        std::int64_t int_val = std::strtoll(val, &end, 10);
        if (end && *end == '\0' && end != val) {
            return Value::integer(&s->gc, int_val);
        }
        // Try to parse as float
        double float_val = std::strtod(val, &end);
        if (end && *end == '\0' && end != val) {
            return Value::floating(float_val);
        }
        // Return as string
        auto* str = new_string(&s->gc, val);
        if (!str) return Value::unit();
        return value_from_object(str);
    }
    case SEXP_LIST: {
        auto* list = new_list(&s->gc, 0);
        if (!list) return Value::unit();
        GcRoot list_root(list);
        for (sexp_t* item = sx->list; item; item = item->next) {
            Value v = sexp_to_value(s, item);
            list_push(&s->gc, list, v);
        }
        return value_from_object(list);
    }
    default:
        return Value::unit();
    }
}

// Recursively convert an On1x Value to an sfsexp s-expression.
// Caller must destroy the result with destroy_sexp().
static sexp_t* value_to_sexp(On1x_State* s, Value value) {
    // Helper to create a BASIC atom
    auto make_atom = [](const char* buf, size_t len) -> sexp_t* {
        return new_sexp_atom(buf, len, SEXP_BASIC);
    };

    switch (value.kind()) {
    case Value::Kind::Unit:
        return make_atom("()", 2);
    case Value::Kind::Bool:
        return make_atom(value.as_bool() ? "true" : "false",
                         value.as_bool() ? 4 : 5);
    case Value::Kind::Int: {
        std::string str = std::to_string(value.as_int());
        return make_atom(str.c_str(), str.size());
    }
    case Value::Kind::Float: {
        std::string str = std::to_string(value.as_float());
        return make_atom(str.c_str(), str.size());
    }
    case Value::Kind::String: {
        auto text = string_view(as_string_const(value));
        return make_atom(text.data(), text.size());
    }
    case Value::Kind::Tag: {
        std::string str = ":" + std::string(tag_text(as_tag_const(value)));
        return make_atom(str.c_str(), str.size());
    }
    case Value::Kind::List: {
        const auto* list = as_list_const(value);
        sexp_t* result = nullptr;
        sexp_t** tail = &result;
        for (std::size_t i = 0; i < list->length; ++i) {
            sexp_t* item = value_to_sexp(s, list->items[i]);
            if (!item) continue;
            if (!*tail) {
                *tail = item;
            } else {
                // Append to list
                sexp_t* cur = *tail;
                while (cur->next) cur = cur->next;
                cur->next = item;
            }
        }
        if (!result) return make_atom("[]", 2);
        return new_sexp_list(result);
    }
    case Value::Kind::Table: {
        // Tables don't have a natural s-expression representation.
        // Render as a list of key-value pairs.
        std::string repr = to_string(value);
        return make_atom(repr.c_str(), repr.size());
    }
    default:
        return make_atom("<value>", 7);
    }
}

// Parse(s: :String) -> :Some[value] | :None
On1x_Status sexp_parse(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 1, "Sexp.Parse")) return ON1X_ERR;
    Value v;
    if (!read_argument(s, 1, v) || v.kind() != Value::Kind::String)
        return bad(s, "Sexp.Parse expects a String");
    auto text = string_view(as_string_const(v));
    // parse_sexp takes char*, not const char*. Copy.
    std::vector<char> input(text.begin(), text.end());
    input.push_back('\0');

    sexp_t* parsed = parse_sexp(input.data(), input.size() - 1);
    if (!parsed) {
        return stack_push(s, make_none(&s->gc, s->reserved)) ? ON1X_OK : ON1X_ERR;
    }
    Value result = sexp_to_value(s, parsed);
    destroy_sexp(parsed);
    if (result.kind() == Value::Kind::Unit && parsed) {
        // Empty parse
    }
    return stack_push(s, make_some(&s->gc, s->reserved, result)) ? ON1X_OK : ON1X_ERR;
}

// Write(v) -> :Some[:String] | :None
On1x_Status sexp_write(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 1, "Sexp.Write")) return ON1X_ERR;
    Value v;
    if (!read_argument(s, 1, v)) return bad(s, "Sexp.Write expects a value");
    sexp_t* sx = value_to_sexp(s, v);
    if (!sx) {
        return stack_push(s, make_none(&s->gc, s->reserved)) ? ON1X_OK : ON1X_ERR;
    }
    // Use print_sexp with a buffer. Start with 256, grow if needed.
    constexpr size_t initial_size = 256;
    std::vector<char> buf(initial_size);
    int len = print_sexp(buf.data(), buf.size(), sx);
    if (len < 0) {
        // Buffer too small, grow and retry
        buf.resize(buf.size() * 4);
        len = print_sexp(buf.data(), buf.size(), sx);
    }
    destroy_sexp(sx);
    if (len < 0) {
        return stack_push(s, make_none(&s->gc, s->reserved)) ? ON1X_OK : ON1X_ERR;
    }
    auto* result = new_string(&s->gc, std::string_view(buf.data(), static_cast<size_t>(len)));
    if (!result) return bad(s, "Sexp.Write: allocation failed");
    return stack_push(s, make_some(&s->gc, s->reserved, value_from_object(result))) ? ON1X_OK : ON1X_ERR;
}

const On1x_FnDesc fns[] = {
    {"Parse", sexp_parse},
    {"Write", sexp_write},
};

const On1x_ModuleDesc desc{"Sexp", ON1X_CAP_NONE, fns, sizeof(fns) / sizeof(*fns)};

}  // namespace

const On1x_ModuleDesc* sexp_module() noexcept { return &desc; }

}  // namespace on1x::stdlib
