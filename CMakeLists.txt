cmake_minimum_required(VERSION 3.20)

project(on1x VERSION 0.1.0 LANGUAGES C CXX)

option(ON1X_BUILD_TESTS "Build the On1x test suite" ON)
option(ON1X_ENABLE_FFI "Enable the dyncall-based host FFI subsystem" OFF)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_subdirectory(third_party/utf8proc EXCLUDE_FROM_ALL)
add_subdirectory(third_party/libgc EXCLUDE_FROM_ALL)
if(ON1X_ENABLE_FFI)
  # dyncall's vendored CMakeLists declares a 2.6 minimum. Modern CMake
  # requires this compatibility floor without modifying vendored sources.
  set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
  set(LANG_CXX OFF CACHE BOOL "Disable dyncall C++ test support" FORCE)
  add_subdirectory(third_party/dyncall EXCLUDE_FROM_ALL)
  add_subdirectory(third_party/dynalo EXCLUDE_FROM_ALL)
endif()
add_subdirectory(src)

if(ON1X_BUILD_TESTS)
  include(CTest)
  add_subdirectory(tests)
endif()
