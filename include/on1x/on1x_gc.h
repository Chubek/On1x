#pragma once

#include "on1x_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* A reference roots the value currently held at stack_index until released. */
ON1X_API On1x_Ref on1x_ref_create(On1x_State* state, int stack_index);
ON1X_API int on1x_ref_release(On1x_State* state, On1x_Ref reference);
ON1X_API On1x_Status on1x_ref_push(On1x_State* state, On1x_Ref reference);

ON1X_API void on1x_gc_collect(On1x_State* state);
ON1X_API size_t on1x_gc_bytes_allocated(const On1x_State* state);

#ifdef __cplusplus
}
#endif
