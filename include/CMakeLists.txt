target_include_directories(on1x INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

install(FILES
  On1x.h
  On1x-Plugin.h
  ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/equinox-ng/EquinoxNG.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/equinox-ng/RdvsgNG.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/equinox-ng/DSLUtils.hpp
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(DIRECTORY on1x
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  FILES_MATCHING
  PATTERN "*.h"
  PATTERN "*.hpp"
)
