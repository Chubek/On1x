#include "api/api_common.hpp"

extern "C" On1x_Status on1x_error(On1x_State* state, const char* message) {
    return on1x::push_api_error(state, message);
}
