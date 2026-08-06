#include "runtime/call.hpp"

#include <on1x/on1x_call.h>

#include "gc/roots.hpp"

#include <climits>

namespace on1x::runtime {

bool invoke_native(
    On1x_State* state,
    FunctionObject* function,
    const Value* arguments,
    std::size_t argument_count,
    Value& result,
    const char*& error) noexcept {
    if (!state || !function || !function->native || argument_count > static_cast<std::size_t>(INT_MAX)) {
        error = "invalid native call";
        return false;
    }
    try {
        GcRoot function_root(function);
        const std::size_t state_top = state->top;
        
        // Save previous captures state
        Value* prev_captures = state->invocation_captures;
        const std::size_t prev_capture_count = state->invocation_capture_count;
        
        // Push function and arguments for on1x_call
        if (!stack_push(state, value_from_object(function))) {
            error = "unable to prepare native call";
            return false;
        }
        for (std::size_t index = 0; index < argument_count; ++index) {
            if (!stack_push(state, arguments[index])) {
                state->top = state_top;
                error = "unable to prepare native call";
                return false;
            }
        }
        
        // Set invocation captures before calling native
        state->invocation_captures = function->captures;
        state->invocation_capture_count = function->capture_count;
        
        const On1x_Status status = on1x_call(
            state,
            static_cast<int>(state_top + 1U),
            static_cast<int>(argument_count));
        
        // Restore captures
        state->invocation_captures = prev_captures;
        state->invocation_capture_count = prev_capture_count;
        
        if (state->top == state_top) {
            error = "native call did not return a result";
            return false;
        }
        result = state->stack[state->top - 1U];
        state->top = state_top;
        if (status != ON1X_OK) {
            error = "native call failed";
            return false;
        }
        return true;
    } catch (...) {
        state->top = state->top > 0U ? state->top - 1U : state->top;
        error = "native call failed";
        return false;
    }
}

}  // namespace on1x::runtime
