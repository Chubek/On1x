cmake_minimum_required(VERSION 3.20)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

project(On1x LANGUAGES C CXX)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
  message(FATAL_ERROR "In-source CMake builds are not supported. Use cmake -S . -B build")
endif()

option(ON1X_ENABLE_ADDONS "Enable editor and packaging assets" ON)
option(ON1X_ENABLE_THIRD_PARTY "Register vendored dependencies" ON)
option(ON1X_ENABLE_TOOLCHAIN "Register toolchain helpers" ON)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(on1x INTERFACE)
add_library(on1x::on1x ALIAS on1x)

add_subdirectory(include)
add_subdirectory(src)

if(ON1X_ENABLE_TOOLCHAIN AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/toolchain/CMakeLists.txt")
  add_subdirectory(toolchain)
endif()

if(ON1X_ENABLE_ADDONS AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/addons/CMakeLists.txt")
  add_subdirectory(addons)
endif()

if(ON1X_ENABLE_THIRD_PARTY AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/CMakeLists.txt")
  add_subdirectory(third_party)
endif()

if(TARGET on1x_core AND TARGET on1x_vendor)
  target_link_libraries(on1x_core PUBLIC on1x::vendor)
endif()

if(TARGET on1x_core)
  target_link_libraries(on1x INTERFACE on1x_core)
endif()

install(TARGETS on1x on1x_core
  EXPORT On1xTargets
)

install(EXPORT On1xTargets
  NAMESPACE on1x::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/On1x
)

configure_package_config_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake/On1xConfig.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/On1xConfig.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/On1x
)

write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/On1xConfigVersion.cmake
  VERSION 0.1.0
  COMPATIBILITY SameMajorVersion
)

install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/On1xConfig.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/On1xConfigVersion.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/On1x
)
