%module On1x

%{
#include "on1x/on1x.h"
/* Pull in version macros for target-language constant generation */
#include "on1x/on1x_version.h"
%}

%include <stdint.i>

/* ------------------------------------------------------------------ */
/*  Opaque types                                                      */
/* ------------------------------------------------------------------ */
typedef struct On1x_State On1x_State;
typedef struct On1x_FfiLibrary On1x_FfiLibrary;
typedef uintptr_t On1x_Ref;
typedef unsigned int On1x_Capability;

/* ------------------------------------------------------------------ */
/*  Enums                                                             */
/* ------------------------------------------------------------------ */
typedef enum On1x_Type {
    ON1X_UNIT, ON1X_BOOL, ON1X_INT, ON1X_FLOAT, ON1X_STRING, ON1X_TAG,
    ON1X_LIST, ON1X_TABLE, ON1X_FN, ON1X_IOTA, ON1X_INVALID
} On1x_Type;

typedef enum On1x_Status { ON1X_OK = 0, ON1X_ERR = 1 } On1x_Status;

/* Capability bitset — mirrors include/on1x/on1x_capability.h */
enum {
    ON1X_CAP_NONE  = 0,
    ON1X_CAP_FS    = 1u << 0,
    ON1X_CAP_IO    = 1u << 1,
    ON1X_CAP_ENV   = 1u << 2,
    ON1X_CAP_TIME  = 1u << 3,
    ON1X_CAP_CLOCK = 1u << 4,
    ON1X_CAP_PROC  = 1u << 5,
    ON1X_CAP_NET   = 1u << 6,
    ON1X_CAP_DL    = 1u << 7,
};

/* ------------------------------------------------------------------ */
/*  Function pointer type                                             */
/* ------------------------------------------------------------------ */
typedef On1x_Status (*On1x_CFn)(On1x_State* state, int argc);

/* ------------------------------------------------------------------ */
/*  Module descriptor structs (on1x_module.h)                         */
/* ------------------------------------------------------------------ */
typedef struct On1x_FnDesc {
    const char* name;
    On1x_CFn function;
} On1x_FnDesc;

typedef struct On1x_ModuleDesc {
    const char* name;
    On1x_Capability capability;
    const On1x_FnDesc* functions;
    size_t function_count;
} On1x_ModuleDesc;

/* ------------------------------------------------------------------ */
/*  Version macros (on1x_version.h)                                   */
/* ------------------------------------------------------------------ */
#define ON1X_VERSION_MAJOR 0
#define ON1X_VERSION_MINOR 1
#define ON1X_VERSION_PATCH 0

/* ================================================================== */
/*  C API — every public entry point in include/on1x/*.h              */
/* ================================================================== */

/* --- State lifecycle (on1x_state.h) --- */
On1x_State* on1x_open(void);
void on1x_close(On1x_State* state);
On1x_Status on1x_eval(On1x_State* state, const char* source, size_t length, const char* chunk_name);

/* --- Stack (on1x_stack.h) --- */
int on1x_top(const On1x_State* state);
int on1x_pop(On1x_State* state, int count);
On1x_Status on1x_dup(On1x_State* state, int index);
On1x_Type on1x_type(const On1x_State* state, int index);

/* --- Value push/access (on1x_value.h) --- */
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

/* --- List (on1x_list.h) --- */
void on1x_new_list(On1x_State* state);
On1x_Status on1x_list_push(On1x_State* state, int list_index);
int on1x_list_get(On1x_State* state, int list_index, int element_index);
int on1x_len(const On1x_State* state, int index);

/* --- Table (on1x_table.h) --- */
void on1x_new_table(On1x_State* state);
On1x_Status on1x_table_set(On1x_State* state, int table_index);
int on1x_table_get(On1x_State* state, int table_index);

/* --- Tagged Lists (on1x_tag.h) --- */
On1x_Status on1x_tag_list(On1x_State* state, const char* tag, size_t length, int list_index);
int on1x_tag_of(On1x_State* state, int index);
int on1x_payload_of(On1x_State* state, int index);

