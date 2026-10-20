# The bus and the run loop: images run through the public API, with the
# beam and the CPU read through src/internal.h.
add_executable(cogmaster-test-bus ${CMAKE_CURRENT_LIST_DIR}/test_bus.c)
target_include_directories(cogmaster-test-bus PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(cogmaster-test-bus PRIVATE cogmaster::cogmaster)
set_target_properties(cogmaster-test-bus PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
cogmaster_warnings(cogmaster-test-bus)

# Fault prevented: a run_frame call that runs past its frame by more than one
# step, or frames whose lengths drift from 179,208 ticks, so a jammed image
# could hold the host past its frame time.
foreach(case IN ITEMS halt_bound.di frame_origin)
  cogmaster_add_test(NAME bus.${case} COMMAND cogmaster-test-bus ${case}
    LABELS bus portable TIMEOUT 10)
endforeach()
