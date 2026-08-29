#ifndef ON1X_EGRAPH_EGRAPH_HPP
#define ON1X_EGRAPH_EGRAPH_HPP

#include "on1x/core/ast.hpp"

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

#include <EquinoxNG.hpp>

namespace on1x::egraph {

using term = equinoxng::Term;
using rewrite_rule = equinoxng::RewriteRule;
using runner_config = equinoxng::RunnerConfig;
using runner_stats = equinoxng::RunnerStats;
using extracted = equinoxng::Extracted;

term to_term(expr const& value);
expr_ptr from_term(term const& value);

term parse_term(std::string_view source);

class graph {
 public:
  graph() = default;

  explicit graph(expr const& value);
  explicit graph(term const& value);

  equinoxng::EClassId add(expr const& value);
  equinoxng::EClassId add(term const& value);

  void clear();
  void rebuild();

  std::size_t run(std::vector<rewrite_rule> const& rules,
                  runner_config config = {});
  runner_stats const& stats() const noexcept;

  extracted extract(equinoxng::EClassId root) const;
  extracted extract() const;

  std::string dump() const;
  bool empty() const noexcept;
  std::size_t num_classes() const noexcept;
  std::size_t num_enodes() const noexcept;

  equinoxng::EGraph& native() noexcept;
  equinoxng::EGraph const& native() const noexcept;
  equinoxng::EClassId root() const;

 private:
  equinoxng::EGraph graph_;
  equinoxng::EClassId root_{};
  bool has_root_ = false;
};

std::vector<rewrite_rule> arithmetic_rules();

}  // namespace on1x::egraph

#endif
