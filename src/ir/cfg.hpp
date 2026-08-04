#pragma once

#include "ir/ir.hpp"

#include <vector>

namespace on1x::ir {

struct ControlFlowGraph {
    std::vector<std::vector<BlockId>> successors;
    std::vector<std::vector<BlockId>> predecessors;
};

[[nodiscard]] ControlFlowGraph build_cfg(const Function& function);

}  // namespace on1x::ir
