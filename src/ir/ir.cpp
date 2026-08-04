#include "ir/ir.hpp"

#include <sstream>

namespace on1x::ir {

bool Instruction::has_side_effects() const noexcept {
    return opcode == Opcode::StoreGlobal || opcode == Opcode::Call ||
        opcode == Opcode::Capture || opcode == Opcode::Return;
}

namespace {

std::string_view opcode_name(Opcode opcode) noexcept {
    switch (opcode) {
    case Opcode::Unit: return "unit";
    case Opcode::Bool: return "bool";
    case Opcode::Int: return "int";
    case Opcode::Float: return "float";
    case Opcode::String: return "string";
    case Opcode::Tag: return "tag";
    case Opcode::LoadGlobal: return "load_global";
    case Opcode::StoreGlobal: return "store_global";
    case Opcode::Unary: return "unary";
    case Opcode::Binary: return "binary";
    case Opcode::MakeList: return "make_list";
    case Opcode::MakeTable: return "make_table";
    case Opcode::MakeTaggedList: return "make_tagged_list";
    case Opcode::Call: return "call";
    case Opcode::Index: return "index";
    case Opcode::Field: return "field";
    case Opcode::Some: return "some";
    case Opcode::None: return "none";
    case Opcode::EffectResult: return "effect_result";
    case Opcode::Capture: return "capture";
    case Opcode::Discard: return "discard";
    case Opcode::LoadLocal: return "load_local";
    case Opcode::StoreLocal: return "store_local";
    case Opcode::BranchIfFalse: return "branch_if_false";
    case Opcode::Jump: return "jump";
    case Opcode::Phi: return "phi";
    case Opcode::LoadUpvalue: return "load_upvalue";
    case Opcode::MakeFunction: return "make_function";
    case Opcode::Return: return "return";
    }
    return "unknown";
}

}

std::string print(const Module& module) {
    std::ostringstream output;
    output << "fn " << module.entry.name << " registers=" << module.entry.register_count << '\n';
    for (const BasicBlock& block : module.entry.blocks) {
        output << "block " << block.id << ":\n";
        for (const Instruction& instruction : block.instructions) {
            output << "  ";
            if (instruction.has_result()) output << '%' << instruction.result << " = ";
            output << opcode_name(instruction.opcode);
            if (!instruction.text.empty()) output << ' ' << instruction.text;
            if (instruction.opcode == Opcode::BranchIfFalse || instruction.opcode == Opcode::Jump) {
                output << " block " << instruction.target;
            }
            if (instruction.opcode == Opcode::MakeFunction) output << " function " << instruction.target;
            for (Register operand : instruction.operands) output << " %" << operand;
            output << '\n';
        }
    }
    for (const Function& function : module.functions) {
        output << "fn " << function.name << " registers=" << function.register_count << '\n';
    }
    return output.str();
}

}  // namespace on1x::ir
