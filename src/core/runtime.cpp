#include "on1x/core/runtime.hpp"

#include <sstream>
#include <stdexcept>
#include <type_traits>

namespace on1x {

value_ptr make_value(value::node_t node) {
  auto out = std::make_shared<value>();
  out->node = std::move(node);
  return out;
}

static value_ptr lookup(const std::shared_ptr<env> &scope, const std::string &name) {
  for (auto cur = scope; cur; cur = cur->parent) {
    auto it = cur->bindings.find(name);
    if (it != cur->bindings.end()) return it->second;
  }
  return {};
}

static std::string join_values(const std::vector<value_ptr> &values, const char *open, const char *close) {
  std::ostringstream out;
  out << open;
  for (size_t i = 0; i < values.size(); ++i) {
    if (i) out << ", ";
    out << to_string(values[i]);
  }
  out << close;
  return out.str();
}

std::string to_string(const value_ptr &v) {
  if (!v) return "None";
  return std::visit([](const auto &node) -> std::string {
    using T = std::decay_t<decltype(node)>;
    if constexpr (std::is_same_v<T, std::monostate>) return "()";
    if constexpr (std::is_same_v<T, bool>) return node ? "true" : "false";
    if constexpr (std::is_same_v<T, long long>) return std::to_string(node);
    if constexpr (std::is_same_v<T, double>) {
      std::ostringstream out;
      out << node;
      return out.str();
    }
    if constexpr (std::is_same_v<T, std::string>) return node;
    if constexpr (std::is_same_v<T, std::vector<value_ptr>>) return join_values(node, "[", "]");
    if constexpr (std::is_same_v<T, std::map<std::string, value_ptr>>) {
      std::ostringstream out;
      out << "{";
      bool first = true;
      for (const auto &kv : node) {
        if (!first) out << ", ";
        first = false;
        out << kv.first << ": " << to_string(kv.second);
      }
      out << "}";
      return out.str();
    }
    return "<fn>";
  }, v->node);
}

static value_ptr truthy(bool v) { return make_value(v); }

static bool as_bool(const value_ptr &v) {
  if (!v) return false;
  if (auto p = std::get_if<bool>(&v->node)) return *p;
  if (auto p = std::get_if<long long>(&v->node)) return *p != 0;
  if (auto p = std::get_if<double>(&v->node)) return *p != 0.0;
  if (auto p = std::get_if<std::string>(&v->node)) return !p->empty();
  if (auto p = std::get_if<std::vector<value_ptr>>(&v->node)) return !p->empty();
  return true;
}

static value_ptr eval_block(const block_t &b, const std::shared_ptr<env> &scope, std::string &err) {
  auto inner = std::make_shared<env>(scope);
  value_ptr last = make_value(std::monostate{});
  for (const auto &stmt : b.statements) {
    last = eval(stmt, inner, err);
    if (!err.empty()) return {};
  }
  if (b.value) {
    last = eval(b.value, inner, err);
  }
  return last;
}

static value_ptr apply_binary(const std::string &op, const value_ptr &lhs, const value_ptr &rhs, std::string &err) {
  if (!lhs || !rhs) return {};
  if (op == "++") return make_value(to_string(lhs) + to_string(rhs));
  if (op == "&&") return truthy(as_bool(lhs) && as_bool(rhs));
  if (op == "||") return truthy(as_bool(lhs) || as_bool(rhs));
  if (op == "==" || op == "!=") return truthy(to_string(lhs) == to_string(rhs) ? op == "==" : op == "!=");
  auto li = std::get_if<long long>(&lhs->node);
  auto ri = std::get_if<long long>(&rhs->node);
  auto ld = std::get_if<double>(&lhs->node);
  auto rd = std::get_if<double>(&rhs->node);
  if (li && ri) {
    if (op == "+") return make_value(*li + *ri);
    if (op == "-") return make_value(*li - *ri);
    if (op == "*") return make_value(*li * *ri);
    if (op == "/") return make_value(*ri == 0 ? 0 : *li / *ri);
    if (op == "%") return make_value(*ri == 0 ? 0 : *li % *ri);
    if (op == "<") return truthy(*li < *ri);
    if (op == "<=") return truthy(*li <= *ri);
    if (op == ">") return truthy(*li > *ri);
    if (op == ">=") return truthy(*li >= *ri);
  }
  if ((li || ld) && (ri || rd)) {
    double a = li ? static_cast<double>(*li) : *ld;
    double b = ri ? static_cast<double>(*ri) : *rd;
    if (op == "+") return make_value(a + b);
    if (op == "-") return make_value(a - b);
    if (op == "*") return make_value(a * b);
    if (op == "/") return make_value(b == 0.0 ? 0.0 : a / b);
    if (op == "<") return truthy(a < b);
    if (op == "<=") return truthy(a <= b);
    if (op == ">") return truthy(a > b);
    if (op == ">=") return truthy(a >= b);
  }
  err = "unsupported binary operator: " + op;
  return {};
}

value_ptr eval(const expr_ptr &e, const std::shared_ptr<env> &scope, std::string &err) {
  if (!e) return make_value(std::monostate{});
  return std::visit([&](const auto &node) -> value_ptr {
    using T = std::decay_t<decltype(node)>;
    if constexpr (std::is_same_v<T, nil_t>) return make_value(std::monostate{});
    if constexpr (std::is_same_v<T, bool_t>) return make_value(node.value);
    if constexpr (std::is_same_v<T, int_t>) return make_value(node.value);
    if constexpr (std::is_same_v<T, float_t>) return make_value(node.value);
    if constexpr (std::is_same_v<T, string_t>) return make_value(node.value);
    if constexpr (std::is_same_v<T, ident_t>) {
      auto found = lookup(scope, node.value);
      if (!found) err = "unknown name: " + node.value;
      return found;
    }
    if constexpr (std::is_same_v<T, unary_t>) {
      auto rhs = eval(node.rhs, scope, err);
      if (!err.empty()) return {};
      if (node.op == "-") {
        if (auto i = std::get_if<long long>(&rhs->node)) return make_value(-*i);
        if (auto d = std::get_if<double>(&rhs->node)) return make_value(-*d);
      }
      if (node.op == "!") return truthy(!as_bool(rhs));
      err = "unsupported unary operator: " + node.op;
      return {};
    }
    if constexpr (std::is_same_v<T, binary_t>) {
      if (node.op == "=") {
        auto rhs = eval(node.rhs, scope, err);
        if (!err.empty()) return {};
        if (auto id = std::get_if<ident_t>(&node.lhs->node)) {
          const_cast<std::shared_ptr<env>&>(scope)->bindings[id->value] = rhs;
          return rhs;
        }
        err = "left side of assignment must be an identifier";
        return {};
      }
      if (node.op == "|>") {
        auto lhs = eval(node.lhs, scope, err);
        if (!err.empty()) return {};
        auto callee = eval(node.rhs, scope, err);
        if (!err.empty()) return {};
        auto fn = std::get_if<native_fn>(&callee->node);
        if (!fn) {
          err = "pipeline target is not callable";
          return {};
        }
        return (*fn)({lhs});
      }
      auto lhs = eval(node.lhs, scope, err);
      if (!err.empty()) return {};
      auto rhs = eval(node.rhs, scope, err);
      if (!err.empty()) return {};
      return apply_binary(node.op, lhs, rhs, err);
    }
    if constexpr (std::is_same_v<T, call_t>) {
      auto callee = eval(node.callee, scope, err);
      if (!err.empty()) return {};
      auto fn = std::get_if<native_fn>(&callee->node);
      if (!fn) {
        err = "value is not callable";
        return {};
      }
      std::vector<value_ptr> args;
      for (const auto &arg : node.args) {
        args.push_back(eval(arg, scope, err));
        if (!err.empty()) return {};
      }
      return (*fn)(args);
    }
    if constexpr (std::is_same_v<T, if_t>) {
      auto cond = eval(node.cond, scope, err);
      if (!err.empty()) return {};
      return as_bool(cond) ? eval(node.then_branch, scope, err) : eval(node.else_branch, scope, err);
    }
    if constexpr (std::is_same_v<T, let_t>) {
      auto val = eval(node.value, scope, err);
      if (!err.empty()) return {};
      auto inner = std::make_shared<env>(scope);
      inner->bindings[node.name] = val;
      return eval(node.body, inner, err);
    }
    if constexpr (std::is_same_v<T, block_t>) return eval_block(node, scope, err);
    if constexpr (std::is_same_v<T, list_t>) {
      std::vector<value_ptr> values;
      for (const auto &el : node.elements) {
        values.push_back(eval(el, scope, err));
        if (!err.empty()) return {};
      }
      return make_value(values);
    }
    if constexpr (std::is_same_v<T, tuple_t>) {
      std::vector<value_ptr> values;
      for (const auto &el : node.elements) {
        values.push_back(eval(el, scope, err));
        if (!err.empty()) return {};
      }
      return make_value(values);
    }
    if constexpr (std::is_same_v<T, record_t>) {
      std::map<std::string, value_ptr> fields;
      for (const auto &[key, ex] : node.fields) {
        fields[key] = eval(ex, scope, err);
        if (!err.empty()) return {};
      }
      return make_value(fields);
    }
    if constexpr (std::is_same_v<T, lambda_t>) {
      auto closure = scope;
      auto params = node.params;
      auto body = node.body;
      return make_value(native_fn([closure, params, body](const std::vector<value_ptr> &args) -> value_ptr {
        auto local = std::make_shared<env>(closure);
        for (size_t i = 0; i < params.size(); ++i) local->bindings[params[i]] = i < args.size() ? args[i] : make_value(std::monostate{});
        std::string err;
        auto out = eval(body, local, err);
        return err.empty() ? out : make_value(std::string(err));
      }));
    }
    return {};
  }, e->node);
}

std::shared_ptr<env> make_prelude(std::ostream *sink) {
  auto scope = std::make_shared<env>();
  scope->bindings["print"] = make_value(native_fn([sink](const std::vector<value_ptr> &args) -> value_ptr {
    if (sink && !args.empty()) (*sink) << to_string(args[0]);
    return make_value(std::monostate{});
  }));
  scope->bindings["println"] = make_value(native_fn([sink](const std::vector<value_ptr> &args) -> value_ptr {
    if (sink) {
      if (!args.empty()) (*sink) << to_string(args[0]);
      (*sink) << '\n';
    }
    return make_value(std::monostate{});
  }));
  scope->bindings["show"] = make_value(native_fn([](const std::vector<value_ptr> &args) -> value_ptr {
    return make_value(args.empty() ? std::string() : to_string(args[0]));
  }));
  scope->bindings["panic"] = make_value(native_fn([](const std::vector<value_ptr> &args) -> value_ptr {
    throw std::runtime_error(args.empty() ? "panic" : to_string(args[0]));
  }));
  scope->bindings["assert"] = make_value(native_fn([](const std::vector<value_ptr> &args) -> value_ptr {
    if (args.size() >= 2 && !as_bool(args[0])) throw std::runtime_error(to_string(args[1]));
    return make_value(std::monostate{});
  }));
  return scope;
}

}  // namespace on1x
