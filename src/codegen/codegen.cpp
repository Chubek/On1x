#include "on1x/codegen/codegen.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace on1x::codegen {
namespace {

std::string trim(std::string s) {
  auto is_space = [](unsigned char c) { return std::isspace(c) != 0; };
  s.erase(s.begin(), std::find_if(s.begin(), s.end(), [&](char c) { return !is_space(static_cast<unsigned char>(c)); }));
  s.erase(std::find_if(s.rbegin(), s.rend(), [&](char c) { return !is_space(static_cast<unsigned char>(c)); }).base(), s.end());
  return s;
}

std::string strip_comment(const std::string &line) {
  bool in_string = false;
  for (std::size_t i = 0; i < line.size(); ++i) {
    if (line[i] == '"' && (i == 0 || line[i - 1] != '\\')) {
      in_string = !in_string;
    }
    if (!in_string && line[i] == '/' && i + 1 < line.size() && line[i + 1] == '/') {
      return line.substr(0, i);
    }
    if (!in_string && line[i] == '#' && trim(line.substr(0, i)).empty()) {
      return {};
    }
  }
  return line;
}

std::string unquote(std::string value) {
  value = trim(std::move(value));
  if (value.size() >= 2 && value.front() == '"' && value.back() == '"') {
    std::string out;
    out.reserve(value.size() - 2);
    for (std::size_t i = 1; i + 1 < value.size(); ++i) {
      char c = value[i];
      if (c == '\\' && i + 1 < value.size() - 1) {
        char n = value[++i];
        switch (n) {
          case 'n': out.push_back('\n'); break;
          case 'r': out.push_back('\r'); break;
          case 't': out.push_back('\t'); break;
          case '\\': out.push_back('\\'); break;
          case '"': out.push_back('"'); break;
          default: out.push_back(n); break;
        }
      } else {
        out.push_back(c);
      }
    }
    return out;
  }
  return value;
}

bool starts_with(const std::string &text, const std::string &prefix) {
  return text.rfind(prefix, 0) == 0;
}

std::optional<unsigned> parse_unsigned(const std::string &text) {
  if (text.empty()) return std::nullopt;
  char *end = nullptr;
  const unsigned long value = std::strtoul(text.c_str(), &end, 10);
  if (!end || *end != '\0') return std::nullopt;
  return static_cast<unsigned>(value);
}

std::vector<std::string> split_entries(const std::string &body) {
  std::vector<std::string> parts;
  std::string current;
  int paren_depth = 0;
  for (char c : body) {
    if (c == '(') ++paren_depth;
    if (c == ')') --paren_depth;
    if ((c == ',' || c == ';') && paren_depth == 0) {
      std::string trimmed = trim(current);
      if (!trimmed.empty()) parts.push_back(trimmed);
      current.clear();
      continue;
    }
    current.push_back(c);
  }
  std::string trimmed = trim(current);
  if (!trimmed.empty()) parts.push_back(trimmed);
  return parts;
}

std::string collapse_block_body(const std::vector<std::string> &lines) {
  std::string out;
  for (const auto &line : lines) {
    if (!out.empty()) out.push_back('\n');
    out += line;
  }
  return out;
}

struct block {
  std::string kind;
  std::string name;
  std::vector<std::string> body;
};

std::vector<std::string> read_lines(const std::filesystem::path &path) {
  std::ifstream in(path);
  if (!in) throw std::runtime_error("failed to open ISA file: " + path.string());
  std::vector<std::string> lines;
  std::string line;
  while (std::getline(in, line)) lines.push_back(line);
  return lines;
}

std::vector<block> parse_blocks(const std::vector<std::string> &lines) {
  std::vector<block> out;
  for (std::size_t i = 0; i < lines.size();) {
    std::string line = trim(strip_comment(lines[i]));
    if (line.empty()) {
      ++i;
      continue;
    }
    const auto brace = line.find('{');
    if (brace == std::string::npos) {
      ++i;
      continue;
    }

    std::istringstream header(line.substr(0, brace));
    block b;
    header >> b.kind >> b.name;
    if (b.kind.empty() || b.name.empty()) {
      throw std::runtime_error("invalid ISA block header: " + line);
    }

    int depth = 1;
    std::string tail = line.substr(brace + 1);
    if (!trim(tail).empty()) b.body.push_back(trim(tail));
    ++i;
    while (i < lines.size() && depth > 0) {
      std::string body_line = strip_comment(lines[i]);
      for (char c : body_line) {
        if (c == '{') ++depth;
        if (c == '}') --depth;
      }
      std::string cleaned = trim(body_line);
      if (depth > 0 && !cleaned.empty()) b.body.push_back(cleaned);
      ++i;
    }
    out.push_back(std::move(b));
  }
  return out;
}

std::pair<std::string, std::string> split_once(const std::string &text, char delim) {
  const auto pos = text.find(delim);
  if (pos == std::string::npos) return {trim(text), {}};
  return {trim(text.substr(0, pos)), trim(text.substr(pos + 1))};
}

reg_entry parse_reg_entry(const std::string &text) {
  reg_entry entry;
  const auto open = text.find('(');
  const auto close = text.find(')', open == std::string::npos ? 0 : open + 1);
  const auto eq = text.find('=', close == std::string::npos ? 0 : close + 1);
  if (open == std::string::npos || close == std::string::npos) {
    throw std::runtime_error("invalid regclass entry: " + text);
  }
  entry.name = trim(text.substr(0, open));
  entry.width = parse_unsigned(text.substr(open + 1, close - open - 1)).value_or(0);
  if (eq != std::string::npos) {
    entry.encoding = parse_unsigned(trim(text.substr(eq + 1)));
  }
  return entry;
}

encoding_field parse_encoding_field(const std::string &text) {
  auto [name, width_text] = split_once(text, ':');
  encoding_field field;
  field.name = name;
  field.width = parse_unsigned(width_text).value_or(0);
  return field;
}

void parse_arch_block(const block &b, document &doc) {
  arch out;
  out.name = b.name;
  for (const auto &line : b.body) {
    auto stmt = trim(line);
    if (stmt.empty()) continue;
    if (!stmt.empty() && stmt.back() == ';') stmt.pop_back();
    auto [key, value] = split_once(stmt, '=');
    value = unquote(value);
    out.properties[key] = value;
    if (key == "word_size") out.word_size = parse_unsigned(value).value_or(0);
    else if (key == "addr_size") out.addr_size = parse_unsigned(value).value_or(0);
    else if (key == "endian") out.endian = value;
    else if (key == "align") out.align = parse_unsigned(value).value_or(0);
  }
  doc.architecture = std::move(out);
}

void parse_regclass_block(const block &b, document &doc) {
  regclass out;
  out.name = b.name;
  const std::string body = collapse_block_body(b.body);
  for (const auto &entry_text : split_entries(body)) {
    out.entries.push_back(parse_reg_entry(entry_text));
  }
  doc.regclasses.push_back(std::move(out));
}

void parse_alias_block(const block &b, document &doc) {
  const std::string body = collapse_block_body(b.body);
  for (const auto &entry_text : split_entries(body)) {
    auto [lhs, rhs] = split_once(entry_text, '=');
    if (lhs.empty() || rhs.empty()) continue;
    doc.aliases.push_back(alias{lhs, rhs});
  }
}

void parse_flags_block(const block &b, document &doc) {
  flag_set out;
  out.name = b.name;
  const std::string body = collapse_block_body(b.body);
  for (const auto &entry_text : split_entries(body)) {
    auto entry = trim(entry_text);
    if (!entry.empty()) out.values.push_back(entry);
  }
  doc.flags.push_back(std::move(out));
}

void parse_encoding_block(const block &b, document &doc) {
  encoding out;
  out.name = b.name;
  for (const auto &line : b.body) {
    auto stmt = trim(line);
    if (stmt.empty()) continue;
    if (!stmt.empty() && stmt.back() == ';') stmt.pop_back();
    out.fields.push_back(parse_encoding_field(stmt));
  }
  doc.encodings.push_back(std::move(out));
}

void parse_op_block(const block &b, document &doc) {
  op out;
  out.name = b.name;
  for (const auto &line : b.body) {
    auto stmt = trim(line);
    if (stmt.empty()) continue;
    if (!stmt.empty() && stmt.back() == ';') stmt.pop_back();
    auto [key, value] = split_once(stmt, '=');
    value = unquote(value);
    out.properties[key] = value;
    if (key == "opcode") out.opcode = parse_unsigned(value);
  }
  doc.operations.push_back(std::move(out));
}

}  // namespace

