#pragma once

#include <on1x/on1x.h>

#include <cstdio>
#include <string>

namespace on1x::tools {

// Install the host prelude: registers Io and any other modules that
// interactive tooling (CLI, REPL, dump tools) needs.  Call once per
// state, after `on1x_open_std`.
//
// The prelude itself contains no Print implementation — it ensures
// Io is installed so that `Io.Print` / `Io.Show` / `Io.Repr` are
// available to On1x code.
On1x_Status install_host_prelude(On1x_State* state) noexcept;

// Render the value at `stack_index` into a human-readable string using
// the same `to_string` engine that `Io.Show` wraps.  The value is not
// consumed; the caller is responsible for popping it separately.
//
// Returns an empty string on failure (e.g. invalid index).
std::string render_value(On1x_State* state, int stack_index) noexcept;

// Print the value at `stack_index` to `out` (typically stdout or stderr)
// using the same rendering as `render_value`.  Returns ON1X_OK on success.
On1x_Status print_value(On1x_State* state, int stack_index, FILE* out) noexcept;

}  // namespace on1x::tools
