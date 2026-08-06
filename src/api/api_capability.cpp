#include "api/api_common.hpp"

#include <on1x/on1x_capability.h>

extern "C" {

On1x_Status on1x_grant(On1x_State* state, On1x_Capability capability) {
    if (!state || capability == ON1X_CAP_NONE) return state ? ON1X_OK : ON1X_ERR;
    state->capabilities |= static_cast<std::uint32_t>(capability);
    return ON1X_OK;
}

On1x_Status on1x_revoke(On1x_State* state, On1x_Capability capability) {
    if (!state) return ON1X_ERR;
    state->capabilities &= ~static_cast<std::uint32_t>(capability);
    return ON1X_OK;
}

int on1x_has_capability(const On1x_State* state, On1x_Capability capability) {
    return state && (capability == ON1X_CAP_NONE ||
                     (state->capabilities & static_cast<std::uint32_t>(capability)) != 0U);
}

}
