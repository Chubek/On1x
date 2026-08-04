add_library(on1x_core OBJECT
  value.cpp
  type.cpp
  tag_table.cpp
  string.cpp
  list.cpp
  table.cpp
  tagged_list.cpp
  optional.cpp
  result.cpp
  iota.cpp
  number.cpp
  equality.cpp
  hashing.cpp
  tostring.cpp
  reserved_tags.cpp
)

target_include_directories(on1x_core PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(on1x_core PRIVATE on1x_util on1x_gc)

if(MSVC)
  target_compile_options(on1x_core PRIVATE /W4 /permissive-)
else()
  target_compile_options(on1x_core PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion)
endif()
