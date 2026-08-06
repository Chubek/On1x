#include "ir/ir.hpp"

#include <cstddef>
#include <vector>

namespace on1x::ir {

void propagate_copies(Function& function) {
    std::vector<Register> replacements(function.register_count);
    for (Register index = 0; index < replacements.size(); ++index) replacements[index] = index;
    auto resolve = [&](Register value) {
        Register current = value;
        while (current < replacements.size() && replacements[current] != current) {
            current = replacements[current];
        }
        return current;
    };
    for (BasicBlock& block : function.blocks) {
        for (Instruction& instruction : block.instructions) {
            for (Register& operand : instruction.operands) operand = resolve(operand);
            if (instruction.opcode == Opcode::Phi &&
                instruction.operands.size() == 1U &&
                instruction.result < replacements.size()) {
                replacements[instruction.result] = resolve(instruction.operands[0]);
            }
        }
    }
}

}  // namespace on1x::ir
