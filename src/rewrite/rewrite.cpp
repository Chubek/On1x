#include "on1x/rewrite/rewrite.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <type_traits>

namespace on1x::rewrite {

// ---------------------------------------------------------------------------
//  Pattern constructors
// ---------------------------------------------------------------------------

static pattern_ptr make_pat(pattern::node_t node) {
  auto p = std::make_shared<pattern>();
  p->node = std::move(node);
  return p;
}

pattern_ptr pat_wildcard() { return make_pat(pattern_wildcard{}); }
pattern_ptr pat_any() { return pat_wildcard(); }

pattern_ptr pat_literal(expr::node_t value) {
  return make_pat(pattern_literal{std::move(value)});
}

pattern_ptr pat_capture(std::string name) {
  return make_pat(pattern_capture{std::move(name)});
}

pattern_ptr pat_unary(std::string op, pattern_ptr rhs) {
  return make_pat(pattern_unary{std::move(op), std::move(rhs)});
}

pattern_ptr pat_binary(std::string op, pattern_ptr lhs, pattern_ptr rhs) {
  return make_pat(pattern_binary{std::move(op), std::move(lhs), std::move(rhs)});
}

pattern_ptr pat_if(pattern_ptr cond, pattern_ptr then_branch, pattern_ptr else_branch) {
  return make_pat(pattern_if{std::move(cond), std::move(then_branch), std::move(else_branch)});
}

pattern_ptr pat_let(std::string name, pattern_ptr value, pattern_ptr body) {
  return make_pat(pattern_let{std::move(name), std::move(value), std::move(body)});
}

pattern_ptr pat_block(std::vector<pattern_ptr> statements, pattern_ptr value) {
  return make_pat(pattern_block{std::move(statements), std::move(value)});
}

pattern_ptr pat_list(std::vector<pattern_ptr> elements) {
  return make_pat(pattern_list{std::move(elements)});
}

pattern_ptr pat_tuple(std::vector<pattern_ptr> elements) {
  return make_pat(pattern_tuple{std::move(elements)});
}

pattern_ptr pat_record(std::vector<std::pair<std::string, pattern_ptr>> fields) {
  return make_pat(pattern_record{std::move(fields)});
}

pattern_ptr pat_lambda(std::vector<std::string> params, pattern_ptr body) {
  return make_pat(pattern_lambda{std::move(params), std::move(body)});
}

pattern_ptr pat_call(pattern_ptr callee, std::vector<pattern_ptr> args) {
  return make_pat(pattern_call{std::move(callee), std::move(args)});
}

pattern_ptr pat_int(long long v) { return pat_literal(int_t{v}); }
pattern_ptr pat_float(double v) { return pat_literal(float_t{v}); }
pattern_ptr pat_bool(bool v) { return pat_literal(bool_t{v}); }
pattern_ptr pat_string(std::string v) { return pat_literal(string_t{std::move(v)}); }
pattern_ptr pat_unit() { return pat_literal(nil_t{}); }
pattern_ptr pat_var(std::string name) { return pat_capture(std::move(name)); }

// ---------------------------------------------------------------------------
//  Pattern matching
// ---------------------------------------------------------------------------

namespace {

bool match_pattern_impl(const pattern_ptr &pat, const expr_ptr &e, capture_map &caps) {
  if (!pat || !e) return !pat && !e;

  return std::visit([&](auto &&pval) -> bool {
    using P = std::decay_t<decltype(pval)>;

    if constexpr (std::is_same_v<P, pattern_wildcard>) {
      return true;
    }

    if constexpr (std::is_same_v<P, pattern_capture>) {
      auto it = caps.find(pval.name);
      if (it != caps.end()) return expr_equal(it->second, e);
      caps.emplace(pval.name, e);
      return true;
    }

    if constexpr (std::is_same_v<P, pattern_literal>) {
      return std::visit([&](auto &&lit) -> bool {
        using L = std::decay_t<decltype(lit)>;
        return std::visit([&](auto &&eval) -> bool {
          using E = std::decay_t<decltype(eval)>;
          if constexpr (std::is_same_v<L, E>) {
            if constexpr (std::is_same_v<L, nil_t>) return true;
            else if constexpr (std::is_same_v<L, bool_t>) return lit.value == eval.value;
            else if constexpr (std::is_same_v<L, int_t>) return lit.value == eval.value;
            else if constexpr (std::is_same_v<L, float_t>) return lit.value == eval.value;
            else if constexpr (std::is_same_v<L, string_t>) return lit.value == eval.value;
          }
          return false;
      }, e->node);
      }, pval.value);
    }

    if constexpr (std::is_same_v<P, pattern_unary>) {
      auto ue = std::get_if<unary_t>(&e->node);
      if (!ue) return false;
      if (!pval.op.empty() && pval.op != ue->op) return false;
      auto saved = caps;
      if (match_pattern_impl(pval.rhs, ue->rhs, caps)) return true;
      caps = std::move(saved);
      return false;
    }

    if constexpr (std::is_same_v<P, pattern_binary>) {
      auto be = std::get_if<binary_t>(&e->node);
      if (!be) return false;
      if (!pval.op.empty() && pval.op != be->op) return false;
      auto saved = caps;
      if (match_pattern_impl(pval.lhs, be->lhs, caps) &&
          match_pattern_impl(pval.rhs, be->rhs, caps)) return true;
      caps = std::move(saved);
      return false;
    }

    if constexpr (std::is_same_v<P, pattern_if>) {
      auto ie = std::get_if<if_t>(&e->node);
      if (!ie) return false;
      auto saved = caps;
      if (match_pattern_impl(pval.cond, ie->cond, caps) &&
          match_pattern_impl(pval.then_branch, ie->then_branch, caps) &&
          match_pattern_impl(pval.else_branch, ie->else_branch, caps)) return true;
      caps = std::move(saved);
      return false;
    }

    if constexpr (std::is_same_v<P, pattern_let>) {
      auto le = std::get_if<let_t>(&e->node);
      if (!le) return false;
      if (!pval.name.empty() && pval.name != le->name) return false;
      auto saved = caps;
      if (match_pattern_impl(pval.value, le->value, caps) &&
          match_pattern_impl(pval.body, le->body, caps)) return true;
      caps = std::move(saved);
      return false;
    }

    if constexpr (std::is_same_v<P, pattern_block>) {
      auto be = std::get_if<block_t>(&e->node);
      if (!be) return false;
      if (pval.statements.size() != be->statements.size()) return false;
      auto saved = caps;
      for (std::size_t i = 0; i < pval.statements.size(); ++i) {
        if (!match_pattern_impl(pval.statements[i], be->statements[i], caps))
          { caps = std::move(saved); return false; }
      }
      if (pval.value && be->value)
        return match_pattern_impl(pval.value, be->value, caps);
      if (!pval.value && !be->value) return true;
      caps = std::move(saved);
      return false;
    }

    if constexpr (std::is_same_v<P, pattern_list>) {
      auto le = std::get_if<list_t>(&e->node);
      if (!le) return false;
      if (pval.elements.size() != le->elements.size()) return false;
      auto saved = caps;
      for (std::size_t i = 0; i < pval.elements.size(); ++i) {
        if (!match_pattern_impl(pval.elements[i], le->elements[i], caps))
          { caps = std::move(saved); return false; }
      }
      return true;
    }

    if constexpr (std::is_same_v<P, pattern_tuple>) {
      auto te = std::get_if<tuple_t>(&e->node);
      if (!te) return false;
      if (pval.elements.size() != te->elements.size()) return false;
      auto saved = caps;
      for (std::size_t i = 0; i < pval.elements.size(); ++i) {
        if (!match_pattern_impl(pval.elements[i], te->elements[i], caps))
          { caps = std::move(saved); return false; }
      }
      return true;
    }

    if constexpr (std::is_same_v<P, pattern_record>) {
      auto re = std::get_if<record_t>(&e->node);
      if (!re) return false;
      if (pval.fields.size() != re->fields.size()) return false;
      auto saved = caps;
      for (std::size_t i = 0; i < pval.fields.size(); ++i) {
        if (pval.fields[i].first != re->fields[i].first) {
          caps = std::move(saved);
          return false;
        }
        if (!match_pattern_impl(pval.fields[i].second, re->fields[i].second, caps))
          { caps = std::move(saved); return false; }
      }
      return true;
    }

    if constexpr (std::is_same_v<P, pattern_lambda>) {
      auto le = std::get_if<lambda_t>(&e->node);
      if (!le) return false;
      if (pval.params.size() != le->params.size()) return false;
      for (std::size_t i = 0; i < pval.params.size(); ++i) {
        if (pval.params[i] != le->params[i]) return false;
      }
      return match_pattern_impl(pval.body, le->body, caps);
    }

    if constexpr (std::is_same_v<P, pattern_call>) {
      auto ce = std::get_if<call_t>(&e->node);
      if (!ce) return false;
      auto saved = caps;
      if (!match_pattern_impl(pval.callee, ce->callee, caps)) {
        caps = std::move(saved);
        return false;
      }
      if (pval.args.size() != ce->args.size()) {
        caps = std::move(saved);
        return false;
      }
      for (std::size_t i = 0; i < pval.args.size(); ++i) {
        if (!match_pattern_impl(pval.args[i], ce->args[i], caps))
          { caps = std::move(saved); return false; }
      }
      return true;
    }

    return false;
  }, pat->node);
}

}  // namespace

std::optional<capture_map> match_pattern(const pattern_ptr &pat, const expr_ptr &e) {
  capture_map caps;
  if (match_pattern_impl(pat, e, caps)) return caps;
  return std::nullopt;
}

// ---------------------------------------------------------------------------
//  Rewrite rule
// ---------------------------------------------------------------------------

std::optional<expr_ptr> rewrite_rule::apply(const expr_ptr &e) const {
  auto caps = match_pattern(lhs, e);
  if (!caps) return std::nullopt;
  return build(*caps);
}

rewrite_rule make_rule(std::string name, pattern_ptr lhs, expr_ptr rhs) {
  auto substitute = [](const expr_ptr &root, const capture_map &caps) -> expr_ptr {
    std::function<expr_ptr(const expr_ptr &)> visit = [&](const expr_ptr &e) -> expr_ptr {
      if (!e) return nullptr;
      if (auto id = std::get_if<ident_t>(&e->node)) {
        auto it = caps.find(id->value);
        if (it != caps.end()) return clone_expr(it->second);
      }
      return std::visit([&](auto &&v) -> expr_ptr {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, unary_t>)
          return make_expr(unary_t{v.op, visit(v.rhs)});
        else if constexpr (std::is_same_v<T, binary_t>)
          return make_expr(binary_t{v.op, visit(v.lhs), visit(v.rhs)});
        else if constexpr (std::is_same_v<T, call_t>) {
          std::vector<expr_ptr> args;
          for (const auto &arg : v.args) args.push_back(visit(arg));
          return make_expr(call_t{visit(v.callee), std::move(args)});
        } else if constexpr (std::is_same_v<T, if_t>)
          return make_expr(if_t{visit(v.cond), visit(v.then_branch), visit(v.else_branch)});
        else if constexpr (std::is_same_v<T, let_t>)
          return make_expr(let_t{v.name, visit(v.value), visit(v.body)});
        else if constexpr (std::is_same_v<T, block_t>) {
          std::vector<expr_ptr> statements;
          for (const auto &statement : v.statements) statements.push_back(visit(statement));
          return make_expr(block_t{std::move(statements), visit(v.value)});
        } else if constexpr (std::is_same_v<T, list_t>) {
          std::vector<expr_ptr> elements;
          for (const auto &element : v.elements) elements.push_back(visit(element));
          return make_expr(list_t{std::move(elements)});
        } else if constexpr (std::is_same_v<T, tuple_t>) {
          std::vector<expr_ptr> elements;
          for (const auto &element : v.elements) elements.push_back(visit(element));
          return make_expr(tuple_t{std::move(elements)});
        } else if constexpr (std::is_same_v<T, record_t>) {
          std::vector<std::pair<std::string, expr_ptr>> fields;
          for (const auto &[field, value] : v.fields) fields.emplace_back(field, visit(value));
          return make_expr(record_t{std::move(fields)});
        } else if constexpr (std::is_same_v<T, lambda_t>)
          return make_expr(lambda_t{v.params, visit(v.body)});
        else
          return make_expr(v);
      }, e->node);
    };
    return visit(root);
  };
  return rewrite_rule{
    std::move(name),
    std::move(lhs),
     [rhs = std::move(rhs), substitute = std::move(substitute)](const capture_map &caps) -> expr_ptr {
      return substitute(rhs, caps);
    }
  };
}

