#pragma once
#include "on1x_types.h"
#ifdef __cplusplus
extern "C" {
#endif
ON1X_API On1x_Status on1x_push_some(On1x_State* state);
ON1X_API void on1x_push_none(On1x_State* state);
ON1X_API int on1x_is_some(const On1x_State* state, int index);
ON1X_API int on1x_is_none(const On1x_State* state, int index);
#ifdef __cplusplus
}
#endif
