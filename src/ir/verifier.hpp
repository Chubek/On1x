#pragma once

#include "ir/ir.hpp"
#include "syntax/diagnostics.hpp"

namespace on1x::ir {

[[nodiscard]] bool verify(
    const Module& module,
    syntax::Diagnostics& diagnostics);

}  // namespace on1x::ir
