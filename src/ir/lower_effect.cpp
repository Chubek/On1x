#include "ir/builder.hpp"

namespace on1x::ir {

void emit_effect_capture_begin(Builder& builder, syntax::SourcePosition position) {
    builder.emit_void(Opcode::BeginCapture, position);
}

void emit_effect_capture_end(Builder& builder, syntax::SourcePosition position) {
    builder.emit_void(Opcode::EndCapture, position);
}

void emit_effect_scope_end(Builder& builder, syntax::SourcePosition position) {
    builder.emit_void(Opcode::EndEffectScope, position);
}

}  // namespace on1x::ir
