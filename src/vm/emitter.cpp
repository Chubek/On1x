#include "vm/emitter.hpp"

#include "core/string.hpp"
#include "core/tag_table.hpp"
#include "gc/alloc.hpp"
#include "gc/roots.hpp"
#include "runtime/state.hpp"
#include "runtime/pattern_match.hpp"
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

bool Emitter::compile_pattern(const ir::Pattern& source, runtime::Pattern*& result) {
    auto* pattern = gc_alloc<runtime::Pattern>(&state_->gc);
    GcRoot pattern_root(pattern);
    switch (source.kind) {
    case ir::PatternKind::Literal:
        pattern->kind = runtime::PatternKind::Literal;
        switch (source.literal_kind) {
        case syntax::AstKind::Unit:
            pattern->literal = Value::unit();
            break;
        case syntax::AstKind::Bool:
            pattern->literal = Value::boolean(source.text == "true");
            break;
        case syntax::AstKind::Int: {
            std::int64_t value = 0;
            if (!syntax::decode_integer(source.text, value)) return false;
            pattern->literal = Value::integer(&state_->gc, value);
            break;
        }
        case syntax::AstKind::Float: {
            double value = 0.0;
            if (!syntax::decode_float(source.text, value)) return false;
            pattern->literal = Value::floating(value);
            break;
        }
        case syntax::AstKind::String: {
            std::string value;
            if (!syntax::decode_string(source.text, value)) return false;
            pattern->literal = value_from_object(new_string(&state_->gc, value));
            break;
        }
        default:
            return false;
        }
        break;
    case ir::PatternKind::Wildcard:
        pattern->kind = runtime::PatternKind::Wildcard;
        break;
    case ir::PatternKind::Binding:
        pattern->kind = runtime::PatternKind::Binding;
        pattern->binding = source.binding;
        local_count_ = std::max(local_count_, static_cast<std::size_t>(source.binding) + 1U);
        break;
    case ir::PatternKind::List:
    case ir::PatternKind::TaggedList:
        pattern->kind = source.kind == ir::PatternKind::List
            ? runtime::PatternKind::List
            : runtime::PatternKind::TaggedList;
        if (source.kind == ir::PatternKind::TaggedList) {
            pattern->tag = state_->tags.intern(&state_->gc, source.text);
        }
        pattern->child_count = source.children.size();
        pattern->has_tail = source.has_tail;
        pattern->tail_binding = source.tail_binding;
        if (source.has_tail) {
            local_count_ = std::max(
                local_count_, static_cast<std::size_t>(source.tail_binding) + 1U);
        }
        if (pattern->child_count != 0) {
            pattern->children = gc_alloc_array<runtime::Pattern>(&state_->gc, pattern->child_count);
            for (std::size_t index = 0; index < pattern->child_count; ++index) {
                runtime::Pattern* child = nullptr;
                if (!compile_pattern(source.children[index], child)) return false;
                pattern->children[index] = *child;
            }
        }
        break;
    }
    result = pattern;
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
            if (instruction.has_integer_value) return emit_constant(Value::integer(&state_->gc, instruction.integer_value));
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
            if (instruction.text == "*") return chunk_->emit(Opcode::Multiply);
            if (instruction.text == "/") return chunk_->emit(Opcode::Divide);
            if (instruction.text == "%") return chunk_->emit(Opcode::Modulo);
            if (instruction.text == "==") return chunk_->emit(Opcode::Equal);
            if (instruction.text == "!=") return chunk_->emit(Opcode::NotEqual);
            if (instruction.text == "<") return chunk_->emit(Opcode::Less);
            if (instruction.text == "<=") return chunk_->emit(Opcode::LessEqual);
            if (instruction.text == ">") return chunk_->emit(Opcode::Greater);
            if (instruction.text == ">=") return chunk_->emit(Opcode::GreaterEqual);
            diagnostics_.add(
                instruction.position,
                "binary operator is not implemented by the bytecode emitter");
            return false;
        case ir::Opcode::Unary:
            if (instruction.text == "not") return chunk_->emit(Opcode::Not);
            if (instruction.text == "-") return chunk_->emit(Opcode::Negate);
            diagnostics_.add(
                instruction.position,
                "unary operator is not implemented by the bytecode emitter");
            return false;
        case ir::Opcode::Some:
            return chunk_->emit(Opcode::MakeSome);
        case ir::Opcode::None:
            return chunk_->emit(Opcode::MakeNone);
        case ir::Opcode::EffectResult:
            return chunk_->emit(Opcode::LoadEffectResult);
        case ir::Opcode::BeginCapture: {
            const std::size_t position = chunk_->instruction_count();
            if (!chunk_->emit(Opcode::BeginCapture)) return false;
            pending_captures_.push_back(position);
            return true;
        }
        case ir::Opcode::EndCapture: {
            if (pending_captures_.empty() || !chunk_->emit(Opcode::EndCapture)) return false;
            chunk_->patch_operand(pending_captures_.back(), static_cast<std::uint32_t>(chunk_->instruction_count()));
            pending_captures_.pop_back();
            return true;
        }
        case ir::Opcode::EndEffectScope:
            return chunk_->emit(Opcode::EndEffectScope);
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
            if (instruction.text == "Iota") {
                return chunk_->emit(
                    Opcode::Iota,
                    static_cast<std::uint32_t>(instruction.operands.size()));
            }
            return chunk_->emit(
                Opcode::Call,
                static_cast<std::uint32_t>(instruction.operands.size() - 1U));
        case ir::Opcode::MakeList:
            return chunk_->emit(
                Opcode::MakeList,
                static_cast<std::uint32_t>(instruction.operands.size()));
        case ir::Opcode::MakeTable:
            return chunk_->emit(
                Opcode::MakeTable,
                static_cast<std::uint32_t>(instruction.operands.size() / 2U));
        case ir::Opcode::MakeTaggedList: {
            std::uint32_t tag = 0;
            return chunk_->add_constant(
                       value_from_object(state_->tags.intern(&state_->gc, instruction.text)),
                       tag) &&
                chunk_->emit(Opcode::Constant, tag) &&
                chunk_->emit(
                    Opcode::MakeTaggedList,
                    static_cast<std::uint32_t>(instruction.operands.size()));
        }
        case ir::Opcode::Index:
            return chunk_->emit(Opcode::Index);
        case ir::Opcode::Field: {
            std::uint32_t key = 0;
            return chunk_->add_constant(
                       value_from_object(state_->tags.intern(&state_->gc, instruction.text)),
                       key) &&
                chunk_->emit(Opcode::Field, key);
        }
        case ir::Opcode::SetIndex:
            return chunk_->emit(Opcode::SetIndex);
        case ir::Opcode::SetField: {
            std::uint32_t key = 0;
            return chunk_->add_constant(
                       value_from_object(state_->tags.intern(&state_->gc, instruction.text)),
                       key) &&
                chunk_->emit(Opcode::SetField, key);
        }
        case ir::Opcode::IterInit:
            return chunk_->emit(Opcode::IterInit);
        case ir::Opcode::IterNext:
            local_count_ = std::max(local_count_, static_cast<std::size_t>(instruction.binding) + 1U);
            return chunk_->emit(Opcode::IterNext, instruction.binding);
        case ir::Opcode::IterClose:
            return chunk_->emit(Opcode::IterClose);
        case ir::Opcode::RequireBool:
            return chunk_->emit(Opcode::AssertBool);
        case ir::Opcode::MatchPattern: {
            if (!instruction.pattern) return false;
            runtime::Pattern* pattern = nullptr;
            std::uint32_t index = 0;
            return compile_pattern(*instruction.pattern, pattern) &&
                chunk_->add_pattern(pattern, index) &&
                chunk_->emit(Opcode::MatchPattern, index);
        }
        case ir::Opcode::MatchFailure:
            return chunk_->emit(Opcode::MatchFailure);
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
    pending_captures_.clear();
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
    if (!pending_captures_.empty()) return false;
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
