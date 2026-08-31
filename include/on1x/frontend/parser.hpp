#ifndef ON1X_FRONTEND_PARSER_HPP
#define ON1X_FRONTEND_PARSER_HPP

#include "on1x/frontend/ast.hpp"

#include <string>
#include <vector>

namespace on1x {

struct program {
  std::vector<expr_ptr> forms;
};

program parse_program(const std::string &source, std::string &error);

}  // namespace on1x

#endif
