#pragma once

#include "syntax/diagnostics.hpp"

#include <cstddef>
#include <string_view>

namespace on1x::syntax {

class SourceMap {
public:
    explicit SourceMap(std::string_view source) : source_(source) {}

    [[nodiscard]] SourcePosition position(std::size_t byte_offset) const noexcept;

private:
    std::string_view source_;
};

}  // namespace on1x::syntax
