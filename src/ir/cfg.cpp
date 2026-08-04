#include "ir/cfg.hpp"

namespace on1x::ir {

ControlFlowGraph build_cfg(const Function& function) {
    ControlFlowGraph graph;
    graph.successors.resize(function.blocks.size());
    graph.predecessors.resize(function.blocks.size());
    return graph;
}

}  // namespace on1x::ir
