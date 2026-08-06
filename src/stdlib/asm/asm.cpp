#include "api/api_common.hpp"
#include "core/optional.hpp"
#include "core/string.hpp"
#include "gc/roots.hpp"
#include "on1x/on1x_capability.h"
#include "on1x/on1x_module.h"
#include "runtime/state.hpp"
#include "stdlib/args.hpp"
#include "stdlib/asm/asm.hpp"

#include <cstring>
#include <string>

// This module gates on ON1X_ENABLE_JIT and ON1X_ENABLE_ASMTK.
// Without the JIT infrastructure (src/jit/), it provides a stub
// that returns :None for Assemble operations.

namespace on1x::stdlib {
namespace {

On1x_Status bad(On1x_State* s, const char* m) noexcept {
    return push_api_error(s, m);
}

// Assemble(source) -> :Some[:Fn] | :None
// Stub: returns :None when JIT is not available.
On1x_Status asm_assemble(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 1, "Asm.Assemble")) return ON1X_ERR;
    Value v;
    if (!read_argument(s, 1, v) || v.kind() != Value::Kind::String)
        return bad(s, "Asm.Assemble expects a String");
    // JIT not yet implemented - return :None
    return stack_push(s, make_none(&s->gc, s->reserved)) ? ON1X_OK : ON1X_ERR;
}

const On1x_FnDesc fns[] = {
    {"Assemble", asm_assemble},
};

const On1x_ModuleDesc desc{"Asm", ON1X_CAP_DL, fns, sizeof(fns) / sizeof(*fns)};

}  // namespace

const On1x_ModuleDesc* asm_module() noexcept { return &desc; }

}  // namespace on1x::stdlib
