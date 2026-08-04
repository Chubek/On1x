#pragma once

#include <source_location>

namespace on1x {

[[noreturn]] void panic(
    const char* expression,
    const char* message,
    std::source_location location = std::source_location::current()) noexcept;

}

#define ON1X_ASSERT(expression, message)                                  \
    do {                                                                  \
        if (!(expression)) {                                              \
            ::on1x::panic(#expression, message, std::source_location::current()); \
        }                                                                 \
    } while (false)
