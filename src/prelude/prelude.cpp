#include "prelude/prelude.hpp"

#include "api/api_common.hpp"
#include "core/table.hpp"
#include "gc/alloc.hpp"
#include "gc/roots.hpp"
#include "runtime/closure.hpp"
#include "runtime/state.hpp"

#include <array>

namespace on1x::prelude {

namespace {

struct NativeDefinition {
    const char* name;
    On1x_CFn function;
};

bool register_native(On1x_State* state, const NativeDefinition& definition) {
    auto* globals = state->globals;
    GcRoot globals_root(globals);
    auto* name = state->tags.intern(&state->gc, definition.name);
    GcRoot name_root(name);
    auto* function = runtime::new_native_function(&state->gc, definition.function);
    GcRoot function_root(function);
    return table_set(
        &state->gc, globals, value_from_object(name), value_from_object(function));
}

bool register_iota(On1x_State* state) {
    auto* globals = state->globals;
    GcRoot globals_root(globals);
    auto* name = state->tags.intern(&state->gc, "Iota");
    GcRoot name_root(name);
    return table_set(&state->gc, globals, value_from_object(name), Value::iota());
}

}  // namespace

On1x_Status push_result(On1x_State* state, Value value) noexcept {
    try {
        return stack_push(state, value) ? ON1X_OK : raise(state, "unable to return prelude result");
    } catch (...) {
        return raise(state, "unable to return prelude result");
    }
}

On1x_Status raise(On1x_State* state, const char* message) noexcept {
    return push_api_error(state, message);
}

bool has_arity(On1x_State* state, int argc, int expected, const char* name) noexcept {
    if (argc == expected) return true;
    (void)raise(state, name);
    return false;
}

bool argument(const On1x_State* state, int index, Value& value) noexcept {
    return stack_at(state, index, value);
}

bool install(On1x_State* state) noexcept {
    if (!state) return false;
    constexpr std::array natives{
        NativeDefinition{"TypeOf", type_of},
        NativeDefinition{"TagOf", tag_of},
        NativeDefinition{"PayloadOf", payload_of},
        NativeDefinition{"Len", length},
        NativeDefinition{"Get", get},
        NativeDefinition{"Set", set},
        NativeDefinition{"Push", push},
        NativeDefinition{"Pop", pop},
        NativeDefinition{"Keys", keys},
        NativeDefinition{"Values", values},
        NativeDefinition{"IsSome", is_some},
        NativeDefinition{"IsNone", is_none},
        NativeDefinition{"Unwrap", unwrap},
        NativeDefinition{"UnwrapOr", unwrap_or},
        NativeDefinition{"IsSuccess", is_success},
        NativeDefinition{"IsError", is_error},
    };
    try {
        for (const NativeDefinition& definition : natives) {
            if (!register_native(state, definition)) return false;
        }
        return register_iota(state);
    } catch (...) {
        return false;
    }
}

}  // namespace on1x::prelude
