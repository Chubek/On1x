#pragma once

#include "on1x_export.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct On1x_State On1x_State;
typedef struct On1x_FfiLibrary On1x_FfiLibrary;
typedef uintptr_t On1x_Ref;

typedef enum On1x_Type {
    ON1X_UNIT, ON1X_BOOL, ON1X_INT, ON1X_FLOAT, ON1X_STRING, ON1X_TAG,
    ON1X_LIST, ON1X_TABLE, ON1X_FN, ON1X_IOTA, ON1X_INVALID
} On1x_Type;

typedef enum On1x_Status { ON1X_OK = 0, ON1X_ERR = 1 } On1x_Status;
typedef On1x_Status (*On1x_CFn)(On1x_State* state, int argc);

#ifdef __cplusplus
}
#endif
