#ifndef ON1X_PLUGIN_PLUGIN_HPP
#define ON1X_PLUGIN_PLUGIN_HPP

#include "On1x-Plugin.h"

#include <string>

#include <dynalo/dynalo.hpp>

namespace on1x::plugin {

struct exports {
  on1x_plugin_init_fn init = nullptr;
  on1x_plugin_init_with_api_fn init_with_api = nullptr;
  on1x_plugin_shutdown_fn shutdown = nullptr;
};

const on1x_plugin_api &host_api() noexcept;

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
  const exports &symbols() const noexcept;

  on1x_status initialize(on1x_vm *vm) const;
  void shutdown(on1x_vm *vm) const noexcept;

 private:
  dynalo::native::handle handle_ = dynalo::native::invalid_handle();
  std::string path_;
  exports exports_;
};

}  // namespace on1x::plugin

#endif
