#include "on1x/dataflow/dataflow.hpp"

#include <algorithm>
#include <cassert>
#include <deque>
#include <queue>
#include <sstream>
#include <stack>
#include <type_traits>
#include <unordered_set>

namespace on1x::dataflow {

// ---------------------------------------------------------------------------
//  CFG utilities
// ---------------------------------------------------------------------------

void cfg::add_edge(std::size_t from, std::size_t to) {
  blocks[from].successors.push_back(to);
  blocks[to].predecessors.push_back(from);
}

std::vector<std::size_t> cfg::reverse_postorder() const {
  auto po = postorder();
  std::reverse(po.begin(), po.end());
  return po;
}

std::vector<std::size_t> cfg::postorder() const {
  std::vector<std::size_t> result;
  std::vector<bool> visited(blocks.size(), false);

  std::function<void(std::size_t)> dfs = [&](std::size_t id) {
    visited[id] = true;
    for (auto succ : blocks[id].successors) {
      if (!visited[succ]) dfs(succ);
    }
    result.push_back(id);
  };

  dfs(entry_id);
  return result;
}

// ---------------------------------------------------------------------------
//  Helpers: collect variables used/defined by an expression
// ---------------------------------------------------------------------------

namespace {

void collect_uses(const expr_ptr &e, std::vector<std::string> &out) {
  if (!e) return;
  std::visit([&](auto &&val) {
    using T = std::decay_t<decltype(val)>;
    if constexpr (std::is_same_v<T, ident_t>) {
      out.push_back(val.value);
    } else if constexpr (std::is_same_v<T, unary_t>) {
      collect_uses(val.rhs, out);
    } else if constexpr (std::is_same_v<T, binary_t>) {
      collect_uses(val.lhs, out);
      collect_uses(val.rhs, out);
    } else if constexpr (std::is_same_v<T, call_t>) {
      collect_uses(val.callee, out);
      for (const auto &arg : val.args) collect_uses(arg, out);
    } else if constexpr (std::is_same_v<T, if_t>) {
      collect_uses(val.cond, out);
      collect_uses(val.then_branch, out);
      collect_uses(val.else_branch, out);
    } else if constexpr (std::is_same_v<T, let_t>) {
      collect_uses(val.value, out);
      collect_uses(val.body, out);
    } else if constexpr (std::is_same_v<T, block_t>) {
      for (const auto &stmt : val.statements) collect_uses(stmt, out);
      if (val.value) collect_uses(val.value, out);
    } else if constexpr (std::is_same_v<T, list_t>) {
      for (const auto &el : val.elements) collect_uses(el, out);
    } else if constexpr (std::is_same_v<T, tuple_t>) {
      for (const auto &el : val.elements) collect_uses(el, out);
    } else if constexpr (std::is_same_v<T, record_t>) {
      for (const auto &[_, ex] : val.fields) collect_uses(ex, out);
    } else if constexpr (std::is_same_v<T, lambda_t>) {
      collect_uses(val.body, out);
    }
  }, e->node);
}

void collect_defs(const expr_ptr &e, std::vector<std::string> &out) {
  if (!e) return;
  std::visit([&](auto &&val) {
    using T = std::decay_t<decltype(val)>;
    if constexpr (std::is_same_v<T, let_t>) {
      out.push_back(val.name);
    } else if constexpr (std::is_same_v<T, block_t>) {
      for (const auto &stmt : val.statements) collect_defs(stmt, out);
      if (val.value) collect_defs(val.value, out);
    }
  }, e->node);
}

// Check if an expression is a conditional branch
bool is_conditional(const expr_ptr &e) {
  if (!e) return false;
  return std::holds_alternative<if_t>(e->node);
}

// Flatten a block into individual instructions
void flatten_block(const expr_ptr &e, std::vector<expr_ptr> &flat) {
  if (!e) return;
  if (auto b = std::get_if<block_t>(&e->node)) {
    for (const auto &s : b->statements) {
       if (std::get_if<block_t>(&s->node)) {
        flatten_block(s, flat);
      } else {
        flat.push_back(s);
      }
    }
    if (b->value) {
       if (std::get_if<block_t>(&b->value->node)) {
        flatten_block(b->value, flat);
      } else {
        flat.push_back(b->value);
      }
    }
  } else {
    flat.push_back(e);
  }
}

}  // namespace

// ---------------------------------------------------------------------------
//  CFG construction from AST
// ---------------------------------------------------------------------------

cfg build_cfg(const program &prog) {
  cfg graph;
  std::vector<expr_ptr> flat;
  for (const auto &form : prog.forms) {
    flatten_block(form, flat);
  }

  if (flat.empty()) {
    auto &entry = graph.blocks.emplace_back();
    entry.id = 0;
    entry.is_entry = true;
    entry.is_exit = true;
    graph.entry_id = 0;
    graph.exit_id = 0;
    return graph;
  }

  std::size_t current_block = 0;
  auto &first = graph.blocks.emplace_back();
  first.id = 0;
  first.is_entry = true;
  graph.entry_id = 0;

  for (std::size_t i = 0; i < flat.size(); ++i) {
    auto &bb = graph.blocks[current_block];
    bb.instructions.push_back(flat[i]);

    bool ends_block = false;
    if (is_conditional(flat[i])) {
      ends_block = true;
    }

    if (ends_block || i + 1 == flat.size()) {
      if (i + 1 < flat.size()) {
        auto &next = graph.blocks.emplace_back();
        next.id = graph.blocks.size() - 1;
        graph.add_edge(current_block, next.id);
        current_block = next.id;
      } else {
        bb.is_exit = true;
        graph.exit_id = current_block;
      }
    }
  }

  // Second pass: refine CFG for conditionals
  cfg refined;
  std::unordered_map<std::size_t, std::size_t> old_to_new;

  for (std::size_t i = 0; i < graph.blocks.size(); ++i) {
    const auto &bb = graph.blocks[i];
    if (bb.instructions.empty()) continue;

    if (bb.instructions.size() == 1 && is_conditional(bb.instructions[0])) {
      auto &cond = std::get<if_t>(bb.instructions[0]->node);

      auto &cond_bb = refined.blocks.emplace_back();
      cond_bb.id = refined.blocks.size() - 1;
      if (bb.is_entry) { cond_bb.is_entry = true; refined.entry_id = cond_bb.id; }
      cond_bb.instructions.push_back(cond.cond);

      auto &then_bb = refined.blocks.emplace_back();
      then_bb.id = refined.blocks.size() - 1;
      std::vector<expr_ptr> then_flat;
      flatten_block(cond.then_branch, then_flat);
      for (auto &ins : then_flat) then_bb.instructions.push_back(ins);

      auto &else_bb = refined.blocks.emplace_back();
      else_bb.id = refined.blocks.size() - 1;
      std::vector<expr_ptr> else_flat;
      flatten_block(cond.else_branch, else_flat);
      for (auto &ins : else_flat) else_bb.instructions.push_back(ins);

      auto &merge_bb = refined.blocks.emplace_back();
      merge_bb.id = refined.blocks.size() - 1;
      if (bb.is_exit) { merge_bb.is_exit = true; refined.exit_id = merge_bb.id; }

      refined.add_edge(cond_bb.id, then_bb.id);
      refined.add_edge(cond_bb.id, else_bb.id);
      refined.add_edge(then_bb.id, merge_bb.id);
      refined.add_edge(else_bb.id, merge_bb.id);

      old_to_new[i] = cond_bb.id;
    } else {
      auto &new_bb = refined.blocks.emplace_back();
      new_bb.id = refined.blocks.size() - 1;
      if (bb.is_entry) { new_bb.is_entry = true; refined.entry_id = new_bb.id; }
      if (bb.is_exit) { new_bb.is_exit = true; refined.exit_id = new_bb.id; }
      new_bb.instructions = bb.instructions;
      old_to_new[i] = new_bb.id;
    }
  }

  // Deduplicate successors
  for (std::size_t i = 0; i < refined.blocks.size(); ++i) {
    auto &bb = refined.blocks[i];
    std::sort(bb.successors.begin(), bb.successors.end());
    bb.successors.erase(std::unique(bb.successors.begin(), bb.successors.end()), bb.successors.end());
  }

  // Rebuild predecessors
  for (auto &bb : refined.blocks) bb.predecessors.clear();
  for (std::size_t i = 0; i < refined.blocks.size(); ++i) {
    for (auto succ : refined.blocks[i].successors) {
      refined.blocks[succ].predecessors.push_back(i);
    }
  }

  return refined;
}

cfg build_cfg(const sema::typed_program &prog) {
  program raw;
  for (const auto &form : prog.forms) {
    if (form && form->source) raw.forms.push_back(form->source);
  }
  return build_cfg(raw);
}

// ---------------------------------------------------------------------------
//  Generic dataflow solver
// ---------------------------------------------------------------------------

template <typename Lattice, direction Dir>
std::vector<Lattice> dataflow_solver<Lattice, Dir>::solve(const cfg &graph) const {
  const std::size_t n = graph.size();
  std::vector<Lattice> in(n, top());
  std::vector<Lattice> out(n, top());

  if constexpr (Dir == direction::forward) {
    in[graph.entry_id] = bot();
  } else {
    out[graph.exit_id] = bot();
  }

  std::deque<std::size_t> worklist;
  for (std::size_t i = 0; i < n; ++i) worklist.push_back(i);

  while (!worklist.empty()) {
    std::size_t id = worklist.front();
    worklist.pop_front();

    const auto &bb = graph.blocks[id];

    if constexpr (Dir == direction::forward) {
      Lattice new_in = bot();
      for (auto pred : bb.predecessors) {
        new_in = meet(new_in, out[pred]);
      }
      if (id == graph.entry_id) new_in = bot();

      if (!(new_in == in[id])) {
        in[id] = new_in;
        out[id] = transfer(bb, in[id]);
        for (auto succ : bb.successors) {
          if (std::find(worklist.begin(), worklist.end(), succ) == worklist.end())
            worklist.push_back(succ);
        }
      }
    } else {
      Lattice new_out = bot();
      for (auto succ : bb.successors) {
        new_out = meet(new_out, in[succ]);
      }
      if (id == graph.exit_id) new_out = bot();

      if (!(new_out == out[id])) {
        out[id] = new_out;
        in[id] = transfer(bb, out[id]);
        for (auto pred : bb.predecessors) {
          if (std::find(worklist.begin(), worklist.end(), pred) == worklist.end())
            worklist.push_back(pred);
        }
      }
    }
  }

  if constexpr (Dir == direction::forward) {
    return in;
  } else {
    return out;
  }
}

template struct dataflow_solver<liveness_set, direction::backward>;
template struct dataflow_solver<reaching_defs_set, direction::forward>;
template struct dataflow_solver<avail_exprs_set, direction::forward>;
template struct dataflow_solver<const_prop_map, direction::forward>;

// ---------------------------------------------------------------------------
//  Liveness analysis
// ---------------------------------------------------------------------------

bool liveness_result::is_live_at_entry(std::size_t block_id, const std::string &var) const {
  auto it = in.find(block_id);
  if (it == in.end()) return false;
  return it->second.variables.count(var) > 0;
}

bool liveness_result::is_live_at_exit(std::size_t block_id, const std::string &var) const {
  auto it = out.find(block_id);
  if (it == out.end()) return false;
  return it->second.variables.count(var) > 0;
}

liveness_result compute_liveness(const cfg &graph) {
  dataflow_solver<liveness_set, direction::backward> solver;

  solver.top = []() -> liveness_set { return {}; };
  solver.bot = []() -> liveness_set { return {}; };

  solver.meet = [](const liveness_set &a, const liveness_set &b) -> liveness_set {
    liveness_set result = a;
    result.variables.insert(b.variables.begin(), b.variables.end());
    return result;
  };

  solver.transfer = [](const basic_block &bb, const liveness_set &out_set) -> liveness_set {
    liveness_set in_set = out_set;
    for (auto it = bb.instructions.rbegin(); it != bb.instructions.rend(); ++it) {
      std::vector<std::string> uses, defs;
      collect_uses(*it, uses);
      collect_defs(*it, defs);
      for (const auto &d : defs) in_set.variables.erase(d);
      for (const auto &u : uses) in_set.variables.insert(u);
    }
    return in_set;
  };

  auto outs = solver.solve(graph);

  liveness_result result;
  for (std::size_t i = 0; i < graph.size(); ++i) {
    result.out[i] = outs[i];
    liveness_set in_set = outs[i];
    const auto &bb = graph.blocks[i];
    for (auto it = bb.instructions.rbegin(); it != bb.instructions.rend(); ++it) {
      std::vector<std::string> uses, defs;
      collect_uses(*it, uses);
      collect_defs(*it, defs);
      for (const auto &d : defs) in_set.variables.erase(d);
      for (const auto &u : uses) in_set.variables.insert(u);
    }
    result.in[i] = in_set;
  }

  return result;
}

// ---------------------------------------------------------------------------
//  Reaching definitions
// ---------------------------------------------------------------------------

std::vector<definition> reaching_defs_result::reaching_at(std::size_t block_id,
                                                          const std::string &var) const {
  std::vector<definition> result;
  auto it = in.find(block_id);
  if (it == in.end()) return result;
  for (const auto &d : it->second.defs) {
    if (d.variable == var) result.push_back(d);
  }
  return result;
}

reaching_defs_result compute_reaching_defs(const cfg &graph) {
  std::vector<definition> all_defs;
  for (std::size_t i = 0; i < graph.size(); ++i) {
    const auto &bb = graph.blocks[i];
    for (std::size_t j = 0; j < bb.instructions.size(); ++j) {
      std::vector<std::string> defs;
      collect_defs(bb.instructions[j], defs);
      for (const auto &d : defs) {
        all_defs.push_back({d, i, j});
      }
    }
  }

  dataflow_solver<reaching_defs_set, direction::forward> solver;

  solver.top = [&all_defs]() -> reaching_defs_set {
    reaching_defs_set s;
    for (const auto &d : all_defs) s.defs.insert(d);
    return s;
  };

  solver.bot = []() -> reaching_defs_set { return {}; };

  solver.meet = [](const reaching_defs_set &a, const reaching_defs_set &b) -> reaching_defs_set {
    reaching_defs_set result = a;
    result.defs.insert(b.defs.begin(), b.defs.end());
    return result;
  };

  solver.transfer = [&](const basic_block &bb, const reaching_defs_set &in_set) -> reaching_defs_set {
    reaching_defs_set out_set = in_set;
    for (std::size_t j = 0; j < bb.instructions.size(); ++j) {
      std::vector<std::string> defs;
      collect_defs(bb.instructions[j], defs);
      for (const auto &d : defs) {
        std::set<definition> filtered;
        for (const auto &def : out_set.defs) {
          if (def.variable != d) filtered.insert(def);
        }
        out_set.defs = std::move(filtered);
        out_set.defs.insert({d, bb.id, j});
      }
    }
    return out_set;
  };

  auto ins = solver.solve(graph);

  reaching_defs_result result;
  for (std::size_t i = 0; i < graph.size(); ++i) {
    result.in[i] = ins[i];
    reaching_defs_set out_set = ins[i];
    const auto &bb = graph.blocks[i];
    for (std::size_t j = 0; j < bb.instructions.size(); ++j) {
      std::vector<std::string> defs;
      collect_defs(bb.instructions[j], defs);
      for (const auto &d : defs) {
        std::set<definition> filtered;
        for (const auto &def : out_set.defs) {
          if (def.variable != d) filtered.insert(def);
        }
        out_set.defs = std::move(filtered);
        out_set.defs.insert({d, bb.id, j});
      }
    }
    result.out[i] = out_set;
  }

  return result;
}

// ---------------------------------------------------------------------------
//  Available expressions
// ---------------------------------------------------------------------------

bool avail_exprs_result::is_available(std::size_t block_id, const expr_key &key) const {
  auto it = in.find(block_id);
  if (it == in.end()) return false;
  return it->second.exprs.count(key) > 0;
}

avail_exprs_result compute_available_exprs(const cfg &graph) {
  std::set<expr_key> all_exprs;
  for (std::size_t i = 0; i < graph.size(); ++i) {
    for (const auto &ins : graph.blocks[i].instructions) {
      if (auto bin = std::get_if<binary_t>(&ins->node)) {
        auto lhs = std::get_if<ident_t>(&bin->lhs->node);
        auto rhs = std::get_if<ident_t>(&bin->rhs->node);
        if (lhs && rhs) {
          all_exprs.insert({bin->op, lhs->value, rhs->value});
        }
      }
    }
  }

  dataflow_solver<avail_exprs_set, direction::forward> solver;

  solver.top = [&all_exprs]() -> avail_exprs_set {
    avail_exprs_set s;
    s.exprs = all_exprs;
    return s;
  };

  solver.bot = []() -> avail_exprs_set { return {}; };

  solver.meet = [](const avail_exprs_set &a, const avail_exprs_set &b) -> avail_exprs_set {
    avail_exprs_set result;
    for (const auto &e : a.exprs) {
      if (b.exprs.count(e)) result.exprs.insert(e);
    }
    return result;
  };

  solver.transfer = [](const basic_block &bb, const avail_exprs_set &in_set) -> avail_exprs_set {
    avail_exprs_set out_set = in_set;
    for (const auto &ins : bb.instructions) {
      std::vector<std::string> defs;
      collect_defs(ins, defs);
      for (const auto &d : defs) {
        std::set<expr_key> filtered;
        for (const auto &e : out_set.exprs) {
          if (e.lhs_name != d && e.rhs_name != d) filtered.insert(e);
        }
        out_set.exprs = std::move(filtered);
      }
      if (auto bin = std::get_if<binary_t>(&ins->node)) {
        auto lhs = std::get_if<ident_t>(&bin->lhs->node);
        auto rhs = std::get_if<ident_t>(&bin->rhs->node);
        if (lhs && rhs) {
          out_set.exprs.insert({bin->op, lhs->value, rhs->value});
        }
      }
    }
    return out_set;
  };

  auto ins = solver.solve(graph);

  avail_exprs_result result;
  for (std::size_t i = 0; i < graph.size(); ++i) {
    result.in[i] = ins[i];
    avail_exprs_set out_set = ins[i];
    const auto &bb = graph.blocks[i];
    for (const auto &ins : bb.instructions) {
      std::vector<std::string> defs;
      collect_defs(ins, defs);
      for (const auto &d : defs) {
        std::set<expr_key> filtered;
        for (const auto &e : out_set.exprs) {
          if (e.lhs_name != d && e.rhs_name != d) filtered.insert(e);
        }
        out_set.exprs = std::move(filtered);
      }
      if (auto bin = std::get_if<binary_t>(&ins->node)) {
        auto lhs = std::get_if<ident_t>(&bin->lhs->node);
        auto rhs = std::get_if<ident_t>(&bin->rhs->node);
        if (lhs && rhs) {
          out_set.exprs.insert({bin->op, lhs->value, rhs->value});
        }
      }
    }
    result.out[i] = out_set;
  }

  return result;
}

// ---------------------------------------------------------------------------
//  Constant propagation
// ---------------------------------------------------------------------------

std::optional<long long> const_prop_result::lookup(std::size_t block_id,
                                                    const std::string &var) const {
  auto it = in.find(block_id);
  if (it == in.end()) return std::nullopt;
  auto jt = it->second.find(var);
  if (jt == it->second.end()) return std::nullopt;
  if (jt->second.tag == const_value::kind::constant) return jt->second.ival;
  return std::nullopt;
}

const_prop_result compute_constant_propagation(const cfg &graph) {
  std::set<std::string> all_vars;
  for (std::size_t i = 0; i < graph.size(); ++i) {
    for (const auto &ins : graph.blocks[i].instructions) {
      std::vector<std::string> uses, defs;
      collect_uses(ins, uses);
      collect_defs(ins, defs);
      all_vars.insert(uses.begin(), uses.end());
      all_vars.insert(defs.begin(), defs.end());
    }
  }

  dataflow_solver<const_prop_map, direction::forward> solver;

  solver.top = [&all_vars]() -> const_prop_map {
    const_prop_map m;
    for (const auto &v : all_vars) m[v] = {const_value::kind::top, 0};
    return m;
  };

  solver.bot = []() -> const_prop_map { return {}; };

  solver.meet = [](const const_prop_map &a, const const_prop_map &b) -> const_prop_map {
    const_prop_map result;
    for (const auto &[var, val] : a) {
      auto jt = b.find(var);
      if (jt == b.end()) {
        result[var] = val;
      } else {
        if (val.tag == const_value::kind::top) {
          result[var] = jt->second;
        } else if (jt->second.tag == const_value::kind::top) {
          result[var] = val;
        } else if (val.tag == const_value::kind::constant &&
                   jt->second.tag == const_value::kind::constant &&
                   val.ival == jt->second.ival) {
          result[var] = val;
        } else {
          result[var] = {const_value::kind::bottom, 0};
        }
      }
    }
    for (const auto &[var, val] : b) {
      if (result.find(var) == result.end()) result[var] = val;
    }
    return result;
  };

  solver.transfer = [](const basic_block &bb, const const_prop_map &in_map) -> const_prop_map {
    const_prop_map out_map = in_map;
    for (const auto &ins : bb.instructions) {
      if (auto let = std::get_if<let_t>(&ins->node)) {
        if (auto int_val = std::get_if<int_t>(&let->value->node)) {
          out_map[let->name] = {const_value::kind::constant, int_val->value};
        } else if (auto ident = std::get_if<ident_t>(&let->value->node)) {
          auto it = out_map.find(ident->value);
          if (it != out_map.end() && it->second.tag == const_value::kind::constant) {
            out_map[let->name] = it->second;
          } else {
            out_map[let->name] = {const_value::kind::bottom, 0};
          }
        } else {
          out_map[let->name] = {const_value::kind::bottom, 0};
        }
      }
      if (auto bin = std::get_if<binary_t>(&ins->node)) {
        if (bin->op == "=") {
          if (auto lhs_id = std::get_if<ident_t>(&bin->lhs->node)) {
            if (auto int_val = std::get_if<int_t>(&bin->rhs->node)) {
              out_map[lhs_id->value] = {const_value::kind::constant, int_val->value};
            } else {
              out_map[lhs_id->value] = {const_value::kind::bottom, 0};
            }
          }
        }
      }
    }
    return out_map;
  };

  auto ins = solver.solve(graph);

  const_prop_result result;
  for (std::size_t i = 0; i < graph.size(); ++i) {
    result.in[i] = ins[i];
    const_prop_map out_map = ins[i];
    const auto &bb = graph.blocks[i];
    for (const auto &ins : bb.instructions) {
      if (auto let = std::get_if<let_t>(&ins->node)) {
        if (auto int_val = std::get_if<int_t>(&let->value->node)) {
          out_map[let->name] = {const_value::kind::constant, int_val->value};
        } else if (auto ident = std::get_if<ident_t>(&let->value->node)) {
          auto it = out_map.find(ident->value);
          if (it != out_map.end() && it->second.tag == const_value::kind::constant) {
            out_map[let->name] = it->second;
          } else {
            out_map[let->name] = {const_value::kind::bottom, 0};
          }
        } else {
          out_map[let->name] = {const_value::kind::bottom, 0};
        }
      }
      if (auto bin = std::get_if<binary_t>(&ins->node)) {
        if (bin->op == "=") {
          if (auto lhs_id = std::get_if<ident_t>(&bin->lhs->node)) {
            if (auto int_val = std::get_if<int_t>(&bin->rhs->node)) {
              out_map[lhs_id->value] = {const_value::kind::constant, int_val->value};
            } else {
              out_map[lhs_id->value] = {const_value::kind::bottom, 0};
            }
          }
        }
      }
    }
    result.out[i] = out_map;
  }

  return result;
}

// ---------------------------------------------------------------------------
//  Use-Def and Def-Use chains
// ---------------------------------------------------------------------------

void use_def_chains::add_use(std::size_t block, std::size_t instr, const definition &def) {
  ud[{block, instr}].push_back(def);
  du[def].push_back({block, instr});
}

std::vector<definition> use_def_chains::defs_for_use(std::size_t block, std::size_t instr) const {
  auto it = ud.find({block, instr});
  if (it != ud.end()) return it->second;
  return {};
}

std::vector<std::pair<std::size_t, std::size_t>> use_def_chains::uses_of_def(const definition &def) const {
  auto it = du.find(def);
  if (it != du.end()) return it->second;
  return {};
}

use_def_chains compute_use_def_chains(const cfg &graph) {
  auto rd = compute_reaching_defs(graph);
  use_def_chains chains;

  for (std::size_t i = 0; i < graph.size(); ++i) {
    const auto &bb = graph.blocks[i];
    for (std::size_t j = 0; j < bb.instructions.size(); ++j) {
      std::vector<std::string> uses;
      collect_uses(bb.instructions[j], uses);

      for (const auto &u : uses) {
        auto defs = rd.reaching_at(i, u);
        if (!defs.empty()) {
          std::sort(defs.begin(), defs.end(),
                    [](const definition &a, const definition &b) {
                      if (a.block_id != b.block_id) return a.block_id > b.block_id;
                      return a.instr_index > b.instr_index;
                    });
          chains.add_use(i, j, defs.front());
        }
      }
    }
  }

  return chains;
}

// ---------------------------------------------------------------------------
//  Dominator tree (Lengauer-Tarjan algorithm)
// ---------------------------------------------------------------------------

namespace {

struct lt_state {
  std::size_t n;
  std::vector<std::size_t> &dfn;
  std::vector<std::size_t> &semi;
  std::vector<std::size_t> &idom;
  std::vector<std::size_t> &parent;
  std::vector<std::size_t> &ancestor;
  std::vector<std::size_t> &label;
  std::vector<std::vector<std::size_t>> bucket;
  std::vector<std::size_t> vertex;
  std::size_t dfn_counter = 0;

