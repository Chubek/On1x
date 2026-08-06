#pragma once

#include "on1x_types.h"

#ifdef __cplusplus
extern "C" {
#endif

ON1X_API On1x_Status on1x_open_std(On1x_State* state);
ON1X_API On1x_Status on1x_open_io(On1x_State* state);
ON1X_API On1x_Status on1x_open_os(On1x_State* state);
ON1X_API On1x_Status on1x_open_time(On1x_State* state);
ON1X_API On1x_Status on1x_open_fs(On1x_State* state);
ON1X_API On1x_Status on1x_open_dl(On1x_State* state);
ON1X_API On1x_Status on1x_open_asm(On1x_State* state);

#ifdef __cplusplus
}
#endif
