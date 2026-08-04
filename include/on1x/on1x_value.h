#pragma once
#include "on1x_types.h"
#ifdef __cplusplus
extern "C" {
#endif
ON1X_API void on1x_push_unit(On1x_State* state);
ON1X_API void on1x_push_bool(On1x_State* state, int value);
ON1X_API void on1x_push_int(On1x_State* state, int64_t value);
ON1X_API void on1x_push_float(On1x_State* state, double value);
ON1X_API On1x_Status on1x_push_string(On1x_State* state, const char* value, size_t length);
ON1X_API On1x_Status on1x_push_tag(On1x_State* state, const char* value, size_t length);
ON1X_API int64_t on1x_as_int(const On1x_State* state, int index);
ON1X_API double on1x_as_float(const On1x_State* state, int index);
ON1X_API int on1x_as_bool(const On1x_State* state, int index);
ON1X_API const char* on1x_as_string(const On1x_State* state, int index, size_t* length);
#ifdef __cplusplus
}
#endif
