add_library(on1x STATIC
  api_common.cpp
  api_state.cpp
  api_stack.cpp
  api_value.cpp
  api_list.cpp
  api_table.cpp
  api_tag.cpp
  api_optional.cpp
  api_result.cpp
  api_call.cpp
  api_error.cpp
  api_gc.cpp
  api_ffi.cpp
  api_version.cpp
)
add_library(on1x_shared SHARED
  api_common.cpp
  api_state.cpp
  api_stack.cpp
  api_value.cpp
  api_list.cpp
  api_table.cpp
  api_tag.cpp
  api_optional.cpp
  api_result.cpp
  api_call.cpp
  api_error.cpp
  api_gc.cpp
  api_ffi.cpp
  api_version.cpp
)
set_target_properties(on1x_shared PROPERTIES OUTPUT_NAME on1x)
foreach(target on1x on1x_shared)
  target_include_directories(${target} PUBLIC ${PROJECT_SOURCE_DIR}/include PRIVATE ${PROJECT_SOURCE_DIR}/src)
  target_link_libraries(${target} PRIVATE on1x_prelude on1x_vm on1x_runtime on1x_syntax on1x_core on1x_gc on1x_util)
  if(ON1X_ENABLE_FFI)
    target_link_libraries(${target} PRIVATE on1x_ffi)
    target_compile_definitions(${target} PRIVATE ON1X_ENABLE_FFI=1)
  endif()
endforeach()

if(UNIX AND NOT APPLE)
  target_link_options(on1x_shared PRIVATE
    "-Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/on1x.map")
endif()
