#pragma once

#include "syntax/ast.hpp"
#include "syntax/diagnostics.hpp"

namespace on1x::sema {

[[nodiscard]] bool check_effect_scopes(
    syntax::AstNode* program,
    syntax::Diagnostics& diagnostics);

}  // namespace on1x::sema
