add_library(on1x_dataflow STATIC
  dataflow.cpp
)
add_library(on1x::dataflow ALIAS on1x_dataflow)

target_include_directories(on1x_dataflow
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_compile_features(on1x_dataflow PUBLIC cxx_std_17)
target_link_libraries(on1x_dataflow PUBLIC on1x::core on1x::sema)

if(MSVC)
  target_compile_options(on1x_dataflow PRIVATE /W4)
else()
  target_compile_options(on1x_dataflow PRIVATE -Wall -Wextra -Wpedantic)
endif()

install(TARGETS on1x_dataflow
  EXPORT On1xTargets
)
