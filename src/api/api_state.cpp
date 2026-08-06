#include "api/api_common.hpp"

#include "core/table.hpp"
#include "ir/lower_ast.hpp"
#include "ir/verifier.hpp"
#include "prelude/prelude.hpp"
#include "sema/resolver.hpp"
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
        state->tags.root();
        GC_add_roots(&state->stack, &state->stack + 1);
        state->reserved = on1x::make_reserved_tags(&state->gc, state->tags);
        state->globals = on1x::new_table(&state->gc);
        if (!on1x::prelude::install(state)) {
            GC_remove_roots(&state->stack, &state->stack + 1);
            state->tags.unroot();
            on1x::gc_shutdown(&state->gc);
            delete state;
            return nullptr;
        }
        state->persistent_roots.push(state->globals);
        state->persistent_roots.push(state->reserved.unit);
        state->persistent_roots.push(state->reserved.boolean);
        state->persistent_roots.push(state->reserved.integer);
        state->persistent_roots.push(state->reserved.floating);
        state->persistent_roots.push(state->reserved.string);
        state->persistent_roots.push(state->reserved.tag);
        state->persistent_roots.push(state->reserved.list);
        state->persistent_roots.push(state->reserved.table);
        state->persistent_roots.push(state->reserved.function);
        state->persistent_roots.push(state->reserved.iota);
        state->persistent_roots.push(state->reserved.some);
        state->persistent_roots.push(state->reserved.none);
        state->persistent_roots.push(state->reserved.success);
        state->persistent_roots.push(state->reserved.error);
        return state;
    } catch (...) {
        return nullptr;
    }
}

void on1x_close(On1x_State* state) {
    if (!state) return;
    on1x::release_api_references(state);
    GC_remove_roots(&state->stack, &state->stack + 1);
    on1x::gc_shutdown(&state->gc);
    state->tags.unroot();
    delete state;
}

On1x_Status on1x_eval(On1x_State* state, const char* source, size_t length, const char*) {
    if (!state || !source) return ON1X_ERR;
    try {
        const std::string_view text(source, length);
        on1x::Arena arena;
        on1x::syntax::Diagnostics diagnostics;
        on1x::syntax::Parser parser(text, arena, diagnostics);
        auto* program = parser.parse_program();
        if (!program || !diagnostics.empty()) {
            return on1x::push_api_error(
                state,
                diagnostics.empty() ? "invalid On1x chunk" : diagnostics.entries().front().message.c_str());
        }
        on1x::sema::Resolver resolver(diagnostics);
        if (!resolver.resolve(program) || !diagnostics.empty()) {
            return on1x::push_api_error(state, "invalid On1x semantics");
        }
        on1x::ir::Module module;
        if (!on1x::ir::lower_ast(program, module, diagnostics) ||
            !on1x::ir::verify(module, diagnostics) ||
            !diagnostics.empty()) {
            return on1x::push_api_error(
                state,
                diagnostics.empty() ? "unable to lower On1x chunk" : diagnostics.entries().front().message.c_str());
        }
        on1x::vm::Chunk chunk(&state->gc);
        on1x::vm::Emitter emitter(state, chunk, diagnostics);
        if (!emitter.emit_module(module) || !diagnostics.empty()) {
            return on1x::push_api_error(
                state,
                diagnostics.empty() ? "unable to compile On1x chunk" : diagnostics.entries().front().message.c_str());
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
