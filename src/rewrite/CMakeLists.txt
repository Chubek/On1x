add_library(on1x_rewrite STATIC
  rewrite.cpp
)
add_library(on1x::rewrite ALIAS on1x_rewrite)

target_include_directories(on1x_rewrite
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_compile_features(on1x_rewrite PUBLIC cxx_std_17)
target_link_libraries(on1x_rewrite PUBLIC on1x::core)

if(MSVC)
  target_compile_options(on1x_rewrite PRIVATE /W4)
else()
  target_compile_options(on1x_rewrite PRIVATE -Wall -Wextra -Wpedantic)
endif()

install(TARGETS on1x_rewrite
  EXPORT On1xTargets
)