rewrite_rule make_conditional_rule(std::string name, pattern_ptr lhs,
                                    std::function<expr_ptr(const capture_map &)> build) {
  return rewrite_rule{std::move(name), std::move(lhs), std::move(build)};
}

// ---------------------------------------------------------------------------
//  Rewrite engine
// ---------------------------------------------------------------------------

rewriter::rewriter(std::vector<rewrite_rule> rules) : rules(std::move(rules)) {}

expr_ptr rewriter::apply_one(const rewrite_rule &rule, const expr_ptr &e) {
  auto result = rule.apply(e);
  if (result) {
    ++stats.rules_applied;
    return *result;
  }
  return e;
}

expr_ptr rewriter::apply(const expr_ptr &e) {
  if (!e) return nullptr;

  // Transform children first (bottom-up) or after (top-down)
  auto transform_children = [this](const expr_ptr &ex) -> expr_ptr {
    return std::visit([this](auto &&val) -> expr_ptr {
      using T = std::decay_t<decltype(val)>;

      if constexpr (std::is_same_v<T, nil_t> || std::is_same_v<T, bool_t> ||
                    std::is_same_v<T, int_t> || std::is_same_v<T, float_t> ||
                    std::is_same_v<T, string_t> || std::is_same_v<T, ident_t>) {
        return make_expr(val);
      }

      if constexpr (std::is_same_v<T, unary_t>) {
        return make_expr(unary_t{val.op, apply(val.rhs)});
      }

      if constexpr (std::is_same_v<T, binary_t>) {
        return make_expr(binary_t{val.op, apply(val.lhs), apply(val.rhs)});
      }

      if constexpr (std::is_same_v<T, call_t>) {
        std::vector<expr_ptr> new_args;
        for (const auto &a : val.args) new_args.push_back(apply(a));
        return make_expr(call_t{apply(val.callee), std::move(new_args)});
      }

      if constexpr (std::is_same_v<T, if_t>) {
        return make_expr(if_t{apply(val.cond), apply(val.then_branch), apply(val.else_branch)});
      }

      if constexpr (std::is_same_v<T, let_t>) {
        return make_expr(let_t{val.name, apply(val.value), apply(val.body)});
      }

      if constexpr (std::is_same_v<T, block_t>) {
        std::vector<expr_ptr> new_stmts;
        for (const auto &s : val.statements) new_stmts.push_back(apply(s));
        return make_expr(block_t{std::move(new_stmts), val.value ? apply(val.value) : nullptr});
      }

      if constexpr (std::is_same_v<T, list_t>) {
        std::vector<expr_ptr> new_els;
        for (const auto &el : val.elements) new_els.push_back(apply(el));
        return make_expr(list_t{std::move(new_els)});
      }

      if constexpr (std::is_same_v<T, tuple_t>) {
        std::vector<expr_ptr> new_els;
        for (const auto &el : val.elements) new_els.push_back(apply(el));
        return make_expr(tuple_t{std::move(new_els)});
      }

      if constexpr (std::is_same_v<T, record_t>) {
        std::vector<std::pair<std::string, expr_ptr>> new_fields;
        for (const auto &[name, f] : val.fields)
          new_fields.emplace_back(name, apply(f));
        return make_expr(record_t{std::move(new_fields)});
      }

      if constexpr (std::is_same_v<T, lambda_t>) {
        return make_expr(lambda_t{val.params, apply(val.body)});
      }

      return make_expr(val);
     }, ex->node);
  };

  ++stats.nodes_visited;

  if (order == traversal::bottom_up) {
    // Bottom-up: transform children first, then apply rules to parent
    auto transformed = transform_children(e);
    for (const auto &rule : rules) {
      auto result = rule.apply(transformed);
      if (result) {
        ++stats.rules_applied;
        if (expr_equal(*result, transformed)) return transformed;
        return *result;
      }
    }
    return transformed;
  } else {
    // Top-down: apply rules to parent first, then transform children
    for (const auto &rule : rules) {
      auto result = rule.apply(e);
      if (result) {
        ++stats.rules_applied;
        if (expr_equal(*result, e)) return e;
        return *result;
      }
    }
    return transform_children(e);
  }
}

