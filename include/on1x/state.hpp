#pragma once

#include "on1x_state.h"

#include <utility>

namespace on1x {

class State {
public:
    State() : state_(on1x_open()) {}
    ~State() { on1x_close(state_); }

    State(const State&) = delete;
    State& operator=(const State&) = delete;
    State(State&& other) noexcept : state_(std::exchange(other.state_, nullptr)) {}
    State& operator=(State&& other) noexcept {
        if (this != &other) {
            on1x_close(state_);
            state_ = std::exchange(other.state_, nullptr);
        }
        return *this;
    }

    [[nodiscard]] On1x_State* get() const noexcept { return state_; }
    [[nodiscard]] explicit operator bool() const noexcept { return state_ != nullptr; }

private:
    On1x_State* state_ = nullptr;
};

}  // namespace on1x
