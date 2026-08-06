#include "stdlib/fs/fs_impl.hpp"

#include "api/api_common.hpp"
#include "core/optional.hpp"
#include "core/string.hpp"
#include "core/table.hpp"
#include "core/tag_table.hpp"
#include "gc/roots.hpp"
#include "runtime/state.hpp"
#include "stdlib/args.hpp"

#include <cstdint>
#include <cstring>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

namespace on1x::stdlib::fs_detail {

static On1x_Status bad(On1x_State* s, const char* m) noexcept {
    return push_api_error(s, m);
}

// Size(path) -> :Some[:Int] | :None
On1x_Status fs_size(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 1, "Fs.Size")) return ON1X_ERR;
    Value v;
    if (!read_argument(s, 1, v) || v.kind() != Value::Kind::String)
        return bad(s, "Fs.Size expects a String path");
    auto text = string_view(as_string_const(v));
    std::string path(text);
    struct stat st;
    if (stat(path.c_str(), &st) != 0) {
        return stack_push(s, make_none(&s->gc, s->reserved)) ? ON1X_OK : ON1X_ERR;
    }
    return stack_push(s, make_some(&s->gc, s->reserved,
        Value::integer(&s->gc, static_cast<std::int64_t>(st.st_size)))) ? ON1X_OK : ON1X_ERR;
}

// Metadata(path) -> :Some[:Table] | :None
// Table has :Size, :Mode, :ModTime (Unix seconds), :IsDir, :IsFile
On1x_Status fs_metadata(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 1, "Fs.Metadata")) return ON1X_ERR;
    Value v;
    if (!read_argument(s, 1, v) || v.kind() != Value::Kind::String)
        return bad(s, "Fs.Metadata expects a String path");
    auto text = string_view(as_string_const(v));
    std::string path(text);
    struct stat st;
    if (stat(path.c_str(), &st) != 0) {
        return stack_push(s, make_none(&s->gc, s->reserved)) ? ON1X_OK : ON1X_ERR;
    }
    auto* table = new_table(&s->gc);
    if (!table) return bad(s, "Fs.Metadata: allocation failed");
    GcRoot table_root(table);

    auto set_field = [&](const char* name, Value val) {
        auto* key_tag = s->tags.intern(&s->gc, name);
        GcRoot kr(key_tag);
        return table_set(&s->gc, table, value_from_object(key_tag), val);
    };

    if (!set_field("Size",    Value::integer(&s->gc, static_cast<std::int64_t>(st.st_size))) ||
        !set_field("Mode",    Value::integer(&s->gc, static_cast<std::int64_t>(st.st_mode & 0777))) ||
        !set_field("ModTime", Value::integer(&s->gc, static_cast<std::int64_t>(st.st_mtime))) ||
        !set_field("IsDir",   Value::boolean(S_ISDIR(st.st_mode))) ||
        !set_field("IsFile",  Value::boolean(S_ISREG(st.st_mode)))) {
        return bad(s, "Fs.Metadata: table set failed");
    }

    return stack_push(s, make_some(&s->gc, s->reserved, value_from_object(table))) ? ON1X_OK : ON1X_ERR;
}

}  // namespace on1x::stdlib::fs_detail
