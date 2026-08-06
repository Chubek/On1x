#include "vm/interpreter.hpp"

#include "core/iota.hpp"
#include "core/list.hpp"
#include "core/number.hpp"
#include "core/optional.hpp"
#include "core/table.hpp"
#include "gc/alloc.hpp"
#include "gc/roots.hpp"
#include <on1x/on1x_call.h>
#include "runtime/state.hpp"
#include "runtime/effect.hpp"
#include "runtime/call.hpp"
#include "runtime/closure.hpp"
#include "runtime/iterate.hpp"
#include "runtime/pattern_match.hpp"
#include "runtime/raise.hpp"

#include <string_view>

namespace on1x::runtime {
bool apply_binary(
    GcState* gc,
    std::string_view operation,
    Value left,
    Value right,
    Value& result,
    const char*& error) noexcept;
bool apply_unary(
    GcState* gc,
    std::string_view operation,
    Value operand,
    Value& result,
    const char*& error) noexcept;
bool index_value(
    GcState* gc,
    const ReservedTags& tags,
    Value container,
    Value key,
    Value& result,
    const char*& error) noexcept;
bool field_value(
    Value container,
    Value key,
    Value& result,
    const char*& error) noexcept;
bool set_index_value(
    GcState* gc,
    Value container,
    Value key,
    Value value,
    const char*& error) noexcept;
bool set_field_value(
    GcState* gc,
    Value container,
    Value key,
    Value value,
    const char*& error) noexcept;
}  // namespace on1x::runtime

namespace on1x::vm {

namespace {

struct VmFailure {};

class VmResult {
public:
    VmResult(bool success) {
        if (!success) throw VmFailure{};
        success_ = true;
    }

    [[nodiscard]] static VmResult failure() noexcept { return VmResult(FailureTag{}); }
    [[nodiscard]] explicit operator bool() const noexcept { return success_; }

private:
    struct FailureTag {};

    explicit VmResult(FailureTag) noexcept : success_(false) {}

