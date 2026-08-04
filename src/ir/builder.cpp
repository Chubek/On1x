#include "ir/builder.hpp"

namespace on1x::ir {

Register Builder::emit(
    Opcode opcode,
    syntax::SourcePosition position,
    std::string_view text,
    std::initializer_list<Register> operands) {
    Instruction instruction;
    instruction.opcode = opcode;
    instruction.result = function_.register_count++;
    instruction.text = text;
    instruction.position = position;
    for (Register operand : operands) instruction.operands.push_back(operand);
    function_.blocks[block_].instructions.push_back(std::move(instruction));
    return function_.register_count - 1U;
}

void Builder::emit_void(
    Opcode opcode,
    syntax::SourcePosition position,
    std::string_view text,
    std::initializer_list<Register> operands) {
    Instruction instruction;
    instruction.opcode = opcode;
    instruction.text = text;
    instruction.position = position;
    for (Register operand : operands) instruction.operands.push_back(operand);
    function_.blocks[block_].instructions.push_back(std::move(instruction));
}

}  // namespace on1x::ir
