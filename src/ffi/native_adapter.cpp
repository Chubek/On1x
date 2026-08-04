#include "ffi/ffi.hpp"

#include <on1x/on1x_types.h>

namespace on1x::ffi {

// This translation unit anchors the C-native adapter dependency boundary. Native
// calls currently enter through src/api/api_call.cpp, which enforces argc and
// single-result/error stack protocol before any FFI bridge can be invoked.
static_assert(sizeof(On1x_CFn) == sizeof(void*));

}  // namespace on1x::ffi