expr_ptr rewriter::apply_fixed_point(const expr_ptr &e, std::size_t max_rounds) {
  expr_ptr current = e;
  for (std::size_t round = 0; round < max_rounds; ++round) {
    auto prev_count = stats.rules_applied;
    auto next = apply(current);
    if (expr_equal(next, current) || stats.rules_applied == prev_count) {
      // No rules fired — fixed point reached
      return next;
    }
    current = next;
  }
  return current;
}

program rewriter::apply_program(const program &prog) {
  program out;
  for (const auto &form : prog.forms) {
    out.forms.push_back(apply(form));
  }
  return out;
}

void rewriter::reset_stats() {
  stats = rewrite_stats{};
}

// ---------------------------------------------------------------------------
//  AST utilities
// ---------------------------------------------------------------------------

expr_ptr clone_expr(const expr_ptr &e) {
  if (!e) return nullptr;
  return std::visit([](auto &&val) -> expr_ptr {
    using T = std::decay_t<decltype(val)>;
    if constexpr (std::is_same_v<T, nil_t> || std::is_same_v<T, bool_t> ||
                  std::is_same_v<T, int_t> || std::is_same_v<T, float_t> ||
                  std::is_same_v<T, string_t> || std::is_same_v<T, ident_t>) {
      return make_expr(val);
    }
    if constexpr (std::is_same_v<T, unary_t>) {
      return make_expr(unary_t{val.op, clone_expr(val.rhs)});
    }
    if constexpr (std::is_same_v<T, binary_t>) {
      return make_expr(binary_t{val.op, clone_expr(val.lhs), clone_expr(val.rhs)});
    }
    if constexpr (std::is_same_v<T, call_t>) {
      std::vector<expr_ptr> args;
      for (const auto &a : val.args) args.push_back(clone_expr(a));
      return make_expr(call_t{clone_expr(val.callee), std::move(args)});
    }
    if constexpr (std::is_same_v<T, if_t>) {
      return make_expr(if_t{clone_expr(val.cond), clone_expr(val.then_branch),
                             clone_expr(val.else_branch)});
    }
    if constexpr (std::is_same_v<T, let_t>) {
      return make_expr(let_t{val.name, clone_expr(val.value), clone_expr(val.body)});
    }
    if constexpr (std::is_same_v<T, block_t>) {
      std::vector<expr_ptr> stmts;
      for (const auto &s : val.statements) stmts.push_back(clone_expr(s));
      return make_expr(block_t{std::move(stmts), val.value ? clone_expr(val.value) : nullptr});
    }
    if constexpr (std::is_same_v<T, list_t>) {
      std::vector<expr_ptr> els;
      for (const auto &el : val.elements) els.push_back(clone_expr(el));
      return make_expr(list_t{std::move(els)});
    }
    if constexpr (std::is_same_v<T, tuple_t>) {
      std::vector<expr_ptr> els;
      for (const auto &el : val.elements) els.push_back(clone_expr(el));
      return make_expr(tuple_t{std::move(els)});
    }
    if constexpr (std::is_same_v<T, record_t>) {
      std::vector<std::pair<std::string, expr_ptr>> fields;
      for (const auto &[name, f] : val.fields)
        fields.emplace_back(name, clone_expr(f));
      return make_expr(record_t{std::move(fields)});
    }
    if constexpr (std::is_same_v<T, lambda_t>) {
      return make_expr(lambda_t{val.params, clone_expr(val.body)});
    }
    return make_expr(val);
  }, e->node);
}

