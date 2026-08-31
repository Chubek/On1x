if(NOT TARGET make_dparser)
  add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/dparser ${CMAKE_CURRENT_BINARY_DIR}/third_party/dparser EXCLUDE_FROM_ALL)
endif()

set(ON1X_FRONTEND_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(ON1X_GENERATED_DIR ${CMAKE_CURRENT_BINARY_DIR})
file(MAKE_DIRECTORY ${ON1X_GENERATED_DIR})

add_custom_command(
  OUTPUT ${ON1X_GENERATED_DIR}/On1x.g.d_parser.c
  DEPENDS make_dparser ${ON1X_FRONTEND_DIR}/On1x.g
  WORKING_DIRECTORY ${ON1X_GENERATED_DIR}
  VERBATIM
  COMMAND $<TARGET_FILE:make_dparser> ${ON1X_FRONTEND_DIR}/On1x.g -o ${ON1X_GENERATED_DIR}/On1x.g.d_parser.c
)
set_source_files_properties(${ON1X_GENERATED_DIR}/On1x.g.d_parser.c PROPERTIES GENERATED 1)

add_library(on1x_frontend STATIC
  ast.cpp
  parser.cpp
  ${ON1X_GENERATED_DIR}/On1x.g.d_parser.c
)
add_library(on1x::frontend ALIAS on1x_frontend)

target_include_directories(on1x_frontend
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  PRIVATE
    ${ON1X_GENERATED_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/dparser
)

target_compile_features(on1x_frontend PUBLIC cxx_std_17)
target_link_libraries(on1x_frontend PRIVATE dparse)

if(MSVC)
  target_compile_options(on1x_frontend PRIVATE /W4)
else()
  target_compile_options(on1x_frontend PRIVATE -Wall -Wextra -Wpedantic)
endif()
