# The bus and the run loop: images run through the public API, with the
# beam and the CPU read through src/internal.h.
add_executable(cogmaster-test-bus ${CMAKE_CURRENT_LIST_DIR}/test_bus.c)
target_include_directories(cogmaster-test-bus PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(cogmaster-test-bus PRIVATE cogmaster::cogmaster)
set_target_properties(cogmaster-test-bus PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
cogmaster_warnings(cogmaster-test-bus)

# Fault prevented: a run_frame call that runs past its frame by more than one
# step, or frames whose lengths drift from 179,208 ticks (214,092 on PAL), so
# a jammed image could hold the host past its frame time.
foreach(case IN ITEMS halt_bound.di halt_bound.di_pal halt_bound.ei_masked frame_origin)
  # Timeout: 10 s floor; 2 x 0.01 s, slowest of 2 runs, run 39376644025, provisional.
  cogmaster_add_test(NAME bus.${case} COMMAND cogmaster-test-bus ${case}
    LABELS bus portable TIMEOUT 10)
endforeach()

# Fault prevented: a frame interrupt that never reaches the CPU, or reaches
# it more than once a frame because a status read does not release /INT.
# Timeout: 10 s floor; 2 x 0.01 s, slowest of 2 runs, run 39376644025, provisional.
cogmaster_add_test(NAME bus.halt_bound.ie_positive COMMAND cogmaster-test-bus
  halt_bound.ie_positive LABELS bus portable TIMEOUT 10)

# Fault prevented: an access stamped off its data T-state, so a VDP event
# lands on the wrong side of a port access.
# Timeout: 10 s floor; 2 x 0.00 s, slowest of 2 runs, run 39376644025, provisional.
cogmaster_add_test(NAME bus.stamps COMMAND cogmaster-test-bus stamps
  LABELS bus portable TIMEOUT 10)

# Fault prevented: a jammed image hammering the VDP ports holding the host
# past its frame time.
# Timeout: 10 s floor; 2 x 0.01 s, slowest of 2 runs, run 39376644025, provisional.
cogmaster_add_test(NAME bus.hostile_vdp_loop COMMAND cogmaster-test-bus hostile_vdp_loop
  LABELS bus portable TIMEOUT 10)

# Fault prevented: an undefined ED opcode that runs silently, so the first
# fault a host reads names nothing.
# Timeout: 10 s floor; 2 x 0.00 s, slowest of 2 runs, run 39376644025, provisional.
cogmaster_add_test(NAME fault.ed_undefined COMMAND cogmaster-test-bus fault.ed_undefined
  LABELS bus portable TIMEOUT 10)

# Fault prevented: a read of a port nothing drives that returns a value
# silently, so a host tracing a broken image learns nothing about where it
# went wrong.
# Timeout: 10 s floor; 2 x 0.00 s, slowest of 2 runs, run 39376644025, provisional.
cogmaster_add_test(NAME fault.unmapped_read COMMAND cogmaster-test-bus fault.unmapped_read
  LABELS bus portable TIMEOUT 10)

# Fault prevented: a write that lands nowhere, a write to ROM or a mapper
# value past the image passing unrecorded, or a probe shipped software makes
# taking the first-fault record, so the record no longer names the real
# fault.
foreach(case IN ITEMS unmapped_write rom_write mapper_range not_faults)
  # Timeout: 10 s floor; 2 x 0.02 s, slowest of 2 runs, run 39376644025, provisional.
  cogmaster_add_test(NAME fault.${case} COMMAND cogmaster-test-bus fault.${case}
    LABELS bus portable TIMEOUT 10)
endforeach()

# The VDP's timing half: counters, ports, status and the Pause NMI tick.
add_executable(cogmaster-test-vdp ${CMAKE_CURRENT_LIST_DIR}/test_vdp.c)
target_include_directories(cogmaster-test-vdp PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(cogmaster-test-vdp PRIVATE cogmaster::cogmaster)
set_target_properties(cogmaster-test-vdp PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
cogmaster_warnings(cogmaster-test-vdp)

# Fault prevented: a counter read, port, status flag or decode that differs
# from the documented hardware, or a Pause NMI a line early or late, which
# moves every raster effect and pause handler that reads the counters.
foreach(case IN ITEMS background vcounter.ntsc vcounter.pal hcounter control_port data_port status
                      port_decode pause_nmi)
  # Timeout: 10 s floor; 2 x 0.01 s, slowest of 2 runs, run 39376644025, provisional.
  cogmaster_add_test(NAME vdp.${case} COMMAND cogmaster-test-vdp ${case}
    LABELS portable vdp TIMEOUT 10)
endforeach()
