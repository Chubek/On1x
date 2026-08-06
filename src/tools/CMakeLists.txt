# src/tools — host prelude object library for CLI / REPL / dump tooling.
# Does NOT contain its own Print implementation; delegates to Io.

add_library(on1x_tools OBJECT
    host_prelude.cpp
)

target_include_directories(on1x_tools PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_BINARY_DIR}/generated
)

target_compile_features(on1x_tools PRIVATE cxx_std_20)
