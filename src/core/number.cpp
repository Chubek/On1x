#include "core/number.hpp"

#include <limits>
#include <cmath>
#include <stdexcept>

namespace on1x {

namespace {
std::int64_t wrapping_add(std::int64_t left, std::int64_t right) noexcept {
    return static_cast<std::int64_t>(static_cast<std::uint64_t>(left) + static_cast<std::uint64_t>(right));
}
std::int64_t wrapping_subtract(std::int64_t left, std::int64_t right) noexcept {
    return static_cast<std::int64_t>(static_cast<std::uint64_t>(left) - static_cast<std::uint64_t>(right));
}
std::int64_t wrapping_multiply(std::int64_t left, std::int64_t right) noexcept {
    return static_cast<std::int64_t>(static_cast<std::uint64_t>(left) * static_cast<std::uint64_t>(right));
}
}

Value numeric_apply(GcState* gc, ArithmeticOp operation, Value left, Value right) {
    if ((!left.is_int() && !left.is_float()) || (!right.is_int() && !right.is_float())) {
        throw std::invalid_argument("numeric operation requires Int or Float operands");
    }
    if (left.is_float() || right.is_float()) {
        const double lhs = left.is_float() ? left.as_float() : static_cast<double>(left.as_int());
        const double rhs = right.is_float() ? right.as_float() : static_cast<double>(right.as_int());
        if ((operation == ArithmeticOp::Divide || operation == ArithmeticOp::Modulo) && rhs == 0.0) {
            throw std::domain_error("division by zero");
        }
        switch (operation) {
        case ArithmeticOp::Add: return Value::floating(lhs + rhs);
        case ArithmeticOp::Subtract: return Value::floating(lhs - rhs);
        case ArithmeticOp::Multiply: return Value::floating(lhs * rhs);
        case ArithmeticOp::Divide: return Value::floating(lhs / rhs);
        case ArithmeticOp::Modulo: return Value::floating(std::fmod(lhs, rhs));
        }
    }
    const std::int64_t lhs = left.as_int();
    const std::int64_t rhs = right.as_int();
    if ((operation == ArithmeticOp::Divide || operation == ArithmeticOp::Modulo) && rhs == 0) {
        throw std::domain_error("division by zero");
    }
    switch (operation) {
    case ArithmeticOp::Add: return Value::integer(gc, wrapping_add(lhs, rhs));
    case ArithmeticOp::Subtract: return Value::integer(gc, wrapping_subtract(lhs, rhs));
    case ArithmeticOp::Multiply: return Value::integer(gc, wrapping_multiply(lhs, rhs));
    case ArithmeticOp::Divide:
        if (lhs == std::numeric_limits<std::int64_t>::min() && rhs == -1) return Value::integer(gc, lhs);
        return Value::integer(gc, lhs / rhs);
    case ArithmeticOp::Modulo:
        if (lhs == std::numeric_limits<std::int64_t>::min() && rhs == -1) return Value::integer(gc, 0);
        return Value::integer(gc, lhs % rhs);
    }
    throw std::logic_error("unknown arithmetic operation");
}

}  // namespace on1x
