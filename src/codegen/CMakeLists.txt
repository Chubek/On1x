add_library(on1x_codegen STATIC
  codegen.cpp
)
add_library(on1x::codegen ALIAS on1x_codegen)

target_include_directories(on1x_codegen
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_compile_features(on1x_codegen PUBLIC cxx_std_17)
target_compile_definitions(on1x_codegen PRIVATE
  ON1X_CODEGEN_ISA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../../extra/ISA"
)

if(MSVC)
  target_compile_options(on1x_codegen PRIVATE /W4)
else()
  target_compile_options(on1x_codegen PRIVATE -Wall -Wextra -Wpedantic)
endif()

install(TARGETS on1x_codegen
  EXPORT On1xTargets
)
