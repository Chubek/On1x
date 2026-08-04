#include <on1x/on1x_ffi.h>

#include "api/api_common.hpp"

#if defined(ON1X_ENABLE_FFI)
#include "core/optional.hpp"
#include "ffi/dynalo_loader.hpp"
#include "ffi/ffi.hpp"
#include "ffi/signature.hpp"

#include <new>
#include <string_view>
#endif

#if defined(ON1X_ENABLE_FFI)
struct On1x_FfiLibrary {
    on1x::ffi::DynamicLibrary library;
};
#endif

extern "C" int on1x_ffi_available(void) {
#if defined(ON1X_ENABLE_FFI)
    return 1;
#else
    return 0;
#endif
}

extern "C" On1x_Status on1x_ffi_open(
    On1x_State* state, const char* path, size_t length, On1x_FfiLibrary** library) {
    if (!state || !library || !path) return on1x::push_api_error(state, "FFI.Open expects a path and output handle");
    *library = nullptr;
#if defined(ON1X_ENABLE_FFI)
    try {
        auto* result = new On1x_FfiLibrary;
        if (!result->library.open(std::string_view(path, length))) {
            delete result;
            return on1x::push_api_error(state, "FFI.Open could not load the requested library");
        }
        *library = result;
        return ON1X_OK;
    } catch (...) {
        return on1x::push_api_error(state, "FFI.Open failed");
    }
#else
    static_cast<void>(length);
    return on1x::push_api_error(state, "FFI is disabled; rebuild with ON1X_ENABLE_FFI");
#endif
}

extern "C" void on1x_ffi_close(On1x_FfiLibrary* library) {
#if defined(ON1X_ENABLE_FFI)
    delete library;
#else
    static_cast<void>(library);
#endif
}

extern "C" void* on1x_ffi_symbol(const On1x_FfiLibrary* library, const char* name, size_t length) {
#if defined(ON1X_ENABLE_FFI)
    if (!library || !name) return nullptr;
    return library->library.find(std::string_view(name, length));
#else
    static_cast<void>(library);
    static_cast<void>(name);
    static_cast<void>(length);
    return nullptr;
#endif
}

extern "C" On1x_Status on1x_ffi_call(
    On1x_State* state, void* symbol, const char* signature_text, size_t signature_length, int argc) {
    if (!state || !symbol || !signature_text || argc < 0 ||
        static_cast<std::size_t>(argc) > on1x::visible_stack_size(state)) {
        return on1x::push_api_error(state, "FFI.Call expects a symbol, signature, and stack arguments");
    }
#if defined(ON1X_ENABLE_FFI)
    const std::size_t argument_base = state->top - static_cast<std::size_t>(argc);
    const auto fail = [state, argument_base](const char* message) {
        state->top = argument_base;
        return on1x::push_api_error(state, message);
    };
    const std::string_view text(signature_text, signature_length);
    on1x::ffi::Signature signature;
    if (!on1x::ffi::parse_signature(text, signature)) {
        state->top -= static_cast<std::size_t>(argc);
        try {
            return on1x::stack_push(state, on1x::make_none(&state->gc, state->reserved))
                ? ON1X_OK
                : on1x::push_api_error(state, "FFI.Call could not push None");
        } catch (...) {
            return on1x::push_api_error(state, "FFI.Call could not create None");
        }
    }
    if (signature.argument_count != static_cast<std::size_t>(argc)) {
        return fail("FFI.Call argument count does not match its signature");
    }

    on1x::ffi::FfiContext context;
    on1x::Value result;
    if (!context.call(&state->gc, symbol, text, state->stack + argument_base, static_cast<std::size_t>(argc), result)) {
        return fail("FFI.Call argument types do not match its signature");
    }
    state->top = argument_base;
    return on1x::stack_push(state, result)
        ? ON1X_OK
        : on1x::push_api_error(state, "FFI.Call could not push its result");
#else
    static_cast<void>(signature_length);
    return on1x::push_api_error(state, "FFI is disabled; rebuild with ON1X_ENABLE_FFI");
#endif
}
