#include "sema/effect_scope.hpp"

namespace on1x::sema {

namespace {

bool visit(syntax::AstNode* node, syntax::Diagnostics& diagnostics, std::uint32_t effect_depth) {
    if (!node) return true;
    if (node->kind == syntax::AstKind::EffectResult) {
        if (effect_depth == 0) {
            diagnostics.add(node->position, "'~' is only available in an effect follower statement");
            return false;
        }
        node->lexical_depth = effect_depth - 1U;
        return true;
    }
    if (node->kind == syntax::AstKind::Effect) {
        syntax::AstNode* operand = node->first;
        syntax::AstNode* follower = operand ? operand->next : nullptr;
        const bool operand_valid = visit(operand, diagnostics, effect_depth);
        // spec §9: '~' binds only in the follower statement; nesting makes the
        // innermost capture the depth-zero result.
        const bool follower_valid = visit(follower, diagnostics, effect_depth + 1U);
        return operand_valid && follower_valid;
    }
    if (node->kind == syntax::AstKind::Block) {
        bool valid = true;
        for (syntax::AstNode* statement = node->first; statement; statement = statement->sibling) {
            valid = visit(statement, diagnostics, effect_depth) && valid;
        }
        return valid;
    }
    bool valid = true;
    for (syntax::AstNode* child = node->first; child; child = child->next) {
        valid = visit(child, diagnostics, effect_depth) && valid;
    }
    return valid;
}

}

bool check_effect_scopes(syntax::AstNode* program, syntax::Diagnostics& diagnostics) {
    if (!program || program->kind != syntax::AstKind::Program) return false;
    bool valid = true;
    for (syntax::AstNode* statement = program->first; statement; statement = statement->sibling) {
        valid = visit(statement, diagnostics, 0) && valid;
    }
    return valid;
}

}  // namespace on1x::sema
