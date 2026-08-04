add_executable(on1x_syntax_tests syntax_tests.cpp)
target_link_libraries(on1x_syntax_tests PRIVATE on1x_syntax on1x_util)

if(MSVC)
  target_compile_options(on1x_syntax_tests PRIVATE /W4 /permissive-)
else()
  target_compile_options(on1x_syntax_tests PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion)
endif()

add_test(NAME on1x_syntax_tests COMMAND on1x_syntax_tests)
