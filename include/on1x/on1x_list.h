#pragma once
#include "on1x_types.h"
#ifdef __cplusplus
extern "C" {
#endif
ON1X_API void on1x_new_list(On1x_State* state);
ON1X_API On1x_Status on1x_list_push(On1x_State* state, int list_index);
ON1X_API int on1x_list_get(On1x_State* state, int list_index, int element_index);
ON1X_API int on1x_len(const On1x_State* state, int index);
#ifdef __cplusplus
}
#endif
