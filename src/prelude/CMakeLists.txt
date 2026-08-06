add_library(on1x_prelude STATIC
  prelude.cpp
  introspect.cpp
  sequence.cpp
  table_ops.cpp
  iota_builtin.cpp
  optional_ops.cpp
  result_ops.cpp
  string_ops.cpp
)
target_include_directories(on1x_prelude PRIVATE ${PROJECT_SOURCE_DIR}/src ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(on1x_prelude PRIVATE on1x_core on1x_gc on1x_runtime)
if(MSVC)
  target_compile_options(on1x_prelude PRIVATE /W4 /permissive-)
else()
  target_compile_options(on1x_prelude PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion)
endif()
