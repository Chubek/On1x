#include "core/type.hpp"

namespace on1x {

Value type_of(Value value, const ReservedTags& tags) noexcept {
    switch (value.kind()) {
    case Value::Kind::Unit: return value_from_object(tags.unit);
    case Value::Kind::Bool: return value_from_object(tags.boolean);
    case Value::Kind::Int: return value_from_object(tags.integer);
    case Value::Kind::Float: return value_from_object(tags.floating);
    case Value::Kind::String: return value_from_object(tags.string);
    case Value::Kind::Tag: return value_from_object(tags.tag);
    case Value::Kind::List: return value_from_object(tags.list);
    case Value::Kind::Table: return value_from_object(tags.table);
    case Value::Kind::Function: return value_from_object(tags.function);
    case Value::Kind::Iota: return value_from_object(tags.iota);
    }
    return Value::unit();
}

}  // namespace on1x
