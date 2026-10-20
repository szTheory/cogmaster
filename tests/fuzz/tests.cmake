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

# The fuzz entries, each linked with the plain replay driver. The replay reads
# the seeds directory at test time, and tests/fuzz/regress (the minimised
# regressions, each a tests/roms/manifest.txt row) when it exists; never the
# oversize directory.
# Fault prevented: an input that once crashed or hung the loader or a short
# run coming back unnoticed, on any lane, without libFuzzer installed.
set(COGMASTER_FUZZ_INPUTS ${COGMASTER_SEEDS_DIR})
if(IS_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/regress)
  list(APPEND COGMASTER_FUZZ_INPUTS ${CMAKE_CURRENT_LIST_DIR}/regress)
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
