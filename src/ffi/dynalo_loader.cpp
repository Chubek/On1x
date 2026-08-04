#include "ffi/dynalo_loader.hpp"

#include <dynload.h>

#include <string>

namespace on1x::ffi {

DynamicLibrary::~DynamicLibrary() {
    close();
}

bool DynamicLibrary::open(std::string_view path) {
    close();
    if (path.empty() || path.find('\0') != std::string_view::npos) return false;
    const std::string native_path(path);
    library_ = dlLoadLibrary(native_path.c_str());
    return library_ != nullptr;
}

void* DynamicLibrary::find(std::string_view symbol) const {
    if (!library_ || symbol.empty() || symbol.find('\0') != std::string_view::npos) return nullptr;
    const std::string native_symbol(symbol);
    return dlFindSymbol(static_cast<DLLib*>(library_), native_symbol.c_str());
}

void DynamicLibrary::close() noexcept {
    if (!library_) return;
    dlFreeLibrary(static_cast<DLLib*>(library_));
    library_ = nullptr;
}

}  // namespace on1x::ffi
