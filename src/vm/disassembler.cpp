#include "vm/chunk.hpp"

#include <sstream>

namespace on1x::vm {

namespace {

const char* opcode_name(Opcode opcode) noexcept {
    switch (opcode) {
    case Opcode::Constant: return "constant";
    case Opcode::LoadGlobal: return "load_global";
    case Opcode::StoreGlobal: return "store_global";
    case Opcode::Add: return "add";
    case Opcode::Subtract: return "subtract";
    case Opcode::Multiply: return "multiply";
    case Opcode::Divide: return "divide";
    case Opcode::Modulo: return "modulo";
    case Opcode::Equal: return "equal";
    case Opcode::NotEqual: return "not_equal";
    case Opcode::Less: return "less";
    case Opcode::LessEqual: return "less_equal";
    case Opcode::Greater: return "greater";
    case Opcode::GreaterEqual: return "greater_equal";
    case Opcode::Not: return "not";
    case Opcode::Negate: return "negate";
    case Opcode::LoadLocal: return "load_local";
    case Opcode::StoreLocal: return "store_local";
    case Opcode::LoadUpvalue: return "load_upvalue";
    case Opcode::MakeClosure: return "make_closure";
    case Opcode::Call: return "call";
    case Opcode::MakeList: return "make_list";
    case Opcode::MakeTable: return "make_table";
    case Opcode::MakeTaggedList: return "make_tagged_list";
    case Opcode::MakeSome: return "make_some";
    case Opcode::MakeNone: return "make_none";
    case Opcode::BeginCapture: return "begin_capture";
    case Opcode::EndCapture: return "end_capture";
    case Opcode::EndEffectScope: return "end_effect_scope";
    case Opcode::LoadEffectResult: return "load_effect_result";
    case Opcode::Index: return "index";
    case Opcode::Field: return "field";
    case Opcode::SetIndex: return "set_index";
    case Opcode::SetField: return "set_field";
    case Opcode::Iota: return "iota";
    case Opcode::IterInit: return "iter_init";
    case Opcode::IterNext: return "iter_next";
    case Opcode::IterClose: return "iter_close";
    case Opcode::AssertBool: return "assert_bool";
    case Opcode::MatchPattern: return "match_pattern";
    case Opcode::MatchFailure: return "match_failure";
    case Opcode::JumpIfFalse: return "jump_if_false";
    case Opcode::Jump: return "jump";
    case Opcode::Pop: return "pop";
    case Opcode::Return: return "return";
    }
    return "unknown";
}

}  // namespace

std::string disassemble(const Chunk& chunk) {
    std::ostringstream output;
    for (std::size_t index = 0; index < chunk.instruction_count(); ++index) {
        const Instruction instruction = chunk.instruction(index);
        output << index << ' ' << opcode_name(instruction.opcode);
        if (instruction.operand != 0U || instruction.opcode == Opcode::Constant) {
            output << ' ' << instruction.operand;
        }
        output << '\n';
    }
    return output.str();
}

}  // namespace on1x::vm
