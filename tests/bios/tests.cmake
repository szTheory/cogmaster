# The boot ROM arbitration on the bus: the image and $3E are set through
# src/internal.h; nothing loads a boot ROM yet.
add_executable(cogmaster-test-bios ${CMAKE_CURRENT_LIST_DIR}/test_bios.c)
target_include_directories(cogmaster-test-bios PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(cogmaster-test-bios PRIVATE cogmaster::cogmaster)
set_target_properties(cogmaster-test-bios PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
cogmaster_warnings(cogmaster-test-bios)

# Fault prevented: a boot ROM and cartridge arbitration copied from one
# emulator and wrong for the other machine (an exclusive overlay, or AND on
# an SMS1, or the cartridge always winning on an SMS2), or an empty-slot
# probe taking the first-fault record from the game.
# Timeout: 20 s floor; no hosted run yet, so provisional.
cogmaster_add_test(NAME bios.arbitration COMMAND cogmaster-test-bios arbitration
  LABELS bios portable TIMEOUT 20)

# Fault prevented: a derived bus flag left stale by a $3E write or a reset,
# which would send a read down the wrong path without a sign.
# Timeout: 20 s floor; no hosted run yet, so provisional.
cogmaster_add_test(NAME bios.slow_flag COMMAND cogmaster-test-bios slow_flag
  LABELS bios portable TIMEOUT 20)
