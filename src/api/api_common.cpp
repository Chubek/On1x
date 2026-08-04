#include "api/api_common.hpp"

#include "core/optional.hpp"
#include "core/result.hpp"
#include "core/string.hpp"
#include "gc/roots.hpp"

namespace on1x {
On1x_Type api_type(Value value) noexcept {
    switch (value.kind()) {
    case Value::Kind::Unit: return ON1X_UNIT;
    case Value::Kind::Bool: return ON1X_BOOL;
    case Value::Kind::Int: return ON1X_INT;
    case Value::Kind::Float: return ON1X_FLOAT;
    case Value::Kind::String: return ON1X_STRING;
    case Value::Kind::Tag: return ON1X_TAG;
    case Value::Kind::List: return ON1X_LIST;
    case Value::Kind::Table: return ON1X_TABLE;
    case Value::Kind::Function: return ON1X_FN;
    case Value::Kind::Iota: return ON1X_IOTA;
    }
    return ON1X_INVALID;
}

On1x_Status push_api_error(On1x_State* state, const char* message) {
    if (!state) return ON1X_ERR;
    try {
        auto* string = new_string(&state->gc, message ? message : "On1x error");
        GcRoot string_root(string);
        const auto text = value_from_object(string);
        const auto payload = make_some(&state->gc, state->reserved, text);
        const auto error = make_error(&state->gc, state->reserved, payload);
        GcRoot error_root(as_list(error));
        (void)stack_push(state, error);
    } catch (...) {
        return ON1X_ERR;
    }
    return ON1X_ERR;
}
}  // namespace on1x
