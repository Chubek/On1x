if(APPLE)
  set(QBE_DEFAULT_TARGET T_amd64_apple)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64)$")
  set(QBE_DEFAULT_TARGET T_arm64)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(riscv64|rv64)")
  set(QBE_DEFAULT_TARGET T_rv64)
else()
  set(QBE_DEFAULT_TARGET T_amd64_sysv)
endif()

file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/config.h" "#define Deftgt ${QBE_DEFAULT_TARGET}\n")

file(GLOB qbe_core_sources CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
)

file(GLOB qbe_amd64_sources CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/amd64/*.c"
)

file(GLOB qbe_arm64_sources CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/arm64/*.c"
)

file(GLOB qbe_rv64_sources CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/rv64/*.c"
)

add_executable(on1x_qbe EXCLUDE_FROM_ALL
  ${qbe_core_sources}
  ${qbe_amd64_sources}
  ${qbe_arm64_sources}
  ${qbe_rv64_sources}
)
add_executable(on1x::qbe ALIAS on1x_qbe)

target_include_directories(on1x_qbe PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
)

target_compile_features(on1x_qbe PRIVATE c_std_99)
