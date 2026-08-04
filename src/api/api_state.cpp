#include "api/api_common.hpp"

#include "core/table.hpp"
#include "syntax/parser.hpp"
#include "vm/emitter.hpp"
#include "vm/interpreter.hpp"
#include "vm/chunk.hpp"

#include "util/arena.hpp"

extern "C" {
On1x_State* on1x_open(void) {
    try {
        auto* state = new On1x_State;
        on1x::gc_init(&state->gc);
        state->reserved = on1x::make_reserved_tags(&state->gc, state->tags);
        state->globals = on1x::new_table(&state->gc);
        return state;
    } catch (...) {
        return nullptr;
    }
}

void on1x_close(On1x_State* state) {
    if (!state) return;
    on1x::release_api_references(state);
    on1x::gc_shutdown(&state->gc);
    delete state;
}

On1x_Status on1x_eval(On1x_State* state, const char* source, size_t length, const char*) {
    if (!state || !source) return ON1X_ERR;
    try {
        const std::string_view text(source, length);
        on1x::Arena arena;
        on1x::syntax::Diagnostics diagnostics;
        on1x::syntax::Parser parser(text, arena, diagnostics);
        const auto* program = parser.parse_program();
        if (!program || !diagnostics.empty()) return on1x::push_api_error(state, "invalid On1x chunk");
        on1x::vm::Chunk chunk(&state->gc);
        on1x::vm::Emitter emitter(state, chunk, diagnostics);
        if (!emitter.emit_program(program) || !diagnostics.empty()) {
            return on1x::push_api_error(state, "unable to compile On1x chunk");
        }
        on1x::Value result;
        const char* error = nullptr;
        if (!on1x::vm::execute(state, chunk, result, error)) {
            return on1x::push_api_error(state, error ? error : "On1x execution failed");
        }
        return on1x::stack_push(state, result) ? ON1X_OK : ON1X_ERR;
    } catch (...) {
        return on1x::push_api_error(state, "evaluation failed");
    }
}
}
