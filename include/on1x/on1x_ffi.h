#pragma once

#include "on1x_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Host FFI is disabled by default because loading arbitrary libraries and
 * calling arbitrary symbols executes native code. Enable it explicitly with
 * ON1X_ENABLE_FFI.
 */
ON1X_API int on1x_ffi_available(void);
ON1X_API On1x_Status on1x_ffi_open(
    On1x_State* state, const char* path, size_t length, On1x_FfiLibrary** library);
ON1X_API void on1x_ffi_close(On1x_FfiLibrary* library);
ON1X_API void* on1x_ffi_symbol(const On1x_FfiLibrary* library, const char* name, size_t length);

/*
 * Calls a symbol with the argc values on top of state. The restricted dyncall
 * signature is "<args>)<result>", using B (Bool), l (Int64), d (Float64),
 * p (null or String pointer argument), and v (void result). Pointer returns
 * are not representable in the 0.1 C value surface and are rejected. It consumes argc values
 * and pushes exactly one result. A malformed signature pushes :None and
 * returns ON1X_OK; type/arity misuse and native-call failure return ON1X_ERR
 * with :Error on top.
 */
ON1X_API On1x_Status on1x_ffi_call(
    On1x_State* state, void* symbol, const char* signature, size_t signature_length, int argc);

#ifdef __cplusplus
}
#endif
