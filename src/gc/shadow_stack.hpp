#pragma once

#include <cstddef>

namespace on1x {

class ShadowStack {
public:
    ShadowStack() = default;
    ~ShadowStack();

    ShadowStack(const ShadowStack&) = delete;
    ShadowStack& operator=(const ShadowStack&) = delete;

    void push(void* value);
    void pop() noexcept;
    [[nodiscard]] std::size_t size() const noexcept;

private:
    struct Slot;
    Slot* top_ = nullptr;
    std::size_t size_ = 0;
};

}
