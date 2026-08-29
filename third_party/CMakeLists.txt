add_subdirectory(klib)
add_subdirectory(mpc)
add_subdirectory(qbe)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/dparser/CMakeLists.txt")
  add_subdirectory(dparser EXCLUDE_FROM_ALL)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/dynalo/CMakeLists.txt")
  add_subdirectory(dynalo EXCLUDE_FROM_ALL)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/dyncall/CMakeLists.txt")
  add_subdirectory(dyncall EXCLUDE_FROM_ALL)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/libgc/CMakeLists.txt")
  add_subdirectory(libgc EXCLUDE_FROM_ALL)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/replxx/CMakeLists.txt")
  add_subdirectory(replxx EXCLUDE_FROM_ALL)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/equinox-ng/CMakeLists.txt")
  add_subdirectory(equinox-ng EXCLUDE_FROM_ALL)
endif()

function(on1x_add_vendor_interface target alias)
  add_library(${target} INTERFACE)
  add_library(on1x::${alias} ALIAS ${target})
endfunction()

on1x_add_vendor_interface(on1x_dep_dparser dep_dparser)
target_include_directories(on1x_dep_dparser INTERFACE
  ${CMAKE_CURRENT_SOURCE_DIR}/dparser
)
if(TARGET dparse)
  target_link_libraries(on1x_dep_dparser INTERFACE dparse)
endif()

on1x_add_vendor_interface(on1x_dep_dynalo dep_dynalo)
target_include_directories(on1x_dep_dynalo INTERFACE
  ${CMAKE_CURRENT_SOURCE_DIR}/dynalo/include
)
if(TARGET dynalo)
  target_link_libraries(on1x_dep_dynalo INTERFACE dynalo)
endif()

on1x_add_vendor_interface(on1x_dep_dyncall dep_dyncall)
target_include_directories(on1x_dep_dyncall INTERFACE
  ${CMAKE_CURRENT_SOURCE_DIR}/dyncall
  ${CMAKE_CURRENT_SOURCE_DIR}/dyncall/dyncall
  ${CMAKE_CURRENT_SOURCE_DIR}/dyncall/dyncallback
  ${CMAKE_CURRENT_SOURCE_DIR}/dyncall/dynload
)
if(TARGET dyncall_s)
  target_link_libraries(on1x_dep_dyncall INTERFACE dyncall_s)
endif()
if(TARGET dyncallback_s)
  target_link_libraries(on1x_dep_dyncall INTERFACE dyncallback_s)
endif()
if(TARGET dynload_s)
  target_link_libraries(on1x_dep_dyncall INTERFACE dynload_s)
endif()

on1x_add_vendor_interface(on1x_dep_equinox_ng dep_equinox_ng)
target_include_directories(on1x_dep_equinox_ng INTERFACE
  ${CMAKE_CURRENT_SOURCE_DIR}/equinox-ng
  ${CMAKE_CURRENT_SOURCE_DIR}/equinox-ng/third_party/QaMRpp
)
if(TARGET qamrpp-sexpr)
  target_link_libraries(on1x_dep_equinox_ng INTERFACE qamrpp-sexpr)
endif()
if(TARGET qamrpp-leqvsg)
  target_link_libraries(on1x_dep_equinox_ng INTERFACE qamrpp-leqvsg)
endif()
if(TARGET eqvsg)
  target_link_libraries(on1x_dep_equinox_ng INTERFACE eqvsg)
endif()

on1x_add_vendor_interface(on1x_dep_keystone dep_keystone)
target_include_directories(on1x_dep_keystone INTERFACE
  ${CMAKE_CURRENT_SOURCE_DIR}/keystone/include
)

on1x_add_vendor_interface(on1x_dep_libgc dep_libgc)
target_include_directories(on1x_dep_libgc INTERFACE
  ${CMAKE_CURRENT_SOURCE_DIR}/libgc/include
)
if(TARGET gc)
  target_link_libraries(on1x_dep_libgc INTERFACE gc)
endif()

on1x_add_vendor_interface(on1x_dep_replxx dep_replxx)
target_include_directories(on1x_dep_replxx INTERFACE
  ${CMAKE_CURRENT_SOURCE_DIR}/replxx/include
)
if(TARGET replxx)
  target_link_libraries(on1x_dep_replxx INTERFACE replxx)
endif()

add_library(on1x_vendor INTERFACE)
add_library(on1x::vendor ALIAS on1x_vendor)
target_link_libraries(on1x_vendor INTERFACE
  on1x_dep_dparser
  on1x_dep_dynalo
  on1x_dep_dyncall
  on1x_dep_equinox_ng
  on1x_dep_keystone
  on1x_dep_libgc
  on1x_dep_replxx
  on1x_klib
  on1x_mpc
)
