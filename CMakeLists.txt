cmake_minimum_required(VERSION 3.20)

project(on1x VERSION 0.1.0 LANGUAGES C CXX)

option(ON1X_BUILD_TESTS "Build the On1x test suite" ON)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(src)

if(ON1X_BUILD_TESTS)
  include(CTest)
  add_subdirectory(tests)
endif()
