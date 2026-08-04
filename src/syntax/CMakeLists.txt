set(ON1X_GENERATED_GRAMMAR
  ${CMAKE_CURRENT_BINARY_DIR}/grammar/on1x.g.d_parser.c
)

add_custom_command(
  OUTPUT ${ON1X_GENERATED_GRAMMAR}
  COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/grammar
  COMMAND $<TARGET_FILE:make_dparser>
          ${CMAKE_CURRENT_SOURCE_DIR}/grammar/on1x.g
          -o ${ON1X_GENERATED_GRAMMAR}
  DEPENDS make_dparser ${CMAKE_CURRENT_SOURCE_DIR}/grammar/on1x.g
  VERBATIM
)

add_custom_target(on1x_syntax_grammar DEPENDS ${ON1X_GENERATED_GRAMMAR})

add_library(on1x_syntax OBJECT
  ast.cpp
  ast_builder.cpp
  ast_printer.cpp
  diagnostics.cpp
  keywords.cpp
  literals.cpp
  parser.cpp
  source_map.cpp
  terminators.cpp
)

add_dependencies(on1x_syntax on1x_syntax_grammar)
target_include_directories(on1x_syntax PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(on1x_syntax PRIVATE on1x_util)

if(MSVC)
  target_compile_options(on1x_syntax PRIVATE /W4 /permissive-)
else()
  target_compile_options(on1x_syntax PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion)
endif()
