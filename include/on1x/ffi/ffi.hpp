#ifndef ON1X_FFI_FFI_HPP
#define ON1X_FFI_FFI_HPP

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <variant>
#include <vector>

namespace on1x::ffi {

enum class ctype {
  void_,
  c_int,
  c_float,
  c_double,
  c_ptr,
  c_string,
  c_struct,
};

using pointer = void *;
using value = std::variant<std::monostate, std::int64_t, double, pointer, std::string>;

struct signature {
  ctype result = ctype::void_;
  std::vector<ctype> arguments;
};

class library {
 public:
  explicit library(std::string path);
  ~library() noexcept;

  library(library &&other) noexcept;
  library &operator=(library &&other) noexcept;

  library(const library &) = delete;
  library &operator=(const library &) = delete;

  bool loaded() const noexcept;
  const std::string &path() const noexcept;
  pointer symbol(const std::string &name) const noexcept;

 private:
  void *handle_ = nullptr;
  std::string path_;
};

class function {
 public:
  function() noexcept = default;
  function(pointer address, signature signature) noexcept;

  bool valid() const noexcept;
  const signature &sig() const noexcept;
  pointer address() const noexcept;
  value call(const std::vector<value> &arguments) const;

 private:
  pointer address_ = nullptr;
  signature signature_;
};

library load_library(const std::string &path);
function get_function(const library &library, const std::string &name, signature signature);
value call(const function &function, const std::vector<value> &arguments);

pointer allocate(std::size_t bytes);
void free(pointer memory) noexcept;
std::vector<std::uint8_t> read(pointer memory, std::size_t bytes);
void write(pointer memory, const std::vector<std::uint8_t> &bytes);
std::int64_t read_int(pointer memory);
void write_int(pointer memory, std::int64_t value);
double read_float(pointer memory);
void write_float(pointer memory, double value);
std::string read_string(pointer memory);
void write_string(pointer memory, const std::string &text);

}  // namespace on1x::ffi

#endif
