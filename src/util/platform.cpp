#include "util/platform.hpp"

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace on1x {

std::size_t page_size() noexcept {
#if defined(_WIN32)
    SYSTEM_INFO info;
    GetSystemInfo(&info);
    return static_cast<std::size_t>(info.dwPageSize);
#else
    const long result = sysconf(_SC_PAGESIZE);
    return result > 0 ? static_cast<std::size_t>(result) : 4096U;
#endif
}

}
