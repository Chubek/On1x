#pragma once
#include "on1x_types.h"
#ifdef __cplusplus
extern "C" {
#endif
ON1X_API On1x_Status on1x_register(On1x_State* state, const char* name, On1x_CFn function);
ON1X_API On1x_Status on1x_call(On1x_State* state, int function_index, int argc);
#ifdef __cplusplus
}
#endif
