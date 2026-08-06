#pragma once

#include "syntax/ast.hpp"
#include "syntax/diagnostics.hpp"

namespace on1x::sema {

[[nodiscard]] bool check_enum(
    syntax::AstNode* enumeration,
    syntax::Diagnostics& diagnostics);

}  // namespace on1x::sema
