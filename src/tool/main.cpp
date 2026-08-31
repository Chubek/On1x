#include "on1x/core/api.hpp"
#include "on1x/frontend/parser.hpp"

#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>

int main(int argc, char **argv) {
  if (argc < 2) {
    std::cerr << "usage: on1x run <file> | on1x -e <expr> | on1x check <file> | on1x repl\n";
    return 2;
  }
  on1x::result r;
  const std::string mode = argv[1];
  if (mode == "-e") {
    if (argc < 3) {
      std::cerr << "missing expression\n";
      return 2;
    }
    r = on1x::eval_string(argv[2]);
  } else if (mode == "check") {
    if (argc < 3) {
      std::cerr << "missing file\n";
      return 2;
    }
    std::string error;
    std::ifstream in(argv[2]);
    if (!in) {
      std::cerr << "failed to open file: " << argv[2] << "\n";
      return 1;
    }
    std::ostringstream buffer;
    buffer << in.rdbuf();
    auto program = on1x::parse_program(buffer.str(), error);
    if (!error.empty()) {
      std::cerr << error << "\n";
      return 1;
    }
    (void)program;
    return 0;
  } else if (mode == "repl") {
    std::string line;
    while (std::cout << "on1x> " && std::getline(std::cin, line)) {
      if (line == ":quit" || line == ":q") break;
      r = on1x::eval_string(line);
      if (r.code != on1x::status::ok) {
        std::cerr << r.error << "\n";
      } else {
        std::cout << r.output << "\n";
      }
    }
    return 0;
  } else if (mode == "run") {
    if (argc < 3) {
      std::cerr << "missing file\n";
      return 2;
    }
    r = on1x::eval_file(argv[2]);
  } else if (argc == 2) {
    r = on1x::eval_file(argv[1]);
  } else {
    r = on1x::eval_file(argv[1]);
  }
  if (r.code != on1x::status::ok) {
    std::cerr << r.error << "\n";
    return 1;
  }
  std::cout << r.output << "\n";
  return 0;
}
