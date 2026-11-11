# The host helpers: colour conversion outside the core's hash.
add_executable(cogmaster-test-host ${CMAKE_CURRENT_LIST_DIR}/test_palette.c)
target_link_libraries(cogmaster-test-host PRIVATE cogmaster-host)
set_target_properties(cogmaster-test-host PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
cogmaster_warnings(cogmaster-test-host)

# Fault prevented: a colour conversion that loses or swaps a channel, so the
# runner's PNG or the libretro frame no longer maps back to the core's frame.
cogmaster_add_test(NAME host.palette
  COMMAND cogmaster-test-host palette
  LABELS host portable TIMEOUT 10)

# The red-cyan composition for the 3-D view.
add_executable(cogmaster-test-eyes ${CMAKE_CURRENT_LIST_DIR}/test_eyes.c)
target_link_libraries(cogmaster-test-eyes PRIVATE cogmaster-host)
set_target_properties(cogmaster-test-eyes PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
cogmaster_warnings(cogmaster-test-eyes)

# Fault prevented: an anaglyph that swaps or drops a channel, or writes into a
# picture the core still owns, so the 3-D view is wrong or the frame changes.
# Timeout: 10 s floor; 2 x 0.08 s, slowest of 1 run, run 42978311844.
cogmaster_add_test(NAME host.eyes
  COMMAND cogmaster-test-eyes eyes
  LABELS host portable TIMEOUT 10)

# The pointer-to-aim mapping and the crosshair's dots for the light gun.
add_executable(cogmaster-test-aim ${CMAKE_CURRENT_LIST_DIR}/test_aim.c)
target_link_libraries(cogmaster-test-aim PRIVATE cogmaster-host)
set_target_properties(cogmaster-test-aim PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
cogmaster_warnings(cogmaster-test-aim)

# Fault prevented: a pointer mapped without the crop offset or off by one dot, an
# off-screen pointer clamped onto an edge pixel, or a crosshair written outside
# its points, so a light-gun title is aimed somewhere else than the player points.
# Timeout: 10 s floor; provisional.
cogmaster_add_test(NAME host.aim
  COMMAND cogmaster-test-aim aim
  LABELS host portable TIMEOUT 10)
