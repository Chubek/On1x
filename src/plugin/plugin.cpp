#include "on1x/plugin/plugin.hpp"

#include <utility>

namespace on1x::plugin {
namespace {

template <typename Signature>
Signature *try_get(dynalo::native::handle handle, const std::string &name) noexcept {
  try {
    return dynalo::get_function<Signature>(handle, name);
  } catch (...) {
    return nullptr;
  }
}

const on1x_plugin_api &api_instance() noexcept {
  static const on1x_plugin_api api{
      ON1X_PLUGIN_ABI_VERSION,
      &on1x_vm_create,
      &on1x_vm_destroy,
      &on1x_vm_reset,
      &on1x_vm_eval_string,
      &on1x_vm_eval_file,
      &on1x_status_string,
      &on1x_string_free,
  };
  return api;
}

}  // namespace

const on1x_plugin_api &host_api() noexcept { return api_instance(); }

library::library(std::string path) : handle_(dynalo::open(path)), path_(std::move(path)) {
  exports_.init = try_get<on1x_status(on1x_vm *)>(handle_, "on1x_plugin_init");
  exports_.init_with_api = try_get<on1x_status(const on1x_plugin_api *, on1x_vm *)>(
      handle_, "on1x_plugin_init_with_api");
  exports_.shutdown = try_get<void(on1x_vm *)>(handle_, "on1x_plugin_shutdown");
}

library::~library() noexcept {
  if (handle_ != dynalo::native::invalid_handle()) {
    try {
      dynalo::close(handle_);
    } catch (...) {
    }
  }
}

library::library(library &&other) noexcept
    : handle_(other.handle_), path_(std::move(other.path_)), exports_(other.exports_) {
  other.handle_ = dynalo::native::invalid_handle();
  other.exports_ = {};
}

library &library::operator=(library &&other) noexcept {
  if (this == &other) return *this;
  if (handle_ != dynalo::native::invalid_handle()) {
    try {
      dynalo::close(handle_);
    } catch (...) {
    }
  }
  handle_ = other.handle_;
  path_ = std::move(other.path_);
  exports_ = other.exports_;
  other.handle_ = dynalo::native::invalid_handle();
  other.exports_ = {};
  return *this;
}

bool library::loaded() const noexcept {
  return handle_ != dynalo::native::invalid_handle();
}

const std::string &library::path() const noexcept { return path_; }

const exports &library::symbols() const noexcept { return exports_; }

on1x_status library::initialize(on1x_vm *vm) const {
  if (!loaded() || !vm) return ON1X_RUNTIME_ERROR;
  if (exports_.init_with_api) {
    return exports_.init_with_api(&host_api(), vm);
  }
  if (exports_.init) {
    return exports_.init(vm);
  }
  return ON1X_RUNTIME_ERROR;
}

void library::shutdown(on1x_vm *vm) const noexcept {
  if (!loaded() || !vm || !exports_.shutdown) return;
  try {
    exports_.shutdown(vm);
  } catch (...) {
  }
}

}  // namespace on1x::plugin
