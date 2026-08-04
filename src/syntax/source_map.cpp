#include "syntax/source_map.hpp"

namespace on1x::syntax {

SourcePosition SourceMap::position(std::size_t byte_offset) const noexcept {
    const std::size_t limit = byte_offset < source_.size() ? byte_offset : source_.size();
    SourcePosition result{limit, 1, 1};
    for (std::size_t index = 0; index < limit; ++index) {
        if (source_[index] == '\n') {
            ++result.line;
            result.column = 1;
        } else {
            ++result.column;
        }
    }
    return result;
}

}  // namespace on1x::syntax
