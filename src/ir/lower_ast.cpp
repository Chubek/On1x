#include "ir/lower_ast.hpp"

#include "ir/builder.hpp"

namespace on1x::ir {

namespace {

class Lowerer {
public:
    Lowerer(Module& module, Function& function, syntax::Diagnostics& diagnostics)
        : module_(module), function_(function), builder_(function_), diagnostics_(diagnostics) {}

    bool lower_program(const syntax::AstNode* program) {
        Register result = no_register;
        for (const syntax::AstNode* statement = program->first; statement; statement = statement->sibling) {
            result = lower_statement(statement);
            if (result == no_register) return false;
            if (statement->sibling) {
                builder_.emit_void(Opcode::Discard, statement->position, {}, {result});
            }
        }
        if (result == no_register) result = builder_.emit(Opcode::Unit, program->position);
        builder_.emit_void(Opcode::Return, program->position, {}, {result});
        return true;
    }

private:
    Register lower_statement(const syntax::AstNode* node) {
        if (node->kind == syntax::AstKind::Let) {
            const Register value = lower_expression(node->first);
            if (value == no_register) return no_register;
            const Opcode opcode =
                node->resolution == syntax::ResolutionKind::Local
                    ? Opcode::StoreLocal
                    : Opcode::StoreGlobal;
            builder_.emit_void(opcode, node->position, node->text, {value});
            function().blocks[active_block_].instructions.back().binding = node->binding_index;
            return value;
        }
        if (node->kind == syntax::AstKind::Assign) {
            const syntax::AstNode* target = node->first;
            const syntax::AstNode* value_node = target ? target->next : nullptr;
            const Register value = lower_expression(value_node);
            if (value == no_register) return no_register;
            if (target && target->kind == syntax::AstKind::Identifier) {
                const Opcode opcode =
                    target->resolution == syntax::ResolutionKind::Local
                        ? Opcode::StoreLocal
                        : Opcode::StoreGlobal;
                builder_.emit_void(opcode, node->position, target->text, {value});
                function().blocks[active_block_].instructions.back().binding = target->binding_index;
                return value;
            }
            diagnostics_.add(node->position, "indexed and field assignment lowering is not available yet");
            return no_register;
        }
        if (node->kind == syntax::AstKind::Effect) {
            const syntax::AstNode* operand = node->first;
            const syntax::AstNode* follower = operand ? operand->next : nullptr;
            const Register captured = lower_expression(operand);
            if (captured == no_register) return no_register;
            builder_.emit_void(Opcode::Capture, node->position, {}, {captured});
            return lower_statement(follower);
        }
        if (node->kind == syntax::AstKind::Function && !node->text.empty()) {
            const Register value = lower_expression(node);
            if (value == no_register) return no_register;
            const Opcode opcode = node->resolution == syntax::ResolutionKind::Local
                ? Opcode::StoreLocal
                : Opcode::StoreGlobal;
            builder_.emit_void(opcode, node->position, node->text, {value});
            function().blocks[active_block_].instructions.back().binding = node->binding_index;
            return value;
        }
        if (node->kind == syntax::AstKind::Return) {
            const Register value = node->first
                ? lower_expression(node->first)
                : builder_.emit(Opcode::Unit, node->position);
            if (value == no_register) return no_register;
            builder_.emit_void(Opcode::Return, node->position, {}, {value});
            return value;
        }
        return lower_expression(node);
    }

