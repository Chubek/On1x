#pragma once

#include <QaMRpp.hpp>

// QaMRpp's stdlib load_* functions are declared in QaMRpp.hpp but the
// implementation .cpp files are missing from the vendored copy.  Provide
// empty inline stubs so the linker succeeds.  The On1x REPL does not
// evaluate Lua code through QaMRpp, so none of these stdlib modules are
// actually needed at runtime.

namespace qamrpp {
namespace stdlib {

inline void load_core(Context&) {}
inline void load_string(Context&) {}
inline void load_table(Context&) {}
inline void load_math(Context&) {}
inline void load_io(Context&) {}
inline void load_os(Context&) {}
inline void load_debug(Context&) {}
inline void load_coroutine(Context&) {}
inline void load_package(Context&) {}
inline void load_utf8(Context&) {}

} // namespace stdlib
} // namespace qamrpp
