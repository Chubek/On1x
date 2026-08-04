#include "ffi/dynalo_loader.hpp"

namespace on1x::ffi {

// DynamicLibrary is RAII-owned by the host-facing FFI layer. Once API-level
// library handles are introduced, their GC finalizers will own this class.
static_assert(sizeof(DynamicLibrary) == sizeof(void*));

}  // namespace on1x::ffi
