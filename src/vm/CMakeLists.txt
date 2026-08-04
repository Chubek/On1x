add_library(on1x_vm STATIC
  chunk.cpp
  emitter.cpp
  frame.cpp
  interpreter.cpp
  disassembler.cpp
)

target_include_directories(on1x_vm
  PUBLIC ${PROJECT_SOURCE_DIR}/src
  PRIVATE ${PROJECT_SOURCE_DIR}/include
)
target_link_libraries(on1x_vm PUBLIC on1x_runtime on1x_core on1x_gc on1x_syntax)

if(MSVC)
  target_compile_options(on1x_vm PRIVATE /W4 /permissive-)
else()
  target_compile_options(on1x_vm PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion)
endif()
