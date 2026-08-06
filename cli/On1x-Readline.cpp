#include "On1x-CLI.hpp"

#include "tools/host_prelude.hpp"

#include <PikoRL.hpp>
#include "qamrpp_stubs.hpp"

#include <on1x/on1x.h>
#include <on1x/on1x_version.h>

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace on1x_cli {

namespace {

// ---------------------------------------------------------------------------
// Continuation heuristics
// ---------------------------------------------------------------------------
bool needs_continuation(const std::string& line) {
    if (line.empty()) return false;
    std::size_t end = line.size();
    while (end > 0 && std::isspace(static_cast<unsigned char>(line[end - 1]))) --end;
    if (end == 0) return false;
    char last = line[end - 1];
    if (last == '{' || last == '[' || last == '(') return true;
    if (last == '+' || last == '-' || last == '*' || last == '/' ||
        last == '=' || last == '<' || last == '>' || last == '|' ||
        last == '&' || last == '^' || last == '%' || last == '!' ||
        last == ',' || last == ':') return true;
    if (last == '\\') return true;
    return false;
}

int bracket_balance(const std::string& line) {
    int bal = 0;
    for (char c : line) {
        switch (c) {
            case '{': case '[': case '(': ++bal; break;
            case '}': case ']': case ')': --bal; break;
            default: break;
        }
    }
    return bal;
}

} // anonymous namespace

int run_repl(On1x_State* state) {
    if (!state) {
        std::cerr << "on1x: cannot start REPL — no state\n";
        return 1;
    }

    // --- Initialize PikoRL REPL for prompt infrastructure ---
    picorl::REPL pikorl_repl;

    // Bind On1x eval into the PikoRL context so Lua extensions can call it.
    pikorl_repl.bind_api("on1x_eval",
        [state](qamrpp::Context&, std::vector<qamrpp::ValuePtr>& args) -> qamrpp::ValuePtr {
            auto result = std::make_shared<qamrpp::Value>();
            if (args.empty()) {
                result->type = qamrpp::Value::Type::STRING;
                result->string_value = "on1x_eval requires an expression";
                return result;
            }
            const std::string& expr = args[0]->string_value;
            On1x_Status st = on1x_eval(state, expr.c_str(), expr.size(), "<repl>");
            result->type = qamrpp::Value::Type::STRING;
            if (st == ON1X_OK) {
                if (on1x_top(state) > 0) {
                    result->string_value = on1x::tools::render_value(state, -1);
                    on1x_pop(state, 1);
                } else {
                    result->string_value = ":Unit";
                }
            } else {
                if (on1x_top(state) > 0) {
                    result->string_value = "error: " + on1x::tools::render_value(state, -1);
                    on1x_pop(state, 1);
                } else {
                    result->string_value = "error: evaluation failed";
                }
            }
            return result;
        });

    // Try to load autocomplete/syntax bundles.
    (void)pikorl_repl.load_example_bundle("cli");

    // --- Banner ---
    std::cout << "On1x " << ON1X_VERSION_MAJOR << "." << ON1X_VERSION_MINOR
              << "." << ON1X_VERSION_PATCH << " REPL\n";
    std::cout << "Type ':quit' or press Ctrl-D to exit.  ':help' for more.\n\n";

    // --- Main REPL loop using On1x eval ---
    const char* prompt_str = "on1x> ";
    const char* cont_prompt_str = "....  ";
    std::string accumulated;

    while (true) {
        std::cout << (accumulated.empty() ? prompt_str : cont_prompt_str) << std::flush;

        std::string line;
        if (!std::getline(std::cin, line)) {
            std::cout << "\n";
            break;
        }

        // Meta-commands
        if (accumulated.empty()) {
            std::string trimmed = line;
            std::size_t start = 0;
            while (start < trimmed.size() && std::isspace(static_cast<unsigned char>(trimmed[start]))) ++start;
            trimmed = trimmed.substr(start);

            if (trimmed == ":quit" || trimmed == ":q" || trimmed == ":exit") break;
            if (trimmed == ":help" || trimmed == ":h") {
                std::cout << "  :quit, :q     Exit the REPL\n";
                std::cout << "  :help, :h     Show this help\n";
                std::cout << "  Anything else is evaluated as On1x.\n";
                continue;
            }
        }

        // Accumulate
        if (accumulated.empty()) {
            accumulated = line;
        } else {
            accumulated += "\n";
            accumulated += line;
        }

        if (needs_continuation(line) || bracket_balance(accumulated) > 0)
            continue;
        if (accumulated.empty()) continue;

        // Evaluate with On1x
        On1x_Status status = on1x_eval(
            state, accumulated.c_str(), accumulated.size(), "<repl>");

        if (status == ON1X_OK) {
            if (on1x_top(state) > 0) {
                On1x_Type t = on1x_type(state, -1);
                if (t != ON1X_UNIT) {
                    std::cout << on1x::tools::render_value(state, -1) << "\n";
                }
                on1x_pop(state, 1);
            }
        } else {
            if (on1x_top(state) > 0) {
                std::cerr << "error: " << on1x::tools::render_value(state, -1) << "\n";
                on1x_pop(state, 1);
            } else {
                std::cerr << "error: evaluation failed\n";
            }
        }

        accumulated.clear();
    }

    return 0;
}

} // namespace on1x_cli
