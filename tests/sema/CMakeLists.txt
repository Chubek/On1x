add_executable(on1x_sema_tests sema_tests.cpp)
target_link_libraries(on1x_sema_tests PRIVATE on1x_sema on1x_syntax on1x_util)

if(MSVC)
  target_compile_options(on1x_sema_tests PRIVATE /W4 /permissive-)
else()
  target_compile_options(on1x_sema_tests PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion)
endif()

add_test(NAME on1x_sema_tests COMMAND on1x_sema_tests)
