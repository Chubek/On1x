#pragma once

#include "ir/ir.hpp"
#include "syntax/ast.hpp"
#include "syntax/diagnostics.hpp"

namespace on1x::ir {

[[nodiscard]] bool lower_ast(
    const syntax::AstNode* program,
    Module& module,
    syntax::Diagnostics& diagnostics);

}  // namespace on1x::ir
