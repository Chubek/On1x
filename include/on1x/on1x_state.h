#pragma once
#include "on1x_types.h"
#ifdef __cplusplus
extern "C" {
#endif
ON1X_API On1x_State* on1x_open(void);
ON1X_API void on1x_close(On1x_State* state);
ON1X_API On1x_Status on1x_eval(On1x_State* state, const char* source, size_t length, const char* chunk_name);
#ifdef __cplusplus
}
#endif
