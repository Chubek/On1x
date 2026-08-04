#include "api/api_common.hpp"

#include "core/result.hpp"
#include "core/table.hpp"
#include "gc/alloc.hpp"
#include "gc/roots.hpp"

namespace {

On1x_Status call_error(On1x_State* state, std::size_t result_base, const char* message) {
    state->top = result_base;
    return on1x::push_api_error(state, message);
}

}  // namespace

extern "C" {

On1x_Status on1x_register(On1x_State* state, const char* name, On1x_CFn function) {
    if (!state || !name || !function) {
        return on1x::push_api_error(state, "Register expects a name and native function");
    }
    try {
        auto* name_tag = state->tags.intern(&state->gc, name);
        auto* native = on1x::gc_alloc<on1x::FunctionObject>(&state->gc);
        on1x::GcRoot root(native);
        native->native = function;
        if (!on1x::table_set(
                &state->gc, state->globals, on1x::value_from_object(name_tag),
                on1x::value_from_object(native))) {
            return on1x::push_api_error(state, "unable to register native function");
        }
        return ON1X_OK;
    } catch (...) {
        return on1x::push_api_error(state, "Register requires a valid UTF-8 name");
    }
}

On1x_Status on1x_call(On1x_State* state, int function_index, int argc) {
    if (!state || argc < 0) return on1x::push_api_error(state, "Call requires a non-negative argc");

    const int normalized = on1x::normalize_stack_index(state, function_index);
    if (normalized < 0) return on1x::push_api_error(state, "Call received an invalid function index");
    const std::size_t function_slot = static_cast<std::size_t>(normalized);
    if (state->top != function_slot + static_cast<std::size_t>(argc) + 1U) {
        return on1x::push_api_error(state, "Call arguments must occupy the slots above the function");
    }

    const on1x::Value function_value = state->stack[function_slot];
    if (function_value.kind() != on1x::Value::Kind::Function) {
        return on1x::push_api_error(state, "Call expects an Fn");
    }
    auto* function = static_cast<on1x::FunctionObject*>(function_value.as_object());
    if (!function->native) return on1x::push_api_error(state, "Call received an invalid native function");
    on1x::GcRoot function_root(function);

    for (std::size_t index = function_slot; index + 1U < state->top; ++index) {
        state->stack[index] = state->stack[index + 1U];
    }
    --state->top;

    const bool previous_frame_active = state->api_frame_active;
    const std::size_t previous_frame_base = state->api_frame_base;
    state->api_frame_active = true;
    state->api_frame_base = function_slot;

    On1x_Status status = ON1X_ERR;
    try {
        status = function->native(state, argc);
    } catch (...) {
        state->api_frame_active = previous_frame_active;
        state->api_frame_base = previous_frame_base;
        return call_error(state, function_slot, "native function threw an exception");
    }

    const std::size_t expected_top = function_slot + static_cast<std::size_t>(argc) + 1U;
    if (state->top != expected_top) {
        state->api_frame_active = previous_frame_active;
        state->api_frame_base = previous_frame_base;
        return call_error(state, function_slot, "native function must push exactly one result");
    }

    const on1x::Value result = state->stack[state->top - 1U];
    on1x::GcRoot result_root(result.is_object() ? result.as_object() : nullptr);
    state->api_frame_active = previous_frame_active;
    state->api_frame_base = previous_frame_base;
    if (status == ON1X_ERR && !on1x::is_error(result, state->reserved)) {
        return call_error(state, function_slot, "failing native function must push an Error result");
    }
    if (status != ON1X_OK && status != ON1X_ERR) {
        return call_error(state, function_slot, "native function returned an invalid status");
    }

    state->top = function_slot;
    if (!on1x::stack_push(state, result)) return on1x::push_api_error(state, "unable to return native result");
    return status;
}

}  // extern "C"
