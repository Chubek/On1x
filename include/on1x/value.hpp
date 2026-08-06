#pragma once

#include "on1x_stack.h"
#include "on1x_value.h"

#include <cstdint>
#include <string>

namespace on1x {

class Value {
public:
    Value() = default;
    Value(On1x_State* state, int index) : state_(state), index_(index) {}

    [[nodiscard]] On1x_Type type() const noexcept { return on1x_type(state_, index_); }
    [[nodiscard]] bool is_valid() const noexcept { return type() != ON1X_INVALID; }
    [[nodiscard]] std::int64_t as_int() const noexcept { return on1x_as_int(state_, index_); }
    [[nodiscard]] double as_float() const noexcept { return on1x_as_float(state_, index_); }
    [[nodiscard]] bool as_bool() const noexcept { return on1x_as_bool(state_, index_) != 0; }
    [[nodiscard]] std::string as_string() const {
        std::size_t length = 0;
        const char* text = on1x_as_string(state_, index_, &length);
        return text ? std::string(text, length) : std::string();
    }
    [[nodiscard]] On1x_State* state() const noexcept { return state_; }
    [[nodiscard]] int index() const noexcept { return index_; }

private:
    On1x_State* state_ = nullptr;
    int index_ = 0;
};

}  // namespace on1x
