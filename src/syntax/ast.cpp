#include "syntax/ast.hpp"

namespace on1x::syntax {
static_assert(sizeof(AstNode) >= sizeof(void*) * 2U);
}  // namespace on1x::syntax
