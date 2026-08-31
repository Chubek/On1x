if(NOT TARGET make_dparser)
  add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../third_party/dparser ${CMAKE_CURRENT_BINARY_DIR}/third_party/dparser EXCLUDE_FROM_ALL)
endif()

set(ON1X_FRONTEND_DIR ${CMAKE_CURRENT_SOURCE_DIR}/frontend)
set(ON1X_GENERATED_DIR ${CMAKE_CURRENT_BINARY_DIR}/frontend)
file(MAKE_DIRECTORY ${ON1X_GENERATED_DIR})

add_custom_command(
  OUTPUT ${ON1X_GENERATED_DIR}/On1x.g.d_parser.c
  DEPENDS make_dparser ${ON1X_FRONTEND_DIR}/On1x.g
  WORKING_DIRECTORY ${ON1X_GENERATED_DIR}
  VERBATIM
  COMMAND $<TARGET_FILE:make_dparser> ${ON1X_FRONTEND_DIR}/On1x.g -o ${ON1X_GENERATED_DIR}/On1x.g.d_parser.c
)
set_source_files_properties(${ON1X_GENERATED_DIR}/On1x.g.d_parser.c PROPERTIES GENERATED 1)

add_library(on1x_core STATIC
  core/ast.cpp
  core/c_api.cpp
  core/builtins.cpp
  core/eval.cpp
  core/parser.cpp
  core/runtime.cpp
  ../third_party/klib/kbarena.c
  ../third_party/klib/kmempool.c
  ../third_party/klib/kstring.c
  ${ON1X_GENERATED_DIR}/On1x.g.d_parser.c
)
add_library(on1x::core ALIAS on1x_core)

target_include_directories(on1x_core
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_compile_features(on1x_core PUBLIC cxx_std_17)
target_include_directories(on1x_core PRIVATE
  ${ON1X_GENERATED_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/dparser
  ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/klib
)
target_link_libraries(on1x_core PRIVATE dparse)

if(MSVC)
  target_compile_options(on1x_core PRIVATE /W4)
else()
  target_compile_options(on1x_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(on1x_tool
  tool/main.cpp
)
target_link_libraries(on1x_tool PRIVATE on1x_core)
set_target_properties(on1x_tool PROPERTIES OUTPUT_NAME on1x)
install(TARGETS on1x_tool
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

 if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/rewrite/CMakeLists.txt")
   add_subdirectory(rewrite)
 endif()
 
 if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/egraph/CMakeLists.txt")
   add_subdirectory(egraph)
 endif()

 if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/sema/CMakeLists.txt")
   add_subdirectory(sema)
 endif()

 if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/dataflow/CMakeLists.txt")
   add_subdirectory(dataflow)
 endif()
