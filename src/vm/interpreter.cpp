#include "vm/interpreter.hpp"

#include "core/number.hpp"
#include "core/table.hpp"
#include "gc/alloc.hpp"
#include "gc/roots.hpp"
#include <on1x/on1x_call.h>
#include "runtime/state.hpp"

namespace on1x::vm {

namespace {

bool execute_function(
    On1x_State* state,
    const Chunk& chunk,
    FunctionObject* closure,
    const Value* arguments,
    std::size_t argument_count,
    Value& result,
    const char*& error) noexcept {
    if (!chunk.variadic() && argument_count != chunk.parameter_count()) {
        error = "function received the wrong number of arguments";
        return false;
    }
    if (chunk.variadic() && argument_count + 1U < chunk.parameter_count()) {
        error = "function received too few arguments";
        return false;
    }
    Value stack[64]{};
    GcRoot closure_root(closure);
    Value* locals = chunk.local_count() == 0
        ? nullptr
        : gc_alloc_array<Value>(&state->gc, chunk.local_count());
    for (std::size_t index = 0; index < chunk.parameter_count(); ++index) {
        const std::uint32_t binding = chunk.parameter_binding(index);
        if (binding >= chunk.local_count()) {
            error = "invalid function parameter";
            return false;
        }
        if (chunk.variadic() && index + 1U == chunk.parameter_count()) {
            auto* rest = new_list(&state->gc);
            for (std::size_t argument = index; argument < argument_count; ++argument) {
                if (!list_push(&state->gc, rest, arguments[argument])) {
                    error = "unable to allocate rest arguments";
                    return false;
                }
            }
            locals[binding] = value_from_object(rest);
        } else {
            locals[binding] = arguments[index];
        }
    }
    std::size_t top = 0;
    for (std::size_t program_counter = 0; program_counter < chunk.instruction_count();) {
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
        case Opcode::StoreGlobal:
            if (top == 0) { error = "VM stack underflow"; return false; }
            try {
                if (!table_set(
                        &state->gc,
                        state->globals,
                        chunk.constant(instruction.operand),
                        stack[top - 1U])) {
                    error = "unable to store global";
                    return false;
                }
            } catch (...) {
                error = "unable to store global";
                return false;
            }
            break;
        case Opcode::LoadLocal:
            if (instruction.operand >= chunk.local_count()) {
                error = "invalid local binding";
                return false;
            }
            if (top == 64) { error = "VM stack overflow"; return false; }
            stack[top++] = locals[instruction.operand];
            break;
        case Opcode::StoreLocal:
            if (instruction.operand >= chunk.local_count() || top == 0) {
                error = "invalid local binding";
                return false;
            }
            locals[instruction.operand] = stack[top - 1U];
            break;
        case Opcode::LoadUpvalue:
            if (!closure || instruction.operand >= closure->capture_count) {
                error = "invalid closure capture";
                return false;
            }
            if (top == 64) { error = "VM stack overflow"; return false; }
            stack[top++] = closure->captures[instruction.operand];
            break;
        case Opcode::MakeClosure: {
            Chunk* target = chunk.function(instruction.operand);
            if (!target || top < target->capture_count()) {
                error = "invalid closure target";
                return false;
            }
            auto* function = gc_alloc<FunctionObject>(&state->gc);
            GcRoot function_root(function);
            function->chunk = target;
            function->capture_count = target->capture_count();
            if (function->capture_count != 0) {
                function->captures = gc_alloc_array<Value>(&state->gc, function->capture_count);
                for (std::size_t index = 0; index < function->capture_count; ++index) {
                    function->captures[index] = stack[top - function->capture_count + index];
                }
            }
            top -= function->capture_count;
            stack[top++] = value_from_object(function);
            break;
        }
        case Opcode::Call: {
            const std::size_t argc = instruction.operand;
            if (top < argc + 1U) { error = "VM stack underflow"; return false; }
            const std::size_t function_index = top - argc - 1U;
            const Value function_value = stack[function_index];
            if (function_value.kind() != Value::Kind::Function) {
                error = "Call expects an Fn";
                return false;
            }
            auto* function = static_cast<FunctionObject*>(function_value.as_object());
            Value call_result;
            if (function->native) {
                const std::size_t state_top = state->top;
                if (!stack_push(state, function_value)) {
                    error = "unable to prepare native call";
                    return false;
                }
                for (std::size_t index = 0; index < argc; ++index) {
                    if (!stack_push(state, stack[function_index + index + 1U])) {
                        state->top = state_top;
                        error = "unable to prepare native call";
                        return false;
                    }
                }
                const On1x_Status status = on1x_call(
                    state,
                    static_cast<int>(state_top + 1U),
                    static_cast<int>(argc));
                if (state->top == state_top) {
                    error = "native call did not return a result";
                    return false;
                }
                call_result = state->stack[state->top - 1U];
                state->top = state_top;
                if (status != ON1X_OK) {
                    error = "native call failed";
                    return false;
                }
            } else if (!function->chunk ||
                       !execute_function(
                           state,
                           *function->chunk,
                           function,
                           stack + function_index + 1U,
                           argc,
                           call_result,
                           error)) {
                return false;
            }
            top = function_index;
            stack[top++] = call_result;
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
        case Opcode::JumpIfFalse:
            if (top == 0) { error = "VM stack underflow"; return false; }
            if (!stack[top - 1U].is_bool()) {
                error = "if condition must be Bool";
                return false;
            }
            --top;
            if (!stack[top].as_bool()) {
                if (instruction.operand >= chunk.instruction_count()) {
                    error = "invalid jump target";
                    return false;
                }
                program_counter = instruction.operand;
                continue;
            }
            break;
        case Opcode::Jump:
            if (instruction.operand >= chunk.instruction_count()) {
                error = "invalid jump target";
                return false;
            }
            program_counter = instruction.operand;
            continue;
        case Opcode::Return:
            if (top == 0) { error = "VM stack underflow"; return false; }
            result = stack[top - 1U];
            return true;
        }
        ++program_counter;
    }
    error = "VM chunk has no Return";
    return false;
}

}  // namespace

bool execute(On1x_State* state, const Chunk& chunk, Value& result, const char*& error) noexcept {
    return execute_function(state, chunk, nullptr, nullptr, 0, result, error);
}

}  // namespace on1x::vm
