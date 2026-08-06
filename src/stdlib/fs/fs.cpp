#include "api/api_common.hpp"
#include "core/list.hpp"
#include "core/optional.hpp"
#include "core/string.hpp"
#include "core/table.hpp"
#include "core/tag_table.hpp"
#include "gc/roots.hpp"
#include "on1x/on1x_capability.h"
#include "on1x/on1x_module.h"
#include "runtime/state.hpp"
#include "stdlib/args.hpp"
#include "stdlib/fs/fs.hpp"
#include "stdlib/fs/fs_impl.hpp"

#include <cstdio>
#include <cstring>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <fstream>
#include <sstream>

namespace on1x::stdlib {
namespace {

On1x_Status bad(On1x_State* s, const char* m) noexcept {
    return push_api_error(s, m);
}

// --- Fs module (capability: ON1X_CAP_FS) ---

// ReadFile(path) -> raises on failure (use '~')
On1x_Status fs_readfile(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 1, "Fs.ReadFile")) return ON1X_ERR;
    Value v;
    if (!read_argument(s, 1, v) || v.kind() != Value::Kind::String)
        return bad(s, "Fs.ReadFile expects a String path");
    auto text = string_view(as_string_const(v));
    std::string path(text);
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return bad(s, "Fs.ReadFile: cannot open file");
    }
    std::ostringstream buf;
    buf << file.rdbuf();
    std::string content = buf.str();
    auto* str = new_string(&s->gc, content);
    if (!str) return bad(s, "Fs.ReadFile: allocation failed");
    return stack_push(s, value_from_object(str)) ? ON1X_OK : ON1X_ERR;
}

// WriteFile(path, contents) -> :Unit or raises
On1x_Status fs_writefile(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 2, "Fs.WriteFile")) return ON1X_ERR;
    Value pv, cv;
    if (!read_argument(s, 1, pv) || pv.kind() != Value::Kind::String)
        return bad(s, "Fs.WriteFile expects a String path");
    if (!read_argument(s, 2, cv) || cv.kind() != Value::Kind::String)
        return bad(s, "Fs.WriteFile expects String contents");
    auto path_text = string_view(as_string_const(pv));
    auto content_text = string_view(as_string_const(cv));
    std::ofstream file(std::string(path_text), std::ios::binary | std::ios::trunc);
    if (!file) {
        return bad(s, "Fs.WriteFile: cannot open file for writing");
    }
    file.write(content_text.data(), static_cast<std::streamsize>(content_text.size()));
    if (!file) {
        return bad(s, "Fs.WriteFile: write failed");
    }
    return stack_push(s, Value::unit()) ? ON1X_OK : ON1X_ERR;
}

// AppendFile(path, contents) -> :Unit or raises
On1x_Status fs_appendfile(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 2, "Fs.AppendFile")) return ON1X_ERR;
    Value pv, cv;
    if (!read_argument(s, 1, pv) || pv.kind() != Value::Kind::String)
        return bad(s, "Fs.AppendFile expects a String path");
    if (!read_argument(s, 2, cv) || cv.kind() != Value::Kind::String)
        return bad(s, "Fs.AppendFile expects String contents");
    auto path_text = string_view(as_string_const(pv));
    auto content_text = string_view(as_string_const(cv));
    std::ofstream file(std::string(path_text), std::ios::binary | std::ios::app);
    if (!file) {
        return bad(s, "Fs.AppendFile: cannot open file for appending");
    }
    file.write(content_text.data(), static_cast<std::streamsize>(content_text.size()));
    if (!file) {
        return bad(s, "Fs.AppendFile: write failed");
    }
    return stack_push(s, Value::unit()) ? ON1X_OK : ON1X_ERR;
}

// Exists(path) -> :Bool
On1x_Status fs_exists(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 1, "Fs.Exists")) return ON1X_ERR;
    Value v;
    if (!read_argument(s, 1, v) || v.kind() != Value::Kind::String)
        return bad(s, "Fs.Exists expects a String path");
    auto text = string_view(as_string_const(v));
    std::string path(text);
    return stack_push(s, Value::boolean(access(path.c_str(), F_OK) == 0)) ? ON1X_OK : ON1X_ERR;
}

// Remove(path) -> :Unit or raises
On1x_Status fs_remove(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 1, "Fs.Remove")) return ON1X_ERR;
    Value v;
    if (!read_argument(s, 1, v) || v.kind() != Value::Kind::String)
        return bad(s, "Fs.Remove expects a String path");
    auto text = string_view(as_string_const(v));
    std::string path(text);
    if (unlink(path.c_str()) != 0) {
        return bad(s, "Fs.Remove: unlink failed");
    }
    return stack_push(s, Value::unit()) ? ON1X_OK : ON1X_ERR;
}

const On1x_FnDesc fs_fns[] = {
    {"ReadFile",   fs_readfile},
    {"WriteFile",  fs_writefile},
    {"AppendFile", fs_appendfile},
    {"Exists",     fs_exists},
    {"Remove",     fs_remove},
    {"ListDir",    fs_detail::fs_listdir},
    {"MakeDir",    fs_detail::fs_makedir},
    {"MakeDirAll", fs_detail::fs_makedir_all},
    {"RemoveDir",  fs_detail::fs_removedir},
    {"IsDir",      fs_detail::fs_isdir},
    {"IsFile",     fs_detail::fs_isfile},
    {"Size",       fs_detail::fs_size},
    {"Metadata",   fs_detail::fs_metadata},
};

const On1x_ModuleDesc fs_desc{"Fs", ON1X_CAP_FS, fs_fns, sizeof(fs_fns) / sizeof(*fs_fns)};

}  // namespace

const On1x_ModuleDesc* fs_module() noexcept { return &fs_desc; }

}  // namespace on1x::stdlib
