# The audio area: the cadence and the drain over the test card. The sound chips'
# executables join this file as they are written.
add_executable(cogmaster-test-audio ${CMAKE_CURRENT_LIST_DIR}/test_synth.c)
target_include_directories(cogmaster-test-audio PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(cogmaster-test-audio PRIVATE cogmaster::cogmaster)
set_target_properties(cogmaster-test-audio PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
cogmaster_warnings(cogmaster-test-audio)

# Fault prevented: a frame that holds a count off the exact rational rate, so a
# frontend that paces on audio drifts, or a drain that loses, repeats or
# overruns the samples of a frame.
foreach(case IN ITEMS cadence drain)
  # Timeout: 10 s floor; 0.22 s slowest local run (no hosted run yet), provisional.
  cogmaster_add_test(NAME audio.${case} COMMAND cogmaster-test-audio ${case}
    LABELS audio portable TIMEOUT 10)
endforeach()