  void dfs(const cfg &graph, std::size_t v) {
    dfn[v] = ++dfn_counter;
    vertex[dfn_counter] = v;
    semi[v] = dfn_counter;
    label[v] = v;

    for (auto w : graph.blocks[v].successors) {
      if (dfn[w] == 0) {
        parent[w] = v;
        dfs(graph, w);
      }
    }
  }

  std::size_t find(std::size_t v) {
    if (ancestor[v] == 0) return v;
    compress(v);
    return label[v];
  }

  void compress(std::size_t v) {
    if (ancestor[ancestor[v]] == 0) return;
    compress(ancestor[v]);
    if (semi[label[ancestor[v]]] < semi[label[v]]) {
      label[v] = label[ancestor[v]];
    }
    ancestor[v] = ancestor[ancestor[v]];
  }
};

}  // namespace

dominator_tree compute_dominators(const cfg &graph) {
  const std::size_t n = graph.size();
  if (n == 0) return {};

  dominator_tree result;
  result.idom.resize(n, 0);
  result.dfn.resize(n, 0);
  result.semi.resize(n, 0);
  result.parent.resize(n, 0);
  result.ancestor.resize(n, 0);
  result.label.resize(n, 0);

  lt_state state{
    n, result.dfn, result.semi, result.idom,
    result.parent, result.ancestor, result.label,
    std::vector<std::vector<std::size_t>>(n + 1),
    std::vector<std::size_t>(n + 1)
  };

  state.dfs(graph, graph.entry_id);

  for (std::size_t i = state.dfn_counter; i >= 2; --i) {
    std::size_t w = state.vertex[i];

    for (auto v : graph.blocks[w].predecessors) {
      if (state.dfn[v] == 0) continue;
      std::size_t u = state.find(v);
      if (state.semi[u] < state.semi[w]) {
        state.semi[w] = state.semi[u];
      }
    }

    state.bucket[state.vertex[state.semi[w]]].push_back(w);
    state.ancestor[w] = state.parent[w];

    std::size_t p = state.parent[w];
    for (auto v : state.bucket[p]) {
      std::size_t u = state.find(v);
      result.idom[v] = (state.semi[u] == state.semi[v]) ? p : u;
    }
    state.bucket[p].clear();
  }

  for (std::size_t i = 2; i <= state.dfn_counter; ++i) {
    std::size_t w = state.vertex[i];
    if (result.idom[w] != state.vertex[state.semi[w]]) {
      result.idom[w] = result.idom[result.idom[w]];
    }
  }

  result.idom[graph.entry_id] = 0;

  result.children.resize(n);
  for (std::size_t i = 0; i < n; ++i) {
    if (result.idom[i] != 0 || i == graph.entry_id) {
      if (result.idom[i] < n) {
        result.children[result.idom[i]].push_back(i);
      }
    }
  }

  return result;
}

bool dominator_tree::dominates(std::size_t a, std::size_t b) const {
  if (a >= idom.size() || b >= idom.size()) return false;
  while (b != 0 && b != a) {
    b = idom[b];
  }
  return b == a;
}

std::vector<std::size_t> dominator_tree::dominance_frontier(std::size_t block_id) const {
  (void)block_id;
  return {};
}

}  // namespace on1x::dataflow
