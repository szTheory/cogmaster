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

# Fault prevented: guessed battery spans, lost same-value writes and reset data loss.
add_executable(cogmaster-test-battery ${CMAKE_CURRENT_LIST_DIR}/test_battery.c)
target_include_directories(cogmaster-test-battery PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(cogmaster-test-battery PRIVATE cogmaster::cogmaster)
set_target_properties(cogmaster-test-battery PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
cogmaster_warnings(cogmaster-test-battery)
foreach(case IN ITEMS spans generation excluded fixture)
  set(args ${case})
  if(case STREQUAL fixture)
    list(APPEND args ${PROJECT_BINARY_DIR}/fixtures/battery.sms)
  endif()
  cogmaster_add_test(NAME battery.${case} COMMAND cogmaster-test-battery ${args}
    LABELS mapper portable TIMEOUT 10)
endforeach()
