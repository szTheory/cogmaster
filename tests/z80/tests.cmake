# The SingleStepTests z80 vector pipeline: the converter that writes the
# committed sample and the harness that runs it through the Z80.
#
# The harness compiles the z80 sources itself and never links the library:
# it defines the bus seam functions, which the library's own bus defines too,
# so linking both would give duplicate definitions. The z80 sources build as
# an object library with the core's own compile options.
get_target_property(z80_options cogmaster COMPILE_OPTIONS)
add_library(cogmaster-z80-sst OBJECT ${PROJECT_SOURCE_DIR}/src/z80.c)
target_include_directories(cogmaster-z80-sst PRIVATE
  ${PROJECT_SOURCE_DIR}/include ${PROJECT_BINARY_DIR}/generated ${PROJECT_SOURCE_DIR}/src)
target_compile_options(cogmaster-z80-sst PRIVATE ${z80_options})
cogmaster_warnings(cogmaster-z80-sst)

add_executable(cogmaster-test-sst
  ${CMAKE_CURRENT_LIST_DIR}/sst_run.c
  ${CMAKE_CURRENT_LIST_DIR}/sst_bus.c
  ${CMAKE_CURRENT_LIST_DIR}/sst_stream.c
  $<TARGET_OBJECTS:cogmaster-z80-sst>)
target_include_directories(cogmaster-test-sst PRIVATE
  ${PROJECT_SOURCE_DIR}/include ${PROJECT_BINARY_DIR}/generated ${PROJECT_SOURCE_DIR}/src)
set_target_properties(cogmaster-test-sst PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
cogmaster_warnings(cogmaster-test-sst)

# A host tool: it reads the upstream JSON and writes the sample.
add_executable(cogmaster-sst2bin
  ${CMAKE_CURRENT_LIST_DIR}/sst2bin.c
  ${CMAKE_CURRENT_LIST_DIR}/sst_stream.c)
set_target_properties(cogmaster-sst2bin PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
cogmaster_warnings(cogmaster-sst2bin)

# Every command that starts a z80 tool goes through the build's
# CMAKE_CROSSCOMPILING_EMULATOR, so an emulated lane runs its own guest binary.
set(z80_sample ${PROJECT_SOURCE_DIR}/tests/vectors/z80/sst-sample.bin)
set(z80_out ${PROJECT_BINARY_DIR}/tests/vectors)
set(z80_expect_fail ${PROJECT_SOURCE_DIR}/tests/cmake/expect_fail.cmake)
file(MAKE_DIRECTORY ${z80_out})

# A copy of the sample in which file 00 test 0 expects a different final a,
# written at build time into the build tree; the committed sample is never
# changed.
add_custom_command(OUTPUT ${z80_out}/sst-corrupt.bin
  COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR} $<TARGET_FILE:cogmaster-test-sst> --sample ${z80_sample}
    --corrupt-copy ${z80_out}/sst-corrupt.bin 00 0 a
  DEPENDS cogmaster-test-sst ${z80_sample}
  VERBATIM)
add_custom_target(cogmaster-sst-corrupt ALL DEPENDS ${z80_out}/sst-corrupt.bin)

# Fault prevented: a harness that reports green whatever the core computes.
# On the corrupted copy it must exit 1 naming the file, the test and the field.
set(cmd ${CMAKE_CROSSCOMPILING_EMULATOR} $<TARGET_FILE:cogmaster-test-sst>
  --sample ${z80_out}/sst-corrupt.bin --only 00)
string(REPLACE ";" "$<SEMICOLON>" cmd "${cmd}")
cogmaster_add_test(NAME vectors.self_test
  COMMAND ${CMAKE_COMMAND} "-DCMD=${cmd}" -DCODE=1 "-DREGEX=mismatch.*00.*0.*[^a-z]a([^a-z]|$)"
    -P ${z80_expect_fail}
  LABELS portable vectors TIMEOUT 30)
