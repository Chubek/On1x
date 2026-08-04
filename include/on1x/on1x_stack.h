#pragma once
#include "on1x_types.h"
#ifdef __cplusplus
extern "C" {
#endif
ON1X_API int on1x_top(const On1x_State* state);
ON1X_API int on1x_pop(On1x_State* state, int count);
ON1X_API On1x_Status on1x_dup(On1x_State* state, int index);
ON1X_API On1x_Type on1x_type(const On1x_State* state, int index);
#ifdef __cplusplus
}
#endif