    bool success_ = false;
};

struct Capture {
    std::size_t stack_top = 0;
    std::size_t failure_target = 0;
};

VmResult execute_function(
    On1x_State* state,
    const Chunk& chunk,
    FunctionObject* closure,
    const Value* arguments,
    std::size_t argument_count,
    Value& result,
    const char*& error) noexcept {
    try {
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
    Capture captures[64]{};
    std::size_t capture_count = 0;
    Value effect_results[64]{};
    std::size_t effect_result_count = 0;
    for (std::size_t program_counter = 0; program_counter < chunk.instruction_count();) {
        try {
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
            auto* function = runtime::new_closure(
                &state->gc,
                target,
                stack + top - target->capture_count(),
                target->capture_count());
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
                if (!runtime::invoke_native(
                        state,
                        function,
                        stack + function_index + 1U,
                        argc,
                        call_result,
                        error)) return false;
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
        case Opcode::MakeList: {
            const std::size_t count = instruction.operand;
            if (top < count) { error = "VM stack underflow"; return false; }
            auto* list = new_list(&state->gc, count);
            GcRoot list_root(list);
            for (std::size_t index = 0; index < count; ++index) {
                if (!list_push(&state->gc, list, stack[top - count + index])) {
                    error = "unable to allocate List";
                    return false;
                }
            }
            top -= count;
            stack[top++] = value_from_object(list);
            break;
        }
        case Opcode::MakeTable: {
            const std::size_t count = instruction.operand;
            if (top < count * 2U) { error = "VM stack underflow"; return false; }
            auto* table = new_table(&state->gc);
            GcRoot table_root(table);
            const std::size_t start = top - count * 2U;
            for (std::size_t index = 0; index < count; ++index) {
                if (!table_set(
                        &state->gc,
                        table,
                        stack[start + index * 2U],
                        stack[start + index * 2U + 1U])) {
                    error = "Table key is not hashable";
                    return false;
                }
            }
            top = start;
            stack[top++] = value_from_object(table);
            break;
        }
        case Opcode::MakeTaggedList: {
            const std::size_t count = instruction.operand;
            if (top < count + 1U) { error = "VM stack underflow"; return false; }
            TagObject* tag = as_tag(stack[top - 1U]);
            if (!tag) { error = "invalid Tagged List constructor"; return false; }
            auto* list = new_list(&state->gc, count);
            GcRoot list_root(list);
            for (std::size_t index = 0; index < count; ++index) {
                if (!list_push(&state->gc, list, stack[top - count - 1U + index])) {
                    error = "unable to allocate Tagged List";
                    return false;
                }
            }
            list->constructor = tag;
            top -= count + 1U;
            stack[top++] = value_from_object(list);
            break;
        }
        case Opcode::MakeSome:
            if (top == 0) { error = "VM stack underflow"; return false; }
            try {
                stack[top - 1U] = make_some(&state->gc, state->reserved, stack[top - 1U]);
            } catch (...) {
                error = "unable to create Some";
                return false;
            }
            break;
        case Opcode::MakeNone:
            if (top == 64) { error = "VM stack overflow"; return false; }
            try {
                stack[top++] = make_none(&state->gc, state->reserved);
            } catch (...) {
                error = "unable to create None";
                return false;
            }
            break;
        case Opcode::BeginCapture:
            if (capture_count == 64 || instruction.operand >= chunk.instruction_count()) {
                error = "invalid effect capture";
                return false;
            }
            captures[capture_count++] = {top, instruction.operand};
            break;
        case Opcode::EndCapture: {
            if (capture_count == 0 || top == 0) {
                error = "invalid effect capture";
                return false;
            }
            const Capture capture = captures[--capture_count];
            if (effect_result_count == 64) {
                error = "effect capture nesting is too deep";
                return false;
            }
            const Value value = stack[top - 1U];
            GcRoot value_root(value.is_object() ? value.as_object() : nullptr);
            effect_results[effect_result_count++] = runtime::capture_success(
                &state->gc, state->reserved, value);
            top = capture.stack_top;
            break;
        }
        case Opcode::EndEffectScope:
            if (effect_result_count == 0) {
                error = "invalid effect scope";
                return false;
            }
            --effect_result_count;
            break;
        case Opcode::LoadEffectResult:
            if (effect_result_count == 0 || top == 64) {
                error = "effect result is not available";
                return false;
            }
            stack[top++] = effect_results[effect_result_count - 1U];
            break;
        case Opcode::Index: {
            if (top < 2) { error = "VM stack underflow"; return false; }
            Value indexed;
            if (!runtime::index_value(
                    &state->gc,
                    state->reserved,
                    stack[top - 2U],
                    stack[top - 1U],
                    indexed,
                    error)) {
                return false;
            }
            stack[top - 2U] = indexed;
            --top;
            break;
        }
        case Opcode::Field: {
            if (top == 0 || instruction.operand >= chunk.constant_count()) {
                error = "invalid field access";
                return false;
            }
            Value field;
            if (!runtime::field_value(
                    stack[top - 1U], chunk.constant(instruction.operand), field, error)) {
                return false;
            }
            stack[top - 1U] = field;
            break;
        }
        case Opcode::SetIndex:
            if (top < 3) { error = "VM stack underflow"; return false; }
            if (!runtime::set_index_value(
                    &state->gc,
                    stack[top - 3U],
                    stack[top - 2U],
                    stack[top - 1U],
                    error)) {
                return false;
            }
            stack[top - 3U] = stack[top - 1U];
            top -= 2;
            break;
        case Opcode::SetField:
            if (top < 2 || instruction.operand >= chunk.constant_count()) {
                error = "invalid field assignment";
                return false;
            }
            if (!runtime::set_field_value(
                    &state->gc,
                    stack[top - 2U],
                    chunk.constant(instruction.operand),
                    stack[top - 1U],
                    error)) {
                return false;
            }
            stack[top - 2U] = stack[top - 1U];
            --top;
            break;
        case Opcode::Iota: {
            const std::size_t count = instruction.operand;
            if (count == 0 || count > 3 || top < count) {
                error = "invalid Iota call";
                return false;
            }
            Value value;
            switch (count) {
            case 1: value = make_iota(&state->gc, state->reserved, {stack[top - 1U]}); break;
            case 2:
                value = make_iota(&state->gc, state->reserved, {stack[top - 2U], stack[top - 1U]});
                break;
            case 3:
                value = make_iota(
                    &state->gc,
                    state->reserved,
                    {stack[top - 3U], stack[top - 2U], stack[top - 1U]});
                break;
            default: error = "invalid Iota call"; return false;
            }
            top -= count;
            stack[top++] = value;
            break;
        }
        case Opcode::IterInit: {
            if (top == 0) { error = "VM stack underflow"; return false; }
            Value iterable;
            if (!runtime::initialize_iteration(
                    &state->gc, stack[top - 1U], iterable, error)) return false;
            stack[top - 1U] = iterable;
            if (top == 64) { error = "VM stack overflow"; return false; }
            stack[top++] = Value::integer(&state->gc, 0);
            break;
        }
        case Opcode::IterNext: {
            if (top < 2) { error = "VM iterator stack underflow"; return false; }
            const Value iterable = stack[top - 2U];
            const Value index = stack[top - 1U];
            if (!index.is_int()) { error = "invalid iterator index"; return false; }
            const auto* list = as_list_const(iterable);
            if (!list) { error = "invalid iterator"; return false; }
            const std::int64_t position = index.as_int();
            if (position < 0 || static_cast<std::size_t>(position) >= list->length) {
                top -= 2;
                stack[top++] = Value::boolean(false);
                break;
            }
            Value item;
            if (!list_get(list, static_cast<std::size_t>(position), item)) {
                error = "invalid iterator item";
                return false;
            }
            if (instruction.operand >= chunk.local_count()) {
                error = "invalid loop binding";
                return false;
            }
            locals[instruction.operand] = item;
            stack[top - 1U] = Value::integer(&state->gc, position + 1);
            if (top == 64) { error = "VM stack overflow"; return false; }
            stack[top++] = Value::boolean(true);
            break;
        }
        case Opcode::IterClose:
            if (top < 2) { error = "VM iterator stack underflow"; return false; }
            top -= 2;
            break;
        case Opcode::MatchPattern: {
            if (top == 0) { error = "VM stack underflow"; return false; }
            const runtime::Pattern* pattern = chunk.pattern(instruction.operand);
            if (!pattern) { error = "invalid match pattern"; return false; }
            const bool matched = runtime::match_pattern(
                &state->gc, *pattern, stack[top - 1U], locals, chunk.local_count(), error);
            if (error) return false;
            stack[top - 1U] = Value::boolean(matched);
            break;
        }
        case Opcode::MatchFailure:
            error = "non-exhaustive match";
            return false;
        case Opcode::Add:
        case Opcode::Subtract:
        case Opcode::Multiply:
        case Opcode::Divide:
        case Opcode::Modulo:
        case Opcode::Equal:
        case Opcode::NotEqual:
        case Opcode::Less:
        case Opcode::LessEqual:
        case Opcode::Greater:
        case Opcode::GreaterEqual: {
            if (top < 2) { error = "VM stack underflow"; return false; }
            const char* operation = "";
            switch (instruction.opcode) {
            case Opcode::Add: operation = "+"; break;
            case Opcode::Subtract: operation = "-"; break;
            case Opcode::Multiply: operation = "*"; break;
            case Opcode::Divide: operation = "/"; break;
            case Opcode::Modulo: operation = "%"; break;
            case Opcode::Equal: operation = "=="; break;
            case Opcode::NotEqual: operation = "!="; break;
            case Opcode::Less: operation = "<"; break;
            case Opcode::LessEqual: operation = "<="; break;
            case Opcode::Greater: operation = ">"; break;
            case Opcode::GreaterEqual: operation = ">="; break;
            default: break;
            }
            Value operation_result;
            if (!runtime::apply_binary(
                    &state->gc, operation, stack[top - 2U], stack[top - 1U], operation_result, error)) {
                return false;
            }
            stack[top - 2U] = operation_result;
            --top;
            break;
        }
        case Opcode::Not:
        case Opcode::Negate: {
            if (top == 0) { error = "VM stack underflow"; return false; }
            Value operation_result;
            if (!runtime::apply_unary(
                    &state->gc,
                    instruction.opcode == Opcode::Not ? "not" : "-",
                    stack[top - 1U],
                    operation_result,
                    error)) {
                return false;
            }
            stack[top - 1U] = operation_result;
            break;
        }
        case Opcode::AssertBool:
            if (top == 0) { error = "VM stack underflow"; return false; }
            if (!stack[top - 1U].is_bool()) {
                error = "logical operator requires Bool operands";
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
        } catch (const VmFailure&) {
            if (capture_count == 0) return VmResult::failure();
            const Capture capture = captures[--capture_count];
            if (effect_result_count == 64) {
                error = "effect capture nesting is too deep";
                return VmResult::failure();
            }
            const char* message = error ? error : "On1x execution failed";
            effect_results[effect_result_count++] = runtime::capture_error(
                &state->gc, state->reserved, message);
            top = capture.stack_top;
            program_counter = capture.failure_target;
            continue;
        }
        ++program_counter;
    }
    error = "VM chunk has no Return";
    return false;
    } catch (const VmFailure&) {
        return VmResult::failure();
    }
}

}  // namespace

bool execute(On1x_State* state, const Chunk& chunk, Value& result, const char*& error) noexcept {
    return static_cast<bool>(execute_function(state, chunk, nullptr, nullptr, 0, result, error));
}

}  // namespace on1x::vm
