#include "on1x/core/runtime.hpp"

#include <cstdint>
#include <random>
#include <stdexcept>

#include "krng.h"

namespace on1x {
namespace {

static bool as_bool(const value_ptr &v) {
  if (!v) return false;
  if (auto p = std::get_if<bool>(&v->node)) return *p;
  if (auto p = std::get_if<long long>(&v->node)) return *p != 0;
  if (auto p = std::get_if<double>(&v->node)) return *p != 0.0;
  if (auto p = std::get_if<std::string>(&v->node)) return !p->empty();
  if (auto p = std::get_if<std::vector<value_ptr>>(&v->node)) return !p->empty();
  return true;
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
