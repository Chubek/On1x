#include "stdlib/args.hpp"

#include "api/api_common.hpp"
#include "runtime/state.hpp"

namespace on1x::stdlib {

bool require_arity(
    On1x_State* state,
    int argc,
    int expected,
    const char* member) noexcept {
    if (argc == expected) return true;
    (void)push_api_error(state, member);
    return false;
}

bool read_argument(const On1x_State* state, int index, Value& value) noexcept {
    return stack_at(state, index, value);
}

}  // namespace on1x::stdlib
