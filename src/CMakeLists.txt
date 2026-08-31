add_subdirectory(frontend)
add_subdirectory(core)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/memory/CMakeLists.txt")
  add_subdirectory(memory)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/ffi/CMakeLists.txt")
  add_subdirectory(ffi)
endif()

if(TARGET on1x_memory)
  target_link_libraries(on1x_core PRIVATE $<BUILD_INTERFACE:on1x_memory>)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/codegen/CMakeLists.txt")
  add_subdirectory(codegen)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/plugin/CMakeLists.txt")
  add_subdirectory(plugin)
endif()

add_executable(on1x_tool
  tool/main.cpp
)
target_link_libraries(on1x_tool PRIVATE on1x_core)
set_target_properties(on1x_tool PROPERTIES OUTPUT_NAME on1x)
install(TARGETS on1x_tool
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/rewrite/CMakeLists.txt")
  add_subdirectory(rewrite)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/egraph/CMakeLists.txt")
  add_subdirectory(egraph)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/sema/CMakeLists.txt")
  add_subdirectory(sema)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/dataflow/CMakeLists.txt")
  add_subdirectory(dataflow)
endif()
