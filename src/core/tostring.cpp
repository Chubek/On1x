#include "core/tostring.hpp"

#include "core/list.hpp"
#include "core/string.hpp"
#include "core/table.hpp"
#include "core/tag_table.hpp"

#include <charconv>
#include <string>

namespace on1x {

namespace {
void append_rendered(std::string& output, Value value) {
    switch (value.kind()) {
    case Value::Kind::Unit: output += "()"; return;
    case Value::Kind::Bool: output += value.as_bool() ? "true" : "false"; return;
    case Value::Kind::Int: output += std::to_string(value.as_int()); return;
    case Value::Kind::Float: output += std::to_string(value.as_float()); return;
    case Value::Kind::String:
        output.push_back('"');
        output += string_view(as_string_const(value));
        output.push_back('"');
        return;
    case Value::Kind::Tag: output += ":" + std::string(tag_text(as_tag_const(value))); return;
    case Value::Kind::Iota: output += "Iota"; return;
    case Value::Kind::Function: output += "<fn>"; return;
    case Value::Kind::List: {
        const auto* list = as_list_const(value);
        if (list->constructor) output += ":" + std::string(tag_text(list->constructor));
        output.push_back('[');
        for (std::size_t index = 0; index < list->length; ++index) {
            if (index != 0) output += ", ";
            append_rendered(output, list->items[index]);
        }
        output.push_back(']');
        return;
    }
    case Value::Kind::Table: {
        output += "%{";
        bool first = true;
        for (const TableEntry* entry = as_table_const(value)->entries; entry; entry = entry->next) {
            if (!first) output += ", ";
            append_rendered(output, entry->key);
            output += " => ";
            append_rendered(output, entry->value);
            first = false;
        }
        output.push_back('}');
        return;
    }
    }
}
}

std::string to_string(Value value) {
    std::string output;
    append_rendered(output, value);
    return output;
}

}  // namespace on1x
