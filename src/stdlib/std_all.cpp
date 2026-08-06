#include "stdlib/registry.hpp"
#include "stdlib/module.hpp"
#include <on1x/on1x_config.h>

#include "stdlib/fs/fs.hpp"

#if !ON1X_STDLIB_PURE_ONLY
#include "stdlib/io/io.hpp"
#include "stdlib/os/os.hpp"
#include "stdlib/time/time.hpp"
#endif

#if ON1X_ENABLE_FFI && !ON1X_STDLIB_PURE_ONLY
#include "stdlib/dl/dl.hpp"
#endif

#if ON1X_ENABLE_JIT && ON1X_ENABLE_ASMTK && !ON1X_STDLIB_PURE_ONLY
#include "stdlib/asm/asm.hpp"
#endif

extern "C" On1x_Status on1x_open_std(On1x_State* state) {
    return on1x::stdlib::install_pure_modules(state) ? ON1X_OK : ON1X_ERR;
}

#if !ON1X_STDLIB_PURE_ONLY
extern "C" On1x_Status on1x_open_io(On1x_State* state) {
    const auto* mod = on1x::stdlib::io_module();
    if (!mod) return ON1X_ERR;
    return on1x::stdlib::install_module(state, *mod) ? ON1X_OK : ON1X_ERR;
}

extern "C" On1x_Status on1x_open_os(On1x_State* state) {
    const auto* mod = on1x::stdlib::os_module();
    if (!mod) return ON1X_ERR;
    return on1x::stdlib::install_module(state, *mod) ? ON1X_OK : ON1X_ERR;
}
 
extern "C" On1x_Status on1x_open_time(On1x_State* state) {
    const auto* mod = on1x::stdlib::time_module();
    if (!mod) return ON1X_ERR;
    return on1x::stdlib::install_module(state, *mod) ? ON1X_OK : ON1X_ERR;
}
 
 extern "C" On1x_Status on1x_open_fs(On1x_State* state) {
     const auto* mod = on1x::stdlib::fs_module();
     if (!mod) return ON1X_ERR;
     return on1x::stdlib::install_module(state, *mod) ? ON1X_OK : ON1X_ERR;
 }
#endif // !ON1X_STDLIB_PURE_ONLY

#if ON1X_ENABLE_FFI && !ON1X_STDLIB_PURE_ONLY
extern "C" On1x_Status on1x_open_dl(On1x_State* state) {
    const auto* mod = on1x::stdlib::dl_module();
    if (!mod) return ON1X_ERR;
    return on1x::stdlib::install_module(state, *mod) ? ON1X_OK : ON1X_ERR;
}
#endif

#if ON1X_ENABLE_JIT && ON1X_ENABLE_ASMTK && !ON1X_STDLIB_PURE_ONLY
extern "C" On1x_Status on1x_open_asm(On1x_State* state) {
    const auto* mod = on1x::stdlib::asm_module();
    if (!mod) return ON1X_ERR;
    return on1x::stdlib::install_module(state, *mod) ? ON1X_OK : ON1X_ERR;
}
#endif
