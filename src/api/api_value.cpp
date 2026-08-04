#include "api/api_common.hpp"

#include "core/string.hpp"
#include "core/tag_table.hpp"

extern "C" {
void on1x_push_unit(On1x_State* state) { if (state) (void)on1x::stack_push(state, on1x::Value::unit()); }
void on1x_push_bool(On1x_State* state, int value) { if (state) (void)on1x::stack_push(state, on1x::Value::boolean(value != 0)); }
void on1x_push_int(On1x_State* state, int64_t value) {
    if (state) (void)on1x::stack_push(state, on1x::Value::integer(&state->gc, value));
}
void on1x_push_float(On1x_State* state, double value) { if (state) (void)on1x::stack_push(state, on1x::Value::floating(value)); }
On1x_Status on1x_push_string(On1x_State* state, const char* value, size_t length) {
    if (!state || !value) return ON1X_ERR;
    try {
        return on1x::stack_push(state, on1x::value_from_object(on1x::new_string(&state->gc, {value, length})))
            ? ON1X_OK : ON1X_ERR;
    } catch (...) { return on1x::push_api_error(state, "invalid UTF-8 string"); }
}
On1x_Status on1x_push_tag(On1x_State* state, const char* value, size_t length) {
    if (!state || !value) return ON1X_ERR;
    try {
        return on1x::stack_push(state, on1x::value_from_object(state->tags.intern(&state->gc, {value, length})))
            ? ON1X_OK : ON1X_ERR;
    } catch (...) { return on1x::push_api_error(state, "invalid UTF-8 tag"); }
}
int64_t on1x_as_int(const On1x_State* state, int index) {
    on1x::Value value; return on1x::stack_at(state, index, value) && value.is_int() ? value.as_int() : 0;
}
double on1x_as_float(const On1x_State* state, int index) {
    on1x::Value value; return on1x::stack_at(state, index, value) && value.is_float() ? value.as_float() : 0.0;
}
int on1x_as_bool(const On1x_State* state, int index) {
    on1x::Value value; return on1x::stack_at(state, index, value) && value.is_bool() && value.as_bool();
}
const char* on1x_as_string(const On1x_State* state, int index, size_t* length) {
    on1x::Value value;
    const auto* text = on1x::stack_at(state, index, value) ? on1x::as_string_const(value) : nullptr;
    if (!text) { if (length) *length = 0; return nullptr; }
    if (length) *length = text->bytes;
    return text->data;
}
}
