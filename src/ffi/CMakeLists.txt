add_library(on1x_ffi STATIC
  ffi.cpp
  signature.cpp
  marshal.cpp
  dyncall_bridge.cpp
  dyncall_callback.cpp
  dynalo_loader.cpp
  native_adapter.cpp
  library_registry.cpp
)

target_include_directories(on1x_ffi
  PUBLIC ${PROJECT_SOURCE_DIR}/src
  PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/third_party/dyncall/dyncall
    ${PROJECT_SOURCE_DIR}/third_party/dyncall/dynload
)
target_link_libraries(on1x_ffi PRIVATE on1x_core on1x_gc dyncall_s dynload_s dynalo)
target_compile_features(on1x_ffi PUBLIC cxx_std_20)

if(MSVC)
  target_compile_options(on1x_ffi PRIVATE /W4 /permissive-)
else()
  target_compile_options(on1x_ffi PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion)
endif()
