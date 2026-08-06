#include "runtime/env.hpp"

#include "gc/alloc.hpp"
#include "gc/roots.hpp"

namespace on1x::runtime {

EnvironmentObject* new_environment(GcState* gc, EnvironmentObject* parent) {
    auto* environment = gc_alloc<EnvironmentObject>(gc);
    GcRoot environment_root(environment);
    environment->parent = parent;
    environment->bindings = new_table(gc);
    return environment;
}

bool environment_get(
    const EnvironmentObject* environment,
    Value key,
    Value& result) noexcept {
    for (const EnvironmentObject* current = environment; current; current = current->parent) {
        if (table_get(current->bindings, key, result)) return true;
    }
    return false;
}

bool environment_set(
    GcState* gc,
    EnvironmentObject* environment,
    Value key,
    Value value) {
    if (!environment) return false;
    GcRoot environment_root(environment);
    return table_set(gc, environment->bindings, key, value);
}

}  // namespace on1x::runtime
