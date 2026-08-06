#include "stdlib/os/os_impl.hpp"

#include "api/api_common.hpp"
#include "core/optional.hpp"
#include "core/string.hpp"
#include "core/table.hpp"
#include "gc/roots.hpp"
#include "runtime/state.hpp"
#include "stdlib/args.hpp"

#include <cstdlib>
#include <cstring>
#include <string>

 extern "C" char** environ;
 
namespace on1x::stdlib::os_detail {

static On1x_Status bad(On1x_State* s, const char* m) noexcept {
    return push_api_error(s, m);
}

On1x_Status os_getenv(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 1, "Os.GetEnv")) return ON1X_ERR;
    Value v;
    if (!read_argument(s, 1, v) || v.kind() != Value::Kind::String)
        return bad(s, "Os.GetEnv expects a String");
    auto text = string_view(as_string_const(v));
    std::string name(text);
    const char* val = std::getenv(name.c_str());
    if (!val) {
        return stack_push(s, make_none(&s->gc, s->reserved)) ? ON1X_OK : ON1X_ERR;
    }
    auto* str = new_string(&s->gc, val);
    if (!str) return bad(s, "Os.GetEnv: allocation failed");
    return stack_push(s, make_some(&s->gc, s->reserved, value_from_object(str))) ? ON1X_OK : ON1X_ERR;
}

On1x_Status os_setenv(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 2, "Os.SetEnv")) return ON1X_ERR;
    Value name_v, val_v;
    if (!read_argument(s, 1, name_v) || name_v.kind() != Value::Kind::String)
        return bad(s, "Os.SetEnv expects a String name");
    if (!read_argument(s, 2, val_v) || val_v.kind() != Value::Kind::String)
        return bad(s, "Os.SetEnv expects a String value");
    auto name = string_view(as_string_const(name_v));
    auto val = string_view(as_string_const(val_v));
    std::string n(name), v(val);
    if (setenv(n.c_str(), v.c_str(), 1) != 0) {
        return bad(s, "Os.SetEnv: setenv failed");
    }
    return stack_push(s, Value::unit()) ? ON1X_OK : ON1X_ERR;
}

On1x_Status os_unsetenv(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 1, "Os.UnsetEnv")) return ON1X_ERR;
    Value v;
    if (!read_argument(s, 1, v) || v.kind() != Value::Kind::String)
        return bad(s, "Os.UnsetEnv expects a String");
    auto text = string_view(as_string_const(v));
    std::string name(text);
    unsetenv(name.c_str());
    return stack_push(s, Value::unit()) ? ON1X_OK : ON1X_ERR;
}

On1x_Status os_envtable(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 0, "Os.EnvTable")) return ON1X_ERR;
    auto* table = new_table(&s->gc);
    if (!table) return bad(s, "Os.EnvTable: allocation failed");
    GcRoot table_root(table);
    if (environ) {
        for (char** env = environ; *env; ++env) {
            const char* eq = std::strchr(*env, '=');
            if (!eq) continue;
            std::string key(*env, static_cast<std::size_t>(eq - *env));
            std::string val(eq + 1);
            auto* key_str = new_string(&s->gc, key);
            auto* val_str = new_string(&s->gc, val);
            if (!key_str || !val_str) return bad(s, "Os.EnvTable: allocation failed");
            GcRoot kr(key_str), vr(val_str);
            if (!table_set(&s->gc, table, value_from_object(key_str), value_from_object(val_str)))
                return bad(s, "Os.EnvTable: table set failed");
        }
    }
    return stack_push(s, value_from_object(table)) ? ON1X_OK : ON1X_ERR;
}

}  // namespace on1x::stdlib::os_detail
