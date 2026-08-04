add_library(on1x_sema STATIC
  resolver.cpp
  effect_scope.cpp
)

target_include_directories(on1x_sema PUBLIC ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(on1x_sema PUBLIC on1x_syntax)

if(MSVC)
  target_compile_options(on1x_sema PRIVATE /W4 /permissive-)
else()
  target_compile_options(on1x_sema PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion)
endif()
