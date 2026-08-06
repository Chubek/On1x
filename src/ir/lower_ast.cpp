#include "ir/lower_ast.hpp"

#include "ir/builder.hpp"

#include <memory>

namespace on1x::ir {

std::shared_ptr<Pattern> lower_match_pattern(
    const syntax::AstNode* node,
    syntax::Diagnostics& diagnostics);
void emit_effect_capture_begin(Builder& builder, syntax::SourcePosition position);
void emit_effect_capture_end(Builder& builder, syntax::SourcePosition position);
void emit_effect_scope_end(Builder& builder, syntax::SourcePosition position);

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
            if (target && target->kind == syntax::AstKind::Identifier) {
                const Register value = lower_expression(value_node);
                if (value == no_register) return no_register;
                const Opcode opcode =
                    target->resolution == syntax::ResolutionKind::Local
                        ? Opcode::StoreLocal
                        : Opcode::StoreGlobal;
                builder_.emit_void(opcode, node->position, target->text, {value});
                function().blocks[active_block_].instructions.back().binding = target->binding_index;
                return value;
            }
            if (target && target->kind == syntax::AstKind::Index) {
                const syntax::AstNode* container_node = target->first;
                const syntax::AstNode* key_node = container_node ? container_node->next : nullptr;
                const Register container = lower_expression(container_node);
                const Register key = lower_expression(key_node);
                const Register value = lower_expression(value_node);
                if (container == no_register || key == no_register || value == no_register) {
                    return no_register;
                }
                return builder_.emit(Opcode::SetIndex, node->position, {}, {container, key, value});
            }
            if (target && target->kind == syntax::AstKind::Field) {
                const Register container = lower_expression(target->first);
                const Register value = lower_expression(value_node);
                if (container == no_register || value == no_register) return no_register;
                return builder_.emit(Opcode::SetField, node->position, target->text, {container, value});
            }
            diagnostics_.add(node->position, "invalid assignment target");
            return no_register;
        }
        if (node->kind == syntax::AstKind::Effect) {
            const syntax::AstNode* operand = node->first;
            const syntax::AstNode* follower = operand ? operand->next : nullptr;
            emit_effect_capture_begin(builder_, node->position);
            const Register captured = lower_expression(operand);
            if (captured == no_register) return no_register;
            emit_effect_capture_end(builder_, node->position);
            const Register result = lower_statement(follower);
            if (result == no_register) return no_register;
            emit_effect_scope_end(builder_, node->position);
            return result;
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
        if (node->kind == syntax::AstKind::While) return lower_while(node);
        if (node->kind == syntax::AstKind::For) return lower_for(node);
        if (node->kind == syntax::AstKind::Break) return lower_loop_control(node, true);
        if (node->kind == syntax::AstKind::Continue) return lower_loop_control(node, false);
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
            return builder_.emit(Opcode::EffectResult, node->position);
        case syntax::AstKind::Unary: {
            const Register operand = lower_expression(node->first);
            return operand == no_register
                ? no_register
                : builder_.emit(Opcode::Unary, node->position, node->text, {operand});
        }
        case syntax::AstKind::Binary: {
            if (node->text == "and" || node->text == "or") return lower_logical(node);
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
            const syntax::AstNode* child = node->first;
            if (node->kind == syntax::AstKind::Call &&
                child &&
                child->kind == syntax::AstKind::Identifier &&
                child->text == "Iota") {
                instruction.text = "Iota";
                child = child->next;
            }
            for (; child; child = child->next) {
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
        case syntax::AstKind::Enum:
            return lower_enum(node);
        case syntax::AstKind::EnumIota:
            if (enum_iota_ < 0) {
                diagnostics_.add(node->position, "Iota is only available inside an enum");
                return no_register;
            }
            {
                const Register result = builder_.emit(Opcode::Int, node->position);
                Instruction& instruction = function().blocks[active_block_].instructions.back();
                instruction.integer_value = enum_iota_;
                instruction.has_integer_value = true;
                return result;
            }
        case syntax::AstKind::Block:
            return lower_block(node);
        case syntax::AstKind::If:
            return lower_if(node);
        case syntax::AstKind::Match:
            return lower_match(node);
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
        emit_branch_if_false(condition, else_id, node->position);
        emit_jump(then_id, node->position);

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

    Register lower_logical(const syntax::AstNode* node) {
        const syntax::AstNode* left_node = node->first;
        const syntax::AstNode* right_node = left_node ? left_node->next : nullptr;
        const Register left = lower_expression(left_node);
        if (left == no_register || !right_node) return no_register;

        const BlockId short_circuit_id = append_block();
        const BlockId right_id = append_block();
        const BlockId merge_id = append_block();
        const bool is_and = node->text == "and";
        emit_branch_if_false(left, is_and ? short_circuit_id : right_id, node->position);
        emit_jump(is_and ? right_id : short_circuit_id, node->position);

        active_block_ = short_circuit_id;
        builder_.set_block(active_block_);
        const Register short_circuit = builder_.emit(
            Opcode::Bool, node->position, is_and ? "false" : "true");
        emit_jump(merge_id, node->position);

        active_block_ = right_id;
        builder_.set_block(active_block_);
        const Register right = lower_expression(right_node);
        if (right == no_register) return no_register;
        const Register checked_right = builder_.emit(Opcode::RequireBool, node->position, {}, {right});
        emit_jump(merge_id, node->position);

        active_block_ = merge_id;
        builder_.set_block(active_block_);
        return builder_.emit(Opcode::Phi, node->position, {}, {short_circuit, checked_right});
    }

    Register lower_match(const syntax::AstNode* node) {
        const syntax::AstNode* subject_node = node->first;
        if (!subject_node) return no_register;
        const Register subject = lower_expression(subject_node);
        if (subject == no_register) return no_register;
        builder_.emit_void(Opcode::StoreLocal, node->position, {}, {subject});
        function().blocks[active_block_].instructions.back().binding = node->binding_index;
        builder_.emit_void(Opcode::Discard, node->position, {}, {subject});

        const BlockId merge_id = append_block();
        const syntax::AstNode* arm = subject_node->next;
        while (arm) {
            const syntax::AstNode* pattern_node = arm->first;
            const syntax::AstNode* body_node = pattern_node ? pattern_node->next : nullptr;
            if (arm->kind != syntax::AstKind::MatchArm || !pattern_node || !body_node) {
                diagnostics_.add(node->position, "invalid match arm in IR lowering");
                return no_register;
            }
            std::shared_ptr<Pattern> pattern = lower_match_pattern(pattern_node, diagnostics_);
            if (!pattern) return no_register;
            const BlockId body_id = append_block();
            const BlockId failure_id = append_block();
            const Register candidate = builder_.emit(Opcode::LoadLocal, node->position);
            function().blocks[active_block_].instructions.back().binding = node->binding_index;
            Instruction match_instruction;
            match_instruction.opcode = Opcode::MatchPattern;
            match_instruction.result = next_register();
            match_instruction.position = pattern_node->position;
            match_instruction.operands.push_back(candidate);
            match_instruction.pattern = std::move(pattern);
            const Register matched = append(std::move(match_instruction));
            emit_branch_if_false(matched, failure_id, pattern_node->position);
            emit_jump(body_id, pattern_node->position);

            active_block_ = body_id;
            builder_.set_block(active_block_);
            const Register arm_result = lower_expression(body_node);
            if (arm_result == no_register) return no_register;
            emit_jump(merge_id, body_node->position);

            active_block_ = failure_id;
            builder_.set_block(active_block_);
            arm = arm->next;
        }

        builder_.emit_void(Opcode::MatchFailure, node->position);
        emit_jump(merge_id, node->position);
        active_block_ = merge_id;
        builder_.set_block(active_block_);
        return builder_.emit(Opcode::Phi, node->position);
    }

    Register lower_while(const syntax::AstNode* node) {
        const syntax::AstNode* condition_node = node->first;
        const syntax::AstNode* body_node = condition_node ? condition_node->next : nullptr;
        if (!condition_node || !body_node) return no_register;
        const BlockId header_id = append_block();
        const BlockId body_id = append_block();
        const BlockId end_id = append_block();
        emit_jump(header_id, node->position);

        active_block_ = header_id;
        builder_.set_block(active_block_);
        const Register condition = lower_expression(condition_node);
        if (condition == no_register) return no_register;
        emit_branch_if_false(condition, end_id, node->position);
        emit_jump(body_id, node->position);

        loop_stack_.push_back({header_id, end_id, false});
        active_block_ = body_id;
        builder_.set_block(active_block_);
        const Register body = lower_expression(body_node);
        if (body == no_register) return no_register;
        builder_.emit_void(Opcode::Discard, body_node->position, {}, {body});
        emit_jump(header_id, node->position);
        loop_stack_.pop_back();

        active_block_ = end_id;
        builder_.set_block(active_block_);
        return builder_.emit(Opcode::Unit, node->position);
    }

    Register lower_for(const syntax::AstNode* node) {
        const syntax::AstNode* iterable_node = node->first;
        const syntax::AstNode* body_node = iterable_node ? iterable_node->next : nullptr;
        if (!iterable_node || !body_node) return no_register;
        const Register iterable = lower_expression(iterable_node);
        if (iterable == no_register) return no_register;
        builder_.emit_void(Opcode::IterInit, node->position, {}, {iterable});

        const BlockId header_id = append_block();
        const BlockId body_id = append_block();
        const BlockId end_id = append_block();
        emit_jump(header_id, node->position);

        active_block_ = header_id;
        builder_.set_block(active_block_);
        const Register next = builder_.emit(Opcode::IterNext, node->position);
        function().blocks[active_block_].instructions.back().binding = node->binding_index;
        emit_branch_if_false(next, end_id, node->position);
        emit_jump(body_id, node->position);

        loop_stack_.push_back({header_id, end_id, true});
        active_block_ = body_id;
        builder_.set_block(active_block_);
        const Register body = lower_expression(body_node);
        if (body == no_register) return no_register;
        builder_.emit_void(Opcode::Discard, body_node->position, {}, {body});
        emit_jump(header_id, node->position);
        loop_stack_.pop_back();

        active_block_ = end_id;
        builder_.set_block(active_block_);
        return builder_.emit(Opcode::Unit, node->position);
    }

    Register lower_loop_control(const syntax::AstNode* node, bool is_break) {
        if (loop_stack_.empty()) {
            diagnostics_.add(node->position, "loop control is not available here");
            return no_register;
        }
        const LoopTarget& loop = loop_stack_.back();
        if (is_break && loop.for_loop) {
            builder_.emit_void(Opcode::IterClose, node->position);
        }
        emit_jump(is_break ? loop.break_target : loop.continue_target, node->position);
        return builder_.emit(Opcode::Unit, node->position);
    }

    Register lower_enum(const syntax::AstNode* node) {
        const std::int64_t previous_iota = enum_iota_;
        enum_iota_ = 0;
        Instruction instruction;
        instruction.opcode = Opcode::MakeTable;
        instruction.result = next_register();
        instruction.position = node->position;
        for (const syntax::AstNode* member = node->first; member; member = member->next) {
            const Register key = builder_.emit(Opcode::Tag, member->position, member->text);
            const Register value = lower_expression(member->first);
            if (value == no_register) {
                enum_iota_ = previous_iota;
                return no_register;
            }
            instruction.operands.push_back(key);
            instruction.operands.push_back(value);
            ++enum_iota_;
        }
        enum_iota_ = previous_iota;
        return append(std::move(instruction));
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

    BlockId append_block() {
        const BlockId id = static_cast<BlockId>(function().blocks.size());
        function().blocks.push_back({id, {}});
        return id;
    }

    void emit_jump(BlockId target, syntax::SourcePosition position) {
        Instruction jump;
        jump.opcode = Opcode::Jump;
        jump.position = position;
        jump.target = target;
        function().blocks[active_block_].instructions.push_back(std::move(jump));
    }

    void emit_branch_if_false(Register condition, BlockId target, syntax::SourcePosition position) {
        Instruction branch;
        branch.opcode = Opcode::BranchIfFalse;
        branch.position = position;
        branch.target = target;
        branch.operands.push_back(condition);
        function().blocks[active_block_].instructions.push_back(std::move(branch));
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
    struct LoopTarget {
        BlockId continue_target;
        BlockId break_target;
        bool for_loop;
    };
    std::vector<LoopTarget> loop_stack_;
    std::int64_t enum_iota_ = -1;
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
