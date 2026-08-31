#include "on1x/core/runtime.hpp"

#include <cmath>
#include <cstdlib>
#include <cstring>
#include <new>
#include <random>
#include <stdexcept>
#include <type_traits>

#include "kmempool.h"
#include "kbarena.h"
#include "khash.h"
#include "krng.h"
#include "kstring.h"

KHASH_MAP_INIT_STR(on1x_binding, on1x::value *)

namespace on1x {
namespace {

static void *value_pool() {
  static void *pool = kmp_init(sizeof(value));
  return pool;
}

static char *copy_key(env *scope, const std::string &name) {
  if (!scope || !scope->strings) return nullptr;
  auto *dst = static_cast<char *>(kba_alloc(scope->strings, static_cast<unsigned>(name.size() + 1), 1));
  if (!dst) return nullptr;
  std::memcpy(dst, name.c_str(), name.size() + 1);
  return dst;
}

static auto *binding_table(void *bindings) {
  return static_cast<khash_t(on1x_binding) *>(bindings);
}

static krng_t &rng() {
  static krng_t state;
  static bool seeded = false;
  if (!seeded) {
    std::random_device rd;
    kr_srand_r(&state, (static_cast<uint64_t>(rd()) << 32) ^ rd());
    seeded = true;
  }
  return state;
}

static value_ptr make_pooled_value() {
  void *mem = kmp_alloc(value_pool());
  if (!mem) return {};
  auto *raw = new (mem) value();
  return value_ptr(raw, [](value *) {});
}

static value_ptr wrap_value(value *raw) {
  return value_ptr(raw, [](value *) {});
}

static std::string join_values(const std::vector<value_ptr> &values, const char *open, const char *close) {
  kstring_t out{0, 0, nullptr};
  kputs(open, &out);
  for (size_t i = 0; i < values.size(); ++i) {
    if (i) kputs(", ", &out);
    const std::string item = to_string(values[i]);
    kputsn(item.data(), static_cast<int>(item.size()), &out);
  }
  kputs(close, &out);
  std::string result = out.s ? out.s : "";
  free(out.s);
  return result;
}

static value_ptr truthy(bool v) {
  return make_value(v);
}

static bool as_bool(const value_ptr &v) {
  if (!v) return false;
  if (auto p = std::get_if<bool>(&v->node)) return *p;
  if (auto p = std::get_if<long long>(&v->node)) return *p != 0;
  if (auto p = std::get_if<double>(&v->node)) return *p != 0.0;
  if (auto p = std::get_if<std::string>(&v->node)) return !p->empty();
  if (auto p = std::get_if<std::vector<value_ptr>>(&v->node)) return !p->empty();
  return true;
}

static value_ptr apply_binary(const std::string &op, const value_ptr &lhs, const value_ptr &rhs, std::string &err) {
  if (!lhs || !rhs) return {};
  if (op == "++") return make_value(to_string(lhs) + to_string(rhs));
  if (op == "&&") return truthy(as_bool(lhs) && as_bool(rhs));
  if (op == "||") return truthy(as_bool(lhs) || as_bool(rhs));
  if (op == "==" || op == "!=") return truthy((to_string(lhs) == to_string(rhs)) == (op == "=="));
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

static value_ptr random_builtin(const std::vector<value_ptr> &args) {
  if (args.empty()) return make_value(kr_drand_r(&rng()));
  auto upper = std::get_if<long long>(&args[0]->node);
  if (!upper) return make_value(std::monostate{});
  if (*upper <= 0) return make_value(static_cast<long long>(0));
  return make_value(static_cast<long long>(kr_drand_r(&rng()) * static_cast<double>(*upper)));
}

static value_ptr randint_builtin(const std::vector<value_ptr> &args) {
  long long lo = 0;
  long long hi = 0;
  if (args.size() == 1) {
    auto upper = std::get_if<long long>(&args[0]->node);
    if (!upper || *upper <= 0) return make_value(static_cast<long long>(0));
    hi = *upper;
  } else if (args.size() >= 2) {
    auto a = std::get_if<long long>(&args[0]->node);
    auto b = std::get_if<long long>(&args[1]->node);
    if (!a || !b || *b < *a) return make_value(static_cast<long long>(0));
    lo = *a;
    hi = *b + 1;
  } else {
    return make_value(static_cast<long long>(0));
  }
  const double r = kr_drand_r(&rng());
  return make_value(lo + static_cast<long long>(r * static_cast<double>(hi - lo)));
}

}  // namespace

env::env(std::shared_ptr<env> p)
    : parent(std::move(p)),
      bindings(kh_init(on1x_binding)),
      strings(kba_init(65536)) {}

env::~env() {
  if (bindings) {
    kh_destroy(on1x_binding, binding_table(bindings));
    bindings = nullptr;
  }
  if (strings) {
    kba_destroy(strings);
    strings = nullptr;
  }
}

value_ptr make_value(value::node_t node) {
  auto out = make_pooled_value();
  if (!out) return {};
  out->node = std::move(node);
  return out;
}

value_ptr lookup(const std::shared_ptr<env> &scope, const std::string &name) {
  for (auto cur = scope; cur; cur = cur->parent) {
    if (!cur->bindings) continue;
    auto *table = binding_table(cur->bindings);
    const khiter_t k = kh_get(on1x_binding, table, name.c_str());
    if (k != kh_end(table)) return wrap_value(kh_val(table, k));
  }
  return {};
}

bool bind(const std::shared_ptr<env> &scope, const std::string &name, value_ptr value) {
  if (!scope || !scope->bindings) return false;
  auto *table = binding_table(scope->bindings);
  int ret = 0;
  const khiter_t k = kh_put(on1x_binding, table, name.c_str(), &ret);
  if (ret < 0) return false;
  if (ret != 0) {
    char *key = copy_key(scope.get(), name);
    if (!key) {
      kh_del(on1x_binding, table, k);
      return false;
    }
    kh_key(table, k) = key;
  }
  kh_val(table, k) = value.get();
  return true;
}

std::string to_string(const value_ptr &v) {
  if (!v) return "None";
  return std::visit([](const auto &node) -> std::string {
    using T = std::decay_t<decltype(node)>;
    if constexpr (std::is_same_v<T, std::monostate>) return "()";
    if constexpr (std::is_same_v<T, bool>) return node ? "true" : "false";
    if constexpr (std::is_same_v<T, long long>) return std::to_string(node);
    if constexpr (std::is_same_v<T, double>) return std::to_string(node);
    if constexpr (std::is_same_v<T, std::string>) return node;
    if constexpr (std::is_same_v<T, std::vector<value_ptr>>) return join_values(node, "[", "]");
    if constexpr (std::is_same_v<T, std::map<std::string, value_ptr>>) {
      kstring_t out{0, 0, nullptr};
      kputc('{', &out);
      bool first = true;
      for (const auto &kv : node) {
        if (!first) kputs(", ", &out);
        first = false;
        kputsn(kv.first.data(), static_cast<int>(kv.first.size()), &out);
        kputs(": ", &out);
        const std::string value = to_string(kv.second);
        kputsn(value.data(), static_cast<int>(value.size()), &out);
      }
      kputc('}', &out);
      std::string result = out.s ? out.s : "";
      free(out.s);
      return result;
    }
    return "<fn>";
  }, v->node);
}

static value_ptr eval_block(const block_t &b, const std::shared_ptr<env> &scope, std::string &err) {
  auto inner = std::make_shared<env>(scope);
  value_ptr last = make_value(std::monostate{});
  for (const auto &stmt : b.statements) {
    last = eval(stmt, inner, err);
    if (!err.empty()) return {};
  }
  if (b.value) last = eval(b.value, inner, err);
  return last;
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
          if (!on1x::bind(scope, id->value, rhs)) {
            err = "failed to bind name: " + id->value;
            return {};
          }
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
      if (!on1x::bind(inner, node.name, val)) {
        err = "failed to bind name: " + node.name;
        return {};
      }
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
        for (size_t i = 0; i < params.size(); ++i) {
          auto arg = i < args.size() ? args[i] : make_value(std::monostate{});
          if (!on1x::bind(local, params[i], arg)) return make_value(std::string("failed to bind parameter"));
        }
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
  on1x::bind(scope, "print", make_value(native_fn([sink](const std::vector<value_ptr> &args) -> value_ptr {
    if (sink && !args.empty()) (*sink) << to_string(args[0]);
    return make_value(std::monostate{});
  })));
  on1x::bind(scope, "println", make_value(native_fn([sink](const std::vector<value_ptr> &args) -> value_ptr {
    if (sink) {
      if (!args.empty()) (*sink) << to_string(args[0]);
      (*sink) << '\n';
    }
    return make_value(std::monostate{});
  })));
  on1x::bind(scope, "show", make_value(native_fn([](const std::vector<value_ptr> &args) -> value_ptr {
    return make_value(args.empty() ? std::string() : to_string(args[0]));
  })));
  on1x::bind(scope, "panic", make_value(native_fn([](const std::vector<value_ptr> &args) -> value_ptr {
    throw std::runtime_error(args.empty() ? "panic" : to_string(args[0]));
  })));
  on1x::bind(scope, "assert", make_value(native_fn([](const std::vector<value_ptr> &args) -> value_ptr {
    if (args.size() >= 2 && !as_bool(args[0])) throw std::runtime_error(to_string(args[1]));
    return make_value(std::monostate{});
  })));
  on1x::bind(scope, "random", make_value(native_fn([](const std::vector<value_ptr> &args) -> value_ptr {
    return random_builtin(args);
  })));
  on1x::bind(scope, "randint", make_value(native_fn([](const std::vector<value_ptr> &args) -> value_ptr {
    return randint_builtin(args);
  })));
  return scope;
}

}  // namespace on1x
