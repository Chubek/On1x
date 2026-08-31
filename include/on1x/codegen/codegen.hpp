#ifndef ON1X_CODEGEN_CODEGEN_HPP
#define ON1X_CODEGEN_CODEGEN_HPP

#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace on1x::codegen {

struct reg_entry {
  std::string name;
  unsigned width = 0;
  std::optional<unsigned> encoding;
};

struct regclass {
  std::string name;
  std::vector<reg_entry> entries;
};

struct alias {
  std::string name;
  std::string target;
};

struct arch {
  std::string name;
  unsigned word_size = 0;
  unsigned addr_size = 0;
  std::string endian;
  unsigned align = 0;
  std::map<std::string, std::string> properties;
};

struct encoding_field {
  std::string name;
  unsigned width = 0;
};

struct encoding {
  std::string name;
  std::vector<encoding_field> fields;
};

struct flag_set {
  std::string name;
  std::vector<std::string> values;
};

struct op {
  std::string name;
  std::optional<unsigned> opcode;
  std::map<std::string, std::string> properties;
};

struct document {
  std::filesystem::path path;
  std::optional<arch> architecture;
  std::vector<regclass> regclasses;
  std::vector<alias> aliases;
  std::vector<flag_set> flags;
  std::vector<encoding> encodings;
  std::vector<op> operations;
};

struct bundle {
  std::vector<document> documents;

  const document *find_architecture(const std::string &name) const noexcept;
  std::vector<std::string> architectures() const;
};

document parse_isa_file(const std::filesystem::path &path);
bundle load_isa_directory(const std::filesystem::path &path);
bundle load_bundled_isa_directory();
std::string describe(const document &doc);

}  // namespace on1x::codegen

#endif
