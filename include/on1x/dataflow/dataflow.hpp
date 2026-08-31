#ifndef ON1X_DATAFLOW_DATAFLOW_HPP
#define ON1X_DATAFLOW_DATAFLOW_HPP

#include "on1x/core/ast.hpp"
#include "on1x/sema/sema.hpp"

#include <cstddef>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

namespace on1x::dataflow {

// ---------------------------------------------------------------------------
//  Control Flow Graph
// ---------------------------------------------------------------------------

struct basic_block {
  std::size_t id = 0;
  std::vector<expr_ptr> instructions;
  std::vector<std::size_t> successors;
  std::vector<std::size_t> predecessors;
  bool is_entry = false;
  bool is_exit = false;
};

struct cfg {
  std::vector<basic_block> blocks;
  std::size_t entry_id = 0;
  std::size_t exit_id = 0;

  const basic_block &entry() const { return blocks[entry_id]; }
  const basic_block &exit() const { return blocks[exit_id]; }
  basic_block &entry() { return blocks[entry_id]; }
  basic_block &exit() { return blocks[exit_id]; }

  std::size_t size() const { return blocks.size(); }

  void add_edge(std::size_t from, std::size_t to);
  std::vector<std::size_t> reverse_postorder() const;
  std::vector<std::size_t> postorder() const;
};

cfg build_cfg(const sema::typed_program &prog);
cfg build_cfg(const program &prog);

// ---------------------------------------------------------------------------
//  Generic dataflow framework
// ---------------------------------------------------------------------------

enum class direction { forward, backward };

template <typename Lattice, direction Dir = direction::forward>
struct dataflow_solver {
  using lattice_t = Lattice;

  std::function<Lattice()> top;
  std::function<Lattice()> bot;
  std::function<Lattice(const Lattice &, const Lattice &)> meet;
  std::function<Lattice(const basic_block &, const Lattice &)> transfer;

  std::vector<Lattice> solve(const cfg &graph) const;
};

// ---------------------------------------------------------------------------
//  Liveness analysis
// ---------------------------------------------------------------------------

struct liveness_set {
  std::set<std::string> variables;

  bool operator==(const liveness_set &o) const { return variables == o.variables; }
  bool operator!=(const liveness_set &o) const { return !(*this == o); }
};

struct liveness_result {
  std::unordered_map<std::size_t, liveness_set> in;
  std::unordered_map<std::size_t, liveness_set> out;

  bool is_live_at_entry(std::size_t block_id, const std::string &var) const;
  bool is_live_at_exit(std::size_t block_id, const std::string &var) const;
};

liveness_result compute_liveness(const cfg &graph);

// ---------------------------------------------------------------------------
//  Reaching definitions
// ---------------------------------------------------------------------------

struct definition {
  std::string variable;
  std::size_t block_id = 0;
  std::size_t instr_index = 0;

  bool operator==(const definition &o) const {
    return variable == o.variable && block_id == o.block_id && instr_index == o.instr_index;
  }
  bool operator!=(const definition &o) const { return !(*this == o); }
  bool operator<(const definition &o) const {
    if (variable != o.variable) return variable < o.variable;
    if (block_id != o.block_id) return block_id < o.block_id;
    return instr_index < o.instr_index;
  }
};

struct reaching_defs_set {
  std::set<definition> defs;

  bool operator==(const reaching_defs_set &o) const { return defs == o.defs; }
  bool operator!=(const reaching_defs_set &o) const { return !(*this == o); }
};

struct reaching_defs_result {
  std::unordered_map<std::size_t, reaching_defs_set> in;
  std::unordered_map<std::size_t, reaching_defs_set> out;

  std::vector<definition> reaching_at(std::size_t block_id, const std::string &var) const;
};

reaching_defs_result compute_reaching_defs(const cfg &graph);

// ---------------------------------------------------------------------------
//  Available expressions
// ---------------------------------------------------------------------------

struct expr_key {
  std::string op;
  std::string lhs_name;
  std::string rhs_name;

  bool operator==(const expr_key &o) const {
    return op == o.op && lhs_name == o.lhs_name && rhs_name == o.rhs_name;
  }
  bool operator<(const expr_key &o) const {
    if (op != o.op) return op < o.op;
    if (lhs_name != o.lhs_name) return lhs_name < o.lhs_name;
    return rhs_name < o.rhs_name;
  }
};

struct avail_exprs_set {
  std::set<expr_key> exprs;

  bool operator==(const avail_exprs_set &o) const { return exprs == o.exprs; }
  bool operator!=(const avail_exprs_set &o) const { return !(*this == o); }
};

struct avail_exprs_result {
  std::unordered_map<std::size_t, avail_exprs_set> in;
  std::unordered_map<std::size_t, avail_exprs_set> out;

  bool is_available(std::size_t block_id, const expr_key &key) const;
};

avail_exprs_result compute_available_exprs(const cfg &graph);

// ---------------------------------------------------------------------------
//  Constant propagation (simple, intraprocedural)
// ---------------------------------------------------------------------------

struct const_value {
  enum class kind { top, constant, bottom };
  kind tag = kind::top;
  long long ival = 0;

  bool operator==(const const_value &o) const {
    return tag == o.tag && (tag != kind::constant || ival == o.ival);
  }
  bool operator!=(const const_value &o) const { return !(*this == o); }
};

using const_prop_map = std::map<std::string, const_value>;

struct const_prop_result {
  std::unordered_map<std::size_t, const_prop_map> in;
  std::unordered_map<std::size_t, const_prop_map> out;

  std::optional<long long> lookup(std::size_t block_id, const std::string &var) const;
};

const_prop_result compute_constant_propagation(const cfg &graph);

// ---------------------------------------------------------------------------
//  Use-Def and Def-Use chains
// ---------------------------------------------------------------------------

struct use_def_chains {
  std::map<std::pair<std::size_t, std::size_t>, std::vector<definition>> ud;
  std::map<definition, std::vector<std::pair<std::size_t, std::size_t>>> du;

  void add_use(std::size_t block, std::size_t instr, const definition &def);
  std::vector<definition> defs_for_use(std::size_t block, std::size_t instr) const;
  std::vector<std::pair<std::size_t, std::size_t>> uses_of_def(const definition &def) const;
};

use_def_chains compute_use_def_chains(const cfg &graph);

// ---------------------------------------------------------------------------
//  Dominator tree
// ---------------------------------------------------------------------------

struct dominator_tree {
  std::vector<std::size_t> idom;
  std::vector<std::vector<std::size_t>> children;
  std::vector<std::size_t> dfn;
  std::vector<std::size_t> semi;
  std::vector<std::size_t> parent;
  std::vector<std::size_t> ancestor;
  std::vector<std::size_t> label;

  bool dominates(std::size_t a, std::size_t b) const;
  std::vector<std::size_t> dominance_frontier(std::size_t block_id) const;
};

dominator_tree compute_dominators(const cfg &graph);

extern template struct dataflow_solver<liveness_set, direction::backward>;
extern template struct dataflow_solver<reaching_defs_set, direction::forward>;
extern template struct dataflow_solver<avail_exprs_set, direction::forward>;
extern template struct dataflow_solver<const_prop_map, direction::forward>;

}  // namespace on1x::dataflow

#endif
