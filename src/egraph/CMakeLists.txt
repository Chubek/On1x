add_library(on1x_egraph STATIC
  egraph.cpp
)
add_library(on1x::egraph ALIAS on1x_egraph)

target_include_directories(on1x_egraph
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/equinox-ng>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_compile_features(on1x_egraph PUBLIC cxx_std_20)
target_link_libraries(on1x_egraph PUBLIC on1x::core)

if(MSVC)
  target_compile_options(on1x_egraph PRIVATE /W4)
else()
  target_compile_options(on1x_egraph PRIVATE -Wall -Wextra -Wpedantic)
endif()

install(TARGETS on1x_egraph
  EXPORT On1xTargets
)
