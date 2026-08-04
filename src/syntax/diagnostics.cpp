#include "syntax/diagnostics.hpp"

namespace on1x::syntax {

void Diagnostics::add(SourcePosition position, std::string message) {
    entries_.push_back({position, std::move(message)});
}

bool Diagnostics::empty() const noexcept { return entries_.empty(); }

const std::vector<Diagnostic>& Diagnostics::entries() const noexcept { return entries_; }

}  // namespace on1x::syntax
