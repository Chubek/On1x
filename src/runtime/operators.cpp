#include "core/equality.hpp"
#include "core/number.hpp"
#include "core/string.hpp"
#include "gc/alloc.hpp"

#include <stdexcept>
#include <string>
#include <string_view>

namespace on1x::runtime {

namespace {

bool numeric_compare(std::string_view operation, Value left, Value right, bool& result) noexcept {
    if ((!left.is_int() && !left.is_float()) || (!right.is_int() && !right.is_float())) {
        return false;
    }
    const double lhs = left.is_float() ? left.as_float() : static_cast<double>(left.as_int());
    const double rhs = right.is_float() ? right.as_float() : static_cast<double>(right.as_int());
    if (operation == "<") result = lhs < rhs;
    else if (operation == "<=") result = lhs <= rhs;
    else if (operation == ">") result = lhs > rhs;
    else result = lhs >= rhs;
    return true;
}

}  // namespace

bool apply_binary(
    GcState* gc,
    std::string_view operation,
    Value left,
    Value right,
    Value& result,
    const char*& error) noexcept {
    try {
        if (operation == "==") {
            result = Value::boolean(value_equals(left, right));
            return true;
        }
        if (operation == "!=") {
            result = Value::boolean(!value_equals(left, right));
            return true;
        }
        if (operation == "<" || operation == "<=" || operation == ">" || operation == ">=") {
            bool comparison = false;
            if (!numeric_compare(operation, left, right, comparison)) {
                error = "comparison requires Int or Float operands";
                return false;
            }
            result = Value::boolean(comparison);
            return true;
        }
        if (operation == "+" && left.kind() == Value::Kind::String &&
            right.kind() == Value::Kind::String) {
            std::string text(string_view(as_string_const(left)));
            text += string_view(as_string_const(right));
            result = value_from_object(new_string(gc, text));
            return true;
        }
        ArithmeticOp arithmetic = ArithmeticOp::Add;
        if (operation == "-") arithmetic = ArithmeticOp::Subtract;
        else if (operation == "*") arithmetic = ArithmeticOp::Multiply;
        else if (operation == "/") arithmetic = ArithmeticOp::Divide;
        else if (operation == "%") arithmetic = ArithmeticOp::Modulo;
        else if (operation != "+") {
            error = "unknown binary operator";
            return false;
        }
        result = numeric_apply(gc, arithmetic, left, right);
        return true;
    } catch (const std::domain_error&) {
        error = "division by zero";
        return false;
    } catch (const std::invalid_argument&) {
        error = operation == "+"
            ? "operator '+' requires two numbers or two Strings"
            : "numeric operator requires Int or Float operands";
        return false;
    } catch (...) {
        error = "unable to apply binary operator";
        return false;
    }
}

bool apply_unary(
    GcState* gc,
    std::string_view operation,
    Value operand,
    Value& result,
    const char*& error) noexcept {
    if (operation == "not") {
        if (!operand.is_bool()) {
            error = "operator 'not' requires a Bool operand";
            return false;
        }
        result = Value::boolean(!operand.as_bool());
        return true;
    }
    if (operation == "-") {
        try {
            result = numeric_apply(gc, ArithmeticOp::Subtract, Value::integer(gc, 0), operand);
            return true;
        } catch (const std::invalid_argument&) {
            error = "unary '-' requires an Int or Float operand";
            return false;
        } catch (...) {
            error = "unable to apply unary operator";
            return false;
        }
    }
    error = "unknown unary operator";
    return false;
}

}  // namespace on1x::runtime
