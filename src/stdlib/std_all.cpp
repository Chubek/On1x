#include "stdlib/registry.hpp"

extern "C" On1x_Status on1x_open_std(On1x_State* state) {
    return on1x::stdlib::install_pure_modules(state) ? ON1X_OK : ON1X_ERR;
}