    Register lower_expression(const syntax::AstNode* node) {
        if (!node) return no_register;
        switch (node->kind) {
        case syntax::AstKind::Unit: return builder_.emit(Opcode::Unit, node->position);
        case syntax::AstKind::Bool: return builder_.emit(Opcode::Bool, node->position, node->text);
        case syntax::AstKind::Int: return builder_.emit(Opcode::Int, node->position, node->text);
        case syntax::AstKind::Float: return builder_.emit(Opcode::Float, node->position, node->text);
        case syntax::AstKind::String: return builder_.emit(Opcode::String, node->position, node->text);
        case syntax::AstKind::Tag: return builder_.emit(Opcode::Tag, node->position, node->text);
        case syntax::AstKind::Identifier: {
            const Opcode opcode = node->resolution == syntax::ResolutionKind::Local
                ? Opcode::LoadLocal
                : node->resolution == syntax::ResolutionKind::Upvalue
                    ? Opcode::LoadUpvalue
                    : Opcode::LoadGlobal;
            const Register result = builder_.emit(opcode, node->position, node->text);
            function().blocks[active_block_].instructions.back().binding = node->binding_index;
            return result;
        }
        case syntax::AstKind::Function:
            return lower_function(node);
        case syntax::AstKind::EffectResult:
            return builder_.emit(Opcode::EffectResult, node->position, node->text);
        case syntax::AstKind::Unary: {
            const Register operand = lower_expression(node->first);
            return operand == no_register
                ? no_register
                : builder_.emit(Opcode::Unary, node->position, node->text, {operand});
        }
        case syntax::AstKind::Binary: {
            const Register left = lower_expression(node->first);
            const Register right = lower_expression(node->first ? node->first->next : nullptr);
            if (left == no_register || right == no_register) return no_register;
            // spec §12: range syntax desugars to Iota.
            return builder_.emit(
                node->text == ".." ? Opcode::Call : Opcode::Binary,
                node->position,
                node->text == ".." ? "Iota" : node->text,
                {left, right});
        }
        case syntax::AstKind::Optional:
            if (!node->first) return builder_.emit(Opcode::None, node->position);
            {
                const Register payload = lower_expression(node->first);
                return payload == no_register
                    ? no_register
                    : builder_.emit(Opcode::Some, node->position, {}, {payload});
            }
        case syntax::AstKind::List: {
            Instruction instruction;
            instruction.opcode = Opcode::MakeList;
            instruction.result = next_register();
            instruction.position = node->position;
            for (const syntax::AstNode* element = node->first; element; element = element->next) {
                const Register value = lower_expression(element);
                if (value == no_register) return no_register;
                instruction.operands.push_back(value);
            }
            return append(std::move(instruction));
        }
        case syntax::AstKind::Table: {
            Instruction instruction;
            instruction.opcode = Opcode::MakeTable;
            instruction.result = next_register();
            instruction.position = node->position;
            for (const syntax::AstNode* entry = node->first; entry; entry = entry->next) {
                const Register key = lower_expression(entry->first);
                const Register value = lower_expression(entry->first ? entry->first->next : nullptr);
                if (key == no_register || value == no_register) return no_register;
                instruction.operands.push_back(key);
                instruction.operands.push_back(value);
            }
            return append(std::move(instruction));
        }
        case syntax::AstKind::TaggedList: {
            Instruction instruction;
            instruction.opcode = Opcode::MakeTaggedList;
            instruction.result = next_register();
            instruction.position = node->position;
            instruction.text = node->text;
            for (const syntax::AstNode* payload = node->first; payload; payload = payload->next) {
                const Register value = lower_expression(payload);
                if (value == no_register) return no_register;
                instruction.operands.push_back(value);
            }
            return append(std::move(instruction));
        }
        case syntax::AstKind::Call:
        case syntax::AstKind::Index: {
            Instruction instruction;
            instruction.opcode = node->kind == syntax::AstKind::Call ? Opcode::Call : Opcode::Index;
            instruction.result = next_register();
            instruction.position = node->position;
            for (const syntax::AstNode* child = node->first; child; child = child->next) {
                const Register value = lower_expression(child);
                if (value == no_register) return no_register;
                instruction.operands.push_back(value);
            }
            return append(std::move(instruction));
        }
        case syntax::AstKind::Field: {
            const Register object = lower_expression(node->first);
            // spec §12: field access desugars to required Get with a Tag key.
            return object == no_register
                ? no_register
                : builder_.emit(Opcode::Field, node->position, node->text, {object});
        }
        case syntax::AstKind::Block:
            return lower_block(node);
        case syntax::AstKind::If:
            return lower_if(node);
        default:
            diagnostics_.add(node->position, "AST node is not supported by IR lowering");
            return no_register;
        }
    }

    Register lower_block(const syntax::AstNode* node) {
        Register result = no_register;
        for (const syntax::AstNode* statement = node->first; statement; statement = statement->sibling) {
            result = lower_statement(statement);
            if (result == no_register) return no_register;
            if (statement->sibling) {
                builder_.emit_void(Opcode::Discard, statement->position, {}, {result});
            }
        }
        return result == no_register ? builder_.emit(Opcode::Unit, node->position) : result;
    }

