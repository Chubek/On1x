#pragma once

#include <on1x/on1x_types.h>

#include "core/reserved_tags.hpp"
#include "core/table.hpp"
#include "core/value.hpp"
#include "gc/gc.hpp"
#include "gc/handle_table.hpp"

#include <cstddef>
#include <cstdint>

namespace on1x::vm {
class Chunk;
}

namespace on1x {

struct FunctionObject {
    ObjectHeader header{ObjectKind::Function};
    On1x_CFn native = nullptr;
    vm::Chunk* chunk = nullptr;
    Value* captures = nullptr;
    std::size_t capture_count = 0;
};

struct ApiReference {
    Value value{};
    GcHandle* handle = nullptr;
    ApiReference* next = nullptr;
};

}  // namespace on1x

struct On1x_State {
    on1x::GcState gc;
    on1x::TagTable tags;
    on1x::ReservedTags reserved;
    on1x::TableObject* globals = nullptr;
    on1x::Value* stack = nullptr;
    std::size_t top = 0;
    std::size_t capacity = 0;
    std::size_t api_frame_base = 0;
    bool api_frame_active = false;
    on1x::GcHandleTable handles;
    on1x::ApiReference* references = nullptr;
    std::uint32_t capabilities = 0;

    On1x_State() : handles(&gc) {}
};

namespace on1x {
[[nodiscard]] bool stack_push(On1x_State* state, Value value);
[[nodiscard]] bool stack_at(const On1x_State* state, int index, Value& value) noexcept;
[[nodiscard]] bool stack_replace(On1x_State* state, int index, Value value) noexcept;
[[nodiscard]] int normalize_stack_index(const On1x_State* state, int index) noexcept;
[[nodiscard]] std::size_t visible_stack_size(const On1x_State* state) noexcept;
void release_api_references(On1x_State* state) noexcept;
}  // namespace on1x
