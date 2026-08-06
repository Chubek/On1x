add_library(on1x_headers INTERFACE)
target_include_directories(on1x_headers INTERFACE
  ${PROJECT_SOURCE_DIR}/include
  ${PROJECT_BINARY_DIR}/generated
)
