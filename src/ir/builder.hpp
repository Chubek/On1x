#pragma once

#include "ir/ir.hpp"

#include <initializer_list>
#include <string_view>

namespace on1x::ir {

class Builder {
public:
    explicit Builder(Function& function) : function_(function) {}

    [[nodiscard]] Register emit(
        Opcode opcode,
        syntax::SourcePosition position,
        std::string_view text = {},
        std::initializer_list<Register> operands = {});
    void emit_void(
        Opcode opcode,
        syntax::SourcePosition position,
        std::string_view text = {},
        std::initializer_list<Register> operands = {});
    void set_block(BlockId block) noexcept { block_ = block; }

private:
    Function& function_;
    BlockId block_ = 0;
};

}  // namespace on1x::ir