const document *bundle::find_architecture(const std::string &name) const noexcept {
  for (const auto &doc : documents) {
    if (doc.architecture && doc.architecture->name == name) return &doc;
  }
  return nullptr;
}

std::vector<std::string> bundle::architectures() const {
  std::vector<std::string> out;
  out.reserve(documents.size());
  for (const auto &doc : documents) {
    if (doc.architecture) out.push_back(doc.architecture->name);
  }
  return out;
}

document parse_isa_file(const std::filesystem::path &path) {
  document doc;
  doc.path = path;
  const auto lines = read_lines(path);
  const auto blocks = parse_blocks(lines);
  for (const auto &b : blocks) {
    if (b.kind == "arch") parse_arch_block(b, doc);
    else if (b.kind == "regclass") parse_regclass_block(b, doc);
    else if (b.kind == "alias") parse_alias_block(b, doc);
    else if (b.kind == "flags") parse_flags_block(b, doc);
    else if (b.kind == "encoding") parse_encoding_block(b, doc);
    else if (b.kind == "op") parse_op_block(b, doc);
  }
  for (const auto &raw_line : lines) {
    std::string line = trim(strip_comment(raw_line));
    if (!starts_with(line, "alias ")) continue;
    if (!line.empty() && line.back() == ';') line.pop_back();
    auto [lhs, rhs] = split_once(line.substr(6), '=');
    if (lhs.empty() || rhs.empty()) continue;
    doc.aliases.push_back(alias{lhs, rhs});
  }
  return doc;
}

bundle load_isa_directory(const std::filesystem::path &path) {
  bundle out;
  if (!std::filesystem::exists(path)) return out;
  for (const auto &entry : std::filesystem::directory_iterator(path)) {
    if (!entry.is_regular_file()) continue;
    if (entry.path().extension() != ".isa") continue;
    out.documents.push_back(parse_isa_file(entry.path()));
  }
  std::sort(out.documents.begin(), out.documents.end(), [](const document &a, const document &b) {
    return a.path.filename().string() < b.path.filename().string();
  });
  return out;
}

bundle load_bundled_isa_directory() {
#ifdef ON1X_CODEGEN_ISA_DIR
  return load_isa_directory(std::filesystem::path(ON1X_CODEGEN_ISA_DIR));
#else
  return {};
#endif
}

std::string describe(const document &doc) {
  std::ostringstream out;
  if (doc.architecture) {
    out << doc.architecture->name << ": ";
    out << doc.regclasses.size() << " regclasses, ";
    out << doc.aliases.size() << " aliases, ";
    out << doc.flags.size() << " flags, ";
    out << doc.encodings.size() << " encodings, ";
    out << doc.operations.size() << " ops";
  } else {
    out << doc.path.filename().string();
  }
  return out.str();
}

}  // namespace on1x::codegen
