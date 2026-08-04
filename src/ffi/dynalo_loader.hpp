#pragma once

#include <string_view>

namespace on1x::ffi {

class DynamicLibrary {
public:
    DynamicLibrary() = default;
    ~DynamicLibrary();

    DynamicLibrary(const DynamicLibrary&) = delete;
    DynamicLibrary& operator=(const DynamicLibrary&) = delete;

    [[nodiscard]] bool open(std::string_view path);
    [[nodiscard]] void* find(std::string_view symbol) const;
    void close() noexcept;

private:
    void* library_ = nullptr;
};

}  // namespace on1x::ffi
