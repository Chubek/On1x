#pragma once

#include <string>
#include <unordered_map>
#include <regex>
#include <vector>

namespace on1x_cli {

// Token type to ANSI color code mapping.
struct SyntaxTheme {
    std::unordered_map<std::string, std::string> colors;
};

// A single regex pattern for token matching, with its token type name.
struct SyntaxPattern {
    std::string token_type;
    std::regex  regex;
};

// Parsed syntax-highlighting definition loaded from a file like On1x.syntax.
class SyntaxHighlighter {
public:
    // Load definitions from `path`. Returns true on success.
    bool load(const std::string& path);

    // Apply syntax highlighting to `line` and return an ANSI-colored string.
    std::string highlight(const std::string& line) const;

    // True after a successful load.
    bool loaded() const { return loaded_; }

private:
    bool loaded_ = false;
    SyntaxTheme theme_;
    std::vector<SyntaxPattern> patterns_;
};

} // namespace on1x_cli
