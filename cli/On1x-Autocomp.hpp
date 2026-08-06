#pragma once

#include <string>
#include <vector>

namespace on1x_cli {

// Parsed autocomplete definition loaded from a file like On1x.autocomp.
class Autocompleter {
public:
    // Load definitions from `path`. Returns true on success.
    bool load(const std::string& path);

    // Given a `prefix`, return all matching keywords.
    std::vector<std::string> complete(const std::string& prefix) const;

    // True after a successful load.
    bool loaded() const { return loaded_; }

    // Access the full keyword list.
    const std::vector<std::string>& keywords() const { return keywords_; }

private:
    bool loaded_ = false;
    std::vector<std::string> keywords_;
};

} // namespace on1x_cli
