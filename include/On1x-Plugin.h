#ifndef ON1X_ON1X_PLUGIN_H
#define ON1X_ON1X_PLUGIN_H

#include "On1x.h"

#define ON1X_PLUGIN_ABI_VERSION 1u

#ifdef __cplusplus
extern "C" {
#endif

typedef struct on1x_plugin_api {
  unsigned abi_version;
  on1x_vm *(*vm_create)(void);
  void (*vm_destroy)(on1x_vm *vm);
  void (*vm_reset)(on1x_vm *vm);
  on1x_status (*vm_eval_string)(on1x_vm *vm, const char *source, char **output);
  on1x_status (*vm_eval_file)(on1x_vm *vm, const char *path, char **output);
  const char *(*status_string)(on1x_status status);
  void (*string_free)(char *text);
} on1x_plugin_api;

typedef on1x_status (*on1x_plugin_init_fn)(on1x_vm *vm);
typedef on1x_status (*on1x_plugin_init_with_api_fn)(const on1x_plugin_api *api, on1x_vm *vm);
typedef void (*on1x_plugin_shutdown_fn)(on1x_vm *vm);

#ifdef __cplusplus
}
#endif

#endif
