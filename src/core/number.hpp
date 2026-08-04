#pragma once

#include "core/value.hpp"

namespace on1x {

enum class ArithmeticOp { Add, Subtract, Multiply, Divide, Modulo };

[[nodiscard]] Value numeric_apply(GcState* gc, ArithmeticOp operation, Value left, Value right);

}  // namespace on1x
