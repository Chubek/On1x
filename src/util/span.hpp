#pragma once

#include <cstddef>
#include <type_traits>

namespace on1x {

template <typename T>
class Span {
public:
    using element_type = T;
    using value_type = std::remove_cv_t<T>;
    using pointer = T*;
    using reference = T&;
    using iterator = pointer;

    constexpr Span() noexcept = default;
    constexpr Span(pointer data, std::size_t size) noexcept : data_(data), size_(size) {}

    template <std::size_t N>
    constexpr Span(element_type (&array)[N]) noexcept : data_(array), size_(N) {}

    [[nodiscard]] constexpr pointer data() const noexcept { return data_; }
    [[nodiscard]] constexpr std::size_t size() const noexcept { return size_; }
    [[nodiscard]] constexpr bool empty() const noexcept { return size_ == 0; }

    [[nodiscard]] constexpr reference operator[](std::size_t index) const noexcept {
        return data_[index];
    }

    [[nodiscard]] constexpr iterator begin() const noexcept { return data_; }
    [[nodiscard]] constexpr iterator end() const noexcept { return data_ + size_; }

private:
    pointer data_ = nullptr;
    std::size_t size_ = 0;
};

}
