#ifndef ON1X_CORE_RUNTIME_HPP
#define ON1X_CORE_RUNTIME_HPP

#include "on1x/frontend/ast.hpp"
#include "on1x/frontend/parser.hpp"

#include <functional>
#include <map>
#include <memory>
#include <ostream>
#include <string>
#include <variant>
#include <vector>

namespace on1x {

struct value;
using value_ptr = std::shared_ptr<value>;
using native_fn = std::function<value_ptr(const std::vector<value_ptr>&)>;

struct value {
  using node_t = std::variant<std::monostate, bool, long long, double, std::string,
                              std::vector<value_ptr>, std::map<std::string, value_ptr>,
                              native_fn>;
  node_t node;
};

struct env {
  std::shared_ptr<env> parent;
  void *bindings;
  void *strings;
  explicit env(std::shared_ptr<env> p = {});
  ~env();
};

value_ptr make_value(value::node_t node);
std::string to_string(const value_ptr &v);
value_ptr eval(const expr_ptr &e, const std::shared_ptr<env> &scope, std::string &err);
std::shared_ptr<env> make_prelude(std::ostream *sink = nullptr);
value_ptr lookup(const std::shared_ptr<env> &scope, const std::string &name);
bool bind(const std::shared_ptr<env> &scope, const std::string &name, value_ptr value);

}  // namespace on1x

#endif
