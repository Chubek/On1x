#include "stdlib/io/io_impl.hpp"

#include "api/api_common.hpp"
#include "core/list.hpp"
#include "core/string.hpp"
#include "core/table.hpp"
#include "core/tag_table.hpp"
#include "core/tostring.hpp"
#include "gc/roots.hpp"
#include "runtime/state.hpp"
#include "stdlib/args.hpp"

#include <algorithm>
#include <string>
#include <vector>

namespace on1x::stdlib::io_detail {

static On1x_Status bad(On1x_State* s, const char* m) noexcept {
    return push_api_error(s, m);
}

static void repr_append(std::string& out, Value value) {
    switch (value.kind()) {
    case Value::Kind::Unit:
        out += ":Unit";
        return;
    case Value::Kind::Bool:
        out += value.as_bool() ? ":True" : ":False";
        return;
    case Value::Kind::Int:
        out += std::to_string(value.as_int());
        return;
    case Value::Kind::Float:
        out += std::to_string(value.as_float());
        return;
    case Value::Kind::String: {
        out.push_back('"');
        out += string_view(as_string_const(value));
        out.push_back('"');
        return;
    }
    case Value::Kind::Tag:
        out += ":" + std::string(tag_text(as_tag_const(value)));
        return;
    case Value::Kind::Iota:
        out += "Iota";
        return;
    case Value::Kind::Function:
        out += "<fn>";
        return;
    case Value::Kind::List: {
        const auto* list = as_list_const(value);
        if (list->constructor) {
            out += ":" + std::string(tag_text(list->constructor));
            out += "[";
        } else {
            out += "[";
        }
        for (std::size_t i = 0; i < list->length; ++i) {
            if (i != 0) out += ", ";
            repr_append(out, list->items[i]);
        }
        out += "]";
        return;
    }
    case Value::Kind::Table: {
        out += "%{";
        struct Entry { Value key; Value val; };
        std::vector<Entry> entries;
        for (const TableEntry* e = as_table_const(value)->entries; e; e = e->next) {
            entries.push_back({e->key, e->value});
        }
        std::sort(entries.begin(), entries.end(), [](const Entry& a, const Entry& b) {
            return to_string(a.key) < to_string(b.key);
        });
        bool first = true;
        for (const auto& e : entries) {
            if (!first) out += ", ";
            repr_append(out, e.key);
            out += " => ";
            repr_append(out, e.val);
            first = false;
        }
        out += "}";
        return;
    }
    }
}

On1x_Status io_show(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 1, "Io.Show")) return ON1X_ERR;
    Value v;
    if (!read_argument(s, 1, v)) return bad(s, "Io.Show");
    std::string text = to_string(v);
    auto* str = new_string(&s->gc, text);
    if (!str) return bad(s, "Io.Show: allocation failed");
    return stack_push(s, value_from_object(str)) ? ON1X_OK : ON1X_ERR;
}

On1x_Status io_repr(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 1, "Io.Repr")) return ON1X_ERR;
    Value v;
    if (!read_argument(s, 1, v)) return bad(s, "Io.Repr");
    std::string text;
    repr_append(text, v);
    auto* str = new_string(&s->gc, text);
    if (!str) return bad(s, "Io.Repr: allocation failed");
    return stack_push(s, value_from_object(str)) ? ON1X_OK : ON1X_ERR;
}

}  // namespace on1x::stdlib::io_detail
