#include "On1x.h"

#include "on1x/core/parser.hpp"
#include "on1x/core/runtime.hpp"

#include <cstdlib>
#include <cstring>
#include <exception>
#include <fstream>
#include <sstream>
#include <string>

struct on1x_vm {
  std::shared_ptr<on1x::env> scope;
  std::ostream *sink = nullptr;
};

namespace {

static char *duplicate_c_string(const std::string &text) {
  char *out = static_cast<char *>(std::malloc(text.size() + 1));
  if (!out) return nullptr;
  std::memcpy(out, text.c_str(), text.size() + 1);
  return out;
}

static on1x_status evaluate_source(on1x_vm *vm, const std::string &source, std::string &output) {
  if (!vm) return ON1X_RUNTIME_ERROR;

  std::string error;
  auto program = on1x::parse_program(source, error);
  if (!error.empty()) {
    return ON1X_PARSE_ERROR;
  }

  struct sink_guard {
    on1x_vm *vm;
    std::ostream *previous;
    ~sink_guard() { vm->sink = previous; }
  } guard{vm, vm->sink};

  std::ostringstream captured;
  vm->sink = &captured;
  auto scope = std::make_shared<on1x::env>(vm->scope);

  std::string runtime_error;
  on1x::value_ptr last = on1x::make_value(std::monostate{});
  try {
    for (const auto &form : program.forms) {
      last = on1x::eval(form, scope, runtime_error);
      if (!runtime_error.empty()) break;
    }
  } catch (const std::exception &ex) {
    output = ex.what();
    return ON1X_RUNTIME_ERROR;
  }

  if (!runtime_error.empty()) {
    output = runtime_error;
    return ON1X_RUNTIME_ERROR;
  }

  vm->scope = std::move(scope);
  output = captured.str();
  if (!output.empty() && output.back() != '\n') output.push_back('\n');
  output += on1x::to_string(last);
  return ON1X_OK;
}

}  // namespace

extern "C" {

on1x_vm *on1x_vm_create(void) {
  auto *vm = new on1x_vm();
  vm->scope = on1x::make_prelude(nullptr);
  on1x::bind(vm->scope, "print", on1x::make_value(on1x::native_fn([vm](const std::vector<on1x::value_ptr> &args) -> on1x::value_ptr {
    if (vm->sink && !args.empty()) (*vm->sink) << on1x::to_string(args[0]);
    return on1x::make_value(std::monostate{});
  })));
  on1x::bind(vm->scope, "println", on1x::make_value(on1x::native_fn([vm](const std::vector<on1x::value_ptr> &args) -> on1x::value_ptr {
    if (vm->sink) {
      if (!args.empty()) (*vm->sink) << on1x::to_string(args[0]);
      (*vm->sink) << '\n';
    }
    return on1x::make_value(std::monostate{});
  })));
  return vm;
}

void on1x_vm_destroy(on1x_vm *vm) {
  delete vm;
}

on1x_status on1x_vm_eval_string(on1x_vm *vm, const char *source, char **output) {
  if (output) *output = nullptr;
  if (!source) return ON1X_RUNTIME_ERROR;

  std::string text;
  auto status = evaluate_source(vm, source, text);
  if (status != ON1X_OK) return status;

  if (output) {
    *output = duplicate_c_string(text);
    if (!*output) return ON1X_RUNTIME_ERROR;
  }
  return ON1X_OK;
}

on1x_status on1x_vm_eval_file(on1x_vm *vm, const char *path, char **output) {
  if (output) *output = nullptr;
  if (!path) return ON1X_RUNTIME_ERROR;

  std::ifstream in(path);
  if (!in) return ON1X_IO_ERROR;

  std::ostringstream buffer;
  buffer << in.rdbuf();
  std::string text;
  auto status = evaluate_source(vm, buffer.str(), text);
  if (status != ON1X_OK) return status;

  if (output) {
    *output = duplicate_c_string(text);
    if (!*output) return ON1X_RUNTIME_ERROR;
  }
  return ON1X_OK;
}

void on1x_string_free(char *text) {
  std::free(text);
}

}  // extern "C"
