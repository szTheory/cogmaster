# The boot ROM on the bus and in the load path: the image and $3E are also set
# through src/internal.h.
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
# Timeout: 20 s floor; 2 x 0.12 s, slowest of 7 runs, run 43060784025.
# The be32 and wasm32 jobs of that run never got past their apt step, so those lanes are unmeasured.
cogmaster_add_test(NAME bios.arbitration COMMAND cogmaster-test-bios arbitration
  LABELS bios portable TIMEOUT 20)

# Fault prevented: a derived bus flag left stale by a $3E write or a reset,
# which would send a read down the wrong path without a sign.
# Timeout: 20 s floor; 2 x 0.39 s, slowest of 7 runs, run 43060784025.
# The be32 and wasm32 jobs of that run never got past their apt step, so those lanes are unmeasured.
cogmaster_add_test(NAME bios.slow_flag COMMAND cogmaster-test-bios slow_flag
  LABELS bios portable TIMEOUT 20)

# Fault prevented: a BIOS load that accepts a hostile size or pointer pair,
# holds a pointer into caller memory, or changes the instance before it can
# still refuse; and a BIOS boot that leaks the fast-boot constants into the
# power-on state.
# Timeout: 20 s floor; 2 x 0.30 s, slowest of 7 runs, run 43060784025.
# The be32 and wasm32 jobs of that run never got past their apt step, so those lanes are unmeasured.
cogmaster_add_test(NAME bios.load COMMAND cogmaster-test-bios load
  LABELS bios portable TIMEOUT 20)

# Fault prevented: a state from one boot mode or BIOS loaded into another, and
# the false belief that a state never holds bytes derived from the BIOS (the
# detector is proven by a control image that copies itself to work RAM).
# Timeout: 20 s floor; 2 x 0.26 s, slowest of 7 runs, run 43060784025.
# The be32 and wasm32 jobs of that run never got past their apt step, so those lanes are unmeasured.
cogmaster_add_test(NAME bios.state COMMAND cogmaster-test-bios state
  LABELS bios portable TIMEOUT 20)
