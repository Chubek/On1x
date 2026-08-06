#include "On1x-Autocomp.hpp"

#include "On1x-Syntax.hpp" // reuse the Lua table parser helpers

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>

namespace on1x_cli {

namespace {

// Forward declarations for the parser helpers (same as in On1x-Syntax.cpp)
// We re-implement the minimal Lua table parsing here to avoid coupling.

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

std::string parse_string(const std::string& src, std::size_t& pos) {
    if (pos >= src.size()) return {};
    char quote = src[pos];
    if (quote != '"' && quote != '\'') return {};
    ++pos;
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
    if (pos < src.size()) ++pos;
    return result;
}

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
        if (pos < src.size() && src[pos] == ',') ++pos;
    }
    if (pos < src.size() && src[pos] == '}') ++pos;
    return result;
}

} // anonymous namespace

bool Autocompleter::load(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) return false;
    std::stringstream buf;
    buf << file.rdbuf();
    std::string src = buf.str();

    std::size_t pos = 0;

    skip_ws_and_comments(src, pos);
    if (pos + 6 < src.size() && src.substr(pos, 6) == "return") {
        pos += 6;
    }
    skip_ws_and_comments(src, pos);

    if (pos >= src.size() || src[pos] != '{') return false;
    ++pos;

    while (pos < src.size()) {
        skip_ws_and_comments(src, pos);
        if (pos >= src.size() || src[pos] == '}') break;

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

        if (key == "keywords") {
            keywords_ = parse_string_list(src, pos);
        } else {
            // Skip value for unknown keys (name, suggest, etc.)
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
                // skip function keyword or other value
                while (pos < src.size() && src[pos] != ',' && src[pos] != '}' && src[pos] != '\n') ++pos;
            }
        }

        skip_ws_and_comments(src, pos);
        if (pos < src.size() && src[pos] == ',') ++pos;
    }

    loaded_ = true;
    return true;
}

std::vector<std::string> Autocompleter::complete(const std::string& prefix) const {
    if (!loaded_ || prefix.empty()) return {};

    std::vector<std::string> results;
    for (const auto& kw : keywords_) {
        if (kw.size() >= prefix.size() &&
            kw.compare(0, prefix.size(), prefix) == 0) {
            results.push_back(kw);
        }
    }
    return results;
}

} // namespace on1x_cli