    Register lower_if(const syntax::AstNode* node) {
        const syntax::AstNode* condition_node = node->first;
        const syntax::AstNode* then_node = condition_node ? condition_node->next : nullptr;
        const syntax::AstNode* else_node = then_node ? then_node->next : nullptr;
        const Register condition = lower_expression(condition_node);
        if (condition == no_register || !then_node) return no_register;

        const BlockId then_id = static_cast<BlockId>(function().blocks.size());
        function().blocks.push_back({then_id, {}});
        const BlockId else_id = static_cast<BlockId>(function().blocks.size());
        function().blocks.push_back({else_id, {}});
        const BlockId merge_id = static_cast<BlockId>(function().blocks.size());
        function().blocks.push_back({merge_id, {}});
        Instruction branch;
        branch.opcode = Opcode::BranchIfFalse;
        branch.position = node->position;
        branch.target = else_id;
        branch.operands.push_back(condition);
        function().blocks[then_id - 1U].instructions.push_back(std::move(branch));

        active_block_ = then_id;
        builder_.set_block(active_block_);
        const Register then_result = lower_expression(then_node);
        if (then_result == no_register) return no_register;
        Instruction then_jump;
        then_jump.opcode = Opcode::Jump;
        then_jump.position = node->position;
        then_jump.target = merge_id;
        function().blocks[active_block_].instructions.push_back(std::move(then_jump));

        active_block_ = else_id;
        builder_.set_block(active_block_);
        const Register else_result = else_node
            ? lower_expression(else_node)
            : builder_.emit(Opcode::Unit, node->position);
        if (else_result == no_register) return no_register;
        Instruction else_jump;
        else_jump.opcode = Opcode::Jump;
        else_jump.position = node->position;
        else_jump.target = merge_id;
        function().blocks[active_block_].instructions.push_back(std::move(else_jump));

        active_block_ = merge_id;
        builder_.set_block(active_block_);
        return builder_.emit(Opcode::Phi, node->position, {}, {then_result, else_result});
    }

    Register lower_function(const syntax::AstNode* node) {
        const BlockId function_id = node->function_index;
        while (module_.functions.size() < function_id) module_.functions.emplace_back();
        Function& nested = module_.function(function_id);
        nested.name = node->text.empty() ? "<lambda>" : std::string(node->text);
        nested.blocks = {{0, {}}};
        nested.register_count = 0;
        nested.parameter_bindings.clear();
        nested.capture_bindings.clear();
        nested.variadic = false;

        const syntax::AstNode* child = node->first;
        while (child && child->kind == syntax::AstKind::Parameter) {
            nested.parameter_bindings.push_back(child->binding_index);
            nested.variadic = child->variadic;
            child = child->next;
        }
        for (const syntax::AstNode* capture = node->captures; capture; capture = capture->next) {
            nested.capture_bindings.push_back(capture->source_binding_index);
        }

        Lowerer nested_lowerer(module_, nested, diagnostics_);
        if (!nested_lowerer.lower_body(child)) return no_register;

        Instruction instruction;
        instruction.opcode = Opcode::MakeFunction;
        instruction.result = next_register();
        instruction.position = node->position;
        instruction.target = function_id;
        for (const syntax::AstNode* capture = node->captures; capture; capture = capture->next) {
            const Opcode load = capture->resolution == syntax::ResolutionKind::Upvalue
                ? Opcode::LoadUpvalue
                : Opcode::LoadLocal;
            const Register value = builder_.emit(load, node->position);
            function().blocks[active_block_].instructions.back().binding = capture->source_binding_index;
            instruction.operands.push_back(value);
        }
        return append(std::move(instruction));
    }

    bool lower_body(const syntax::AstNode* body) {
        const Register result = lower_expression(body);
        if (result == no_register) return false;
        if (function().blocks[active_block_].instructions.empty() ||
            function().blocks[active_block_].instructions.back().opcode != Opcode::Return) {
            builder_.emit_void(Opcode::Return, body ? body->position : syntax::SourcePosition{}, {}, {result});
        }
        return true;
    }

    Register next_register() {
        return function().register_count++;
    }

    Register append(Instruction instruction) {
        const Register result = instruction.result;
        function().blocks[active_block_].instructions.push_back(std::move(instruction));
        return result;
    }

    Function& function() {
        return function_;
    }

    Module& module_;
    Function& function_;
    Builder builder_;
    syntax::Diagnostics& diagnostics_;
    BlockId active_block_ = 0;
};

}

bool lower_ast(const syntax::AstNode* program, Module& module, syntax::Diagnostics& diagnostics) {
    if (!program || program->kind != syntax::AstKind::Program) return false;
    module.entry = {};
    module.entry.name = "<chunk>";
    module.entry.blocks = {{0, {}}};
    module.functions.clear();
    Lowerer lowerer(module, module.entry, diagnostics);
    return lowerer.lower_program(program);
}

}  // namespace on1x::ir
