# The controller ports at pin level, against the formulas they replaced: every
# pair of $3F values and every pad value, written and read through the I/O
# functions in src/internal.h.
add_executable(cogmaster-test-ports ${CMAKE_CURRENT_LIST_DIR}/test_ports.c)
target_include_directories(cogmaster-test-ports PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(cogmaster-test-ports PRIVATE cogmaster::cogmaster)
set_target_properties(cogmaster-test-ports PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
cogmaster_warnings(cogmaster-test-ports)

# Fault prevented: a refactor that moves the H-counter latch or a pad pin, so every
# light-gun, H-counter and pad game changes behaviour without a test noticing.
foreach(case IN ITEMS latch latch_control pins)
  # Timeout: 20 s floor; 2 x 0.49 s, slowest of 1 run, run 42973268415.
  cogmaster_add_test(NAME ports.${case} COMMAND cogmaster-test-ports ${case}
    LABELS ports portable TIMEOUT 20)
endforeach()

# The size-tagged input against callers of every size: whole fields only, a poisoned tail, an
# exact-size block, every refusal and every axis bound.
add_executable(cogmaster-test-ports-input ${CMAKE_CURRENT_LIST_DIR}/test_input.c)
target_include_directories(cogmaster-test-ports-input PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(cogmaster-test-ports-input PRIVATE cogmaster::cogmaster)
set_target_properties(cogmaster-test-ports-input PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
cogmaster_warnings(cogmaster-test-ports-input)

# Fault prevented: a caller built against the 32-byte struct is read past its end when the struct grows,
# or a hostile axis escapes validation.
foreach(case IN ITEMS poison exact_malloc tags refusals range outputs)
  # Timeout: 20 s floor; 2 x 0.33 s, slowest of 1 run, run 42973268415.
  cogmaster_add_test(NAME ports.input.${case} COMMAND cogmaster-test-ports-input ${case}
    LABELS ports portable TIMEOUT 20)
endforeach()

# The 3-D glasses eye: the cut rule against an independent closed form, the end-of-window
# snapshot it must not equal, the gate-free hook, every profile, the lag poll and the state bounds.
add_executable(cogmaster-test-ports-glasses ${CMAKE_CURRENT_LIST_DIR}/test_glasses.c)
target_include_directories(cogmaster-test-ports-glasses PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(cogmaster-test-ports-glasses PRIVATE cogmaster::cogmaster)
set_target_properties(cogmaster-test-ports-glasses PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
cogmaster_warnings(cogmaster-test-ports-glasses)

# Fault prevented: a 3-D eye reported in the wrong picture (inverted depth), gated by a bit no
# source supports, or a frame hash that depends on the glasses.
foreach(case IN ITEMS rule control gate profiles nolag state)
  # Timeout: 20 s floor; 2 x 3.14 s, slowest of 1 run, run 42973268415.
  cogmaster_add_test(NAME ports.glasses.${case} COMMAND cogmaster-test-ports-glasses ${case}
    LABELS ports portable TIMEOUT 20)
endforeach()

# The Light Phaser on the internal API: the tick-to-H function against the formula it replaced.
add_executable(cogmaster-test-ports-gun ${CMAKE_CURRENT_LIST_DIR}/test_gun.c)
target_include_directories(cogmaster-test-ports-gun PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(cogmaster-test-ports-gun PRIVATE cogmaster::cogmaster)
set_target_properties(cogmaster-test-ports-gun PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
cogmaster_warnings(cogmaster-test-ports-gun)

# Fault prevented: a refactor of the H-counter origin or of the latch site that moves a latch
# position, so every light-gun, H-counter and pad game changes behaviour without a test noticing.
foreach(case IN ITEMS latch_at)
  # Timeout: 20 s floor; provisional.
  cogmaster_add_test(NAME ports.gun.${case} COMMAND cogmaster-test-ports-gun ${case}
    LABELS ports portable TIMEOUT 20)
endforeach()

# The Sports Pad and the Paddle on the internal API: the read sequence at the tick limits, the walk
# and its buttons, the clamps, the sum of pending motion, device changes, resets, profiles and
# availability, a poll hammer, a state round trip and a replay of recorded motion. Its record mode is the
# library's side of the replay case in tests/fixtures: it runs an image with a device and writes the script.
add_executable(cogmaster-test-ports-devices ${CMAKE_CURRENT_LIST_DIR}/test_devices.c
  ${PROJECT_SOURCE_DIR}/runner/sha256.c)
target_include_directories(cogmaster-test-ports-devices PRIVATE ${PROJECT_SOURCE_DIR}/src
  ${PROJECT_SOURCE_DIR}/runner)
target_link_libraries(cogmaster-test-ports-devices PRIVATE cogmaster::cogmaster)
set_target_properties(cogmaster-test-ports-devices PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
cogmaster_warnings(cogmaster-test-ports-devices)

# Fault prevented: a device that desyncs a read sequence, drops or doubles motion, hoards unbounded
# motion, or saves a state the core could not have produced, so a replay or a restart changes the game.
foreach(case IN ITEMS sports_ticks sports_walk clamp pending device_switch resets paddle profiles
    hammer roundtrip replay)
  # Timeout: 20 s floor; 2 x 6.00 s (replay), slowest of 1 run, run 42973268415. The million-iteration
  # hammer keeps 60 s: 0.82 s there, and it is the case that cannot hang quietly.
  set(timeout 20)
  if(case STREQUAL "hammer")
    set(timeout 60)
  endif()
  cogmaster_add_test(NAME ports.devices.${case} COMMAND cogmaster-test-ports-devices ${case}
    LABELS ports portable TIMEOUT ${timeout})
endforeach()

# The saved device state against an independent bounds checker: every byte of every new field
# changed and loaded, each bound just outside, each cross-check violated once, a sweep with each
# device attached, a state saved at every step of a read sequence and a Paddle on an export profile.
add_executable(cogmaster-test-ports-state ${CMAKE_CURRENT_LIST_DIR}/test_state.c)
target_include_directories(cogmaster-test-ports-state PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(cogmaster-test-ports-state PRIVATE cogmaster::cogmaster)
set_target_properties(cogmaster-test-ports-state PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
cogmaster_warnings(cogmaster-test-ports-state)

# Fault prevented: a loader that accepts a device state no run produces, or refuses one a run does, so a
# corrupt save drives the core outside its bounds or a valid save is lost.
foreach(case IN ITEMS mutation outside sweep midsequence profile_device)
  # Timeout: 80 s; 2 x 38.44 s, slowest of 1 run, run 42973268415 (mutation on the be32 lane, 4.7 s native).
  cogmaster_add_test(NAME ports.state.${case} COMMAND cogmaster-test-ports-state ${case}
    LABELS ports portable TIMEOUT 80)
endforeach()
