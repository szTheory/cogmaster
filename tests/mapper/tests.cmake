# The cartridge boards at register level: synthetic images built in the test,
# read and written through the bus in src/internal.h.
add_executable(cogmaster-test-mapper ${CMAKE_CURRENT_LIST_DIR}/test_mapper.c)
target_include_directories(cogmaster-test-mapper PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(cogmaster-test-mapper PRIVATE cogmaster::cogmaster)
set_target_properties(cogmaster-test-mapper PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
cogmaster_warnings(cogmaster-test-mapper)

# Fault prevented: a bank register that selects the wrong 16 KiB page, a
# slot 0 switch that moves the interrupt vectors, cartridge RAM that loses
# its bytes or shadows the wrong slot, or a register no longer readable from
# its RAM mirror, so a paged game runs code or reads data from the wrong
# place; or a cartridge or work RAM disabled by $3E that still takes writes,
# or drops them unrecorded.
foreach(case IN ITEMS sega.banks sega.wrap sega.ram sega.mirror sega.shift codemasters.banks
                      codemasters.ram codemasters.wrap none.layout pad_wrap slot_off)
  # Timeout: 10 s floor; 2 x 0.01 s, slowest of 2 runs, run 39376644025, provisional.
  cogmaster_add_test(NAME mapper.${case} COMMAND cogmaster-test-mapper ${case}
    LABELS mapper portable TIMEOUT 10)
endforeach()

# The Korean, Korean no-bank, Hi-Com, Zemina, Nemesis and 4 PAK boards at register level: images of
# exact sizes on each board, every register value written through the bus and
# compared with a model kept in the test, run under the address sanitizer in CI.
add_executable(cogmaster-test-mapper-ext ${CMAKE_CURRENT_LIST_DIR}/test_mapper_ext.c)
target_include_directories(cogmaster-test-mapper-ext PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(cogmaster-test-mapper-ext PRIVATE cogmaster::cogmaster)
set_target_properties(cogmaster-test-mapper-ext PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
cogmaster_warnings(cogmaster-test-mapper-ext)

# Fault prevented: a bank register that selects the wrong 8 KiB bank, a read
# that runs past the end of the image, a documented register write recorded as
# a fault, or a bank state that a saved state restores out of range.
foreach(case IN ITEMS korean korean_nb hicom zemina nemesis hes_4pak state)
  # Timeout: 10 s floor; 7 x 0.27 s, slowest of 3 runs, provisional.
  cogmaster_add_test(NAME mapper.ext.${case} COMMAND cogmaster-test-mapper-ext ${case}
    LABELS mapper portable TIMEOUT 10)
endforeach()

# Fault prevented: guessed battery spans, lost same-value writes and reset data loss.
add_executable(cogmaster-test-battery ${CMAKE_CURRENT_LIST_DIR}/test_battery.c)
target_include_directories(cogmaster-test-battery PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(cogmaster-test-battery PRIVATE cogmaster::cogmaster)
set_target_properties(cogmaster-test-battery PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
cogmaster_warnings(cogmaster-test-battery)
foreach(case IN ITEMS spans generation excluded fixture import continuation)
  set(args ${case})
  if(case STREQUAL "fixture" OR case STREQUAL "continuation")
    list(APPEND args ${PROJECT_BINARY_DIR}/fixtures/battery.sms)
  endif()
  cogmaster_add_test(NAME battery.${case} COMMAND cogmaster-test-battery ${args}
    LABELS mapper portable TIMEOUT 10)
endforeach()

# The 93C46 serial EEPROM as a device with no board, clocked bit by bit, against
# closed-form images and a model written from the data sheet.
add_executable(cogmaster-test-eeprom ${CMAKE_CURRENT_LIST_DIR}/test_eeprom.c)
target_include_directories(cogmaster-test-eeprom PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(cogmaster-test-eeprom PRIVATE cogmaster::cogmaster)
set_target_properties(cogmaster-test-eeprom PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
cogmaster_warnings(cogmaster-test-eeprom)

# Fault prevented: a 93C46 command that writes while locked, reads in the wrong order, or reports a change per clock edge, so a save is lost or rewritten on every frame; a board that wires the window, the bank registers or the chip's reset wrongly; or a state that resumes a command differently or loads an out-of-range chip state.
foreach(case IN ITEMS commands lock read changes differential board.window board.pins
                      board.reset_bit board.banks board.span state.window_open state.mid_command
                      state.bounds)
  # Timeout: 10 s floor; 13 x 0.05 s, slowest of 1 run, provisional.
  cogmaster_add_test(NAME eeprom.${case} COMMAND cogmaster-test-eeprom ${case}
    LABELS mapper portable TIMEOUT 10)
endforeach()
