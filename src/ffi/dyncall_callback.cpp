#include "ffi/ffi.hpp"

namespace on1x::ffi {

// Reverse callbacks require a stable On1x function invocation path. The runtime
// has not exposed closures to this subsystem yet, so no callback thunk is made.
// Keeping the implementation here prevents dyncallback headers from leaking out
// of src/ffi while the direct-call path remains fully operational.
static_assert(sizeof(FfiContext*) == sizeof(void*));

}  // namespace on1x::ffi
