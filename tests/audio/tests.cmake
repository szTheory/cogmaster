# The audio area: the cadence, the drain and the synthesis over the test card,
# and the sound chips. Each chip's executable joins this file as it is written.
add_executable(cogmaster-test-audio ${CMAKE_CURRENT_LIST_DIR}/test_synth.c)
target_include_directories(cogmaster-test-audio PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(cogmaster-test-audio PRIVATE cogmaster::cogmaster)
set_target_properties(cogmaster-test-audio PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
cogmaster_warnings(cogmaster-test-audio)

# Fault prevented: a frame that holds a count off the exact rational rate, so a
# frontend that paces on audio drifts, a drain that loses, repeats or overruns
# the samples of a frame, or a level change that is not heard as a band-limited
# step at its place in the stream.
foreach(case IN ITEMS cadence drain step spectral no_clip amplitude_bound ring_overshoot
                  stereo_mask state_continue)
  # Timeout: 10 s floor; 0.32 s slowest local run (audio.cadence; audio.spectral
  # takes 0.05 s for its integer DFT), no hosted run yet, so provisional. be32 and
  # wasm32 were not run locally; their 8 and 5 times scales leave 80 s and 50 s,
  # over a thousand times the native cost of the slowest test.
  cogmaster_add_test(NAME audio.${case} COMMAND cogmaster-test-audio ${case}
    LABELS audio portable TIMEOUT 10)
endforeach()

# The Sega PSG through the bus: closed-form oracles over the drained trace and the chip's
# registers.
add_executable(cogmaster-test-psg ${CMAKE_CURRENT_LIST_DIR}/test_psg.c)
target_include_directories(cogmaster-test-psg PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(cogmaster-test-psg PRIVATE cogmaster::cogmaster)
set_target_properties(cogmaster-test-psg PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
cogmaster_warnings(cogmaster-test-psg)

# Fault prevented: a Z80 write to the sound port that never reaches the chip, a chip that
# runs off the tick grid or lags its writes, or a level that is not heard in the stream.
foreach(case IN ITEMS through_bus)
  # Timeout: 10 s floor; 0.01 s local run, no hosted run yet, so provisional.
  cogmaster_add_test(NAME psg.${case} COMMAND cogmaster-test-psg ${case}
    LABELS psg portable TIMEOUT 10)
endforeach()
