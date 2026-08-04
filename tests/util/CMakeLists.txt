add_executable(on1x_util_tests util_tests.cpp)
target_link_libraries(on1x_util_tests PRIVATE on1x_util)

if(MSVC)
  target_compile_options(on1x_util_tests PRIVATE /W4 /permissive-)
else()
  target_compile_options(on1x_util_tests PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion)
endif()

add_test(NAME on1x_util_tests COMMAND on1x_util_tests)
