#include "syntax/ast.hpp"
#include "syntax/diagnostics.hpp"

namespace on1x::sema {

bool check_assignment_target(
    const syntax::AstNode* target,
    syntax::Diagnostics& diagnostics) {
    if (!target || (target->kind != syntax::AstKind::Identifier &&
                    target->kind != syntax::AstKind::Index &&
                    target->kind != syntax::AstKind::Field)) {
        diagnostics.add(
            target ? target->position : syntax::SourcePosition{},
            "left side of assignment is not assignable");
        return false;
    }
    return true;
}

}  // namespace on1x::sema
