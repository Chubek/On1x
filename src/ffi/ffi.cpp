#include "ffi/ffi.hpp"

extern "C" {
#include <dyncall.h>
}

namespace on1x::ffi {

FfiContext::FfiContext() : vm_(dcNewCallVM(4096)) {}

FfiContext::~FfiContext() {
    if (vm_) dcFree(vm_);
}

bool FfiContext::available() const noexcept {
    return vm_ != nullptr;
}

}  // namespace on1x::ffi
