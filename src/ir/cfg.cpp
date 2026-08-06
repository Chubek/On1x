#include "ir/cfg.hpp"

namespace on1x::ir {

ControlFlowGraph build_cfg(const Function& function) {
    ControlFlowGraph graph;
    graph.successors.resize(function.blocks.size());
    graph.predecessors.resize(function.blocks.size());
    for (std::size_t index = 0; index < function.blocks.size(); ++index) {
        const BasicBlock& block = function.blocks[index];
        const Instruction* terminator = block.instructions.empty()
            ? nullptr
            : &block.instructions.back();
        auto add_edge = [&](BlockId target) {
            if (target >= function.blocks.size()) return;
            auto& successors = graph.successors[index];
            if (std::find(successors.begin(), successors.end(), target) == successors.end()) {
                successors.push_back(target);
            }
            auto& predecessors = graph.predecessors[target];
            if (std::find(predecessors.begin(), predecessors.end(), static_cast<BlockId>(index)) ==
                predecessors.end()) {
                predecessors.push_back(static_cast<BlockId>(index));
            }
        };
        if (terminator) {
            if (terminator->opcode == Opcode::BranchIfFalse) {
                add_edge(terminator->target);
                if (index + 1U < function.blocks.size()) {
                    add_edge(static_cast<BlockId>(index + 1U));
                }
            } else if (terminator->opcode == Opcode::Jump) {
                add_edge(terminator->target);
            } else if (terminator->opcode != Opcode::Return &&
                       index + 1U < function.blocks.size()) {
                add_edge(static_cast<BlockId>(index + 1U));
            }
        } else if (index + 1U < function.blocks.size()) {
            add_edge(static_cast<BlockId>(index + 1U));
        }
    }
    return graph;
}

}  // namespace on1x::ir
