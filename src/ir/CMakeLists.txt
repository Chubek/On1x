add_library(on1x_ir STATIC
  ir.cpp
  builder.cpp
  lower_ast.cpp
  lower_effect.cpp
  lower_match.cpp
  cfg.cpp
  verifier.cpp
)

target_include_directories(on1x_ir PUBLIC ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(on1x_ir PUBLIC on1x_sema on1x_syntax on1x_util)

if(MSVC)
  target_compile_options(on1x_ir PRIVATE /W4 /permissive-)
else()
  target_compile_options(on1x_ir PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion)
endif()
