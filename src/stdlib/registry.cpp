#include "stdlib/registry.hpp"

#include <on1x/on1x_config.h>

#include "stdlib/module.hpp"
#include "stdlib/cmp/cmp.hpp"
#include "stdlib/math/math.hpp"
#include "stdlib/fn/fn.hpp"
#include "stdlib/iter/iter.hpp"
#include "stdlib/rand/rand.hpp"
#include "stdlib/bit/bit.hpp"
#include "stdlib/tag/tag.hpp"
#include "stdlib/str/str.hpp"
#include "stdlib/list/list.hpp"
#include "stdlib/table/table.hpp"
#include "stdlib/opt/opt.hpp"
#include "stdlib/res/res.hpp"
#include "stdlib/fs/fs.hpp"

#if ON1X_ENABLE_SEXP
#include "stdlib/sexp/sexp.hpp"
#endif

namespace on1x::stdlib {

const ModuleEntry* modules(std::size_t& count) noexcept {
    static const ModuleEntry entries[] = {
        {cmp_module()}, {math_module()}, {bit_module()}, {tag_module()}, {str_module()},
        {fn_module()},
        {iter_module()},
        {rand_module()},
        {list_module()},
        {table_module()},
        {opt_module()},
        {res_module()},
        {path_module()},
#if ON1X_ENABLE_SEXP
        {sexp_module()},
#endif
    };
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
