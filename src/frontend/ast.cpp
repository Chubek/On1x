#include "on1x/frontend/ast.hpp"

namespace on1x {

expr_ptr make_expr(expr::node_t node) {
  auto out = std::make_shared<expr>();
  out->node = std::move(node);
  return out;
}

}  // namespace on1x
