#include "ir/ir.hpp"

#include <algorithm>
#include <cstddef>
#include <vector>

namespace on1x::ir {

namespace {

bool removable(const Instruction& instruction) {
    return instruction.has_result() && !instruction.has_side_effects() &&
        instruction.opcode != Opcode::Phi;
}

}  // namespace

void eliminate_dead_code(Function& function) {
    std::vector<bool> used(function.register_count, false);
    for (const BasicBlock& block : function.blocks) {
        for (const Instruction& instruction : block.instructions) {
            for (Register operand : instruction.operands) {
                if (operand < used.size()) used[operand] = true;
            }
        }
    }
    for (BasicBlock& block : function.blocks) {
        block.instructions.erase(
            std::remove_if(
                block.instructions.begin(),
                block.instructions.end(),
                [&](const Instruction& instruction) {
                    return removable(instruction) &&
                        instruction.result < used.size() &&
                        !used[instruction.result];
                }),
            block.instructions.end());
    }
}

}  // namespace on1x::ir
