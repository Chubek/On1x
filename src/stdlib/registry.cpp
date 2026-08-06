#include "stdlib/registry.hpp"

#include "stdlib/module.hpp"

namespace on1x::stdlib {

const ModuleEntry* modules(std::size_t& count) noexcept {
    static const ModuleEntry entries[] = {};
    count = 0;
    return entries;
}

bool install_pure_modules(On1x_State* state) noexcept {
    std::size_t count = 0;
    const ModuleEntry* entries = modules(count);
    for (std::size_t index = 0; index < count; ++index) {
        if (!entries[index].descriptor || !install_module(state, *entries[index].descriptor)) return false;
    }
    return true;
}

}  // namespace on1x::stdlib
