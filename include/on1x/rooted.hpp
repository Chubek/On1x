#pragma once

#include "on1x_gc.h"

namespace on1x {

class Rooted {
public:
    Rooted() = default;
    Rooted(On1x_State* state, int index) : state_(state), reference_(on1x_ref_create(state, index)) {}
    ~Rooted() { reset(); }

    Rooted(const Rooted&) = delete;
    Rooted& operator=(const Rooted&) = delete;
    Rooted(Rooted&& other) noexcept
        : state_(other.state_), reference_(other.reference_) {
        other.state_ = nullptr;
        other.reference_ = 0;
    }
    Rooted& operator=(Rooted&& other) noexcept {
        if (this != &other) {
            reset();
            state_ = other.state_;
            reference_ = other.reference_;
            other.state_ = nullptr;
            other.reference_ = 0;
        }
        return *this;
    }

    void reset() noexcept {
        if (state_ && reference_ != 0) (void)on1x_ref_release(state_, reference_);
        state_ = nullptr;
        reference_ = 0;
    }
    [[nodiscard]] On1x_Ref reference() const noexcept { return reference_; }
    [[nodiscard]] bool valid() const noexcept { return state_ != nullptr && reference_ != 0; }

private:
    On1x_State* state_ = nullptr;
    On1x_Ref reference_ = 0;
};

}  // namespace on1x
