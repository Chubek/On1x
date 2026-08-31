add_library(on1x_sema STATIC
  sema.cpp
)
add_library(on1x::sema ALIAS on1x_sema)

target_include_directories(on1x_sema
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_compile_features(on1x_sema PUBLIC cxx_std_17)
target_link_libraries(on1x_sema PUBLIC on1x::core on1x::egraph)


if(MSVC)
  target_compile_options(on1x_sema PRIVATE /W4)
else()
  target_compile_options(on1x_sema PRIVATE -Wall -Wextra -Wpedantic)
endif()
install(TARGETS on1x_sema
  EXPORT On1xTargets
)

install(DIRECTORY error-patterns/
  DESTINATION ${CMAKE_INSTALL_DATADIR}/on1x/sema/error-patterns
  FILES_MATCHING PATTERN "*.rwr"
)
