add_library(on1x_ffi STATIC
  ffi.cpp
)
add_library(on1x::ffi ALIAS on1x_ffi)

target_include_directories(on1x_ffi
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/dyncall/dyncall
    ${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/dyncall/dynload
)

target_compile_features(on1x_ffi PUBLIC cxx_std_17)
target_link_libraries(on1x_ffi PRIVATE on1x::dep_dyncall)

if(MSVC)
  target_compile_options(on1x_ffi PRIVATE /W4)
else()
  target_compile_options(on1x_ffi PRIVATE -Wall -Wextra -Wpedantic)
endif()