bool expr_equal(const expr_ptr &a, const expr_ptr &b) {
  if (!a && !b) return true;
  if (!a || !b) return false;

  return std::visit([&](auto &&av) -> bool {
    using T = std::decay_t<decltype(av)>;
    auto bv = std::get_if<T>(&b->node);
    if (!bv) return false;

    if constexpr (std::is_same_v<T, nil_t>) return true;
    if constexpr (std::is_same_v<T, bool_t>) return av.value == bv->value;
    if constexpr (std::is_same_v<T, int_t>) return av.value == bv->value;
    if constexpr (std::is_same_v<T, float_t>) return av.value == bv->value;
    if constexpr (std::is_same_v<T, string_t>) return av.value == bv->value;
    if constexpr (std::is_same_v<T, ident_t>) return av.value == bv->value;
    if constexpr (std::is_same_v<T, unary_t>)
      return av.op == bv->op && expr_equal(av.rhs, bv->rhs);
    if constexpr (std::is_same_v<T, binary_t>)
      return av.op == bv->op && expr_equal(av.lhs, bv->lhs) && expr_equal(av.rhs, bv->rhs);
    if constexpr (std::is_same_v<T, call_t>) {
      if (av.args.size() != bv->args.size()) return false;
      if (!expr_equal(av.callee, bv->callee)) return false;
      for (std::size_t i = 0; i < av.args.size(); ++i)
        if (!expr_equal(av.args[i], bv->args[i])) return false;
      return true;
    }
    if constexpr (std::is_same_v<T, if_t>)
      return expr_equal(av.cond, bv->cond) && expr_equal(av.then_branch, bv->then_branch) &&
             expr_equal(av.else_branch, bv->else_branch);
    if constexpr (std::is_same_v<T, let_t>)
      return av.name == bv->name && expr_equal(av.value, bv->value) &&
             expr_equal(av.body, bv->body);
    if constexpr (std::is_same_v<T, block_t>) {
      if (av.statements.size() != bv->statements.size()) return false;
      for (std::size_t i = 0; i < av.statements.size(); ++i)
        if (!expr_equal(av.statements[i], bv->statements[i])) return false;
      return expr_equal(av.value, bv->value);
    }
    if constexpr (std::is_same_v<T, list_t>) {
      if (av.elements.size() != bv->elements.size()) return false;
      for (std::size_t i = 0; i < av.elements.size(); ++i)
        if (!expr_equal(av.elements[i], bv->elements[i])) return false;
      return true;
    }
    if constexpr (std::is_same_v<T, tuple_t>) {
      if (av.elements.size() != bv->elements.size()) return false;
      for (std::size_t i = 0; i < av.elements.size(); ++i)
        if (!expr_equal(av.elements[i], bv->elements[i])) return false;
      return true;
    }
    if constexpr (std::is_same_v<T, record_t>) {
      if (av.fields.size() != bv->fields.size()) return false;
      for (std::size_t i = 0; i < av.fields.size(); ++i) {
        if (av.fields[i].first != bv->fields[i].first) return false;
        if (!expr_equal(av.fields[i].second, bv->fields[i].second)) return false;
      }
      return true;
    }
    if constexpr (std::is_same_v<T, lambda_t>) {
      if (av.params.size() != bv->params.size()) return false;
      for (std::size_t i = 0; i < av.params.size(); ++i)
        if (av.params[i] != bv->params[i]) return false;
      return expr_equal(av.body, bv->body);
    }
    return false;
  }, a->node);
}

