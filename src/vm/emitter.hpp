#pragma once

#include "ir/ir.hpp"
#include "syntax/diagnostics.hpp"
#include "vm/chunk.hpp"

#include <utility>
#include <vector>

struct On1x_State;

namespace on1x::vm {

class Emitter {
public:
    Emitter(On1x_State* state, Chunk& chunk, syntax::Diagnostics& diagnostics)
        : state_(state), chunk_(&chunk), diagnostics_(diagnostics) {}

    [[nodiscard]] bool emit_module(const ir::Module& module);

private:
    [[nodiscard]] bool emit_instruction(const ir::Instruction& instruction);
    [[nodiscard]] bool emit_constant(Value value);
    [[nodiscard]] bool emit_jump(Opcode opcode, ir::BlockId target);
    [[nodiscard]] bool emit_function(const ir::Function& function, Chunk& chunk);

    On1x_State* state_;
    Chunk* chunk_;
    syntax::Diagnostics& diagnostics_;
    std::vector<std::pair<std::size_t, ir::BlockId>> pending_jumps_;
    std::size_t local_count_ = 0;
};

}  // namespace on1x::vm
