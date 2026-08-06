#include "sema/resolver.hpp"

#include "sema/effect_scope.hpp"
#include "sema/enum_check.hpp"
#include "sema/pattern_check.hpp"

#include <algorithm>

namespace on1x::sema {

bool check_assignment_target(
    const syntax::AstNode* target,
    syntax::Diagnostics& diagnostics);

const Resolver::Binding* Resolver::find_binding(std::string_view name) const noexcept {
    for (auto iterator = bindings_.rbegin(); iterator != bindings_.rend(); ++iterator) {
        if (iterator->name == name) return &*iterator;
    }
    return nullptr;
}

void Resolver::annotate_reference(syntax::AstNode* node, bool assignment) {
    const Binding* binding = find_binding(node->text);
    if (!binding) {
        if (assignment) {
            diagnostics_.add(node->position, "assignment to undefined binding '" +
                std::string(node->text) + "'");
        } else {
            node->resolution = syntax::ResolutionKind::Global;
        }
        return;
    }
    if (binding->scope_depth == 0) {
        node->resolution = syntax::ResolutionKind::Global;
        node->binding_index = binding->index;
        return;
    }
    if (binding->function_index != current_function_) {
        if (function_stack_.empty()) {
            diagnostics_.add(node->position, "invalid closure reference");
            return;
        }
        node->resolution = syntax::ResolutionKind::Upvalue;
        node->source_binding_index = binding->index;
        node->binding_index = capture(function_stack_.back(), *binding);
        return;
    }
    node->resolution = syntax::ResolutionKind::Local;
    node->binding_index = binding->index;
    node->lexical_depth = binding->scope_depth;
}

std::uint32_t Resolver::capture(syntax::AstNode* function, const Binding& binding) {
    for (syntax::AstNode* capture = function->captures; capture; capture = capture->next) {
        if (capture->function_index == binding.function_index &&
            capture->source_binding_index == binding.index) {
            return capture->binding_index;
        }
    }
    auto capture_owner = std::make_unique<syntax::AstNode>();
    auto* capture_node = capture_owner.get();
    capture_node->kind = syntax::AstKind::Identifier;
    capture_node->function_index = binding.function_index;
    const auto function_iterator =
        std::find(function_stack_.begin(), function_stack_.end(), function);
    const bool has_parent = function_iterator != function_stack_.begin();
    syntax::AstNode* parent = has_parent ? *(function_iterator - 1) : nullptr;
    if (parent && parent->function_index == binding.function_index) {
        capture_node->resolution = syntax::ResolutionKind::Local;
        capture_node->source_binding_index = binding.index;
    } else if (parent) {
        capture_node->resolution = syntax::ResolutionKind::Upvalue;
        capture_node->source_binding_index = capture(parent, binding);
    } else {
        capture_node->resolution = syntax::ResolutionKind::Global;
        capture_node->source_binding_index = binding.index;
    }
    capture_node->binding_index = 0;
    syntax::AstNode** tail = &function->captures;
    while (*tail) {
        ++capture_node->binding_index;
        tail = &(*tail)->next;
    }
    *tail = capture_node;
    captures_.push_back(std::move(capture_owner));
    return capture_node->binding_index;
}

bool Resolver::resolve_expression(syntax::AstNode* node) {
    if (!node) return true;
    if (node->kind == syntax::AstKind::Identifier) {
        annotate_reference(node, false);
        return true;
    }
    if (node->kind == syntax::AstKind::Block) return resolve_block(node);
    if (node->kind == syntax::AstKind::If) {
        syntax::AstNode* condition = node->first;
        syntax::AstNode* then_block = condition ? condition->next : nullptr;
        syntax::AstNode* else_branch = then_block ? then_block->next : nullptr;
        return resolve_expression(condition) && resolve_block(then_block) &&
            (!else_branch || resolve_expression(else_branch));
    }
    if (node->kind == syntax::AstKind::Function) return resolve_function(node);
    if (node->kind == syntax::AstKind::Enum) {
        if (!check_enum(node, diagnostics_)) return false;
        bool valid = true;
        for (syntax::AstNode* member = node->first; member; member = member->next) {
            valid = resolve_expression(member->first) && valid;
        }
        return valid;
    }
    if (node->kind == syntax::AstKind::While) {
        syntax::AstNode* condition = node->first;
        syntax::AstNode* body = condition ? condition->next : nullptr;
        if (!resolve_expression(condition)) return false;
        ++loop_depth_;
        const bool valid = resolve_block(body);
        --loop_depth_;
        return valid;
    }
    if (node->kind == syntax::AstKind::For) return resolve_for(node);
    if (node->kind == syntax::AstKind::Match) return resolve_match(node);
    bool valid = true;
    for (syntax::AstNode* child = node->first; child; child = child->next) {
        valid = resolve_expression(child) && valid;
    }
    return valid;
}

void Resolver::introduce_pattern_bindings(syntax::AstNode* pattern) {
    if (!pattern) return;
    if (pattern->kind == syntax::AstKind::PatternBinding) {
        pattern->resolution = syntax::ResolutionKind::Local;
        pattern->binding_index = next_binding_;
        pattern->lexical_depth = scope_depth_;
        bindings_.push_back({pattern->text, next_binding_++, scope_depth_, current_function_});
        return;
    }
    for (syntax::AstNode* child = pattern->first; child; child = child->next) {
        introduce_pattern_bindings(child);
    }
}

bool Resolver::resolve_match(syntax::AstNode* node) {
    syntax::AstNode* subject = node->first;
    if (!subject || !resolve_expression(subject)) return false;
    node->binding_index = next_binding_++;
    bool valid = true;
    for (syntax::AstNode* arm = subject->next; arm; arm = arm->next) {
        if (arm->kind != syntax::AstKind::MatchArm || !arm->first || !arm->first->next) {
            diagnostics_.add(node->position, "invalid match arm");
            valid = false;
            continue;
        }
        syntax::AstNode* pattern = arm->first;
        syntax::AstNode* body = pattern->next;
        valid = check_pattern(pattern, diagnostics_) && valid;
        const std::size_t bindings_before = bindings_.size();
        ++scope_depth_;
        introduce_pattern_bindings(pattern);
        valid = resolve_expression(body) && valid;
        --scope_depth_;
        bindings_.resize(bindings_before);
    }
    return valid;
}

bool Resolver::resolve_for(syntax::AstNode* node) {
    syntax::AstNode* iterable = node->first;
    syntax::AstNode* body = iterable ? iterable->next : nullptr;
    if (!resolve_expression(iterable) || !body || body->kind != syntax::AstKind::Block) return false;
    const std::size_t bindings_before = bindings_.size();
    ++scope_depth_;
    node->resolution = syntax::ResolutionKind::Local;
    node->binding_index = next_binding_;
    node->lexical_depth = scope_depth_;
    bindings_.push_back({node->text, next_binding_++, scope_depth_, current_function_});
    ++loop_depth_;
    const bool valid = resolve_block(body);
    --loop_depth_;
    --scope_depth_;
    bindings_.resize(bindings_before);
    return valid;
}

bool Resolver::resolve_function(syntax::AstNode* node) {
    const std::size_t bindings_before = bindings_.size();
    const std::uint32_t previous_function = current_function_;
    const std::uint32_t previous_scope = scope_depth_;
    const std::uint32_t previous_loop_depth = loop_depth_;
    node->function_index = next_function_++;
    current_function_ = node->function_index;
    scope_depth_ = 1;
    loop_depth_ = 0;
    function_stack_.push_back(node);

    syntax::AstNode* child = node->first;
    while (child && child->kind == syntax::AstKind::Parameter) {
        child->resolution = syntax::ResolutionKind::Local;
        child->binding_index = next_binding_;
        child->lexical_depth = scope_depth_;
        bindings_.push_back({child->text, next_binding_++, scope_depth_, current_function_});
        child = child->next;
    }
    const bool valid = child && child->kind == syntax::AstKind::Block && resolve_block(child);
    function_stack_.pop_back();
    bindings_.resize(bindings_before);
    current_function_ = previous_function;
    scope_depth_ = previous_scope;
    loop_depth_ = previous_loop_depth;
    return valid;
}

bool Resolver::resolve_block(syntax::AstNode* block) {
    if (!block || block->kind != syntax::AstKind::Block) return false;
    const std::size_t bindings_before = bindings_.size();
    ++scope_depth_;
    bool valid = true;
    for (syntax::AstNode* statement = block->first; statement; statement = statement->sibling) {
        valid = resolve_statement(statement) && valid;
    }
    --scope_depth_;
    bindings_.resize(bindings_before);
    return valid;
}

bool Resolver::resolve_statement(syntax::AstNode* node) {
    if (!node) return true;
    if (node->kind == syntax::AstKind::Let) {
        const bool initializer_valid = resolve_expression(node->first);
        node->resolution = scope_depth_ == 0
            ? syntax::ResolutionKind::Global
            : syntax::ResolutionKind::Local;
        node->binding_index = next_binding_;
        node->lexical_depth = scope_depth_;
        bindings_.push_back({node->text, next_binding_++, scope_depth_, current_function_});
        return initializer_valid;
    }
    if (node->kind == syntax::AstKind::Function && !node->text.empty()) {
        node->resolution = scope_depth_ == 0
            ? syntax::ResolutionKind::Global
            : syntax::ResolutionKind::Local;
        node->binding_index = next_binding_;
        node->lexical_depth = scope_depth_;
        bindings_.push_back({node->text, next_binding_++, scope_depth_, current_function_});
        return resolve_function(node);
    }
    if (node->kind == syntax::AstKind::Return) {
        if (current_function_ == 0) {
            diagnostics_.add(node->position, "return is only valid inside a function");
            return false;
        }
        return resolve_expression(node->first);
    }
    if (node->kind == syntax::AstKind::Break || node->kind == syntax::AstKind::Continue) {
        if (loop_depth_ == 0) {
            diagnostics_.add(
                node->position,
                node->kind == syntax::AstKind::Break
                    ? "break is only valid inside a loop"
                    : "continue is only valid inside a loop");
            return false;
        }
        return true;
    }
    if (node->kind == syntax::AstKind::Assign) {
        syntax::AstNode* target = node->first;
        syntax::AstNode* value = target ? target->next : nullptr;
        if (!check_assignment_target(target, diagnostics_)) return false;
        if (target && target->kind == syntax::AstKind::Identifier) {
            annotate_reference(target, true);
        } else {
            if (!resolve_expression(target)) return false;
        }
        return resolve_expression(value);
    }
    if (node->kind == syntax::AstKind::Effect) {
        syntax::AstNode* operand = node->first;
        syntax::AstNode* follower = operand ? operand->next : nullptr;
        return resolve_expression(operand) && resolve_statement(follower);
    }
    return resolve_expression(node);
}

bool Resolver::resolve(syntax::AstNode* program) {
    if (!program || program->kind != syntax::AstKind::Program) return false;
    const std::size_t diagnostics_before = diagnostics_.entries().size();
    bool valid = check_effect_scopes(program, diagnostics_);
    for (syntax::AstNode* statement = program->first; statement; statement = statement->sibling) {
        valid = resolve_statement(statement) && valid;
    }
    return valid && diagnostics_.entries().size() == diagnostics_before;
}

}  // namespace on1x::sema
