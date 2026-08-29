#include "on1x/egraph/egraph.hpp"

#include <algorithm>
#include <charconv>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace on1x::egraph {
namespace {

term from_expr(expr const& value) {
  return std::visit(
      [](auto const& node) -> term {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, nil_t>) {
          return term::op("nil");
        } else if constexpr (std::is_same_v<T, bool_t>) {
          return term::lit(equinoxng::Literal{node.value});
        } else if constexpr (std::is_same_v<T, int_t>) {
          return term::lit(equinoxng::Literal{static_cast<std::int64_t>(node.value)});
        } else if constexpr (std::is_same_v<T, float_t>) {
          return term::lit(equinoxng::Literal{node.value});
        } else if constexpr (std::is_same_v<T, string_t>) {
          return term::lit(equinoxng::Literal{node.value});
        } else if constexpr (std::is_same_v<T, ident_t>) {
          return term::op("ident", {term::lit(equinoxng::Literal{node.value})});
        } else if constexpr (std::is_same_v<T, unary_t>) {
          return term::op(node.op, {from_expr(*node.rhs)});
        } else if constexpr (std::is_same_v<T, binary_t>) {
          return term::op(node.op, {from_expr(*node.lhs), from_expr(*node.rhs)});
        } else if constexpr (std::is_same_v<T, call_t>) {
          std::vector<term> args;
          args.reserve(node.args.size() + 1);
          args.push_back(from_expr(*node.callee));
          for (auto const& arg : node.args) args.push_back(from_expr(*arg));
          return term::op("call", std::move(args));
        } else if constexpr (std::is_same_v<T, if_t>) {
          return term::op("if", {from_expr(*node.cond), from_expr(*node.then_branch),
                                 from_expr(*node.else_branch)});
        } else if constexpr (std::is_same_v<T, let_t>) {
          return term::op("let", {term::lit(equinoxng::Literal{node.name}),
                                  from_expr(*node.value), from_expr(*node.body)});
        } else if constexpr (std::is_same_v<T, block_t>) {
          std::vector<term> args;
          args.reserve(node.statements.size() + 1);
          for (auto const& statement : node.statements) args.push_back(from_expr(*statement));
          args.push_back(from_expr(*node.value));
          return term::op("block", std::move(args));
        } else if constexpr (std::is_same_v<T, list_t>) {
          std::vector<term> args;
          args.reserve(node.elements.size());
          for (auto const& element : node.elements) args.push_back(from_expr(*element));
          return term::op("list", std::move(args));
        } else if constexpr (std::is_same_v<T, tuple_t>) {
          std::vector<term> args;
          args.reserve(node.elements.size());
          for (auto const& element : node.elements) args.push_back(from_expr(*element));
          return term::op("tuple", std::move(args));
        } else if constexpr (std::is_same_v<T, record_t>) {
          std::vector<term> args;
          args.reserve(node.fields.size() * 2);
          for (auto const& [name, field] : node.fields) {
            args.push_back(term::lit(equinoxng::Literal{name}));
            args.push_back(from_expr(*field));
          }
          return term::op("record", std::move(args));
        } else if constexpr (std::is_same_v<T, lambda_t>) {
          std::vector<term> args;
          args.reserve(node.params.size() + 1);
          for (auto const& param : node.params) {
            args.push_back(term::lit(equinoxng::Literal{param}));
          }
          args.push_back(from_expr(*node.body));
          return term::op("lambda", std::move(args));
        }
      },
      value.node);
}

expr_ptr to_expr(term const& value) {
  if (value.is_lit()) {
    return std::visit(
        [](auto const& literal) -> expr_ptr {
          using T = std::decay_t<decltype(literal)>;
          if constexpr (std::is_same_v<T, std::int64_t>) {
            return make_expr(int_t{static_cast<long long>(literal)});
          } else if constexpr (std::is_same_v<T, double>) {
            return make_expr(float_t{literal});
          } else if constexpr (std::is_same_v<T, bool>) {
            return make_expr(bool_t{literal});
          } else {
            return make_expr(string_t{literal});
          }
        },
        value.as_lit().value);
  }

  if (value.is_var()) {
    return make_expr(ident_t{value.as_var().name});
  }

  auto const& node = value.as_node();
  std::vector<expr_ptr> children;
  children.reserve(node.children.size());
  for (auto const& child : node.children) children.push_back(to_expr(child));

  if (node.op.name == "nil" && children.empty()) return make_expr(nil_t{});
  if (node.op.name == "ident" && children.size() == 1 &&
      std::holds_alternative<string_t>(children[0]->node)) {
    return make_expr(ident_t{std::get<string_t>(children[0]->node).value});
  }
  if (node.op.name == "call" && !children.empty()) {
    auto callee = children.front();
    children.erase(children.begin());
    return make_expr(call_t{std::move(callee), std::move(children)});
  }
  if (node.op.name == "if" && children.size() == 3) {
    return make_expr(if_t{children[0], children[1], children[2]});
  }
  if (node.op.name == "let" && children.size() == 3 && children[0]->node.index() == 4) {
    auto const& name = std::get<string_t>(children[0]->node).value;
    return make_expr(let_t{name, children[1], children[2]});
  }
  if (node.op.name == "list") return make_expr(list_t{std::move(children)});
  if (node.op.name == "tuple") return make_expr(tuple_t{std::move(children)});

  if (children.size() == 1) return make_expr(unary_t{node.op.name, children[0]});
  if (children.size() == 2) return make_expr(binary_t{node.op.name, children[0], children[1]});
  return make_expr(call_t{make_expr(ident_t{node.op.name}), std::move(children)});
}

