#pragma once

#include "on1x_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct On1x_FnDesc {
    const char* name;
    On1x_CFn function;
} On1x_FnDesc;

typedef struct On1x_ModuleDesc {
    const char* name;
    On1x_Capability capability;
    const On1x_FnDesc* functions;
    size_t function_count;
} On1x_ModuleDesc;

ON1X_API On1x_Status on1x_install_module(On1x_State* state, const On1x_ModuleDesc* module);

#ifdef __cplusplus
}
#endif
