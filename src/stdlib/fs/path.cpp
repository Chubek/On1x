#include "api/api_common.hpp"
#include "core/list.hpp"
#include "core/string.hpp"
#include "gc/roots.hpp"
#include "on1x/on1x_capability.h"
#include "on1x/on1x_module.h"
#include "runtime/state.hpp"
#include "stdlib/args.hpp"
#include "stdlib/fs/fs.hpp"

#include <algorithm>
#include <cstring>
#include <string>
#include <vector>

namespace on1x::stdlib {
namespace {

On1x_Status bad(On1x_State* s, const char* m) noexcept {
    return push_api_error(s, m);
}

static std::string path_str(Value v) {
    return std::string(string_view(as_string_const(v)));
}

// Path.Join(parts..) -> :String (variadic)
On1x_Status path_join(On1x_State* s, int argc) noexcept {
    std::string result;
    for (int i = 1; i <= argc; ++i) {
        Value v;
        if (!read_argument(s, i, v) || v.kind() != Value::Kind::String)
            return bad(s, "Path.Join expects String arguments");
        std::string part = path_str(v);
        if (result.empty()) {
            result = part;
        } else {
            if (result.back() != '/') result.push_back('/');
            result += part;
        }
    }
    auto* str = new_string(&s->gc, result);
    if (!str) return bad(s, "Path.Join: allocation failed");
    return stack_push(s, value_from_object(str)) ? ON1X_OK : ON1X_ERR;
}

// Path.Dirname(p) -> :String
On1x_Status path_dirname(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 1, "Path.Dirname")) return ON1X_ERR;
    Value v;
    if (!read_argument(s, 1, v) || v.kind() != Value::Kind::String)
        return bad(s, "Path.Dirname expects a String");
    std::string p = path_str(v);
    auto pos = p.find_last_of('/');
    if (pos == std::string::npos) {
        auto* str = new_string(&s->gc, ".");
        if (!str) return bad(s, "Path.Dirname: allocation failed");
        return stack_push(s, value_from_object(str)) ? ON1X_OK : ON1X_ERR;
    }
    if (pos == 0) {
        auto* str = new_string(&s->gc, "/");
        if (!str) return bad(s, "Path.Dirname: allocation failed");
        return stack_push(s, value_from_object(str)) ? ON1X_OK : ON1X_ERR;
    }
    std::string dir = p.substr(0, pos);
    auto* str = new_string(&s->gc, dir);
    if (!str) return bad(s, "Path.Dirname: allocation failed");
    return stack_push(s, value_from_object(str)) ? ON1X_OK : ON1X_ERR;
}

// Path.Basename(p) -> :String
On1x_Status path_basename(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 1, "Path.Basename")) return ON1X_ERR;
    Value v;
    if (!read_argument(s, 1, v) || v.kind() != Value::Kind::String)
        return bad(s, "Path.Basename expects a String");
    std::string p = path_str(v);
    auto pos = p.find_last_of('/');
    std::string base = (pos == std::string::npos) ? p : p.substr(pos + 1);
    auto* str = new_string(&s->gc, base);
    if (!str) return bad(s, "Path.Basename: allocation failed");
    return stack_push(s, value_from_object(str)) ? ON1X_OK : ON1X_ERR;
}

// Path.Extension(p) -> :String (including the dot, or empty)
On1x_Status path_extension(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 1, "Path.Extension")) return ON1X_ERR;
    Value v;
    if (!read_argument(s, 1, v) || v.kind() != Value::Kind::String)
        return bad(s, "Path.Extension expects a String");
    std::string p = path_str(v);
    auto slash_pos = p.find_last_of('/');
    std::string filename = (slash_pos == std::string::npos) ? p : p.substr(slash_pos + 1);
    auto dot_pos = filename.find_last_of('.');
    if (dot_pos == std::string::npos || dot_pos == 0) {
        auto* str = new_string(&s->gc, "");
        if (!str) return bad(s, "Path.Extension: allocation failed");
        return stack_push(s, value_from_object(str)) ? ON1X_OK : ON1X_ERR;
    }
    std::string ext = filename.substr(dot_pos);
    auto* str = new_string(&s->gc, ext);
    if (!str) return bad(s, "Path.Extension: allocation failed");
    return stack_push(s, value_from_object(str)) ? ON1X_OK : ON1X_ERR;
}

// Path.Normalize(p) -> :String
// Removes redundant slashes, resolves '.' and '..'
On1x_Status path_normalize(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 1, "Path.Normalize")) return ON1X_ERR;
    Value v;
    if (!read_argument(s, 1, v) || v.kind() != Value::Kind::String)
        return bad(s, "Path.Normalize expects a String");
    std::string p = path_str(v);
    bool is_absolute = !p.empty() && p[0] == '/';
    // Split by '/'
    std::vector<std::string> parts;
    std::size_t start = 0;
    while (start < p.size()) {
        auto end = p.find('/', start);
        if (end == std::string::npos) end = p.size();
        std::string part = p.substr(start, end - start);
        start = end + 1;
        if (part.empty() || part == ".") continue;
        if (part == "..") {
            if (!parts.empty() && parts.back() != "..") {
                parts.pop_back();
            } else if (!is_absolute) {
                parts.push_back("..");
            }
        } else {
            parts.push_back(part);
        }
    }
    std::string result = is_absolute ? "/" : "";
    for (std::size_t i = 0; i < parts.size(); ++i) {
        if (i > 0) result += "/";
        result += parts[i];
    }
    if (result.empty() && !is_absolute) result = ".";
    auto* str = new_string(&s->gc, result);
    if (!str) return bad(s, "Path.Normalize: allocation failed");
    return stack_push(s, value_from_object(str)) ? ON1X_OK : ON1X_ERR;
}

// Path.IsAbsolute(p) -> :Bool
On1x_Status path_isabsolute(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 1, "Path.IsAbsolute")) return ON1X_ERR;
    Value v;
    if (!read_argument(s, 1, v) || v.kind() != Value::Kind::String)
        return bad(s, "Path.IsAbsolute expects a String");
    std::string p = path_str(v);
    return stack_push(s, Value::boolean(!p.empty() && p[0] == '/')) ? ON1X_OK : ON1X_ERR;
}

const On1x_FnDesc path_fns[] = {
    {"Join",       path_join},
    {"Dirname",    path_dirname},
    {"Basename",   path_basename},
    {"Extension",  path_extension},
    {"Normalize",  path_normalize},
    {"IsAbsolute", path_isabsolute},
};

const On1x_ModuleDesc path_desc{"Path", ON1X_CAP_NONE, path_fns, sizeof(path_fns) / sizeof(*path_fns)};

}  // namespace

const On1x_ModuleDesc* path_module() noexcept { return &path_desc; }

}  // namespace on1x::stdlib
