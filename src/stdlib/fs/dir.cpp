#include "stdlib/fs/fs_impl.hpp"

#include "api/api_common.hpp"
#include "core/list.hpp"
#include "core/optional.hpp"
#include "core/string.hpp"
#include "gc/roots.hpp"
#include "runtime/state.hpp"
#include "stdlib/args.hpp"

#include <cstring>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>

namespace on1x::stdlib::fs_detail {

static On1x_Status bad(On1x_State* s, const char* m) noexcept {
    return push_api_error(s, m);
}

// ListDir(path) -> :List of :String, or raises
On1x_Status fs_listdir(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 1, "Fs.ListDir")) return ON1X_ERR;
    Value v;
    if (!read_argument(s, 1, v) || v.kind() != Value::Kind::String)
        return bad(s, "Fs.ListDir expects a String path");
    auto text = string_view(as_string_const(v));
    std::string path(text);
    DIR* dir = opendir(path.c_str());
    if (!dir) {
        return bad(s, "Fs.ListDir: opendir failed");
    }
    auto* list = new_list(&s->gc, 0);
    if (!list) { closedir(dir); return bad(s, "Fs.ListDir: allocation failed"); }
    GcRoot list_root(list);
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (std::strcmp(entry->d_name, ".") == 0 || std::strcmp(entry->d_name, "..") == 0)
            continue;
        auto* name_str = new_string(&s->gc, entry->d_name);
        if (!name_str) { closedir(dir); return bad(s, "Fs.ListDir: allocation failed"); }
        GcRoot name_root(name_str);
        list_push(&s->gc, list, value_from_object(name_str));
    }
    closedir(dir);
    return stack_push(s, value_from_object(list)) ? ON1X_OK : ON1X_ERR;
}

// MakeDir(path) -> :Unit or raises
On1x_Status fs_makedir(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 1, "Fs.MakeDir")) return ON1X_ERR;
    Value v;
    if (!read_argument(s, 1, v) || v.kind() != Value::Kind::String)
        return bad(s, "Fs.MakeDir expects a String path");
    auto text = string_view(as_string_const(v));
    std::string path(text);
    if (mkdir(path.c_str(), 0755) != 0) {
        return bad(s, "Fs.MakeDir: mkdir failed");
    }
    return stack_push(s, Value::unit()) ? ON1X_OK : ON1X_ERR;
}

// MakeDirAll(path) -> :Unit or raises (mkdir -p equivalent)
On1x_Status fs_makedir_all(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 1, "Fs.MakeDirAll")) return ON1X_ERR;
    Value v;
    if (!read_argument(s, 1, v) || v.kind() != Value::Kind::String)
        return bad(s, "Fs.MakeDirAll expects a String path");
    auto text = string_view(as_string_const(v));
    std::string path(text);
    // Simple recursive mkdir
    std::string current;
    for (std::size_t i = 0; i < path.size(); ++i) {
        current.push_back(path[i]);
        if (path[i] == '/' || i == path.size() - 1) {
            if (!current.empty() && current != "/") {
                // Remove trailing slash for mkdir check
                std::string dir = current;
                if (dir.back() == '/') dir.pop_back();
                if (!dir.empty()) {
                    struct stat st;
                    if (stat(dir.c_str(), &st) != 0) {
                        if (mkdir(dir.c_str(), 0755) != 0 && errno != EEXIST) {
                            return bad(s, "Fs.MakeDirAll: mkdir failed");
                        }
                    }
                }
            }
        }
    }
    return stack_push(s, Value::unit()) ? ON1X_OK : ON1X_ERR;
}

// RemoveDir(path) -> :Unit or raises
On1x_Status fs_removedir(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 1, "Fs.RemoveDir")) return ON1X_ERR;
    Value v;
    if (!read_argument(s, 1, v) || v.kind() != Value::Kind::String)
        return bad(s, "Fs.RemoveDir expects a String path");
    auto text = string_view(as_string_const(v));
    std::string path(text);
    if (rmdir(path.c_str()) != 0) {
        return bad(s, "Fs.RemoveDir: rmdir failed");
    }
    return stack_push(s, Value::unit()) ? ON1X_OK : ON1X_ERR;
}

// IsDir(path) -> :Bool
On1x_Status fs_isdir(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 1, "Fs.IsDir")) return ON1X_ERR;
    Value v;
    if (!read_argument(s, 1, v) || v.kind() != Value::Kind::String)
        return bad(s, "Fs.IsDir expects a String path");
    auto text = string_view(as_string_const(v));
    std::string path(text);
    struct stat st;
    if (stat(path.c_str(), &st) != 0) return stack_push(s, Value::boolean(false)) ? ON1X_OK : ON1X_ERR;
    return stack_push(s, Value::boolean(S_ISDIR(st.st_mode))) ? ON1X_OK : ON1X_ERR;
}

// IsFile(path) -> :Bool
On1x_Status fs_isfile(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 1, "Fs.IsFile")) return ON1X_ERR;
    Value v;
    if (!read_argument(s, 1, v) || v.kind() != Value::Kind::String)
        return bad(s, "Fs.IsFile expects a String path");
    auto text = string_view(as_string_const(v));
    std::string path(text);
    struct stat st;
    if (stat(path.c_str(), &st) != 0) return stack_push(s, Value::boolean(false)) ? ON1X_OK : ON1X_ERR;
    return stack_push(s, Value::boolean(S_ISREG(st.st_mode))) ? ON1X_OK : ON1X_ERR;
}

}  // namespace on1x::stdlib::fs_detail
