#include "stdlib/registry.hpp"

#include "stdlib/module.hpp"
#include "stdlib/cmp/cmp.hpp"
#include "stdlib/math/math.hpp"
#include "stdlib/bit/bit.hpp"
#include "stdlib/tag/tag.hpp"
#include "stdlib/str/str.hpp"

namespace on1x::stdlib {

const ModuleEntry* modules(std::size_t& count) noexcept {
    static const ModuleEntry entries[] = {
        {cmp_module()}, {math_module()}, {bit_module()}, {tag_module()}, {str_module()}};
    count = sizeof(entries) / sizeof(entries[0]);
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
