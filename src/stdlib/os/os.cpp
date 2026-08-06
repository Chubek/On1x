#include "api/api_common.hpp"
#include "core/list.hpp"
#include "core/optional.hpp"
#include "core/string.hpp"
#include "core/tag_table.hpp"
#include "gc/roots.hpp"
#include "on1x/on1x_capability.h"
#include "on1x/on1x_module.h"
#include "runtime/state.hpp"
#include "stdlib/args.hpp"
#include "stdlib/os/os.hpp"
#include "stdlib/os/os_impl.hpp"

#include <cstdlib>
#include <cstring>
#include <string>
#include <unistd.h>
#include <climits>

namespace on1x::stdlib {
namespace {

On1x_Status bad(On1x_State* s, const char* m) noexcept {
    return push_api_error(s, m);
}

static Value make_tag(On1x_State* s, const char* name) {
    return value_from_object(s->tags.intern(&s->gc, name));
}

On1x_Status os_platform(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 0, "Os.Platform")) return ON1X_ERR;
    const char* tag_name = "Unknown";
#if defined(__linux__)
    tag_name = "Linux";
#elif defined(__APPLE__) || defined(__MACH__)
    tag_name = "Darwin";
#elif defined(_WIN32)
    tag_name = "Windows";
#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__) || defined(__DragonFly__)
    tag_name = "Bsd";
#endif
    return stack_push(s, make_tag(s, tag_name)) ? ON1X_OK : ON1X_ERR;
}

On1x_Status os_arch(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 0, "Os.Arch")) return ON1X_ERR;
    const char* tag_name = "Unknown";
#if defined(__x86_64__) || defined(_M_X64)
    tag_name = "X86_64";
#elif defined(__aarch64__) || defined(_M_ARM64)
    tag_name = "Aarch64";
#endif
    return stack_push(s, make_tag(s, tag_name)) ? ON1X_OK : ON1X_ERR;
}

On1x_Status os_hostname(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 0, "Os.Hostname")) return ON1X_ERR;
    char buf[256];
    if (gethostname(buf, sizeof(buf)) != 0) {
        return bad(s, "Os.Hostname: gethostname failed");
    }
    buf[sizeof(buf) - 1] = '\0';
    auto* str = new_string(&s->gc, buf);
    if (!str) return bad(s, "Os.Hostname: allocation failed");
    return stack_push(s, value_from_object(str)) ? ON1X_OK : ON1X_ERR;
}

On1x_Status os_cwd(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 0, "Os.Cwd")) return ON1X_ERR;
    char* cwd = getcwd(nullptr, 0);
    if (!cwd) {
        return stack_push(s, make_none(&s->gc, s->reserved)) ? ON1X_OK : ON1X_ERR;
    }
    auto* str = new_string(&s->gc, cwd);
    std::free(cwd);
    if (!str) return bad(s, "Os.Cwd: allocation failed");
    return stack_push(s, make_some(&s->gc, s->reserved, value_from_object(str))) ? ON1X_OK : ON1X_ERR;
}

On1x_Status os_chdir(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 1, "Os.Chdir")) return ON1X_ERR;
    Value v;
    if (!read_argument(s, 1, v) || v.kind() != Value::Kind::String)
        return bad(s, "Os.Chdir expects a String");
    auto text = string_view(as_string_const(v));
    std::string path(text);
    if (chdir(path.c_str()) != 0) {
        return bad(s, "Os.Chdir: chdir failed");
    }
    return stack_push(s, Value::unit()) ? ON1X_OK : ON1X_ERR;
}

On1x_Status os_args(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 0, "Os.Args")) return ON1X_ERR;
    auto* list = new_list(&s->gc, 0);
    if (!list) return bad(s, "Os.Args: allocation failed");
    return stack_push(s, value_from_object(list)) ? ON1X_OK : ON1X_ERR;
}

const On1x_FnDesc fns[] = {
    {"Platform", os_platform},
    {"Arch",     os_arch},
    {"Hostname", os_hostname},
    {"Cwd",      os_cwd},
    {"Chdir",    os_chdir},
    {"Args",     os_args},
    {"GetEnv",   os_detail::os_getenv},
    {"SetEnv",   os_detail::os_setenv},
    {"UnsetEnv", os_detail::os_unsetenv},
    {"EnvTable", os_detail::os_envtable},
};

const On1x_ModuleDesc desc{"Os", ON1X_CAP_ENV, fns, sizeof(fns) / sizeof(*fns)};

}  // namespace

const On1x_ModuleDesc* os_module() noexcept { return &desc; }

}  // namespace on1x::stdlib
