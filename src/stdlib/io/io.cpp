#include "api/api_common.hpp"
#include "core/string.hpp"
#include "core/tostring.hpp"
#include "gc/roots.hpp"
#include "on1x/on1x_capability.h"
#include "on1x/on1x_module.h"
#include "runtime/state.hpp"
#include "stdlib/args.hpp"
#include "stdlib/io/io.hpp"
#include "stdlib/io/io_impl.hpp"

#include <cstdio>
#include <cstring>
#include <string>

namespace on1x::stdlib {
namespace {

On1x_Status bad(On1x_State* s, const char* m) noexcept {
    return push_api_error(s, m);
}

static std::string print_str(Value value) {
    switch (value.kind()) {
    case Value::Kind::Unit:   return "";
    case Value::Kind::Bool:   return value.as_bool() ? "true" : "false";
    case Value::Kind::Int:    return std::to_string(value.as_int());
    case Value::Kind::Float:  return std::to_string(value.as_float());
    case Value::Kind::String: return std::string(string_view(as_string_const(value)));
    case Value::Kind::Tag:    return ":" + std::string(tag_text(as_tag_const(value)));
    case Value::Kind::Iota:   return "Iota";
    case Value::Kind::Function: return "<fn>";
    default: return to_string(value);
    }
}

static On1x_Status do_print(On1x_State* s, int argc, bool newline, FILE* out) noexcept {
    for (int i = 1; i <= argc; ++i) {
        Value v;
        if (!read_argument(s, i, v)) return bad(s, "Io.Print");
        if (i > 1) std::fputc(' ', out);
        std::string text = print_str(v);
        std::fwrite(text.data(), 1, text.size(), out);
    }
    if (newline) std::fputc('\n', out);
    std::fflush(out);
    return stack_push(s, Value::unit()) ? ON1X_OK : ON1X_ERR;
}

On1x_Status io_print(On1x_State* s, int argc) noexcept {
    return do_print(s, argc, false, stdout);
}

On1x_Status io_println(On1x_State* s, int argc) noexcept {
    return do_print(s, argc, true, stdout);
}

On1x_Status io_eprint(On1x_State* s, int argc) noexcept {
    return do_print(s, argc, false, stderr);
}

On1x_Status io_eprintln(On1x_State* s, int argc) noexcept {
    return do_print(s, argc, true, stderr);
}

const On1x_FnDesc fns[] = {
    {"Print",    io_print},
    {"Println",  io_println},
    {"EPrint",   io_eprint},
    {"EPrintln", io_eprintln},
    {"ReadLine", io_detail::io_readline},
    {"ReadAll",  io_detail::io_readall},
    {"Show",     io_detail::io_show},
    {"Repr",     io_detail::io_repr},
};

const On1x_ModuleDesc desc{"Io", ON1X_CAP_IO, fns, sizeof(fns) / sizeof(*fns)};

}  // namespace

const On1x_ModuleDesc* io_module() noexcept { return &desc; }

}  // namespace on1x::stdlib
