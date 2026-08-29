#include "on1x/core/api.hpp"

#include <iostream>
#include <iterator>

int main() {
  std::string source((std::istreambuf_iterator<char>(std::cin)), std::istreambuf_iterator<char>());
  auto result = on1x::eval_string(source);
  if (result.code != on1x::status::ok) {
    std::cerr << result.error << "\n";
    return 1;
  }
  std::cout << result.output;
  return 0;
}
