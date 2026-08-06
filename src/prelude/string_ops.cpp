#include "prelude/prelude.hpp"

#include "core/string.hpp"

namespace on1x::prelude {

std::size_t string_length(Value value, bool& valid) noexcept {
    const StringObject* string = as_string_const(value);
    valid = string != nullptr;
    return string ? string->bytes : 0;
}

}  // namespace on1x::prelude