std::size_t expr_node_count(const expr_ptr &e) {
  if (!e) return 0;
  std::size_t count = 1;
  std::visit([&](auto &&val) {
    using T = std::decay_t<decltype(val)>;
    if constexpr (std::is_same_v<T, unary_t>) count += expr_node_count(val.rhs);
    if constexpr (std::is_same_v<T, binary_t>)
      count += expr_node_count(val.lhs) + expr_node_count(val.rhs);
    if constexpr (std::is_same_v<T, call_t>) {
      count += expr_node_count(val.callee);
      for (const auto &a : val.args) count += expr_node_count(a);
    }
    if constexpr (std::is_same_v<T, if_t>)
      count += expr_node_count(val.cond) + expr_node_count(val.then_branch) +
               expr_node_count(val.else_branch);
    if constexpr (std::is_same_v<T, let_t>)
      count += expr_node_count(val.value) + expr_node_count(val.body);
    if constexpr (std::is_same_v<T, block_t>) {
      for (const auto &s : val.statements) count += expr_node_count(s);
      if (val.value) count += expr_node_count(val.value);
    }
    if constexpr (std::is_same_v<T, list_t>)
      for (const auto &el : val.elements) count += expr_node_count(el);
    if constexpr (std::is_same_v<T, tuple_t>)
      for (const auto &el : val.elements) count += expr_node_count(el);
    if constexpr (std::is_same_v<T, record_t>)
      for (const auto &[_, f] : val.fields) count += expr_node_count(f);
    if constexpr (std::is_same_v<T, lambda_t>)
      count += expr_node_count(val.body);
  }, e->node);
  return count;
}

// ---------------------------------------------------------------------------
//  Built-in optimization rule sets
// ---------------------------------------------------------------------------

