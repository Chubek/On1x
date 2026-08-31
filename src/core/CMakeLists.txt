add_library(on1x_core STATIC
  builtins.cpp
  c_api.cpp
  eval.cpp
  runtime.cpp
)
add_library(on1x::core ALIAS on1x_core)

target_include_directories(on1x_core
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/klib
)

target_compile_features(on1x_core PUBLIC cxx_std_17)
target_link_libraries(on1x_core PUBLIC on1x::frontend)

if(MSVC)
  target_compile_options(on1x_core PRIVATE /W4)
else()
  target_compile_options(on1x_core PRIVATE -Wall -Wextra -Wpedantic)
endif()
