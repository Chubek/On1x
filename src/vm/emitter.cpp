#include "vm/emitter.hpp"

#include "core/string.hpp"
#include "core/tag_table.hpp"
#include "runtime/state.hpp"
#include "syntax/literals.hpp"

namespace on1x::vm {

bool Emitter::emit_constant(Value value) {
    std::uint32_t index = 0;
    return chunk_.add_constant(value, index) && chunk_.emit(Opcode::Constant, index);
}

bool Emitter::emit_expression(const syntax::AstNode* node) {
    if (!node) return false;
    try {
        switch (node->kind) {
        case syntax::AstKind::Unit: return emit_constant(Value::unit());
        case syntax::AstKind::Bool: return emit_constant(Value::boolean(node->text == "true"));
        case syntax::AstKind::Int: {
            std::int64_t value = 0;
            return syntax::decode_integer(node->text, value) && emit_constant(Value::integer(&state_->gc, value));
        }
        case syntax::AstKind::Float: {
            double value = 0.0;
            return syntax::decode_float(node->text, value) && emit_constant(Value::floating(value));
        }
        case syntax::AstKind::String: {
            std::string value;
            return syntax::decode_string(node->text, value) &&
                emit_constant(value_from_object(new_string(&state_->gc, value)));
        }
        case syntax::AstKind::Tag:
            return emit_constant(value_from_object(state_->tags.intern(&state_->gc, node->text)));
        case syntax::AstKind::Identifier: {
            std::uint32_t index = 0;
            return chunk_.add_constant(value_from_object(state_->tags.intern(&state_->gc, node->text)), index) &&
                chunk_.emit(Opcode::LoadGlobal, index);
        }
        case syntax::AstKind::Binary:
            if (!emit_expression(node->first) || !emit_expression(node->first->next)) return false;
            return chunk_.emit(node->text == "+" ? Opcode::Add : Opcode::Subtract);
        default:
            diagnostics_.add(node->position, "expression is not implemented by the bytecode emitter");
            return false;
        }
    } catch (...) {
        diagnostics_.add(node->position, "unable to emit expression");
        return false;
    }
}

bool Emitter::emit_program(const syntax::AstNode* program) {
    if (!program || program->kind != syntax::AstKind::Program) return false;
    if (!program->first) return emit_constant(Value::unit()) && chunk_.emit(Opcode::Return);
    for (const syntax::AstNode* statement = program->first; statement; statement = statement->sibling) {
        if (!emit_expression(statement)) return false;
        if (statement->sibling && !chunk_.emit(Opcode::Pop)) return false;
    }
    return chunk_.emit(Opcode::Return);
}

}  // namespace on1x::vm