class sexpr_parser {
 public:
  explicit sexpr_parser(std::string_view input) : input_(input) {}

  term parse() {
    skip_space();
    auto value = parse_term();
    skip_space();
    if (pos_ != input_.size()) throw std::invalid_argument("unexpected trailing input");
    return value;
  }

 private:
  term parse_term() {
    skip_space();
    if (pos_ == input_.size()) throw std::invalid_argument("unexpected end of term");
    if (input_[pos_] != '(') return parse_atom();
    ++pos_;
    auto op = parse_atom_name();
    std::vector<term> args;
    while (true) {
      skip_space();
      if (pos_ == input_.size()) throw std::invalid_argument("unterminated list");
      if (input_[pos_] == ')') {
        ++pos_;
        break;
      }
      args.push_back(parse_term());
    }
    return term::op(std::move(op), std::move(args));
  }

  term parse_atom() {
    auto token = parse_token();
    if (token.empty()) throw std::invalid_argument("empty atom");
    if (token[0] == '?') return term::var(token.substr(1));
    if (token == "true") return term::lit(equinoxng::Literal{true});
    if (token == "false") return term::lit(equinoxng::Literal{false});
    if (token.front() == '"' && token.back() == '"' && token.size() >= 2) {
      return term::lit(equinoxng::Literal{token.substr(1, token.size() - 2)});
    }
    std::int64_t integer = 0;
    auto [end, ec] = std::from_chars(token.data(), token.data() + token.size(), integer);
    if (ec == std::errc{} && end == token.data() + token.size()) {
      return term::lit(equinoxng::Literal{integer});
    }
    char* float_end = nullptr;
    auto floating = std::strtod(token.c_str(), &float_end);
    if (float_end == token.c_str() + token.size()) {
      return term::lit(equinoxng::Literal{floating});
    }
    return term::op(token);
  }

  std::string parse_atom_name() {
    skip_space();
    auto start = pos_;
    while (pos_ < input_.size() && input_[pos_] != ')' &&
           !std::isspace(static_cast<unsigned char>(input_[pos_]))) {
      ++pos_;
    }
    if (start == pos_) throw std::invalid_argument("missing operator");
    return std::string(input_.substr(start, pos_ - start));
  }

  std::string parse_token() {
    skip_space();
    if (pos_ == input_.size()) return {};
    if (input_[pos_] == '"') {
      auto start = pos_++;
      while (pos_ < input_.size()) {
        if (input_[pos_] == '\\') {
          pos_ += std::min<std::size_t>(2, input_.size() - pos_);
        } else if (input_[pos_++] == '"') {
          break;
        }
      }
      if (pos_ > input_.size() || input_[pos_ - 1] != '"') {
        throw std::invalid_argument("unterminated string");
      }
      return std::string(input_.substr(start, pos_ - start));
    }
    auto start = pos_;
    while (pos_ < input_.size() && input_[pos_] != ')' &&
           !std::isspace(static_cast<unsigned char>(input_[pos_]))) {
      ++pos_;
    }
    return std::string(input_.substr(start, pos_ - start));
  }

  void skip_space() {
    while (pos_ < input_.size() && std::isspace(static_cast<unsigned char>(input_[pos_]))) ++pos_;
  }

  std::string_view input_;
  std::size_t pos_ = 0;
};

}  // namespace

term to_term(expr const& value) { return from_expr(value); }
expr_ptr from_term(term const& value) { return to_expr(value); }
term parse_term(std::string_view source) { return sexpr_parser(source).parse(); }

graph::graph(expr const& value) { add(value); }
graph::graph(term const& value) { add(value); }

equinoxng::EClassId graph::add(expr const& value) { return add(to_term(value)); }

equinoxng::EClassId graph::add(term const& value) {
  root_ = graph_.add(value);
  has_root_ = true;
  return root_;
}

void graph::clear() {
  graph_ = equinoxng::EGraph{};
  root_ = {};
  has_root_ = false;
}

void graph::rebuild() { graph_.rebuild(); }

std::size_t graph::run(std::vector<rewrite_rule> const& rules, runner_config config) {
  if (!has_root_) return 0;
  graph_.run(rules, config);
  return graph_.stats().rewrites_applied;
}

runner_stats const& graph::stats() const noexcept { return graph_.stats(); }

extracted graph::extract(equinoxng::EClassId root) const {
  return const_cast<equinoxng::EGraph&>(graph_).extract(root);
}

extracted graph::extract() const {
  if (!has_root_) return {};
  return extract(root_);
}

std::string graph::dump() const { return graph_.dump(); }
bool graph::empty() const noexcept { return !has_root_; }
std::size_t graph::num_classes() const noexcept { return graph_.num_classes(); }
std::size_t graph::num_enodes() const noexcept { return graph_.num_enodes(); }
equinoxng::EGraph& graph::native() noexcept { return graph_; }
equinoxng::EGraph const& graph::native() const noexcept { return graph_; }

equinoxng::EClassId graph::root() const {
  if (!has_root_) throw std::logic_error("egraph has no root");
  return graph_.find_const(root_);
}

std::vector<rewrite_rule> arithmetic_rules() {
  return equinoxng::rules::arithmetic_ring_basic();
}

}  // namespace on1x::egraph
