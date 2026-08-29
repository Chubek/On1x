add_library(on1x_mpc STATIC EXCLUDE_FROM_ALL
  mpc.c
)
add_library(on1x::mpc ALIAS on1x_mpc)

target_include_directories(on1x_mpc PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_features(on1x_mpc PUBLIC c_std_99)
