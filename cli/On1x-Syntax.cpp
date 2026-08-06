#include "On1x-Syntax.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <iostream>

namespace on1x_cli {

namespace {

// -----------------------------------------------------------------------
// Minimal Lua table-value parser for the subset used in On1x.syntax
// -----------------------------------------------------------------------

// Skip whitespace and Lua comments (`--` to end of line).
void skip_ws_and_comments(const std::string& src, std::size_t& pos) {
    while (pos < src.size()) {
        while (pos < src.size() && (src[pos] == ' ' || src[pos] == '\t' ||
                                    src[pos] == '\n' || src[pos] == '\r'))
            ++pos;
        if (pos < src.size() && src[pos] == '-' && pos + 1 < src.size() && src[pos + 1] == '-') {
            pos += 2;
            while (pos < src.size() && src[pos] != '\n') ++pos;
            continue;
        }
        break;
    }
}

// Parse a double-quoted or single-quoted Lua string.
std::string parse_string(const std::string& src, std::size_t& pos) {
    if (pos >= src.size()) return {};
    char quote = src[pos];
    if (quote != '"' && quote != '\'') return {};
    ++pos; // skip opening quote
    std::string result;
    while (pos < src.size() && src[pos] != quote) {
        if (src[pos] == '\\') {
            ++pos;
            if (pos >= src.size()) break;
            switch (src[pos]) {
                case 'n': result += '\n'; break;
                case 't': result += '\t'; break;
                case 'r': result += '\r'; break;
                case '\\': result += '\\'; break;
                case '"': result += '"'; break;
                case '\'': result += '\''; break;
                default: result += src[pos]; break;
            }
        } else {
            result += src[pos];
        }
        ++pos;
    }
    if (pos < src.size()) ++pos; // skip closing quote
    return result;
}

// Parse a Lua table value into a simplified map of string->string
// (for theme and patterns sections).  Stops at `}` or end.
// Table nesting is not handled — we flatten one level.
std::unordered_map<std::string, std::string> parse_simple_table(
    const std::string& src, std::size_t& pos)
{
    std::unordered_map<std::string, std::string> result;
    if (pos >= src.size() || src[pos] != '{') return result;
    ++pos; // skip '{'

    while (pos < src.size()) {
        skip_ws_and_comments(src, pos);
        if (pos >= src.size() || src[pos] == '}') break;

        // Parse key (identifier or string)
        std::string key;
        if (src[pos] == '"' || src[pos] == '\'') {
            key = parse_string(src, pos);
        } else {
            while (pos < src.size() && (std::isalnum(static_cast<unsigned char>(src[pos])) || src[pos] == '_')) {
                key += src[pos++];
            }
        }

        skip_ws_and_comments(src, pos);

        // '='
        if (pos < src.size() && src[pos] == '=') {
            ++pos;
            skip_ws_and_comments(src, pos);
        }

        // Parse value (string, or skip nested tables/functions)
        if (pos < src.size() && (src[pos] == '"' || src[pos] == '\'')) {
            result[key] = parse_string(src, pos);
        } else if (pos < src.size() && src[pos] == '{') {
            // Skip nested tables (like the `suggest` function body)
            int depth = 0;
            while (pos < src.size()) {
                if (src[pos] == '{') ++depth;
                else if (src[pos] == '}') { --depth; if (depth == 0) { ++pos; break; } }
                else if (src[pos] == '"' || src[pos] == '\'') {
                    // skip string inside nested table
                    char q = src[pos]; ++pos;
                    while (pos < src.size() && src[pos] != q) {
                        if (src[pos] == '\\') ++pos;
                        ++pos;
                    }
                    if (pos < src.size()) ++pos;
                }
                ++pos;
            }
        } else {
            // Skip non-string values (numbers, identifiers, function keywords)
            if (pos + 7 < src.size() && src.substr(pos, 8) == "function") {
                // skip function body
                int depth = 0;
                bool in_body = false;
                while (pos < src.size()) {
                    if (src[pos] == '{' || src[pos] == '(') {
                        ++depth;
                        if (src[pos] == '(') in_body = true;
                    }
                    else if (src[pos] == '}' || src[pos] == ')') {
                        --depth;
                        if (depth == 0 && in_body) { ++pos; break; }
                    }
                    else if (src[pos] == '"' || src[pos] == '\'') {
                        char q = src[pos]; ++pos;
                        while (pos < src.size() && src[pos] != q) {
                            if (src[pos] == '\\') ++pos;
                            ++pos;
                        }
                        if (pos < src.size()) ++pos;
                        continue;
                    }
                    ++pos;
                }
            } else {
                while (pos < src.size() && src[pos] != ',' && src[pos] != '}' && src[pos] != '\n' &&
                       src[pos] != ')' && src[pos] != '(') {
                    ++pos;
                }
            }
        }

        skip_ws_and_comments(src, pos);
        if (pos < src.size() && src[pos] == ',') {
            ++pos;
        }
    }
    if (pos < src.size() && src[pos] == '}') ++pos;
    return result;
}

// Parse a Lua array-style list of strings (e.g., `keywords = { "if", "else", ... }`).
std::vector<std::string> parse_string_list(const std::string& src, std::size_t& pos) {
    std::vector<std::string> result;
    if (pos >= src.size() || src[pos] != '{') return result;
    ++pos;

    while (pos < src.size()) {
        skip_ws_and_comments(src, pos);
        if (pos >= src.size() || src[pos] == '}') break;

        if (src[pos] == '"' || src[pos] == '\'') {
            result.push_back(parse_string(src, pos));
        }

        skip_ws_and_comments(src, pos);
        if (pos < src.size() && src[pos] == ',') {
            ++pos;
        }
    }
    if (pos < src.size() && src[pos] == '}') ++pos;
    return result;
}

// Map a color name from the theme to an ANSI escape code.
const char* color_name_to_ansi(const std::string& name) {
    // Standard terminal colors
    if (name == "default")       return "\033[39m";
    if (name == "black")         return "\033[30m";
    if (name == "red")           return "\033[31m";
    if (name == "green")         return "\033[32m";
    if (name == "yellow")        return "\033[33m";
    if (name == "blue")          return "\033[34m";
    if (name == "magenta")       return "\033[35m";
    if (name == "cyan")          return "\033[36m";
    if (name == "white")         return "\033[37m";
    // Bright variants
    if (name == "bright-black")  return "\033[90m";
    if (name == "bright-red")    return "\033[91m";
    if (name == "bright-green")  return "\033[92m";
    if (name == "bright-yellow") return "\033[93m";
    if (name == "bright-blue")   return "\033[94m";
    if (name == "bright-magenta")return "\033[95m";
    if (name == "bright-cyan")   return "\033[96m";
    if (name == "bright-white")  return "\033[97m";
    return "\033[39m"; // default
}

} // anonymous namespace

bool SyntaxHighlighter::load(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) return false;
    std::stringstream buf;
    buf << file.rdbuf();
    std::string src = buf.str();

