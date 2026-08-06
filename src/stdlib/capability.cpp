#include "stdlib/capability.hpp"

#include "runtime/state.hpp"

namespace on1x::stdlib {

bool has_capability(const On1x_State* state, On1x_Capability capability) noexcept {
    return state && (capability == ON1X_CAP_NONE ||
                     (state->capabilities & static_cast<std::uint32_t>(capability)) != 0U);
}

}  // namespace on1x::stdlib