std::vector<rewrite_rule> constant_folding_rules() {
  std::vector<rewrite_rule> rules;

  // Binary arithmetic on constants
  // x + y → result (when both are int constants)
  rules.push_back(make_conditional_rule("fold-int-add",
    pat_binary("+", pat_var("a"), pat_var("b")),
    [](const capture_map &caps) -> expr_ptr {
      auto a = std::get_if<int_t>(&caps.at("a")->node);
      auto b = std::get_if<int_t>(&caps.at("b")->node);
      if (a && b) return make_expr(int_t{a->value + b->value});
      return nullptr;
    }));

  rules.push_back(make_conditional_rule("fold-int-sub",
    pat_binary("-", pat_var("a"), pat_var("b")),
    [](const capture_map &caps) -> expr_ptr {
      auto a = std::get_if<int_t>(&caps.at("a")->node);
      auto b = std::get_if<int_t>(&caps.at("b")->node);
      if (a && b) return make_expr(int_t{a->value - b->value});
      return nullptr;
    }));

  rules.push_back(make_conditional_rule("fold-int-mul",
    pat_binary("*", pat_var("a"), pat_var("b")),
    [](const capture_map &caps) -> expr_ptr {
      auto a = std::get_if<int_t>(&caps.at("a")->node);
      auto b = std::get_if<int_t>(&caps.at("b")->node);
      if (a && b) return make_expr(int_t{a->value * b->value});
      return nullptr;
    }));

  rules.push_back(make_conditional_rule("fold-int-div",
    pat_binary("/", pat_var("a"), pat_var("b")),
    [](const capture_map &caps) -> expr_ptr {
      auto a = std::get_if<int_t>(&caps.at("a")->node);
      auto b = std::get_if<int_t>(&caps.at("b")->node);
      if (a && b && b->value != 0) return make_expr(int_t{a->value / b->value});
      return nullptr;
    }));

  rules.push_back(make_conditional_rule("fold-int-mod",
    pat_binary("%", pat_var("a"), pat_var("b")),
    [](const capture_map &caps) -> expr_ptr {
      auto a = std::get_if<int_t>(&caps.at("a")->node);
      auto b = std::get_if<int_t>(&caps.at("b")->node);
      if (a && b && b->value != 0) return make_expr(int_t{a->value % b->value});
      return nullptr;
    }));

  // Float constant folding
  rules.push_back(make_conditional_rule("fold-float-add",
    pat_binary("+", pat_var("a"), pat_var("b")),
    [](const capture_map &caps) -> expr_ptr {
      auto a = std::get_if<float_t>(&caps.at("a")->node);
      auto b = std::get_if<float_t>(&caps.at("b")->node);
      if (a && b) return make_expr(float_t{a->value + b->value});
      return nullptr;
    }));

  rules.push_back(make_conditional_rule("fold-float-sub",
    pat_binary("-", pat_var("a"), pat_var("b")),
    [](const capture_map &caps) -> expr_ptr {
      auto a = std::get_if<float_t>(&caps.at("a")->node);
      auto b = std::get_if<float_t>(&caps.at("b")->node);
      if (a && b) return make_expr(float_t{a->value - b->value});
      return nullptr;
    }));

  rules.push_back(make_conditional_rule("fold-float-mul",
    pat_binary("*", pat_var("a"), pat_var("b")),
    [](const capture_map &caps) -> expr_ptr {
      auto a = std::get_if<float_t>(&caps.at("a")->node);
      auto b = std::get_if<float_t>(&caps.at("b")->node);
      if (a && b) return make_expr(float_t{a->value * b->value});
      return nullptr;
    }));

  rules.push_back(make_conditional_rule("fold-float-div",
    pat_binary("/", pat_var("a"), pat_var("b")),
    [](const capture_map &caps) -> expr_ptr {
      auto a = std::get_if<float_t>(&caps.at("a")->node);
      auto b = std::get_if<float_t>(&caps.at("b")->node);
      if (a && b && b->value != 0.0) return make_expr(float_t{a->value / b->value});
      return nullptr;
    }));

  // Boolean constant folding
  rules.push_back(make_conditional_rule("fold-bool-and",
    pat_binary("&&", pat_var("a"), pat_var("b")),
    [](const capture_map &caps) -> expr_ptr {
      auto a = std::get_if<bool_t>(&caps.at("a")->node);
      auto b = std::get_if<bool_t>(&caps.at("b")->node);
      if (a && b) return make_expr(bool_t{a->value && b->value});
      return nullptr;
    }));

  rules.push_back(make_conditional_rule("fold-bool-or",
    pat_binary("||", pat_var("a"), pat_var("b")),
    [](const capture_map &caps) -> expr_ptr {
      auto a = std::get_if<bool_t>(&caps.at("a")->node);
      auto b = std::get_if<bool_t>(&caps.at("b")->node);
      if (a && b) return make_expr(bool_t{a->value || b->value});
      return nullptr;
    }));

  // Comparison constant folding
  rules.push_back(make_conditional_rule("fold-int-eq",
    pat_binary("==", pat_var("a"), pat_var("b")),
    [](const capture_map &caps) -> expr_ptr {
      auto a = std::get_if<int_t>(&caps.at("a")->node);
      auto b = std::get_if<int_t>(&caps.at("b")->node);
      if (a && b) return make_expr(bool_t{a->value == b->value});
      return nullptr;
    }));

  rules.push_back(make_conditional_rule("fold-int-ne",
    pat_binary("!=", pat_var("a"), pat_var("b")),
    [](const capture_map &caps) -> expr_ptr {
      auto a = std::get_if<int_t>(&caps.at("a")->node);
      auto b = std::get_if<int_t>(&caps.at("b")->node);
      if (a && b) return make_expr(bool_t{a->value != b->value});
      return nullptr;
    }));

  rules.push_back(make_conditional_rule("fold-int-lt",
    pat_binary("<", pat_var("a"), pat_var("b")),
    [](const capture_map &caps) -> expr_ptr {
      auto a = std::get_if<int_t>(&caps.at("a")->node);
      auto b = std::get_if<int_t>(&caps.at("b")->node);
      if (a && b) return make_expr(bool_t{a->value < b->value});
      return nullptr;
    }));

  rules.push_back(make_conditional_rule("fold-int-gt",
    pat_binary(">", pat_var("a"), pat_var("b")),
    [](const capture_map &caps) -> expr_ptr {
      auto a = std::get_if<int_t>(&caps.at("a")->node);
      auto b = std::get_if<int_t>(&caps.at("b")->node);
      if (a && b) return make_expr(bool_t{a->value > b->value});
      return nullptr;
    }));

  // Unary negation constant folding
  rules.push_back(make_conditional_rule("fold-neg-int",
    pat_unary("-", pat_var("a")),
    [](const capture_map &caps) -> expr_ptr {
      auto a = std::get_if<int_t>(&caps.at("a")->node);
      if (a) return make_expr(int_t{-a->value});
      return nullptr;
    }));

  rules.push_back(make_conditional_rule("fold-not-bool",
    pat_unary("!", pat_var("a")),
    [](const capture_map &caps) -> expr_ptr {
      auto a = std::get_if<bool_t>(&caps.at("a")->node);
      if (a) return make_expr(bool_t{!a->value});
      return nullptr;
    }));

  return rules;
}

