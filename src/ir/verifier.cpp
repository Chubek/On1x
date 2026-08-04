#include "ir/verifier.hpp"

namespace on1x::ir {

bool verify(const Module& module, syntax::Diagnostics& diagnostics) {
    const auto verify_function = [&diagnostics](const Function& function) {
    if (function.blocks.empty()) {
        diagnostics.add({}, "IR function has no entry block");
        return false;
    }
    bool valid = true;
    std::vector<bool> defined(function.register_count, false);
    for (const BasicBlock& block : function.blocks) {
        for (const Instruction& instruction : block.instructions) {
            for (Register operand : instruction.operands) {
                if (operand >= defined.size() || !defined[operand]) {
                    diagnostics.add(instruction.position, "IR instruction uses an undefined register");
                    valid = false;
                }
            }
            if (instruction.has_result()) {
                if (instruction.result >= defined.size() || defined[instruction.result]) {
                    diagnostics.add(instruction.position, "IR instruction defines an invalid register");
                    valid = false;
                } else {
                    defined[instruction.result] = true;
                }
            }
        }
        if (block.instructions.empty() ||
            (block.instructions.back().opcode != Opcode::Return &&
             block.instructions.back().opcode != Opcode::Jump &&
             block.instructions.back().opcode != Opcode::BranchIfFalse)) {
            diagnostics.add({}, "IR block is missing a terminator");
            valid = false;
        }
    }
    return valid;
    };
    bool valid = verify_function(module.entry);
    for (const Function& function : module.functions) {
        valid = verify_function(function) && valid;
    }
    return valid;
}

}  // namespace on1x::ir