/* --- Optionals (on1x_optional.h) --- */
On1x_Status on1x_push_some(On1x_State* state);
void on1x_push_none(On1x_State* state);
int on1x_is_some(const On1x_State* state, int index);
int on1x_is_none(const On1x_State* state, int index);

/* --- Results (on1x_result.h) --- */
On1x_Status on1x_push_success(On1x_State* state);
On1x_Status on1x_push_error_result(On1x_State* state);
int on1x_is_success(const On1x_State* state, int index);
int on1x_is_error(const On1x_State* state, int index);

/* --- Call / register (on1x_call.h) --- */
On1x_Status on1x_register(On1x_State* state, const char* name, On1x_CFn function);
On1x_Status on1x_call(On1x_State* state, int function_index, int argc);

/* --- Error (on1x_error.h) --- */
On1x_Status on1x_error(On1x_State* state, const char* message);

/* --- GC references (on1x_gc.h) --- */
On1x_Ref on1x_ref_create(On1x_State* state, int stack_index);
int on1x_ref_release(On1x_State* state, On1x_Ref reference);
On1x_Status on1x_ref_push(On1x_State* state, On1x_Ref reference);
void on1x_gc_collect(On1x_State* state);
size_t on1x_gc_bytes_allocated(const On1x_State* state);

/* --- FFI (on1x_ffi.h) — gated behind ON1X_ENABLE_FFI at build time --- */
int on1x_ffi_available(void);
On1x_Status on1x_ffi_open(On1x_State* state, const char* path, size_t length, On1x_FfiLibrary** library);
void on1x_ffi_close(On1x_FfiLibrary* library);
void* on1x_ffi_symbol(const On1x_FfiLibrary* library, const char* name, size_t length);
On1x_Status on1x_ffi_call(On1x_State* state, void* symbol, const char* signature, size_t signature_length, int argc);

/* --- Capability management (on1x_capability.h) --- */
On1x_Status on1x_grant(On1x_State* state, On1x_Capability capability);
On1x_Status on1x_revoke(On1x_State* state, On1x_Capability capability);
int on1x_has_capability(const On1x_State* state, On1x_Capability capability);

/* --- Module installation (on1x_module.h) --- */
On1x_Status on1x_install_module(On1x_State* state, const On1x_ModuleDesc* module);

/* --- Stdlib loaders (on1x_stdlib.h) --- */
On1x_Status on1x_open_std(On1x_State* state);
On1x_Status on1x_open_io(On1x_State* state);
On1x_Status on1x_open_os(On1x_State* state);
On1x_Status on1x_open_time(On1x_State* state);
On1x_Status on1x_open_fs(On1x_State* state);
On1x_Status on1x_open_dl(On1x_State* state);
On1x_Status on1x_open_asm(On1x_State* state);

/* --- Version (on1x_version.h) --- */
const char* on1x_version_string(void);

/* ================================================================== */
/*  XFeats — target-language sugar gated by SWIGON1X_XFEAT_* defines  */
/* ================================================================== */

#ifdef SWIGPYTHON
/* Return a new object for on1x_open; enable docstrings on wrappers. */
%newobject on1x_open;
%feature("autodoc", "1");

/* Map capability constants into a Python IntEnum-style experience. */
%pythoncode %{
ON1X_CAP_NONE  = 0
ON1X_CAP_FS    = 1 << 0
ON1X_CAP_IO    = 1 << 1
ON1X_CAP_ENV   = 1 << 2
ON1X_CAP_TIME  = 1 << 3
ON1X_CAP_CLOCK = 1 << 4
ON1X_CAP_PROC  = 1 << 5
ON1X_CAP_NET   = 1 << 6
ON1X_CAP_DL    = 1 << 7

ON1X_VERSION = (ON1X_VERSION_MAJOR, ON1X_VERSION_MINOR, ON1X_VERSION_PATCH)
%}
#endif

