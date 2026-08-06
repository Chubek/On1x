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

# --- PikoRL header-only interface target ---
add_library(pikorl INTERFACE)
target_include_directories(pikorl INTERFACE
  ${PROJECT_SOURCE_DIR}/third_party/PikoRL/include
  ${PROJECT_SOURCE_DIR}/third_party/PikoRL/third_party/QaMRpp/include
  ${PROJECT_SOURCE_DIR}/third_party/PikoRL/third_party/QaMRpp/third_party/SerdeTk
)
target_compile_features(pikorl INTERFACE cxx_std_17)

# --- On1x CLI executable ---
add_executable(on1x-cli
  on1x.cpp
  On1x-CLI.cpp
  On1x-Readline.cpp
)
set_target_properties(on1x-cli PROPERTIES OUTPUT_NAME on1x)

target_include_directories(on1x-cli PRIVATE
  ${PROJECT_SOURCE_DIR}/cli
  ${PROJECT_SOURCE_DIR}/include
  ${PROJECT_BINARY_DIR}/generated
)

target_link_libraries(on1x-cli PRIVATE
  klyspec
  pikorl
  on1x
  on1x_tools
  on1x_stdlib
)
target_link_options(on1x-cli PRIVATE
  "LINKER:--whole-archive,$<TARGET_FILE:on1x>,--no-whole-archive"
)

# Link against libdl for PikoRL's plugin loading.
if(UNIX)
  target_link_libraries(on1x-cli PRIVATE ${CMAKE_DL_LIBS})
endif()
