#include "tools/host_prelude.hpp"

#include "core/tostring.hpp"
#include "core/value.hpp"
#include "runtime/state.hpp"
#include "stdlib/io/io.hpp"

#include <on1x/on1x_capability.h>
#include <on1x/on1x_stdlib.h>
#include <on1x/on1x_config.h>

#include <cstdio>
#include <string>

namespace on1x::tools {

On1x_Status install_host_prelude(On1x_State* state) noexcept {
    if (!state) return ON1X_ERR;

#if !ON1X_STDLIB_PURE_ONLY
    // Io.Print / Io.Show / Io.Repr are the sole Print implementations.
    // The host prelude never carries a duplicate; it merely ensures Io is
    // present so that tooling (REPL, dump, CLI) and user code can call them.
    if (on1x_has_capability(state, ON1X_CAP_IO)) {
        On1x_Status st = on1x_open_io(state);
        if (st != ON1X_OK) return st;
    }
#endif // !ON1X_STDLIB_PURE_ONLY

    return ON1X_OK;
}

std::string render_value(On1x_State* state, int stack_index) noexcept {
    if (!state) return {};
    Value value;
    if (!stack_at(state, stack_index, value)) return {};
    return to_string(value);
}

On1x_Status print_value(On1x_State* state, int stack_index, FILE* out) noexcept {
    if (!state || !out) return ON1X_ERR;
    Value value;
    if (!stack_at(state, stack_index, value)) return ON1X_ERR;
    std::string text = to_string(value);
    std::fwrite(text.data(), 1, text.size(), out);
    return ON1X_OK;
}

}  // namespace on1x::tools
