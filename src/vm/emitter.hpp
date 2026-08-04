#pragma once

#include "syntax/ast.hpp"
#include "syntax/diagnostics.hpp"
#include "vm/chunk.hpp"

struct On1x_State;

namespace on1x::vm {

class Emitter {
public:
    Emitter(On1x_State* state, Chunk& chunk, syntax::Diagnostics& diagnostics)
        : state_(state), chunk_(chunk), diagnostics_(diagnostics) {}

    [[nodiscard]] bool emit_program(const syntax::AstNode* program);

private:
    [[nodiscard]] bool emit_expression(const syntax::AstNode* node);
    [[nodiscard]] bool emit_constant(Value value);

    On1x_State* state_;
    Chunk& chunk_;
    syntax::Diagnostics& diagnostics_;
};

}  // namespace on1x::vm
