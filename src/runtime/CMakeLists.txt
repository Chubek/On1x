add_library(on1x_runtime STATIC state.cpp stack.cpp)
target_include_directories(on1x_runtime
  PUBLIC ${PROJECT_SOURCE_DIR}/src
  PRIVATE ${PROJECT_SOURCE_DIR}/include
)
target_link_libraries(on1x_runtime PUBLIC on1x_core on1x_gc)
