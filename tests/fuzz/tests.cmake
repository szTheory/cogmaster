# The fuzz targets and their seeds.
#
# Run as a script (cmake -DFUZZER=<binary> -DSEEDS=<dir> -DCORPUS=<dir>
# [-DREGRESS=<dir>] -DARTIFACTS=<dir> -P tests.cmake), this file is one timed
# libFuzzer run; include()d by tests/tests.cmake, it registers the tests.
if(CMAKE_SCRIPT_MODE_FILE)
  # A fresh copy of the seeds is the corpus libFuzzer grows, so the build's
  # seeds stay as generated; the regressions are read only. -max_len is 1 MiB,
  # a 512-byte copier header and one byte more, so the size-reject path stays
  # reachable. -timeout=2 turns a hang into a finding well inside the test's
  # TIMEOUT.
  file(REMOVE_RECURSE ${CORPUS})
  file(MAKE_DIRECTORY ${CORPUS} ${ARTIFACTS})
  file(COPY ${SEEDS}/ DESTINATION ${CORPUS})
  execute_process(COMMAND ${FUZZER} -max_total_time=60 -timeout=2 -max_len=1049089
      -print_final_stats=1 -artifact_prefix=${ARTIFACTS}/ ${CORPUS} ${REGRESS}
    RESULT_VARIABLE rc)
  if(NOT rc EQUAL 0)
    message(FATAL_ERROR "${FUZZER}: exit ${rc}; the input is in ${ARTIFACTS}")
  endif()
  return()
endif()

# The synthetic images, generated at build time into the build tree:
# build/<preset>/tests/seeds holds every image the fuzz replay and the runner
# tests read; build/<preset>/tests/oversize holds the one file over 64 MiB,
# which the runner must refuse unread and the fuzz replay must never load.
# On a cross lane the generator runs through the preset's emulator.
add_executable(cogmaster-test-seeds ${CMAKE_CURRENT_LIST_DIR}/seeds.c)
set_target_properties(cogmaster-test-seeds PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
cogmaster_warnings(cogmaster-test-seeds)

set(COGMASTER_SEEDS_DIR ${PROJECT_BINARY_DIR}/tests/seeds)
set(COGMASTER_OVERSIZE_FILE ${PROJECT_BINARY_DIR}/tests/oversize/oversize.sms)
add_custom_command(OUTPUT ${PROJECT_BINARY_DIR}/tests/seeds.stamp
  COMMAND ${CMAKE_COMMAND} -E make_directory ${COGMASTER_SEEDS_DIR}
    ${PROJECT_BINARY_DIR}/tests/oversize
  COMMAND cogmaster-test-seeds ${COGMASTER_SEEDS_DIR}
  COMMAND cogmaster-test-seeds --oversize ${COGMASTER_OVERSIZE_FILE}
  COMMAND ${CMAKE_COMMAND} -E touch ${PROJECT_BINARY_DIR}/tests/seeds.stamp
  DEPENDS cogmaster-test-seeds
  COMMENT "Writing the synthetic cartridge images"
  VERBATIM)
add_custom_target(cogmaster-seeds ALL DEPENDS ${PROJECT_BINARY_DIR}/tests/seeds.stamp)

set(COGMASTER_FUZZ_REGRESS ${CMAKE_CURRENT_LIST_DIR}/regress)

# The fuzz lane: each entry linked with libFuzzer, run for 60 s from the
# seeds. Every other target was compiled with the fuzzer-no-link
# instrumentation by the preset.
# Fault prevented: a loader or short-run crash, hang or sanitizer report that
# the fixed seeds never reach.
if(COGMASTER_LIBFUZZER)
  set(regress "")
  if(IS_DIRECTORY ${COGMASTER_FUZZ_REGRESS})
    set(regress -DREGRESS=${COGMASTER_FUZZ_REGRESS})
  endif()
  foreach(target IN ITEMS loader run)
    add_executable(cogmaster-fuzz-${target} ${CMAKE_CURRENT_LIST_DIR}/fuzz_${target}.c)
    target_link_libraries(cogmaster-fuzz-${target} PRIVATE cogmaster::cogmaster)
    target_link_options(cogmaster-fuzz-${target} PRIVATE -fsanitize=fuzzer)
    set_target_properties(cogmaster-fuzz-${target} PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
    cogmaster_warnings(cogmaster-fuzz-${target})
    add_dependencies(cogmaster-fuzz-${target} cogmaster-seeds)
    cogmaster_add_test(NAME fuzz.libfuzzer.${target}
      COMMAND ${CMAKE_COMMAND} -DFUZZER=$<TARGET_FILE:cogmaster-fuzz-${target}>
        -DSEEDS=${COGMASTER_SEEDS_DIR} -DCORPUS=${PROJECT_BINARY_DIR}/tests/corpus-${target}
        ${regress} -DARTIFACTS=${PROJECT_BINARY_DIR}/tests/fuzz-artifacts
        -P ${CMAKE_CURRENT_LIST_FILE}
      LABELS fuzz TIMEOUT 120)
  endforeach()
  return()
endif()

# The fuzz entries, each linked with the plain replay driver. The replay reads
# the seeds directory at test time, and tests/fuzz/regress (the minimised
# regressions, each a tests/roms/manifest.txt row) when it exists; never the
# oversize directory.
# Fault prevented: an input that once crashed or hung the loader or a short
# run coming back unnoticed, on any lane, without libFuzzer installed.
set(COGMASTER_FUZZ_INPUTS ${COGMASTER_SEEDS_DIR})
if(IS_DIRECTORY ${COGMASTER_FUZZ_REGRESS})
  list(APPEND COGMASTER_FUZZ_INPUTS ${COGMASTER_FUZZ_REGRESS})
endif()
foreach(target IN ITEMS loader run)
  add_executable(cogmaster-fuzz-replay-${target}
    ${CMAKE_CURRENT_LIST_DIR}/replay.c ${CMAKE_CURRENT_LIST_DIR}/fuzz_${target}.c)
  target_link_libraries(cogmaster-fuzz-replay-${target} PRIVATE cogmaster::cogmaster)
  target_compile_definitions(cogmaster-fuzz-replay-${target} PRIVATE _POSIX_C_SOURCE=200809L)
  set_target_properties(cogmaster-fuzz-replay-${target} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
  cogmaster_warnings(cogmaster-fuzz-replay-${target})
  add_dependencies(cogmaster-fuzz-replay-${target} cogmaster-seeds)
  cogmaster_add_test(NAME fuzz.regress.${target}
    COMMAND cogmaster-fuzz-replay-${target} ${COGMASTER_FUZZ_INPUTS}
    LABELS fuzz portable TIMEOUT 60)
endforeach()