    std::size_t pos = 0;

    // Skip whitespace and optional `return`
    skip_ws_and_comments(src, pos);
    if (pos + 6 < src.size() && src.substr(pos, 6) == "return") {
        pos += 6;
    }
    skip_ws_and_comments(src, pos);

    // Expect `{`
    if (pos >= src.size() || src[pos] != '{') return false;
    ++pos;

    // Parse top-level fields
    std::string name;
    while (pos < src.size()) {
        skip_ws_and_comments(src, pos);
        if (pos >= src.size() || src[pos] == '}') break;

        // Parse key
        std::string key;
        if (src[pos] == '"' || src[pos] == '\'') {
            key = parse_string(src, pos);
        } else {
            while (pos < src.size() && (std::isalnum(static_cast<unsigned char>(src[pos])) || src[pos] == '_')) {
                key += src[pos++];
            }
        }

        skip_ws_and_comments(src, pos);
        if (pos < src.size() && src[pos] == '=') {
            ++pos;
            skip_ws_and_comments(src, pos);
        }

        if (key == "theme") {
            theme_.colors = parse_simple_table(src, pos);
        } else if (key == "patterns") {
            auto pattern_map = parse_simple_table(src, pos);
            for (const auto& [type, pattern_str] : pattern_map) {
                try {
                    SyntaxPattern pat;
                    pat.token_type = type;
                    pat.regex = std::regex(pattern_str, std::regex::optimize);
                    patterns_.push_back(std::move(pat));
                } catch (const std::regex_error&) {
                    // Skip malformed patterns
                }
            }
        } else if (key == "name") {
            name = parse_string(src, pos);
        } else {
            // Skip value for unknown keys
            if (pos < src.size() && (src[pos] == '"' || src[pos] == '\'')) {
                parse_string(src, pos);
            } else if (pos < src.size() && src[pos] == '{') {
                int depth = 0;
                while (pos < src.size()) {
                    if (src[pos] == '{') ++depth;
                    else if (src[pos] == '}') { --depth; if (depth == 0) { ++pos; break; } }
                    else if (src[pos] == '"' || src[pos] == '\'') {
                        char q = src[pos]; ++pos;
                        while (pos < src.size() && src[pos] != q) {
                            if (src[pos] == '\\') ++pos;
                            ++pos;
                        }
                        if (pos < src.size()) ++pos;
                    }
                    ++pos;
                }
            } else {
                while (pos < src.size() && src[pos] != ',' && src[pos] != '}') ++pos;
            }
        }

        skip_ws_and_comments(src, pos);
        if (pos < src.size() && src[pos] == ',') ++pos;
    }

