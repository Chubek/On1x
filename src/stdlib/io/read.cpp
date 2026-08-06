#include "stdlib/io/io_impl.hpp"

#include "api/api_common.hpp"
#include "core/optional.hpp"
#include "core/string.hpp"
#include "gc/roots.hpp"
#include "runtime/state.hpp"
#include "stdlib/args.hpp"

#include <cstdio>
#include <string>

namespace on1x::stdlib::io_detail {

static On1x_Status bad(On1x_State* s, const char* m) noexcept {
    return push_api_error(s, m);
}

On1x_Status io_readline(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 0, "Io.ReadLine")) return ON1X_ERR;
    std::string line;
    int ch;
    while ((ch = std::fgetc(stdin)) != EOF) {
        if (ch == '\n') break;
        line.push_back(static_cast<char>(ch));
    }
    if (line.empty() && ch == EOF) {
        if (std::ferror(stdin)) return bad(s, "Io.ReadLine: read error");
        return stack_push(s, make_none(&s->gc, s->reserved)) ? ON1X_OK : ON1X_ERR;
    }
    auto* str = new_string(&s->gc, line);
    if (!str) return bad(s, "Io.ReadLine: allocation failed");
    return stack_push(s, make_some(&s->gc, s->reserved, value_from_object(str))) ? ON1X_OK : ON1X_ERR;
}

On1x_Status io_readall(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 0, "Io.ReadAll")) return ON1X_ERR;
    std::string content;
    char buf[4096];
    std::size_t n;
    bool any = false;
    while ((n = std::fread(buf, 1, sizeof(buf), stdin)) > 0) {
        content.append(buf, n);
        any = true;
    }
    if (std::ferror(stdin)) return bad(s, "Io.ReadAll: read error");
    if (!any && std::feof(stdin)) {
        return stack_push(s, make_none(&s->gc, s->reserved)) ? ON1X_OK : ON1X_ERR;
    }
    auto* str = new_string(&s->gc, content);
    if (!str) return bad(s, "Io.ReadAll: allocation failed");
    return stack_push(s, make_some(&s->gc, s->reserved, value_from_object(str))) ? ON1X_OK : ON1X_ERR;
}

}  // namespace on1x::stdlib::io_detail
