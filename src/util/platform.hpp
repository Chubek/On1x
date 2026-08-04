#pragma once

#include <cstddef>

namespace on1x {

enum class OperatingSystem {
    Windows,
    MacOS,
    Linux,
    Other,
};

enum class Architecture {
    X86_64,
    AArch64,
    Other,
};

[[nodiscard]] constexpr OperatingSystem operating_system() noexcept {
#if defined(_WIN32)
    return OperatingSystem::Windows;
#elif defined(__APPLE__)
    return OperatingSystem::MacOS;
#elif defined(__linux__)
    return OperatingSystem::Linux;
#else
    return OperatingSystem::Other;
#endif
}

[[nodiscard]] constexpr Architecture architecture() noexcept {
#if defined(_M_X64) || defined(__x86_64__)
    return Architecture::X86_64;
#elif defined(_M_ARM64) || defined(__aarch64__)
    return Architecture::AArch64;
#else
    return Architecture::Other;
#endif
}

[[nodiscard]] std::size_t page_size() noexcept;

}
