#include "ffi/ffi.hpp"

#include "ffi/marshal.hpp"

extern "C" {
#include <dyncall.h>
}

namespace on1x::ffi {

bool FfiContext::call(
    GcState* gc, void* function, std::string_view signature_text,
    const Value* arguments, std::size_t argument_count, Value& result) noexcept {
    Signature signature;
    if (!gc || !vm_ || !function || !arguments || !parse_signature(signature_text, signature) ||
        signature.argument_count != argument_count) {
        return false;
    }
    try {
        dcReset(vm_);
        for (std::size_t index = 0; index < argument_count; ++index) {
            SignatureType type;
            switch (signature.arguments[index]) {
            case 'B': type = SignatureType::Bool; break;
            case 'l': type = SignatureType::Int; break;
            case 'd': type = SignatureType::Float; break;
            case 'p': type = SignatureType::Pointer; break;
            default: return false;
            }
            if (!push_argument(vm_, type, arguments[index])) return false;
        }
        result = read_result(gc, vm_, function, signature.result);
        return dcGetError(vm_) == DC_ERROR_NONE;
    } catch (...) {
        return false;
    }
}

}  // namespace on1x::ffi
