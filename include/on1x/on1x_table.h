#pragma once
#include "on1x_types.h"
#ifdef __cplusplus
extern "C" {
#endif
ON1X_API void on1x_new_table(On1x_State* state);
ON1X_API On1x_Status on1x_table_set(On1x_State* state, int table_index);
ON1X_API int on1x_table_get(On1x_State* state, int table_index);
#ifdef __cplusplus
}
#endif
