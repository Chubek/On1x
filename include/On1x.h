#ifndef ON1X_ON1X_H
#define ON1X_ON1X_H

#define ON1X_VERSION_MAJOR 0
#define ON1X_VERSION_MINOR 1
#define ON1X_VERSION_PATCH 0
#define ON1X_VERSION_STRING "0.1.0"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct on1x_vm on1x_vm;

typedef enum on1x_status {
  ON1X_OK = 0,
  ON1X_PARSE_ERROR = 1,
  ON1X_RUNTIME_ERROR = 2,
  ON1X_IO_ERROR = 3
} on1x_status;

on1x_vm *on1x_vm_create(void);
void on1x_vm_destroy(on1x_vm *vm);
void on1x_vm_reset(on1x_vm *vm);

on1x_status on1x_vm_eval_string(on1x_vm *vm, const char *source, char **output);
on1x_status on1x_vm_eval_file(on1x_vm *vm, const char *path, char **output);
const char *on1x_status_string(on1x_status status);
void on1x_string_free(char *text);

#ifdef __cplusplus
}
#endif

#endif
