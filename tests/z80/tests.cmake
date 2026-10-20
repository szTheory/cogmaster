# The SingleStepTests z80 vector pipeline: the converter that writes the
# committed sample and the harness that runs it through the Z80.
#
# Run as a script (cmake -DSST2BIN=... -DSST=... -DFIXTURE=... -DOUT=...
# [-DEMULATOR=...] -P tests.cmake), this file converts one fixture and runs
# the harness on the result, requiring exit 0 and one executed test.
if(CMAKE_SCRIPT_MODE_FILE)
  execute_process(COMMAND ${EMULATOR} ${SST2BIN} --dir ${FIXTURE} --count 1 --files 00 --out ${OUT}
    RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE out)
  if(NOT rc STREQUAL "0")
    message(FATAL_ERROR "cogmaster-sst2bin on ${FIXTURE}: exit ${rc}\n${out}")
  endif()
  execute_process(COMMAND ${EMULATOR} ${SST} --sample ${OUT} --only 00 --expect-count 1
    RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE out)
  if(NOT rc STREQUAL "0" OR NOT out MATCHES "executed 1\n")
    message(FATAL_ERROR "cogmaster-test-sst on ${OUT}: exit ${rc}\n${out}")
  endif()
  return()
endif()

# The harness compiles the z80 sources itself and never links the library:
# it defines the bus seam functions, which the library's own bus defines too,
# so linking both would give duplicate definitions. The z80 sources build as
# an object library with the core's own compile options.
get_target_property(z80_options cogmaster COMPILE_OPTIONS)
add_library(cogmaster-z80-sst OBJECT ${PROJECT_SOURCE_DIR}/src/z80.c
  ${PROJECT_SOURCE_DIR}/src/z80_cb.c)
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

# Fault prevented: an allow list that keeps excusing a failure after the core
# is fixed. A line for a test that passes must fail the run.
file(WRITE ${z80_out}/allow-unmatched.txt
  "2026-01-01 \"00\" 5 a https://github.com/SingleStepTests/z80/issues/1 excuses a test that passes\n")
set(cmd ${CMAKE_CROSSCOMPILING_EMULATOR} $<TARGET_FILE:cogmaster-test-sst>
  --sample ${z80_sample} --only 00 --allow ${z80_out}/allow-unmatched.txt)
string(REPLACE ";" "$<SEMICOLON>" cmd "${cmd}")
cogmaster_add_test(NAME vectors.allow.self_test
  COMMAND ${CMAKE_COMMAND} "-DCMD=${cmd}" -DCODE=1 "-DREGEX=allow.*matches no failure"
    -P ${z80_expect_fail}
  LABELS portable vectors TIMEOUT 30)

# The converter on hand-written fixtures in the upstream schema, one NOP
# test each. Fault prevented: a converter that drops or misreads a test
# silently instead of refusing it.
set(z80_fixtures ${CMAKE_CURRENT_LIST_DIR}/fixtures)
string(REPLACE ";" "$<SEMICOLON>" z80_emulator "${CMAKE_CROSSCOMPILING_EMULATOR}")
cogmaster_add_test(NAME vectors.sst2bin.accept
  COMMAND ${CMAKE_COMMAND} -DSST2BIN=$<TARGET_FILE:cogmaster-sst2bin>
    -DSST=$<TARGET_FILE:cogmaster-test-sst> "-DEMULATOR=${z80_emulator}"
    -DFIXTURE=${z80_fixtures}/accept -DOUT=${z80_out}/accept.bin
    -P ${CMAKE_CURRENT_LIST_FILE}
  LABELS portable vectors TIMEOUT 30)
# An unknown key, such as one a later upstream release adds, is skipped.
cogmaster_add_test(NAME vectors.sst2bin.unknown_key
  COMMAND cogmaster-sst2bin --dir ${z80_fixtures}/unknown_key --count 1 --files 00
    --out ${z80_out}/unknown_key.bin
  LABELS portable vectors TIMEOUT 30)
foreach(case IN ITEMS "missing_key;missing key: wz"
    "bad_stream;00: test 0: cycles do not rebuild")
  list(GET case 0 name)
  list(GET case 1 regex)
  set(cmd ${CMAKE_CROSSCOMPILING_EMULATOR} $<TARGET_FILE:cogmaster-sst2bin>
    --dir ${z80_fixtures}/${name} --count 1 --files 00 --out ${z80_out}/${name}.bin)
  string(REPLACE ";" "$<SEMICOLON>" cmd "${cmd}")
  cogmaster_add_test(NAME vectors.sst2bin.${name}
    COMMAND ${CMAKE_COMMAND} "-DCMD=${cmd}" -DCODE=1 "-DREGEX=${regex}" -P ${z80_expect_fail}
    LABELS portable vectors TIMEOUT 30)
endforeach()

# The prefix-group shards of the committed sample: every sampled test of the
# group must match final state, RAM and every T-state, with only the allow
# list's lines excused. Fault prevented: a Z80 opcode that drifts from the
# vectors. Timeouts are provisional until the emulated lanes are measured.
cogmaster_add_test(NAME vectors.base
  COMMAND cogmaster-test-sst --sample ${z80_sample} --group base
    --allow ${PROJECT_SOURCE_DIR}/tests/vectors/z80/allow.txt --expect-count 25200
  LABELS portable vectors TIMEOUT 60)
foreach(shard IN ITEMS "cb;25600" "dd;25200" "fd;25200" "ddcb;25600" "fdcb;25600")
  list(GET shard 0 group)
  list(GET shard 1 count)
  cogmaster_add_test(NAME vectors.${group}
    COMMAND cogmaster-test-sst --sample ${z80_sample} --group ${group}
      --allow ${PROJECT_SOURCE_DIR}/tests/vectors/z80/allow.txt --expect-count ${count}
    LABELS portable vectors TIMEOUT 60)
endforeach()
