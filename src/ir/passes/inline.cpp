#include "ir/ir.hpp"

#include <algorithm>
#include <cstddef>
#include <vector>

namespace on1x::ir {

void inline_small_functions(Module& module) {
    auto eligible = [&](const Function& function) {
        if (function.blocks.size() != 1U ||
            !function.parameter_bindings.empty() ||
            !function.capture_bindings.empty() ||
            function.blocks.front().instructions.size() > 8U) {
            return false;
        }
        const auto& instructions = function.blocks.front().instructions;
        if (instructions.empty() || instructions.back().opcode != Opcode::Return ||
            instructions.back().operands.size() != 1U) {
            return false;
        }
        for (std::size_t index = 0; index + 1U < instructions.size(); ++index) {
            const Instruction& instruction = instructions[index];
            if (!instruction.has_result() || instruction.has_side_effects() ||
                instruction.opcode == Opcode::Call ||
                instruction.opcode == Opcode::MakeFunction ||
                instruction.opcode == Opcode::BranchIfFalse ||
                instruction.opcode == Opcode::Jump) {
                return false;
            }
        }
        return true;
    };

    for (Function& caller : module.functions) {
        for (BasicBlock& block : caller.blocks) {
            for (std::size_t index = 0; index < block.instructions.size(); ++index) {
                Instruction& call = block.instructions[index];
                if (call.opcode != Opcode::Call || call.operands.size() != 1U) continue;
                const Register callee_register = call.operands[0];
                auto definition = std::find_if(
                    block.instructions.begin(),
                    block.instructions.begin() + static_cast<std::ptrdiff_t>(index),
                    [&](const Instruction& instruction) {
                        return instruction.opcode == Opcode::MakeFunction &&
                            instruction.result == callee_register;
                    });
                if (definition == block.instructions.begin() + static_cast<std::ptrdiff_t>(index)) {
                    continue;
                }
                if (definition->target == 0U || definition->target > module.functions.size()) continue;
                const Function& callee = module.function(definition->target);
                if (!eligible(callee)) continue;

                const auto& body = callee.blocks.front().instructions;
                std::vector<Register> remap(callee.register_count, no_register);
                std::vector<Instruction> replacement;
                replacement.reserve(body.size() - 1U);
                for (std::size_t body_index = 0; body_index + 1U < body.size(); ++body_index) {
                    Instruction instruction = body[body_index];
                    for (Register& operand : instruction.operands) {
                        if (operand >= remap.size() || remap[operand] == no_register) {
                            replacement.clear();
                            break;
                        }
                        operand = remap[operand];
                    }
                    if (replacement.empty() && body_index != 0U) break;
                    const Register result = body_index + 1U == body.size() - 1U
                        ? call.result
                        : caller.register_count++;
                    if (instruction.result >= remap.size()) {
                        replacement.clear();
                        break;
                    }
                    remap[instruction.result] = result;
                    instruction.result = result;
                    replacement.push_back(std::move(instruction));
                }
                if (replacement.size() != body.size() - 1U) continue;
                const Register returned = body.back().operands[0];
                if (returned >= remap.size() || remap[returned] == no_register) continue;
                block.instructions.erase(
                    block.instructions.begin() + static_cast<std::ptrdiff_t>(index));
                block.instructions.insert(
                    block.instructions.begin() + static_cast<std::ptrdiff_t>(index),
                    replacement.begin(),
                    replacement.end());
                index += replacement.size() - 1U;
            }
        }
    }
}

}  // namespace on1x::ir
