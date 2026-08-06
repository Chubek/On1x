#pragma once

#include <on1x/on1x_types.h>

#include "core/value.hpp"
#include "runtime/state.hpp"

#include <cstddef>

namespace on1x::prelude {

[[nodiscard]] bool install(On1x_State* state) noexcept;
[[nodiscard]] On1x_Status push_result(On1x_State* state, Value value) noexcept;
[[nodiscard]] On1x_Status raise(On1x_State* state, const char* message) noexcept;
[[nodiscard]] bool has_arity(On1x_State* state, int argc, int expected, const char* name) noexcept;
[[nodiscard]] bool argument(const On1x_State* state, int index, Value& value) noexcept;
[[nodiscard]] std::size_t string_length(Value value, bool& valid) noexcept;

On1x_Status type_of(On1x_State* state, int argc);
On1x_Status tag_of(On1x_State* state, int argc);
On1x_Status payload_of(On1x_State* state, int argc);
On1x_Status length(On1x_State* state, int argc);
On1x_Status get(On1x_State* state, int argc);
On1x_Status set(On1x_State* state, int argc);
On1x_Status push(On1x_State* state, int argc);
On1x_Status pop(On1x_State* state, int argc);
On1x_Status keys(On1x_State* state, int argc);
On1x_Status values(On1x_State* state, int argc);
On1x_Status iota(On1x_State* state, int argc);
On1x_Status is_some(On1x_State* state, int argc);
On1x_Status is_none(On1x_State* state, int argc);
On1x_Status unwrap(On1x_State* state, int argc);
On1x_Status unwrap_or(On1x_State* state, int argc);
On1x_Status is_success(On1x_State* state, int argc);
On1x_Status is_error(On1x_State* state, int argc);

}  // namespace on1x::prelude
