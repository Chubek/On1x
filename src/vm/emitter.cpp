#include "vm/emitter.hpp"

#include "core/string.hpp"
#include "core/tag_table.hpp"
#include "gc/alloc.hpp"
#include "gc/roots.hpp"
#include "runtime/state.hpp"
#include "syntax/literals.hpp"

#include <algorithm>

namespace on1x::vm {

bool Emitter::emit_constant(Value value) {
    std::uint32_t index = 0;
    return chunk_->add_constant(value, index) && chunk_->emit(Opcode::Constant, index);
}

bool Emitter::emit_jump(Opcode opcode, ir::BlockId target) {
    const std::size_t position = chunk_->instruction_count();
    if (!chunk_->emit(opcode)) return false;
    pending_jumps_.push_back({position, target});
    return true;
}

bool Emitter::emit_instruction(const ir::Instruction& instruction) {
    try {
        switch (instruction.opcode) {
        case ir::Opcode::Unit:
            return emit_constant(Value::unit());
        case ir::Opcode::Bool:
            return emit_constant(Value::boolean(instruction.text == "true"));
        case ir::Opcode::Int: {
            std::int64_t value = 0;
            return syntax::decode_integer(instruction.text, value) &&
                emit_constant(Value::integer(&state_->gc, value));
        }
        case ir::Opcode::Float: {
            double value = 0.0;
            return syntax::decode_float(instruction.text, value) &&
                emit_constant(Value::floating(value));
        }
        case ir::Opcode::String: {
            std::string value;
            return syntax::decode_string(instruction.text, value) &&
                emit_constant(value_from_object(new_string(&state_->gc, value)));
        }
        case ir::Opcode::Tag: {
            std::string tag_text;
            if (!instruction.text.empty() && instruction.text.front() == '"') {
                if (!syntax::decode_string(instruction.text, tag_text)) return false;
            } else {
                tag_text = instruction.text;
            }
            return emit_constant(value_from_object(state_->tags.intern(&state_->gc, tag_text)));
        }
        case ir::Opcode::LoadGlobal: {
            std::uint32_t index = 0;
            return chunk_->add_constant(
                       value_from_object(state_->tags.intern(&state_->gc, instruction.text)),
                       index) &&
                chunk_->emit(Opcode::LoadGlobal, index);
        }
        case ir::Opcode::StoreGlobal: {
            std::uint32_t index = 0;
            return chunk_->add_constant(
                       value_from_object(state_->tags.intern(&state_->gc, instruction.text)),
                       index) &&
                chunk_->emit(Opcode::StoreGlobal, index);
        }
        case ir::Opcode::LoadLocal:
            local_count_ = std::max(local_count_, static_cast<std::size_t>(instruction.binding) + 1U);
            return chunk_->emit(Opcode::LoadLocal, instruction.binding);
        case ir::Opcode::StoreLocal:
            local_count_ = std::max(local_count_, static_cast<std::size_t>(instruction.binding) + 1U);
            return chunk_->emit(Opcode::StoreLocal, instruction.binding);
        case ir::Opcode::LoadUpvalue:
            return chunk_->emit(Opcode::LoadUpvalue, instruction.binding);
        case ir::Opcode::Binary:
            if (instruction.text == "+") return chunk_->emit(Opcode::Add);
            if (instruction.text == "-") return chunk_->emit(Opcode::Subtract);
            diagnostics_.add(
                instruction.position,
                "binary operator is not implemented by the bytecode emitter");
            return false;
        case ir::Opcode::Discard:
            return chunk_->emit(Opcode::Pop);
        case ir::Opcode::BranchIfFalse:
            return emit_jump(Opcode::JumpIfFalse, instruction.target);
        case ir::Opcode::Jump:
            return emit_jump(Opcode::Jump, instruction.target);
        case ir::Opcode::Phi:
            return true;
        case ir::Opcode::MakeFunction:
            return chunk_->emit(Opcode::MakeClosure, instruction.target);
        case ir::Opcode::Call:
            return chunk_->emit(
                Opcode::Call,
                static_cast<std::uint32_t>(instruction.operands.size() - 1U));
        case ir::Opcode::Return:
            return chunk_->emit(Opcode::Return);
        default:
            diagnostics_.add(
                instruction.position,
                "IR instruction is not implemented by the bytecode emitter");
            return false;
        }
    } catch (...) {
        diagnostics_.add(instruction.position, "unable to emit IR instruction");
        return false;
    }
}

bool Emitter::emit_function(const ir::Function& function, Chunk& chunk) {
    pending_jumps_.clear();
    local_count_ = 0;
    Chunk* previous_chunk = chunk_;
    chunk_ = &chunk;
    if (function.blocks.empty()) return false;
    std::vector<std::size_t> block_offsets(function.blocks.size(), 0);
    for (const ir::BasicBlock& block : function.blocks) {
        block_offsets[block.id] = chunk_->instruction_count();
        for (const ir::Instruction& instruction : block.instructions) {
            if (!emit_instruction(instruction)) return false;
        }
    }
    for (const auto& [position, target] : pending_jumps_) {
        if (target >= block_offsets.size()) return false;
        chunk_->patch_operand(position, static_cast<std::uint32_t>(block_offsets[target]));
    }
    chunk_->set_local_count(local_count_);
    chunk_->set_signature(
        function.parameter_bindings.empty() ? nullptr : function.parameter_bindings.data(),
        function.parameter_bindings.size(),
        function.variadic,
        function.capture_bindings.size());
    chunk_ = previous_chunk;
    return true;
}

bool Emitter::emit_module(const ir::Module& module) {
    try {
        const std::size_t function_count = module.functions.size() + 1U;
        Chunk** functions = gc_alloc_array<Chunk*>(&state_->gc, function_count);
        functions[0] = chunk_;
        GcRoot root_functions(functions);
        for (std::size_t index = 1; index < function_count; ++index) {
            functions[index] = gc_alloc<Chunk>(&state_->gc, &state_->gc);
        }
        for (std::size_t index = 0; index < function_count; ++index) {
            functions[index]->set_functions(functions, function_count);
        }
        if (!emit_function(module.entry, *chunk_)) return false;
        for (std::size_t index = 1; index < function_count; ++index) {
            if (!emit_function(module.functions[index - 1U], *functions[index])) return false;
        }
        return true;
    } catch (...) {
        diagnostics_.add({}, "unable to allocate bytecode function");
        return false;
    }
}

}  // namespace on1x::vm
