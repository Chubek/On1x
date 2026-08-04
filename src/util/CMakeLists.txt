add_library(on1x_util STATIC
  arena.cpp
  hash.cpp
  panic.cpp
  platform.cpp
  string_builder.cpp
  utf8.cpp
)

target_include_directories(on1x_util PUBLIC "${PROJECT_SOURCE_DIR}/src")
target_compile_features(on1x_util PUBLIC cxx_std_20)
target_link_libraries(on1x_util PUBLIC utf8proc)

if(MSVC)
  target_compile_options(on1x_util PRIVATE /W4 /permissive-)
else()
  target_compile_options(on1x_util PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion)
endif()
