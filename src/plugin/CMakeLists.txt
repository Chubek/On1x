add_library(on1x_plugin STATIC
  plugin.cpp
)
add_library(on1x::plugin ALIAS on1x_plugin)

target_include_directories(on1x_plugin
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/dynalo/include
)

target_compile_features(on1x_plugin PUBLIC cxx_std_17)
target_link_libraries(on1x_plugin PUBLIC on1x::core)

if(MSVC)
  target_compile_options(on1x_plugin PRIVATE /W4)
else()
  target_compile_options(on1x_plugin PRIVATE -Wall -Wextra -Wpedantic)
endif()

install(TARGETS on1x_plugin
  EXPORT On1xTargets
)
