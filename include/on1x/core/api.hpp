#ifndef ON1X_CORE_API_HPP
#define ON1X_CORE_API_HPP

#include <string>

namespace on1x {

inline constexpr int version_major = 0;
inline constexpr int version_minor = 1;
inline constexpr int version_patch = 0;
inline constexpr const char *version_string = "0.1.0";

enum class status {
  ok = 0,
  parse_error = 1,
  runtime_error = 2,
  io_error = 3,
};

struct result {
  status code = status::ok;
  std::string output;
  std::string error;
};

struct program;

program parse_program(const std::string &source, std::string &error);
result eval_string(const std::string &source);
result eval_file(const std::string &path);

}  // namespace on1x

#endif
