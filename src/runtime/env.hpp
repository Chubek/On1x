#pragma once

#include "core/table.hpp"

namespace on1x::runtime {

struct EnvironmentObject {
    ObjectHeader header{ObjectKind::Environment};
    EnvironmentObject* parent = nullptr;
    TableObject* bindings = nullptr;
};

[[nodiscard]] EnvironmentObject* new_environment(
    GcState* gc,
    EnvironmentObject* parent = nullptr);
[[nodiscard]] bool environment_get(
    const EnvironmentObject* environment,
    Value key,
    Value& result) noexcept;
[[nodiscard]] bool environment_set(
    GcState* gc,
    EnvironmentObject* environment,
    Value key,
    Value value);

}  // namespace on1x::runtime
