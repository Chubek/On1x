#ifndef ON1X_CORE_AST_HPP
#define ON1X_CORE_AST_HPP

#include <memory>
#include <string>
#include <variant>
#include <vector>

namespace on1x {

struct expr;
using expr_ptr = std::shared_ptr<expr>;

struct nil_t {};
struct bool_t { bool value; };
struct int_t { long long value; };
struct float_t { double value; };
struct string_t { std::string value; };
struct ident_t { std::string value; };
struct unary_t { std::string op; expr_ptr rhs; };
struct binary_t { std::string op; expr_ptr lhs; expr_ptr rhs; };
struct call_t { expr_ptr callee; std::vector<expr_ptr> args; };
struct if_t { expr_ptr cond; expr_ptr then_branch; expr_ptr else_branch; };
struct let_t { std::string name; expr_ptr value; expr_ptr body; };
struct block_t { std::vector<expr_ptr> statements; expr_ptr value; };
struct list_t { std::vector<expr_ptr> elements; };
struct tuple_t { std::vector<expr_ptr> elements; };
struct record_t { std::vector<std::pair<std::string, expr_ptr>> fields; };
struct lambda_t { std::vector<std::string> params; expr_ptr body; };

struct expr {
  using node_t = std::variant<nil_t, bool_t, int_t, float_t, string_t, ident_t, unary_t,
                              binary_t, call_t, if_t, let_t, block_t, list_t, tuple_t,
                              record_t, lambda_t>;
  node_t node;
};

expr_ptr make_expr(expr::node_t node);

}  // namespace on1x

#endif
