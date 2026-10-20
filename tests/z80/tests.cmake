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
