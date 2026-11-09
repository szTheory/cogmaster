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

# Fault prevented: a ROM that hammers the sound ports at the Z80's fastest OUT rate and makes
# a frame cost more PSG steps, kernel additions or ring reach than the counters bound.
# Timeout: 10 s floor; 0.00 s slowest local run, no hosted run yet, so provisional. The be32
# and wasm32 scales leave 80 s and 50 s; confirm against the first hosted run before freezing.
cogmaster_add_test(NAME audio.cost COMMAND cogmaster-test-audio cost
  LABELS audio portable TIMEOUT 10)

# The Sega PSG through the bus: closed-form oracles over the drained trace and the chip's
# registers.
add_executable(cogmaster-test-psg ${CMAKE_CURRENT_LIST_DIR}/test_psg.c)
target_include_directories(cogmaster-test-psg PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(cogmaster-test-psg PRIVATE cogmaster::cogmaster)
set_target_properties(cogmaster-test-psg PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
cogmaster_warnings(cogmaster-test-psg)

# Fault prevented: a Z80 write to the sound port that never reaches the chip, a chip that
# runs off the tick grid or lags its writes, a period, volume or noise setting that differs
# from its closed form, a reset or a loaded state that leaves the chip and the synthesiser
# disagreeing, or a level that is not heard in the stream.
foreach(case IN ITEMS through_bus snapshot reset_soft state_continue periods volumes
                  noise_controls lfsr_period protocol tie_rule frame_flush)
  # Timeout: 10 s floor; 0.08 s slowest local run (psg.state_continue), no hosted run yet, so
  # provisional. The be32 and wasm32 legs were not run locally; their 8 and 5 times scales
  # leave 80 s and 50 s.
  cogmaster_add_test(NAME psg.${case} COMMAND cogmaster-test-psg ${case}
    LABELS psg portable TIMEOUT 10)
endforeach()

# Fault prevented: a Game Gear stereo mask that sends a channel to the wrong output or to
# neither, a write to the register that leaves the chip behind it or the log without its
# record, or a Game Gear console in Master System video mode that lets the port split the
# outputs.
foreach(case IN ITEMS routing port06_sms_mode)
  # Timeout: 10 s floor; the slowest local run is under 0.1 s, no hosted run yet, so
  # provisional. be32 and wasm32 scales leave 80 s and 50 s.
  cogmaster_add_test(NAME psg.gg.${case} COMMAND cogmaster-test-psg gg_${case}
    LABELS psg portable TIMEOUT 10)
endforeach()

# The YM2413 chip module on its own: the port against the original's frozen digest, then the
# chip's behaviours through its entry points.
add_executable(cogmaster-test-fm ${CMAKE_CURRENT_LIST_DIR}/test_opll.c
  ${PROJECT_SOURCE_DIR}/runner/sha256.c)
target_include_directories(cogmaster-test-fm PRIVATE ${PROJECT_SOURCE_DIR}/src
  ${PROJECT_SOURCE_DIR}/runner ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(cogmaster-test-fm PRIVATE cogmaster::cogmaster)
set_target_properties(cogmaster-test-fm PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
cogmaster_warnings(cogmaster-test-fm)

# Fault prevented: a ported chip that differs from the original it was taken from, in any
# sample of two million over a seeded register stream, so a sound that drifted from the
# reference reaches a player unseen. The digest is frozen from the unmodified original at the
# pinned commit, so this test fetches nothing.
# Timeout: 10 s floor; 0.22 s native and 0.59 s under the address and undefined sanitizers
# (2 x 0.59 s is below the floor), no hosted run yet, so provisional. be32 and wasm32 were not
# run locally; their 8 and 5 times scales leave 80 s and 50 s, over 25 times the 1.8 s that
# eight times the native cost comes to.
cogmaster_add_test(NAME fm.lockstep COMMAND cogmaster-test-fm lockstep
  ${CMAKE_CURRENT_LIST_DIR}/opll_lockstep.txt LABELS fm portable TIMEOUT 10)

# Fault prevented: a key-on that never sounds or a key-off that never stops, a drum key that
# sounds the wrong drum, a noise generator, an LFO or a test-register bit that differs from
# the original, or a sample whose cost depends on what the ROM wrote.
foreach(case IN ITEMS key_on rhythm noise lfo test_register slot_count)
  # Timeout: 10 s floor; 0.05 s slowest local run, no hosted run yet, so provisional. The be32
  # and wasm32 scales leave 80 s and 50 s.
  cogmaster_add_test(NAME fm.${case} COMMAND cogmaster-test-fm ${case}
    LABELS fm portable TIMEOUT 10)
endforeach()
