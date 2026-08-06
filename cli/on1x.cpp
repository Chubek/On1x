#include "On1x-CLI.hpp"

#include "tools/host_prelude.hpp"

#include <on1x/on1x.h>
#include <on1x/on1x_capability.h>
#include <on1x/on1x_stdlib.h>
#include <on1x/on1x_version.h>
#include <on1x/on1x_config.h>

#include <iostream>
#include <fstream>
#include <string>
#include <vector>

namespace {

void print_banner() {
    std::cout << "On1x " << on1x_version_string() << " — The On1x programming language\n";
}

int run_with_manifest(const std::string& manifest_file, int argc, char** argv) {
    auto cli = on1x_cli::build_registry(manifest_file);
    if (!cli.ok) {
        for (const auto& diag : cli.diagnostics)
            std::cerr << "on1x: " << diag << "\n";
        return 1;
    }

    // Quick flags that don't need state.
    for (int i = 1; i < argc; ++i) {
        std::string arg(argv[i]);
        if (arg == "-V" || arg == "--version") {
            std::cout << "on1x " << on1x_version_string() << "\n";
            return 0;
        }
    }

    // Determine capability flags from args.
    bool pure_only = false;
    for (int i = 1; i < argc; ++i) {
        std::string arg(argv[i]);
        if (arg == "--pure") pure_only = true;
    }

    // Create On1x state.
    On1x_State* state = on1x_open();
    if (!state) {
        std::cerr << "on1x: failed to create interpreter state\n";
        return 1;
    }

    // Install standard library.
    if (!pure_only) {
        // Grant capabilities for a full-featured environment.
        on1x_grant(state, ON1X_CAP_IO);
        on1x_grant(state, ON1X_CAP_FS);
        on1x_grant(state, ON1X_CAP_ENV);
        on1x_grant(state, ON1X_CAP_TIME);
        on1x_grant(state, ON1X_CAP_CLOCK);
    }
    on1x_open_std(state);

    // Install host prelude (Io.Print, etc.) and capability-gated modules.
#if !ON1X_STDLIB_PURE_ONLY
    on1x::tools::install_host_prelude(state);
    if (on1x_has_capability(state, ON1X_CAP_ENV)) {
        on1x_open_os(state);
    }
    if (on1x_has_capability(state, ON1X_CAP_TIME)) {
        on1x_open_time(state);
    }
    if (on1x_has_capability(state, ON1X_CAP_FS)) {
        on1x_open_fs(state);
    }
#endif // !ON1X_STDLIB_PURE_ONLY

    int exit_code = on1x_cli::dispatch(cli, state, argc, argv);

    on1x_close(state);
    return exit_code;
}

} // anonymous namespace

int main(int argc, char** argv) {
    // Resolve manifest path.
    const std::vector<std::string> candidates = {
        "cli/On1x-CLI.yaml",
        "../cli/On1x-CLI.yaml",
        "../../cli/On1x-CLI.yaml",
        "/usr/local/share/on1x/On1x-CLI.yaml",
    };

    std::string manifest_file = "cli/On1x-CLI.yaml";
    const char* env = std::getenv("ON1X_CLI_MANIFEST");
    if (env && env[0]) {
        manifest_file = env;
    } else {
        // Try to find the manifest.
        for (const auto& cand : candidates) {
            std::ifstream test(cand);
            if (test.good()) {
                manifest_file = cand;
                break;
            }
        }
    }

    return run_with_manifest(manifest_file, argc, argv);
}
