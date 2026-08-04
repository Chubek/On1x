#include "util/assert.hpp"

#include <cstdio>
#include <cstdlib>

namespace on1x {

void panic(
    const char* expression,
    const char* message,
    std::source_location location) noexcept {
    std::fprintf(
        stderr,
        "on1x invariant failed at %s:%u: %s (%s)\n",
        location.file_name(),
        location.line(),
        message,
        expression);
    std::abort();
}

}
