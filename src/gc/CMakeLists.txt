add_library(on1x_gc STATIC
  alloc.cpp
  finalizer.cpp
  gc.cpp
  handle_table.cpp
  roots.cpp
  shadow_stack.cpp
)

target_include_directories(on1x_gc PUBLIC "${PROJECT_SOURCE_DIR}/src")
target_link_libraries(on1x_gc PUBLIC bdwgc::gc)
target_compile_features(on1x_gc PUBLIC cxx_std_20)

if(MSVC)
  target_compile_options(on1x_gc PRIVATE /W4 /permissive-)
else()
  target_compile_options(on1x_gc PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion)
endif()
