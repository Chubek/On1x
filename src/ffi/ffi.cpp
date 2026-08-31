#include "on1x/ffi/ffi.hpp"

#include <cstring>
#include <cstdlib>
#include <stdexcept>
#include <utility>

#include "dyncall.h"
#include "dynload.h"

namespace on1x::ffi {
namespace {

[[noreturn]] void unsupported(const char *what) {
  throw std::runtime_error(what);
}

void push_argument(DCCallVM *vm, ctype type, const value &argument) {
  switch (type) {
    case ctype::c_int:
      dcArgInt(vm, static_cast<DCint>(std::get<std::int64_t>(argument)));
      return;
    case ctype::c_float:
      dcArgFloat(vm, static_cast<DCfloat>(std::get<double>(argument)));
      return;
    case ctype::c_double:
      dcArgDouble(vm, static_cast<DCdouble>(std::get<double>(argument)));
      return;
    case ctype::c_ptr:
      dcArgPointer(vm, std::get<pointer>(argument));
      return;
    case ctype::c_string:
      dcArgPointer(vm, const_cast<char *>(std::get<std::string>(argument).c_str()));
      return;
    case ctype::void_:
    case ctype::c_struct:
      unsupported("ffi: unsupported argument type");
  }
}

value invoke(DCCallVM *vm, pointer address, const signature &signature, const std::vector<value> &arguments) {
  if (signature.arguments.size() != arguments.size()) {
    throw std::invalid_argument("ffi: argument count mismatch");
  }

  dcReset(vm);
  for (std::size_t index = 0; index < arguments.size(); ++index) {
    push_argument(vm, signature.arguments[index], arguments[index]);
  }

  switch (signature.result) {
    case ctype::void_:
      dcCallVoid(vm, address);
      return std::monostate{};
    case ctype::c_int:
      return static_cast<std::int64_t>(dcCallInt(vm, address));
    case ctype::c_float:
      return static_cast<double>(dcCallFloat(vm, address));
    case ctype::c_double:
      return static_cast<double>(dcCallDouble(vm, address));
    case ctype::c_ptr:
      return static_cast<pointer>(dcCallPointer(vm, address));
    case ctype::c_string:
      if (auto *text = static_cast<const char *>(dcCallPointer(vm, address)); text) {
        return std::string(text);
      }
      return std::string();
    case ctype::c_struct:
      unsupported("ffi: struct return types are not supported yet");
  }
  return std::monostate{};
}

}  // namespace

library::library(std::string path) : handle_(dlLoadLibrary(path.c_str())), path_(std::move(path)) {}

library::~library() noexcept {
  if (handle_) {
    dlFreeLibrary(static_cast<DLLib *>(handle_));
    handle_ = nullptr;
  }
}

library::library(library &&other) noexcept : handle_(std::exchange(other.handle_, nullptr)), path_(std::move(other.path_)) {}

library &library::operator=(library &&other) noexcept {
  if (this == &other) return *this;
  if (handle_) {
    dlFreeLibrary(static_cast<DLLib *>(handle_));
  }
  handle_ = other.handle_;
  path_ = std::move(other.path_);
  other.handle_ = nullptr;
  return *this;
}

bool library::loaded() const noexcept {
  return handle_ != nullptr;
}

const std::string &library::path() const noexcept {
  return path_;
}

pointer library::symbol(const std::string &name) const noexcept {
  if (!handle_) return nullptr;
  return dlFindSymbol(static_cast<DLLib *>(handle_), name.c_str());
}

function::function(pointer address, signature signature) noexcept
    : address_(address), signature_(std::move(signature)) {}

bool function::valid() const noexcept {
  return address_ != nullptr;
}

const signature &function::sig() const noexcept {
  return signature_;
}

pointer function::address() const noexcept {
  return address_;
}

value function::call(const std::vector<value> &arguments) const {
  DCCallVM *vm = dcNewCallVM(4096);
  if (!vm) {
    throw std::runtime_error("ffi: failed to allocate call vm");
  }
  dcMode(vm, DC_CALL_C_DEFAULT);
  try {
    auto result = invoke(vm, address_, signature_, arguments);
    dcFree(vm);
    return result;
  } catch (...) {
    dcFree(vm);
    throw;
  }
}

library load_library(const std::string &path) {
  return library(path);
}

function get_function(const library &library, const std::string &name, signature signature) {
  return function(library.symbol(name), std::move(signature));
}

value call(const function &function, const std::vector<value> &arguments) {
  if (!function.valid()) {
    throw std::runtime_error("ffi: invalid function handle");
  }
  return function.call(arguments);
}

pointer allocate(std::size_t bytes) {
  return std::malloc(bytes);
}

void free(pointer memory) noexcept {
  std::free(memory);
}

std::vector<std::uint8_t> read(pointer memory, std::size_t bytes) {
  std::vector<std::uint8_t> buffer(bytes);
  if (bytes != 0 && memory) {
    std::memcpy(buffer.data(), memory, bytes);
  }
  return buffer;
}

void write(pointer memory, const std::vector<std::uint8_t> &bytes) {
  if (memory && !bytes.empty()) {
    std::memcpy(memory, bytes.data(), bytes.size());
  }
}

std::int64_t read_int(pointer memory) {
  std::int64_t value = 0;
  if (memory) {
    std::memcpy(&value, memory, sizeof(value));
  }
  return value;
}

void write_int(pointer memory, std::int64_t value) {
  if (memory) {
    std::memcpy(memory, &value, sizeof(value));
  }
}

double read_float(pointer memory) {
  double value = 0.0;
  if (memory) {
    std::memcpy(&value, memory, sizeof(value));
  }
  return value;
}

void write_float(pointer memory, double value) {
  if (memory) {
    std::memcpy(memory, &value, sizeof(value));
  }
}

std::string read_string(pointer memory) {
  if (!memory) return {};
  return static_cast<const char *>(memory);
}

void write_string(pointer memory, const std::string &text) {
  if (!memory) return;
  std::memcpy(memory, text.c_str(), text.size() + 1);
}

}  // namespace on1x::ffi