std::vector<rewrite_rule> algebraic_simplification_rules() {
  std::vector<rewrite_rule> rules;

  // x + 0 → x
  rules.push_back(make_conditional_rule("add-zero",
    pat_binary("+", pat_var("x"), pat_int(0)),
    [](const capture_map &caps) -> expr_ptr {
      return clone_expr(caps.at("x"));
    }));

  rules.push_back(make_conditional_rule("zero-add",
    pat_binary("+", pat_int(0), pat_var("x")),
    [](const capture_map &caps) -> expr_ptr {
      return clone_expr(caps.at("x"));
    }));

  // x - 0 → x
  rules.push_back(make_conditional_rule("sub-zero",
    pat_binary("-", pat_var("x"), pat_int(0)),
    [](const capture_map &caps) -> expr_ptr {
      return clone_expr(caps.at("x"));
    }));

  // x * 1 → x
  rules.push_back(make_conditional_rule("mul-one",
    pat_binary("*", pat_var("x"), pat_int(1)),
    [](const capture_map &caps) -> expr_ptr {
      return clone_expr(caps.at("x"));
    }));

  rules.push_back(make_conditional_rule("one-mul",
    pat_binary("*", pat_int(1), pat_var("x")),
    [](const capture_map &caps) -> expr_ptr {
      return clone_expr(caps.at("x"));
    }));

  // x * 0 → 0
  rules.push_back(make_conditional_rule("mul-zero",
    pat_binary("*", pat_var("x"), pat_int(0)),
    [](const capture_map &caps) -> expr_ptr {
      return make_expr(int_t{0});
    }));

  rules.push_back(make_conditional_rule("zero-mul",
    pat_binary("*", pat_int(0), pat_var("x")),
    [](const capture_map &caps) -> expr_ptr {
      return make_expr(int_t{0});
    }));

  // x / 1 → x
  rules.push_back(make_conditional_rule("div-one",
    pat_binary("/", pat_var("x"), pat_int(1)),
    [](const capture_map &caps) -> expr_ptr {
      return clone_expr(caps.at("x"));
    }));

  // x - x → 0
  rules.push_back(make_conditional_rule("sub-self",
    pat_binary("-", pat_var("x"), pat_var("x")),
    [](const capture_map &caps) -> expr_ptr {
      return make_expr(int_t{0});
    }));

  // x / x → 1 (when x != 0)
  rules.push_back(make_conditional_rule("div-self",
    pat_binary("/", pat_var("x"), pat_var("x")),
    [](const capture_map &caps) -> expr_ptr {
      auto x = caps.at("x");
      auto iv = std::get_if<int_t>(&x->node);
      if (iv && iv->value != 0) return make_expr(int_t{1});
      return nullptr;
    }));

  // Double negation: --x → x
  rules.push_back(make_conditional_rule("double-neg",
    pat_unary("-", pat_unary("-", pat_var("x"))),
    [](const capture_map &caps) -> expr_ptr {
      return clone_expr(caps.at("x"));
    }));

  return rules;
}

