#pragma once

#include "core/value.hpp"
#include "ffi/signature.hpp"

#include <cstddef>
#include <string_view>

struct DCCallVM_;

namespace on1x {
struct GcState;
}

namespace on1x::ffi {

class FfiContext {
public:
    FfiContext();
    ~FfiContext();

    FfiContext(const FfiContext&) = delete;
    FfiContext& operator=(const FfiContext&) = delete;

    [[nodiscard]] bool available() const noexcept;
    [[nodiscard]] bool call(
        GcState* gc, void* function, std::string_view signature,
        const Value* arguments, std::size_t argument_count, Value& result) noexcept;

private:
    DCCallVM_* vm_ = nullptr;
};

}  // namespace on1x::ffi
