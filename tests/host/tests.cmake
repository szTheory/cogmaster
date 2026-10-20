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
