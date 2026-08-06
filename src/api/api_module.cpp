#include "api/api_common.hpp"

#include <on1x/on1x_module.h>

#include "stdlib/module.hpp"

extern "C" On1x_Status on1x_install_module(
    On1x_State* state,
    const On1x_ModuleDesc* module) {
    if (!module) return on1x::push_api_error(state, "InstallModule expects a module");
    return on1x::stdlib::install_module(state, *module) ? ON1X_OK : ON1X_ERR;
}
