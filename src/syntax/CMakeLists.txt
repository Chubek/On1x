add_library(on1x_syntax OBJECT
  ast.cpp
  diagnostics.cpp
  keywords.cpp
  literals.cpp
  parser.cpp
  source_map.cpp
  terminators.cpp
)

target_include_directories(on1x_syntax PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(on1x_syntax PRIVATE on1x_util)

if(MSVC)
  target_compile_options(on1x_syntax PRIVATE /W4 /permissive-)
else()
  target_compile_options(on1x_syntax PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion)
endif()
