#include "On1x-CLI.hpp"

#include "On1x-Autocomp.hpp"
#include "On1x-Syntax.hpp"
#include "tools/host_prelude.hpp"

#include <on1x/on1x.h>
#include <on1x/on1x_version.h>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#if defined(_WIN32)
#include <io.h>
#define ON1X_ISATTY(fd) _isatty(fd)
#else
#include <unistd.h>
#define ON1X_ISATTY(fd) isatty(fd)
#endif

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

// ANSI helpers. Colors are only emitted when stdout is a terminal.
bool stdout_is_tty() {
    return ON1X_ISATTY(1) != 0;
}

const char* ansi_reset() { return "\033[0m"; }
const char* ansi_prompt() { return "\033[1;36m"; }

std::string ansi_echo(const SyntaxHighlighter& highlighter,
                      const std::string& source, bool tty) {
    if (!tty || !highlighter.loaded()) return source;
    return highlighter.highlight(source);
}

// Find the prefix of the last whitespace-separated word on the line.
std::string current_word_prefix(const std::string& line) {
    std::size_t start = line.size();
    while (start > 0) {
        char c = line[start - 1];
        if (std::isspace(static_cast<unsigned char>(c)) || c == '{' ||
            c == '}' || c == '(' || c == ')' || c == '[' || c == ']' ||
            c == ',' || c == ';') {
            break;
        }
        --start;
    }
    return line.substr(start);
}

} // anonymous namespace

int run_repl(On1x_State* state) {
    if (!state) {
        std::cerr << "on1x: cannot start REPL — no state\n";
        return 1;
    }

    const bool tty = stdout_is_tty();

    // --- Load On1x syntax and autocompletion definitions ---
    SyntaxHighlighter highlighter;
    Autocompleter autocompleter;

    // Resolve the cli directory: try the same candidates as on1x.cpp.
    std::string cli_dir;
    {
        const std::string candidates[] = {
            "cli/",
            "../cli/",
            "../../cli/",
        };
        std::string syntax_path;
        for (const auto& prefix : candidates) {
            std::ifstream test(prefix + "On1x.syntax");
            if (test.good()) {
                cli_dir = prefix;
                break;
            }
        }
    }
    if (!cli_dir.empty()) {
        if (!highlighter.load(cli_dir + "On1x.syntax")) {
            std::cerr << "on1x: warning: could not load " << cli_dir
                      << "On1x.syntax\n";
        }
        if (!autocompleter.load(cli_dir + "On1x.autocomp")) {
            std::cerr << "on1x: warning: could not load " << cli_dir
                      << "On1x.autocomp\n";
        }
    } else {
        std::cerr << "on1x: warning: could not find cli/On1x.syntax or "
                     "cli/On1x.autocomp — syntax highlighting and completion "
                     "disabled\n";
    }

    // --- Banner ---
    std::cout << "On1x " << ON1X_VERSION_MAJOR << "." << ON1X_VERSION_MINOR
              << "." << ON1X_VERSION_PATCH << " REPL\n";
    std::cout << "Type ':quit' or press Ctrl-D to exit.  ':help' for more.\n";
    if (autocompleter.loaded()) {
        std::cout << "Completion: ':complete <prefix>' lists matching "
                     "keywords.\n";
    }
    std::cout << "\n";

    // --- Main REPL loop ---
    const char* prompt_str = "on1x> ";
    const char* cont_prompt_str = "....  ";
    std::string accumulated;

    auto show_help = []() {
        std::cout << "  :quit, :q, :exit   Exit the REPL\n";
        std::cout << "  :help, :h          Show this help\n";
        std::cout << "  :complete <prefix> Show keyword completions\n";
        std::cout << "  Anything else is evaluated as On1x.\n";
    };

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
                show_help();
                continue;
            }
            if (trimmed.rfind(":complete", 0) == 0) {
                std::string prefix = trimmed.substr(9);
                // Trim leading whitespace
                std::size_t ws = 0;
                while (ws < prefix.size() && std::isspace(static_cast<unsigned char>(prefix[ws]))) ++ws;
                prefix = prefix.substr(ws);
                auto matches = autocompleter.complete(prefix);
                if (matches.empty()) {
                    std::cout << "  (no matches)\n";
                } else {
                    for (const auto& m : matches) {
                        std::cout << "  " << m << "\n";
                    }
                }
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

        // Echo the statement with syntax highlighting (tty only), then
        // evaluate.
        if (tty && highlighter.loaded() && !accumulated.empty()) {
            std::cout << "\033[2K\r" << prompt_str
                      << ansi_echo(highlighter, accumulated, tty)
                      << ansi_reset() << "\n";
        }

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
