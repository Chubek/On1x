add_library(on1x_stdlib STATIC
  cmp/cmp.cpp
  math/math.cpp
  bit/bit.cpp
  rand/rand.cpp
  fn/fn.cpp
  iter/iter.cpp
  tag/tag.cpp
  str/str.cpp
  list/list.cpp
  list/access.cpp
  list/higher_order.cpp
  list/sort.cpp
  list/zip.cpp
  table/table.cpp
  table/query.cpp
  table/higher_order.cpp
  opt/opt.cpp
  res/res.cpp
  capability.cpp
  module.cpp
  registry.cpp
  args.cpp
  std_all.cpp
  fs/path.cpp
  embed.cpp
)

# Generate embedded sources
set(EMBED_GEN "${CMAKE_CURRENT_BINARY_DIR}/embedded_sources.gen.cpp")
add_custom_command(
  OUTPUT ${EMBED_GEN}
  COMMAND ${Python3_EXECUTABLE} ${PROJECT_SOURCE_DIR}/tools/embed_sources.py
    ${PROJECT_SOURCE_DIR}/lib/on1x/
    ${EMBED_GEN}
  DEPENDS
    ${PROJECT_SOURCE_DIR}/tools/embed_sources.py
    ${PROJECT_SOURCE_DIR}/lib/on1x/opt.on1x
    ${PROJECT_SOURCE_DIR}/lib/on1x/res.on1x
    ${PROJECT_SOURCE_DIR}/lib/on1x/iter_ext.on1x
    ${PROJECT_SOURCE_DIR}/lib/on1x/list_ext.on1x
    ${PROJECT_SOURCE_DIR}/lib/on1x/prelude_ext.on1x
    ${PROJECT_SOURCE_DIR}/lib/on1x/assert.on1x
  COMMENT "Embedding On1x source files"
)
target_sources(on1x_stdlib PRIVATE ${EMBED_GEN})

# Find Python3 for the embed script
find_package(Python3 COMPONENTS Interpreter QUIET)
if(NOT Python3_FOUND)
  find_package(Python3 REQUIRED COMPONENTS Interpreter)
endif()

# Io module (capability: ON1X_CAP_IO)
if(NOT ON1X_STDLIB_PURE_ONLY)
  target_sources(on1x_stdlib PRIVATE
    io/io.cpp
    io/read.cpp
    io/render.cpp
  )
endif()

# Os module (capability: ON1X_CAP_ENV)
if(NOT ON1X_STDLIB_PURE_ONLY)
  target_sources(on1x_stdlib PRIVATE
    os/os.cpp
    os/env.cpp
  )
endif()

# Time module (capability: ON1X_CAP_TIME)
if(NOT ON1X_STDLIB_PURE_ONLY)
  target_sources(on1x_stdlib PRIVATE
    time/time.cpp
    time/calendar.cpp
  )
endif()

# Fs module (capability: ON1X_CAP_FS)
if(NOT ON1X_STDLIB_PURE_ONLY)
  target_sources(on1x_stdlib PRIVATE
    fs/fs.cpp
    fs/dir.cpp
    fs/stat.cpp
  )
endif()

# Rand system (SeedFromSystem, gated on non-pure)
if(NOT ON1X_STDLIB_PURE_ONLY)
  target_sources(on1x_stdlib PRIVATE
    rand/system.cpp
  )
endif()

 # Sexp module (optional, gate: ON1X_ENABLE_SEXP)
 if(ON1X_ENABLE_SEXP)
   target_sources(on1x_stdlib PRIVATE
     sexp/sexp.cpp
   )
   target_include_directories(on1x_stdlib PRIVATE ${PROJECT_SOURCE_DIR}/third_party)
   target_link_libraries(on1x_stdlib PUBLIC sfsexp)
 endif()
 
 # Dl module (optional, gate: ON1X_ENABLE_FFI)
 if(ON1X_ENABLE_FFI AND NOT ON1X_STDLIB_PURE_ONLY)
   target_sources(on1x_stdlib PRIVATE
     dl/dl.cpp
   )
 endif()
 
 # Asm module (optional, gate: ON1X_ENABLE_JIT AND ON1X_ENABLE_ASMTK)
 if(ON1X_ENABLE_JIT AND ON1X_ENABLE_ASMTK AND NOT ON1X_STDLIB_PURE_ONLY)
   target_sources(on1x_stdlib PRIVATE
     asm/asm.cpp
   )
 endif()
 
target_include_directories(on1x_stdlib PUBLIC ${PROJECT_SOURCE_DIR}/src ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(on1x_stdlib PUBLIC on1x_headers on1x_runtime on1x_core on1x_gc)
if(MSVC)
  target_compile_options(on1x_stdlib PRIVATE /W4 /permissive-)
else()
  target_compile_options(on1x_stdlib PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion)
endif()
