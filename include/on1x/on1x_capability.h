#pragma once

#include "on1x_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum On1x_Capability {
    ON1X_CAP_NONE = 0,
    ON1X_CAP_FS = 1u << 0,
    ON1X_CAP_IO = 1u << 1,
    ON1X_CAP_ENV = 1u << 2,
    ON1X_CAP_TIME = 1u << 3,
    ON1X_CAP_CLOCK = 1u << 4,
    ON1X_CAP_PROC = 1u << 5,
    ON1X_CAP_NET = 1u << 6,
    ON1X_CAP_DL = 1u << 7,
} On1x_Capability;

ON1X_API On1x_Status on1x_grant(On1x_State* state, On1x_Capability capability);
ON1X_API On1x_Status on1x_revoke(On1x_State* state, On1x_Capability capability);
ON1X_API int on1x_has_capability(const On1x_State* state, On1x_Capability capability);

#ifdef __cplusplus
}
#endif
