# The core library through its public header. One executable runs each case
# named on its command line; it also sees src/internal.h, so it can compare
# the frame with the test card's pixel function.
add_executable(cogmaster-test-core ${CMAKE_CURRENT_LIST_DIR}/test_core.c)
target_include_directories(cogmaster-test-core PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(cogmaster-test-core PRIVATE cogmaster::cogmaster)
set_target_properties(cogmaster-test-core PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
cogmaster_warnings(cogmaster-test-core)

foreach(case IN ITEMS testcard.static testcard.boundaries testcard.all_colours testcard.full_field
    info load.profile_rejected api.size_tags api.statuses api.first_released_sizes
    create.allocator frame.pitch_wide)
  cogmaster_add_test(NAME core.${case}
    COMMAND cogmaster-test-core ${case}
    LABELS core portable TIMEOUT 10)
endforeach()

# The internal state visitor, called directly: the library has no public
# state API yet.
add_executable(cogmaster-test-state ${CMAKE_CURRENT_LIST_DIR}/test_state.c)
target_include_directories(cogmaster-test-state PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(cogmaster-test-state PRIVATE cogmaster::cogmaster)
set_target_properties(cogmaster-test-state PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
cogmaster_warnings(cogmaster-test-state)

foreach(case IN ITEMS state.round_trip state.record_sweep state.rejects_header state.frame_counter)
  cogmaster_add_test(NAME core.${case}
    COMMAND cogmaster-test-state ${case}
    LABELS core portable TIMEOUT 10)
endforeach()