std::vector<rewrite_rule> strength_reduction_rules() {
  std::vector<rewrite_rule> rules;

  // x * 2 → x << 1
  rules.push_back(make_conditional_rule("mul2-to-shl",
    pat_binary("*", pat_var("x"), pat_int(2)),
    [](const capture_map &caps) -> expr_ptr {
      return make_expr(binary_t{"<<", clone_expr(caps.at("x")), make_expr(int_t{1})});
    }));

  // x * 4 → x << 2
  rules.push_back(make_conditional_rule("mul4-to-shl",
    pat_binary("*", pat_var("x"), pat_int(4)),
    [](const capture_map &caps) -> expr_ptr {
      return make_expr(binary_t{"<<", clone_expr(caps.at("x")), make_expr(int_t{2})});
    }));

  // x * 8 → x << 3
  rules.push_back(make_conditional_rule("mul8-to-shl",
    pat_binary("*", pat_var("x"), pat_int(8)),
    [](const capture_map &caps) -> expr_ptr {
      return make_expr(binary_t{"<<", clone_expr(caps.at("x")), make_expr(int_t{3})});
    }));

  // x / 2 → x >> 1
  rules.push_back(make_conditional_rule("div2-to-shr",
    pat_binary("/", pat_var("x"), pat_int(2)),
    [](const capture_map &caps) -> expr_ptr {
      return make_expr(binary_t{">>", clone_expr(caps.at("x")), make_expr(int_t{1})});
    }));

  // x * x → x ** 2
  rules.push_back(make_conditional_rule("square-to-pow",
    pat_binary("*", pat_var("x"), pat_var("x")),
    [](const capture_map &caps) -> expr_ptr {
      return make_expr(binary_t{"**", clone_expr(caps.at("x")), make_expr(int_t{2})});
    }));

  return rules;
}

std::vector<rewrite_rule> boolean_simplification_rules() {
  std::vector<rewrite_rule> rules;

  // !!x → x
  rules.push_back(make_conditional_rule("double-not",
    pat_unary("!", pat_unary("!", pat_var("x"))),
    [](const capture_map &caps) -> expr_ptr {
      return clone_expr(caps.at("x"));
    }));

  // x && true → x
  rules.push_back(make_conditional_rule("and-true",
    pat_binary("&&", pat_var("x"), pat_bool(true)),
    [](const capture_map &caps) -> expr_ptr {
      return clone_expr(caps.at("x"));
    }));

  rules.push_back(make_conditional_rule("true-and",
    pat_binary("&&", pat_bool(true), pat_var("x")),
    [](const capture_map &caps) -> expr_ptr {
      return clone_expr(caps.at("x"));
    }));

  // x && false → false
  rules.push_back(make_conditional_rule("and-false",
    pat_binary("&&", pat_var("x"), pat_bool(false)),
    [](const capture_map &caps) -> expr_ptr {
      return make_expr(bool_t{false});
    }));

  rules.push_back(make_conditional_rule("false-and",
    pat_binary("&&", pat_bool(false), pat_var("x")),
    [](const capture_map &caps) -> expr_ptr {
      return make_expr(bool_t{false});
    }));

  // x || true → true
  rules.push_back(make_conditional_rule("or-true",
    pat_binary("||", pat_var("x"), pat_bool(true)),
    [](const capture_map &caps) -> expr_ptr {
      return make_expr(bool_t{true});
    }));

  rules.push_back(make_conditional_rule("true-or",
    pat_binary("||", pat_bool(true), pat_var("x")),
    [](const capture_map &caps) -> expr_ptr {
      return make_expr(bool_t{true});
    }));

  // x || false → x
  rules.push_back(make_conditional_rule("or-false",
    pat_binary("||", pat_var("x"), pat_bool(false)),
    [](const capture_map &caps) -> expr_ptr {
      return clone_expr(caps.at("x"));
    }));

  rules.push_back(make_conditional_rule("false-or",
    pat_binary("||", pat_bool(false), pat_var("x")),
    [](const capture_map &caps) -> expr_ptr {
      return clone_expr(caps.at("x"));
    }));

  // if true then a else b → a
  rules.push_back(make_conditional_rule("if-true",
    pat_if(pat_bool(true), pat_var("a"), pat_var("b")),
    [](const capture_map &caps) -> expr_ptr {
      return clone_expr(caps.at("a"));
    }));

  // if false then a else b → b
  rules.push_back(make_conditional_rule("if-false",
    pat_if(pat_bool(false), pat_var("a"), pat_var("b")),
    [](const capture_map &caps) -> expr_ptr {
      return clone_expr(caps.at("b"));
    }));

  return rules;
}

std::vector<rewrite_rule> all_optimization_rules() {
  std::vector<rewrite_rule> rules;
  auto add = [&](auto &&v) {
    rules.insert(rules.end(),
                 std::make_move_iterator(v.begin()),
                 std::make_move_iterator(v.end()));
  };
  add(constant_folding_rules());
  add(algebraic_simplification_rules());
  add(strength_reduction_rules());
  add(boolean_simplification_rules());
  return rules;
}

}  // namespace on1x::rewrite
