#include "sema/pattern_check.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace on1x::sema {

namespace {

bool check(
    const syntax::AstNode* pattern,
    syntax::Diagnostics& diagnostics,
    std::vector<std::string_view>& bindings) {
    if (!pattern) {
        diagnostics.add({}, "match arm has no pattern");
        return false;
    }
    switch (pattern->kind) {
    case syntax::AstKind::PatternWildcard:
    case syntax::AstKind::Unit:
    case syntax::AstKind::Bool:
    case syntax::AstKind::Int:
    case syntax::AstKind::Float:
    case syntax::AstKind::String:
        return true;
    case syntax::AstKind::PatternBinding:
        for (std::string_view binding : bindings) {
            if (binding == pattern->text) {
                diagnostics.add(
                    pattern->position,
                    "pattern binds '" + std::string(pattern->text) + "' more than once");
                return false;
            }
        }
        bindings.push_back(pattern->text);
        return true;
    case syntax::AstKind::PatternList:
    case syntax::AstKind::PatternTaggedList: {
        bool saw_tail = false;
        bool valid = true;
        for (const syntax::AstNode* child = pattern->first; child; child = child->next) {
            if (child->variadic) {
                if (pattern->kind != syntax::AstKind::PatternList || saw_tail || child->next) {
                    diagnostics.add(child->position, "pattern tail must be the final List pattern element");
                    valid = false;
                }
                saw_tail = true;
            }
            valid = check(child, diagnostics, bindings) && valid;
        }
        return valid;
    }
    default:
        diagnostics.add(pattern->position, "invalid match pattern");
        return false;
    }
}

}  // namespace

bool check_pattern(const syntax::AstNode* pattern, syntax::Diagnostics& diagnostics) {
    std::vector<std::string_view> bindings;
    return check(pattern, diagnostics, bindings);
}

}  // namespace on1x::sema
