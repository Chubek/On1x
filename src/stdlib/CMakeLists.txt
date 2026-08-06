add_library(on1x_stdlib STATIC
  capability.cpp
  module.cpp
  registry.cpp
  args.cpp
  std_all.cpp
)
target_include_directories(on1x_stdlib PUBLIC ${PROJECT_SOURCE_DIR}/src ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(on1x_stdlib PUBLIC on1x_runtime on1x_core on1x_gc)
if(MSVC)
  target_compile_options(on1x_stdlib PRIVATE /W4 /permissive-)
else()
  target_compile_options(on1x_stdlib PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion)
endif()
