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
