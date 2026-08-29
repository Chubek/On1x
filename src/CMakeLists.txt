add_library(on1x_core STATIC
  core/ast.cpp
  core/builtins.cpp
  core/eval.cpp
  core/parser.cpp
  core/runtime.cpp
)
add_library(on1x::core ALIAS on1x_core)

target_include_directories(on1x_core
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_compile_features(on1x_core PUBLIC cxx_std_17)

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
