#pragma once

#include <on1x/on1x.h>

#include <memory>
#include <string>
#include <vector>

// Forward declarations for Klyspec types (avoid including Klyspec headers
// in translation units that also include PikoRL headers — both vendored
// copies of DSLUtils.hpp conflict).
namespace klyspec {
struct CLIManifestSpec;
}

namespace on1x_cli {

struct CliResult {
    std::shared_ptr<klyspec::CLIManifestSpec> manifest;
    std::vector<std::string> diagnostics{};
    bool ok{false};
};

// Load and validate a CLI manifest from `manifest_file`.
CliResult build_registry(const std::string& manifest_file);

// Run an On1x source file.
int run_file(On1x_State* state, const std::string& path);

// Evaluate a single On1x expression.
int eval_expr(On1x_State* state, const std::string& expr);

// Run the interactive REPL (implemented in On1x-Readline.cpp).
int run_repl(On1x_State* state);

// Parse argv, dispatch to the appropriate command, and return an exit code.
// Uses `state` for evaluation; the caller owns the state lifetime.
int dispatch(const CliResult& cli, On1x_State* state, int argc, char** argv);

} // namespace on1x_cli
