#include "vm/interpreter.hpp"

#include "core/number.hpp"
#include "core/table.hpp"
#include "runtime/state.hpp"

namespace on1x::vm {

bool execute(On1x_State* state, const Chunk& chunk, Value& result, const char*& error) noexcept {
    Value stack[64]{};
    std::size_t top = 0;
    for (std::size_t program_counter = 0; program_counter < chunk.instruction_count(); ++program_counter) {
        const Instruction instruction = chunk.instruction(program_counter);
        switch (instruction.opcode) {
        case Opcode::Constant:
            if (top == 64) { error = "VM stack overflow"; return false; }
            stack[top++] = chunk.constant(instruction.operand);
            break;
        case Opcode::LoadGlobal: {
            Value value;
            if (!table_get(state->globals, chunk.constant(instruction.operand), value)) {
                error = "undefined global";
                return false;
            }
            if (top == 64) { error = "VM stack overflow"; return false; }
            stack[top++] = value;
            break;
        }
        case Opcode::Add:
        case Opcode::Subtract:
            if (top < 2) { error = "VM stack underflow"; return false; }
            try {
                stack[top - 2U] = numeric_apply(
                    &state->gc, instruction.opcode == Opcode::Add ? ArithmeticOp::Add : ArithmeticOp::Subtract,
                    stack[top - 2U], stack[top - 1U]);
                --top;
            } catch (...) {
                error = "invalid numeric operands";
                return false;
            }
            break;
        case Opcode::Pop:
            if (top == 0) { error = "VM stack underflow"; return false; }
            --top;
            break;
        case Opcode::Return:
            if (top == 0) { error = "VM stack underflow"; return false; }
            result = stack[top - 1U];
            return true;
        }
    }
    error = "VM chunk has no Return";
    return false;
}

}  // namespace on1x::vm
