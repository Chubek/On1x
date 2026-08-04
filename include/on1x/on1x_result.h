#pragma once
#include "on1x_types.h"
#ifdef __cplusplus
extern "C" {
#endif
ON1X_API On1x_Status on1x_push_success(On1x_State* state);
ON1X_API On1x_Status on1x_push_error_result(On1x_State* state);
ON1X_API int on1x_is_success(const On1x_State* state, int index);
ON1X_API int on1x_is_error(const On1x_State* state, int index);
#ifdef __cplusplus
}
#endif
