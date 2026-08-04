#pragma once
#include "on1x_types.h"
#ifdef __cplusplus
extern "C" {
#endif
ON1X_API On1x_Status on1x_tag_list(On1x_State* state, const char* tag, size_t length, int list_index);
ON1X_API int on1x_tag_of(On1x_State* state, int index);
ON1X_API int on1x_payload_of(On1x_State* state, int index);
#ifdef __cplusplus
}
#endif
