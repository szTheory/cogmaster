# The I/O ports per machine and region, read and written through the bus in
# src/internal.h.
add_executable(cogmaster-test-io ${CMAKE_CURRENT_LIST_DIR}/test_io.c)
target_include_directories(cogmaster-test-io PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(cogmaster-test-io PRIVATE cogmaster::cogmaster)
set_target_properties(cogmaster-test-io PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
cogmaster_warnings(cogmaster-test-io)

# Fault prevented: a port that answers on the wrong machine or region, a
# region or controller probe that reads the wrong bits, or an enable bit that
# leaves the cartridge or work RAM mapped, so a game takes the wrong region
# branch or reads the wrong bytes.
foreach(case IN ITEMS absolute_input pad_pins pause_history lag_decode port3e port3f_region port_dc_dd gg_ports open_bus decode_mirrors)
  # Timeout: 10 s floor; 2 x 0.00 s, slowest of 2 runs, run 39376644025, provisional.
  cogmaster_add_test(NAME io.${case} COMMAND cogmaster-test-io ${case}
    LABELS io portable TIMEOUT 10)
endforeach()
