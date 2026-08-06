#include "stdlib/module.hpp"

#include "core/table.hpp"
#include "gc/alloc.hpp"
#include "gc/roots.hpp"
#include "runtime/closure.hpp"
#include "runtime/state.hpp"
#include "stdlib/capability.hpp"

namespace on1x::stdlib {

bool install_module(On1x_State* state, const On1x_ModuleDesc& module) noexcept {
    if (!state || !module.name || !has_capability(state, module.capability)) return false;
    try {
        GcRoot globals_root(state->globals);
        GcRootArray<14> reserved_roots;
        reserved_roots.set(0, state->reserved.unit);
        reserved_roots.set(1, state->reserved.boolean);
        reserved_roots.set(2, state->reserved.integer);
        reserved_roots.set(3, state->reserved.floating);
        reserved_roots.set(4, state->reserved.string);
        reserved_roots.set(5, state->reserved.tag);
        reserved_roots.set(6, state->reserved.list);
        reserved_roots.set(7, state->reserved.table);
        reserved_roots.set(8, state->reserved.function);
        reserved_roots.set(9, state->reserved.iota);
        reserved_roots.set(10, state->reserved.some);
        reserved_roots.set(11, state->reserved.none);
        reserved_roots.set(12, state->reserved.success);
        reserved_roots.set(13, state->reserved.error);
        auto* table = new_table(&state->gc);
        GcRoot table_root(table);
        state->persistent_roots.push(table);
        auto* module_tag = state->tags.intern(&state->gc, module.name);
        GcRoot module_tag_root(module_tag);
        for (std::size_t index = 0; index < module.function_count; ++index) {
            const On1x_FnDesc& function = module.functions[index];
            if (!function.name || !function.function) return false;
            auto* member_tag = state->tags.intern(&state->gc, function.name);
            GcRoot member_tag_root(member_tag);
            auto* native = runtime::new_native_function(&state->gc, function.function);
            GcRoot native_root(native);
            state->persistent_roots.push(native);
            if (!table_set(
                    &state->gc,
                    table,
                    value_from_object(member_tag),
                    value_from_object(native))) return false;
        }
        return table_set(
            &state->gc,
            state->globals,
            value_from_object(module_tag),
            value_from_object(table));
    } catch (...) {
        return false;
    }
}

}  // namespace on1x::stdlib
