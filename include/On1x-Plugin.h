#ifndef ON1X_ON1X_PLUGIN_H
#define ON1X_ON1X_PLUGIN_H

#include "On1x.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef on1x_status (*on1x_plugin_init_fn)(on1x_vm *vm);

#ifdef __cplusplus
}
#endif

#endif
