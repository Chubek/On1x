#pragma once

#include <cstddef>
#include <string>
#include <string_view>

namespace on1x {

class StringBuilder {
public:
    explicit StringBuilder(std::size_t initial_capacity = 0);

    void append(std::string_view text);
    void append(char character);
    [[nodiscard]] bool append_code_point(char32_t code_point);
    void clear() noexcept;

    [[nodiscard]] std::size_t size() const noexcept;
    [[nodiscard]] bool empty() const noexcept;
    [[nodiscard]] std::string_view view() const noexcept;
    [[nodiscard]] const std::string& str() const noexcept;
    [[nodiscard]] std::string take();

private:
    std::string buffer_;
};

}
