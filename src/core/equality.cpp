#include "core/equality.hpp"

#include "core/list.hpp"
#include "core/string.hpp"
#include "core/table.hpp"
#include "core/tag_table.hpp"

namespace on1x {

bool value_equals(Value left, Value right) {
    if (left == right) return true;
    if (left.is_int() && right.is_float()) return static_cast<double>(left.as_int()) == right.as_float();
    if (left.is_float() && right.is_int()) return left.as_float() == static_cast<double>(right.as_int());
    if (left.kind() != right.kind()) return false;
    switch (left.kind()) {
    case Value::Kind::Float: return left.as_float() == right.as_float();
    case Value::Kind::String: return string_view(as_string_const(left)) == string_view(as_string_const(right));
    case Value::Kind::Tag: return as_tag_const(left) == as_tag_const(right);
    case Value::Kind::List: {
        const auto* lhs = as_list_const(left);
        const auto* rhs = as_list_const(right);
        if (lhs->constructor != rhs->constructor || lhs->length != rhs->length) return false;
        for (std::size_t index = 0; index < lhs->length; ++index)
            if (!value_equals(lhs->items[index], rhs->items[index])) return false;
        return true;
    }
    case Value::Kind::Table: {
        const auto* lhs = as_table_const(left);
        const auto* rhs = as_table_const(right);
        if (lhs->length != rhs->length) return false;
        for (const TableEntry* entry = lhs->entries; entry; entry = entry->next) {
            Value candidate;
            if (!table_get(rhs, entry->key, candidate) || !value_equals(entry->value, candidate)) return false;
        }
        return true;
    }
    default: return false;
    }
}

}  // namespace on1x
