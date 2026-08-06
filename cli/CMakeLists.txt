# --- Klyspec header-only interface target ---
add_library(klyspec INTERFACE)
target_include_directories(klyspec INTERFACE
  ${PROJECT_SOURCE_DIR}/third_party/Klyspec/include
  ${PROJECT_SOURCE_DIR}/third_party/Klyspec/third_party
  ${PROJECT_SOURCE_DIR}/third_party/Klyspec/third_party/yaml-cpp/include
)
target_compile_features(klyspec INTERFACE cxx_std_20)

# --- yaml-cpp (vendored in Klyspec) ---
set(YAML_CPP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
set(YAML_CPP_BUILD_CONTRIB OFF CACHE BOOL "" FORCE)
add_subdirectory(${PROJECT_SOURCE_DIR}/third_party/Klyspec/third_party/yaml-cpp
  ${CMAKE_CURRENT_BINARY_DIR}/yaml-cpp
  EXCLUDE_FROM_ALL)
target_link_libraries(klyspec INTERFACE yaml-cpp)

# --- On1x CLI executable ---
add_executable(on1x-cli
  on1x.cpp
  On1x-CLI.cpp
  On1x-Readline.cpp
  On1x-Syntax.cpp
  On1x-Autocomp.cpp
)
set_target_properties(on1x-cli PROPERTIES OUTPUT_NAME on1x)

target_include_directories(on1x-cli PRIVATE
  ${PROJECT_SOURCE_DIR}/cli
  ${PROJECT_SOURCE_DIR}/include
  ${PROJECT_BINARY_DIR}/generated
)

target_link_libraries(on1x-cli PRIVATE
  klyspec
  on1x
  on1x_tools
  on1x_stdlib
)
target_link_options(on1x-cli PRIVATE
  "LINKER:--whole-archive,$<TARGET_FILE:on1x>,--no-whole-archive"
)