/* ---- XFeat: py-builtin ---- */
#if defined(SWIGON1X_XFEAT_PY_BUILTIN) && defined(SWIGPYTHON)
%feature("python:slot", "tp_repr", functype="reprfunc") On1x_State;
/* Expose capability bitmask as a readable Python int with flag names. */
%pythoncode %{
def _capability_repr(caps):
    names = []
    for name, val in [("NONE",0),("FS",ON1X_CAP_FS),("IO",ON1X_CAP_IO),
                      ("ENV",ON1X_CAP_ENV),("TIME",ON1X_CAP_TIME),
                      ("CLOCK",ON1X_CAP_CLOCK),("PROC",ON1X_CAP_PROC),
                      ("NET",ON1X_CAP_NET),("DL",ON1X_CAP_DL)]:
        if caps & val:
            names.append(name)
    return " | ".join(names) if names else "NONE"
%}
#endif

/* ---- XFeat: py-context-manager ---- */
#if defined(SWIGON1X_XFEAT_PY_CONTEXT_MANAGER) && defined(SWIGPYTHON)
%extend On1x_State {
    %pythoncode %{
    def __enter__(self):
        return self
    def __exit__(self, exc_type, exc_val, exc_tb):
        on1x_close(self)
        return False
    %}
}
#endif

/* ---- XFeat: exception-mapping ---- */
#if defined(SWIGON1X_XFEAT_EXCEPTION_MAPPING) && defined(SWIGPYTHON)
%exception {
    $action
    if (PyErr_Occurred()) SWIG_fail;
}
#elif defined(SWIGON1X_XFEAT_EXCEPTION_MAPPING) && defined(SWIGJAVA)
%typemap(throws, throws="java.lang.RuntimeException") std::exception %{
    jclass clazz = jenv->FindClass("java/lang/RuntimeException");
    jenv->ThrowNew(clazz, $1.what());
    return $null;
%}
#elif defined(SWIGON1X_XFEAT_EXCEPTION_MAPPING) && defined(SWIGCSHARP)
%typemap(throws, canthrow=1) std::exception %{
    SWIG_CSharpSetPendingException(SWIG_CSharpApplicationException, $1.what());
    return $null;
%}
#elif defined(SWIGON1X_XFEAT_EXCEPTION_MAPPING) && defined(SWIGRUBY)
%exception {
    try {
        $action
    } catch (const std::exception& e) {
        rb_raise(rb_eRuntimeError, "%s", e.what());
    }
}
#endif

/* ---- XFeat: py-dunder-repr ---- */
#if defined(SWIGON1X_XFEAT_PY_DUNDER_REPR) && defined(SWIGPYTHON)
%rename(__repr__) on1x_version_string;
#endif

/* ---- XFeat: py-properties ---- */
#if defined(SWIGON1X_XFEAT_PY_PROPERTIES) && defined(SWIGPYTHON)
%pythoncode %{
class State:
    """High-level wrapper around an On1x_State pointer."""
    def __init__(self):
        self._state = on1x_open()
    def close(self):
        if self._state:
            on1x_close(self._state)
            self._state = None
    @property
    def top(self):
        return on1x_top(self._state)
    @property
    def version(self):
        return on1x_version_string()
    def __del__(self):
        self.close()
%}
#endif

/* ---- XFeat: go-error-returns ---- */
#if defined(SWIGON1X_XFEAT_GO_ERROR_RETURNS) && defined(SWIGGO)
%rename(opened) on1x_open;
%rename(closed) on1x_close;
%rename(evaluated) on1x_eval;
%rename(registered) on1x_register;
%rename(called) on1x_call;
%rename(granted) on1x_grant;
%rename(revoked) on1x_revoke;
#endif

/* ---- XFeat: std-container-sugar ---- */
#if defined(SWIGON1X_XFEAT_STD_CONTAINER_SUGAR) && defined(SWIGPYTHON)
%extend On1x_State {
    /* len(state) → on1x_top */
    %pythoncode %{
    def __len__(self):
        return on1x_top(self)
    %}
}
#endif
