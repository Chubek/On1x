add_library(on1x_memory STATIC
  memory.cpp
  ../../third_party/klib/kbarena.c
  ../../third_party/klib/kmempool.c
)
add_library(on1x::memory ALIAS on1x_memory)

target_include_directories(on1x_memory
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/klib
    ${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/libgc/include
)

target_compile_features(on1x_memory PUBLIC cxx_std_17)
target_link_libraries(on1x_memory PRIVATE on1x::dep_libgc on1x::klib)

if(MSVC)
  target_compile_options(on1x_memory PRIVATE /W4)
else()
  target_compile_options(on1x_memory PRIVATE -Wall -Wextra -Wpedantic)
endif()
