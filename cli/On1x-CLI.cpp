#include "On1x-CLI.hpp"

#include "tools/host_prelude.hpp"

#include <Klyspec.hpp>
#include <Klyspec-Manifest.hpp>

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace on1x_cli {

namespace {

std::string read_file(const std::string& path, bool& failed) {
    std::ifstream stream(path, std::ios::binary);
    if (!stream) {
        failed = true;
        return {};
    }
    failed = false;
    std::ostringstream buffer;
    buffer << stream.rdbuf();
    return buffer.str();
}

} // anonymous namespace

CliResult build_registry(const std::string& manifest_file) {
    CliResult result;
    bool failed = false;
    const auto text = read_file(manifest_file, failed);
    if (failed) {
        result.diagnostics.push_back("could not open manifest: " + manifest_file);
        return result;
    }

    auto parsed = klyspec::load_cli_manifest(text, manifest_file);
    if (!parsed.ok || !parsed.manifest) {
        result.diagnostics = std::move(parsed.diagnostics);
        return result;
    }

    result.manifest = std::make_shared<klyspec::CLIManifestSpec>(std::move(*parsed.manifest));
    result.ok = true;
    return result;
}

int run_file(On1x_State* state, const std::string& path) {
    bool failed = false;
    const auto source = read_file(path, failed);
    if (failed) {
        std::cerr << "on1x: could not open file: " << path << "\n";
        return 1;
    }
    On1x_Status status = on1x_eval(state, source.c_str(), source.size(), path.c_str());
    if (status != ON1X_OK) {
        on1x_pop(state, 1);
        return 1;
    }
    on1x_pop(state, 1);
    return 0;
}

int eval_expr(On1x_State* state, const std::string& expr) {
    On1x_Status status = on1x_eval(state, expr.c_str(), expr.size(), "<expr>");
    if (status != ON1X_OK) {
        on1x_pop(state, 1);
        std::cerr << "on1x: evaluation failed\n";
        return 1;
    }
    int top = on1x_top(state);
    if (top > 0) {
        std::string rendered = on1x::tools::render_value(state, -1);
        std::cout << rendered << "\n";
    }
    on1x_pop(state, 1);
    return 0;
}

int dispatch(const CliResult& cli, On1x_State* state, int argc, char** argv) {
    if (!cli.manifest) {
        std::cerr << "on1x: no manifest loaded\n";
        return 1;
    }
    const auto& manifest = *cli.manifest;

    std::vector<std::string> args;
    for (int i = 1; i < argc; ++i) args.emplace_back(argv[i]);

    // Check for help/version flags before full parse.
    for (const auto& token : args) {
        if (token == "-h" || token == "--help") {
            std::cout << manifest.program << " " << manifest.version << "\n";
            if (!manifest.about.empty()) std::cout << manifest.about << "\n";
            std::cout << "\nUsage: " << manifest.program << " [options] [command]\n";
            std::cout << "\nOptions:\n";
            for (const auto& arg : manifest.arguments) {
                std::string label;
                for (std::size_t j = 0; j < arg.names.size(); ++j) {
                    if (j) label += ", ";
                    label += arg.names[j];
                }
                std::cout << "  " << label << "\n      " << arg.help << "\n";
            }
            if (!manifest.commands.empty()) {
                std::cout << "\nCommands:\n";
                for (const auto& cmd : manifest.commands) {
                    std::cout << "  " << cmd.name << "  - " << cmd.help << "\n";
                }
            }
            return 0;
        }
        if (token == "-V" || token == "--version") {
            std::cout << manifest.program << " " << manifest.version << "\n";
            return 0;
        }
    }

    // Build Klyspec registry from manifest.
    klyspec::Registry registry;
    klyspec::CommandSpec main_cmd;
    main_cmd.name = manifest.program;
    main_cmd.help = manifest.about;
    for (const auto& arg : manifest.arguments) {
        klyspec::ArgumentSpec spec;
        spec.id = arg.id;
        if (arg.kind == "flag") {
            spec.kind = klyspec::ArgumentKind::flag;
            spec.value_policy = klyspec::ValuePolicy::none;
        } else {
            spec.kind = klyspec::ArgumentKind::option;
            spec.value_policy = klyspec::ValuePolicy::required;
        }
        spec.names = arg.names;
        spec.help = arg.help;
        spec.required = arg.required;
        if (arg.default_value) spec.default_value = *arg.default_value;
        main_cmd.arguments.push_back(std::move(spec));
    }
    registry.register_command(std::move(main_cmd));

    klyspec::KlyCLIService cli_service(registry);
    auto parse_result = cli_service.parse(manifest.program, args);

    if (!parse_result.ok) {
        for (const auto& diag : parse_result.diagnostics)
            std::cerr << "on1x: " << diag << "\n";
        return 1;
    }

    // Determine subcommand.
    std::string command;
    if (!parse_result.positionals.empty()) {
        command = parse_result.positionals[0];
    }

    // --repl flag or "repl" command.
    bool want_repl = false;
    auto repl_it = parse_result.values.find("repl");
    if (repl_it != parse_result.values.end() && !repl_it->second.empty()) {
        want_repl = true;
    }
    if (command == "repl" || want_repl) {
        return run_repl(state);
    }

    // "run" command or --file flag.
    std::string file_path;
    if (command == "run" && parse_result.positionals.size() > 1) {
        file_path = parse_result.positionals[1];
    } else {
        auto file_it = parse_result.values.find("file");
        if (file_it != parse_result.values.end() && !file_it->second.empty()) {
            file_path = file_it->second[0];
        }
    }
    if (!file_path.empty()) {
        return run_file(state, file_path);
    }

    // "eval" command or --expr flag.
    std::string expr;
    if (command == "eval" && parse_result.positionals.size() > 1) {
        expr = parse_result.positionals[1];
    } else {
        auto expr_it = parse_result.values.find("expr");
        if (expr_it != parse_result.values.end() && !expr_it->second.empty()) {
            expr = expr_it->second[0];
        }
    }
    if (!expr.empty()) {
        return eval_expr(state, expr);
    }

    // "version" command.
    if (command == "version") {
        std::cout << manifest.program << " " << manifest.version << "\n";
        return 0;
    }

    // Default: if no command/flag, start REPL.
    if (command.empty() && parse_result.positionals.empty()) {
        return run_repl(state);
    }

    std::cerr << "on1x: unknown command: " << command << "\n";
    return 1;
}

} // namespace on1x_cli
