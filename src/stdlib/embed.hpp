#pragma once

#include "on1x/on1x_types.h"

#include <cstddef>

struct On1x_State;

namespace on1x::stdlib {

// Embedded source entry.
struct EmbeddedSource {
    const char* name;
    const char* path;
    const char* text;
};

// Access embedded On1x source files by name.
// Returns nullptr if not found; the pointer is valid for the lifetime of the process.
[[nodiscard]] const char* embedded_source_text(const char* name) noexcept;

// Number of embedded source files.
[[nodiscard]] std::size_t embedded_source_count() noexcept;

// Install all embedded pure source modules into the given state.
// Each source is compiled and the resulting module bound as a global.
[[nodiscard]] bool install_embedded_sources(On1x_State* state) noexcept;

}  // namespace on1x::stdlib
