%module On1x

%{
#include "on1x/on1x.h"
%}

%include <stdint.i>

typedef struct On1x_State On1x_State;
typedef struct On1x_FfiLibrary On1x_FfiLibrary;
typedef uintptr_t On1x_Ref;

typedef enum On1x_Type {
    ON1X_UNIT, ON1X_BOOL, ON1X_INT, ON1X_FLOAT, ON1X_STRING, ON1X_TAG,
    ON1X_LIST, ON1X_TABLE, ON1X_FN, ON1X_IOTA, ON1X_INVALID
} On1x_Type;

typedef enum On1x_Status { ON1X_OK = 0, ON1X_ERR = 1 } On1x_Status;
typedef On1x_Status (*On1x_CFn)(On1x_State* state, int argc);

On1x_State* on1x_open(void);
void on1x_close(On1x_State* state);
On1x_Status on1x_eval(On1x_State* state, const char* source, size_t length, const char* chunk_name);
int on1x_top(const On1x_State* state);
int on1x_pop(On1x_State* state, int count);
On1x_Status on1x_dup(On1x_State* state, int index);
On1x_Type on1x_type(const On1x_State* state, int index);
void on1x_push_unit(On1x_State* state);
void on1x_push_bool(On1x_State* state, int value);
void on1x_push_int(On1x_State* state, int64_t value);
void on1x_push_float(On1x_State* state, double value);
On1x_Status on1x_push_string(On1x_State* state, const char* value, size_t length);
On1x_Status on1x_push_tag(On1x_State* state, const char* value, size_t length);
int64_t on1x_as_int(const On1x_State* state, int index);
double on1x_as_float(const On1x_State* state, int index);
int on1x_as_bool(const On1x_State* state, int index);
const char* on1x_as_string(const On1x_State* state, int index, size_t* length);
void on1x_new_list(On1x_State* state);
On1x_Status on1x_list_push(On1x_State* state, int list_index);
int on1x_list_get(On1x_State* state, int list_index, int element_index);
int on1x_len(const On1x_State* state, int index);
void on1x_new_table(On1x_State* state);
On1x_Status on1x_table_set(On1x_State* state, int table_index);
int on1x_table_get(On1x_State* state, int table_index);
On1x_Status on1x_tag_list(On1x_State* state, const char* tag, size_t length, int list_index);
int on1x_tag_of(On1x_State* state, int index);
int on1x_payload_of(On1x_State* state, int index);
On1x_Status on1x_push_some(On1x_State* state);
void on1x_push_none(On1x_State* state);
int on1x_is_some(const On1x_State* state, int index);
int on1x_is_none(const On1x_State* state, int index);
On1x_Status on1x_push_success(On1x_State* state);
On1x_Status on1x_push_error_result(On1x_State* state);
int on1x_is_success(const On1x_State* state, int index);
int on1x_is_error(const On1x_State* state, int index);
On1x_Status on1x_register(On1x_State* state, const char* name, On1x_CFn function);
On1x_Status on1x_call(On1x_State* state, int function_index, int argc);
On1x_Status on1x_error(On1x_State* state, const char* message);
On1x_Ref on1x_ref_create(On1x_State* state, int stack_index);
int on1x_ref_release(On1x_State* state, On1x_Ref reference);
On1x_Status on1x_ref_push(On1x_State* state, On1x_Ref reference);
void on1x_gc_collect(On1x_State* state);
size_t on1x_gc_bytes_allocated(const On1x_State* state);
int on1x_ffi_available(void);
On1x_Status on1x_ffi_open(On1x_State* state, const char* path, size_t length, On1x_FfiLibrary** library);
void on1x_ffi_close(On1x_FfiLibrary* library);
void* on1x_ffi_symbol(const On1x_FfiLibrary* library, const char* name, size_t length);
On1x_Status on1x_ffi_call(
    On1x_State* state, void* symbol, const char* signature, size_t signature_length, int argc);
const char* on1x_version_string(void);

#ifdef SWIGPYTHON
%newobject on1x_open;
%feature("autodoc", "1");
#endif

#if defined(SWIGON1X_XFEAT_PY_BUILTIN) && defined(SWIGPYTHON)
%feature("python:slot", "tp_repr", functype="reprfunc") On1x_State;
#endif

#if defined(SWIGON1X_XFEAT_EXCEPTION_MAPPING) && defined(SWIGPYTHON)
%exception {
  $action
  if (PyErr_Occurred()) SWIG_fail;
}
#endif
