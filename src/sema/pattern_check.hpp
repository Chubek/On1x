#pragma once

#include "syntax/ast.hpp"
#include "syntax/diagnostics.hpp"

namespace on1x::sema {

[[nodiscard]] bool check_pattern(
    const syntax::AstNode* pattern,
    syntax::Diagnostics& diagnostics);

}  // namespace on1x::sema
