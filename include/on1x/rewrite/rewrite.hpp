#ifndef ON1X_REWRITE_REWRITE_HPP
#define ON1X_REWRITE_REWRITE_HPP

#include "on1x/core/ast.hpp"
#include "on1x/core/parser.hpp"

#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace on1x::rewrite {

// ---------------------------------------------------------------------------
//  Pattern matching
// ---------------------------------------------------------------------------

struct pattern;
using pattern_ptr = std::shared_ptr<pattern>;

using capture_map = std::map<std::string, expr_ptr>;

struct pattern_wildcard {};
struct pattern_literal { expr::node_t value; };
struct pattern_capture { std::string name; };
struct pattern_unary { std::string op; pattern_ptr rhs; };
struct pattern_binary { std::string op; pattern_ptr lhs; pattern_ptr rhs; };
struct pattern_if { pattern_ptr cond; pattern_ptr then_branch; pattern_ptr else_branch; };
struct pattern_let { std::string name; pattern_ptr value; pattern_ptr body; };
struct pattern_block { std::vector<pattern_ptr> statements; pattern_ptr value; };
struct pattern_list { std::vector<pattern_ptr> elements; };
struct pattern_tuple { std::vector<pattern_ptr> elements; };
struct pattern_record { std::vector<std::pair<std::string, pattern_ptr>> fields; };
struct pattern_lambda { std::vector<std::string> params; pattern_ptr body; };
struct pattern_call { pattern_ptr callee; std::vector<pattern_ptr> args; };

struct pattern {
  using node_t = std::variant<
    pattern_wildcard,
    pattern_literal,
    pattern_capture,
    pattern_unary,
    pattern_binary,
    pattern_if,
    pattern_let,
    pattern_block,
    pattern_list,
    pattern_tuple,
    pattern_record,
    pattern_lambda,
    pattern_call
  >;
  node_t node;
};

// Pattern constructors
pattern_ptr pat_wildcard();
pattern_ptr pat_literal(expr::node_t value);
pattern_ptr pat_capture(std::string name);
pattern_ptr pat_unary(std::string op, pattern_ptr rhs);
pattern_ptr pat_binary(std::string op, pattern_ptr lhs, pattern_ptr rhs);
pattern_ptr pat_if(pattern_ptr cond, pattern_ptr then_branch, pattern_ptr else_branch);
pattern_ptr pat_let(std::string name, pattern_ptr value, pattern_ptr body);
pattern_ptr pat_block(std::vector<pattern_ptr> statements, pattern_ptr value);
pattern_ptr pat_list(std::vector<pattern_ptr> elements);
pattern_ptr pat_tuple(std::vector<pattern_ptr> elements);
pattern_ptr pat_record(std::vector<std::pair<std::string, pattern_ptr>> fields);
pattern_ptr pat_lambda(std::vector<std::string> params, pattern_ptr body);
pattern_ptr pat_call(pattern_ptr callee, std::vector<pattern_ptr> args);

// Convenience: match any expression
pattern_ptr pat_any();

// Convenience: match specific literal values
pattern_ptr pat_int(long long v);
pattern_ptr pat_float(double v);
pattern_ptr pat_bool(bool v);
pattern_ptr pat_string(std::string v);
pattern_ptr pat_unit();
pattern_ptr pat_var(std::string name);

// Match a pattern against an expression, returning captures if successful
std::optional<capture_map> match_pattern(const pattern_ptr &pat, const expr_ptr &e);

// ---------------------------------------------------------------------------
//  Rewrite rules
// ---------------------------------------------------------------------------

struct rewrite_rule {
  std::string name;
  pattern_ptr lhs;
  std::function<expr_ptr(const capture_map &)> build;

  std::optional<expr_ptr> apply(const expr_ptr &e) const;
};

// Convenience: simple replace rule from a pattern to a replacement template.
rewrite_rule make_rule(std::string name, pattern_ptr lhs, expr_ptr rhs);

// Convenience: conditional rule with builder function
rewrite_rule make_conditional_rule(std::string name, pattern_ptr lhs,
                                    std::function<expr_ptr(const capture_map &)> build);

// ---------------------------------------------------------------------------
//  Rewrite engine
// ---------------------------------------------------------------------------

struct rewrite_stats {
  std::size_t rules_applied = 0;
  std::size_t nodes_visited = 0;
};

enum class traversal { bottom_up, top_down };

struct rewriter {
  std::vector<rewrite_rule> rules;
  traversal order = traversal::bottom_up;
  rewrite_stats stats;

  explicit rewriter(std::vector<rewrite_rule> rules = {});

  // Apply all rules to a single expression (returns transformed expression)
  expr_ptr apply(const expr_ptr &e);

  // Apply rules repeatedly until fixed point
  expr_ptr apply_fixed_point(const expr_ptr &e, std::size_t max_rounds = 1024);

  // Apply a single rule to a single expression (returns transformed if matched)
  expr_ptr apply_one(const rewrite_rule &rule, const expr_ptr &e);

  // Transform a program (all top-level forms)
  program apply_program(const program &prog);

  void reset_stats();
};

// ---------------------------------------------------------------------------
//  Built-in optimization rule sets
// ---------------------------------------------------------------------------

// Constant folding: 1 + 2 → 3, true && false → false, etc.
std::vector<rewrite_rule> constant_folding_rules();

// Algebraic simplification: x + 0 → x, x * 1 → x, x * 0 → 0, etc.
std::vector<rewrite_rule> algebraic_simplification_rules();

// Strength reduction: x * 2 → x << 1, etc.
std::vector<rewrite_rule> strength_reduction_rules();

// Boolean simplification: !!x → x, x && true → x, etc.
std::vector<rewrite_rule> boolean_simplification_rules();

// All built-in optimization rules combined
std::vector<rewrite_rule> all_optimization_rules();

// ---------------------------------------------------------------------------
//  AST utilities
// ---------------------------------------------------------------------------

// Deep clone an expression
expr_ptr clone_expr(const expr_ptr &e);

// Check if two expressions are structurally equal
bool expr_equal(const expr_ptr &a, const expr_ptr &b);

// Count nodes in an expression tree
std::size_t expr_node_count(const expr_ptr &e);

}  // namespace on1x::rewrite

#endif
