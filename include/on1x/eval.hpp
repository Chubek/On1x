#pragma once

#include "state.hpp"
#include "value.hpp"

#include <cstddef>
#include <string_view>

namespace on1x {

struct EvalResult {
    On1x_Status status = ON1X_ERR;
    Value value{};
};

inline EvalResult eval(State& state, std::string_view source, const char* chunk_name = nullptr) {
    const On1x_Status status = on1x_eval(
        state.get(), source.data(), source.size(), chunk_name);
    return {status, Value(state.get(), -1)};
}

}  // namespace on1x
