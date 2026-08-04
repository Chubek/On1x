#pragma once

#include "on1x_export.h"

#define ON1X_VERSION_MAJOR 0
#define ON1X_VERSION_MINOR 1
#define ON1X_VERSION_PATCH 0

#ifdef __cplusplus
extern "C" {
#endif

ON1X_API const char* on1x_version_string(void);

#ifdef __cplusplus
}
#endif
