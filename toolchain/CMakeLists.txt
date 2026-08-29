add_executable(on1x_fmt EXCLUDE_FROM_ALL on1x-fmt/fmt.cpp)
target_link_libraries(on1x_fmt PRIVATE on1x_core)
set_target_properties(on1x_fmt PROPERTIES OUTPUT_NAME on1x-fmt)
install(TARGETS on1x_fmt RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
