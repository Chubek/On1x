#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace on1x::syntax {

struct SourcePosition {
    std::size_t byte_offset = 0;
    std::size_t line = 1;
    std::size_t column = 1;
};

struct Diagnostic {
    SourcePosition position;
    std::string message;
};

class Diagnostics {
public:
    void add(SourcePosition position, std::string message);
    [[nodiscard]] bool empty() const noexcept;
    [[nodiscard]] const std::vector<Diagnostic>& entries() const noexcept;

private:
    std::vector<Diagnostic> entries_;
};

}  // namespace on1x::syntax
