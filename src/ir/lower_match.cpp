#include "ir/ir.hpp"

#include <memory>

namespace on1x::ir {

std::shared_ptr<Pattern> lower_match_pattern(
    const syntax::AstNode* node,
    syntax::Diagnostics& diagnostics) {
    if (!node) return {};
    auto pattern = std::make_shared<Pattern>();
    switch (node->kind) {
    case syntax::AstKind::Unit:
    case syntax::AstKind::Bool:
    case syntax::AstKind::Int:
    case syntax::AstKind::Float:
    case syntax::AstKind::String:
        pattern->kind = PatternKind::Literal;
        pattern->literal_kind = node->kind;
        pattern->text = node->text;
        return pattern;
    case syntax::AstKind::PatternWildcard:
        pattern->kind = PatternKind::Wildcard;
        return pattern;
    case syntax::AstKind::PatternBinding:
        pattern->kind = PatternKind::Binding;
        pattern->binding = node->binding_index;
        return pattern;
    case syntax::AstKind::PatternList:
    case syntax::AstKind::PatternTaggedList:
        pattern->kind = node->kind == syntax::AstKind::PatternList
            ? PatternKind::List
            : PatternKind::TaggedList;
        pattern->text = node->text;
        for (const syntax::AstNode* child = node->first; child; child = child->next) {
            if (child->variadic) {
                pattern->has_tail = true;
                pattern->tail_binding = child->binding_index;
                continue;
            }
            std::shared_ptr<Pattern> child_pattern = lower_match_pattern(child, diagnostics);
            if (!child_pattern) return {};
            pattern->children.push_back(*child_pattern);
        }
        return pattern;
    default:
        diagnostics.add(node->position, "invalid match pattern in IR lowering");
        return {};
    }
}

}  // namespace on1x::ir