    loaded_ = true;
    return true;
}

std::string SyntaxHighlighter::highlight(const std::string& line) const {
    if (!loaded_ || patterns_.empty() || line.empty()) {
        return line;
    }

    // Tokenize: build a list of (token_type, matched_text) segments.
    struct Token {
        std::string type;
        std::string text;
        std::size_t pos;
    };
    std::vector<Token> tokens;

    // Start with the whole line as "text" (default).
    tokens.push_back({"text", line, 0});

    // Apply each pattern to split tokens further.
    for (const auto& pat : patterns_) {
        std::vector<Token> new_tokens;
        for (const auto& tok : tokens) {
            if (tok.type != "text") {
                // Already classified; keep it.
                new_tokens.push_back(tok);
                continue;
            }

            std::string remaining = tok.text;
            std::size_t base_offset = tok.pos;
            auto words_begin = std::sregex_iterator(remaining.begin(), remaining.end(), pat.regex);
            auto words_end = std::sregex_iterator();

            std::size_t last_end = 0;
            for (auto it = words_begin; it != words_end; ++it) {
                std::size_t match_start = static_cast<std::size_t>(it->position());
                std::size_t match_len = it->length();
                std::string match_text = it->str();

                // Text before this match
                if (match_start > last_end) {
                    new_tokens.push_back({"text", remaining.substr(last_end, match_start - last_end),
                                         base_offset + last_end});
                }

                // The matched token
                new_tokens.push_back({pat.token_type, match_text, base_offset + match_start});
                last_end = match_start + match_len;
            }

            // Remaining text after the last match
            if (last_end < remaining.size()) {
                new_tokens.push_back({"text", remaining.substr(last_end), base_offset + last_end});
            }

            // If no matches, keep the original token
            if (words_begin == words_end) {
                new_tokens.push_back(tok);
            }
        }
        tokens = std::move(new_tokens);
    }

    // Build the ANSI-colored string.
    std::string result;
    for (const auto& tok : tokens) {
        std::string color;
        if (tok.type == "text") {
            color = "\033[39m"; // default
        } else {
            auto it = theme_.colors.find(tok.type);
            if (it != theme_.colors.end()) {
                color = color_name_to_ansi(it->second);
            } else {
                color = "\033[39m";
            }
        }
        result += color + tok.text;
    }
    result += "\033[0m"; // reset
    return result;
}

} // namespace on1x_cli
