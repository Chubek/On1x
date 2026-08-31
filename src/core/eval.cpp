#include "on1x/core/api.hpp"
#include "on1x/core/runtime.hpp"
#include "on1x/frontend/parser.hpp"

#include <fstream>
#include <exception>
#include <sstream>

namespace on1x {

result eval_string(const std::string &source) {
  result out;
  std::string error;
  auto program = parse_program(source, error);
  if (!error.empty()) {
    out.code = status::parse_error;
    out.error = error;
    return out;
  }

  std::ostringstream captured;
  auto scope = make_prelude(&captured);
  std::string runtime_error;
  value_ptr last = make_value(std::monostate{});
  try {
    for (const auto &form : program.forms) {
      last = eval(form, scope, runtime_error);
      if (!runtime_error.empty()) break;
    }
  } catch (const std::exception &ex) {
    out.code = status::runtime_error;
    out.error = ex.what();
    return out;
  }

  if (!runtime_error.empty()) {
    out.code = status::runtime_error;
    out.error = runtime_error;
    return out;
  }

  out.output = captured.str();
  if (!out.output.empty() && out.output.back() != '\n') {
    out.output.push_back('\n');
  }
  out.output += to_string(last);
  return out;
}

result eval_file(const std::string &path) {
  std::ifstream in(path);
  if (!in) {
    return {status::io_error, {}, "failed to open file: " + path};
  }
  std::ostringstream buffer;
  buffer << in.rdbuf();
  return eval_string(buffer.str());
}

}  // namespace on1x
