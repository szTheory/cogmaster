# The controller ports at pin level, against the formulas they replaced: every
# pair of $3F values and every pad value, written and read through the I/O
# functions in src/internal.h.
add_executable(cogmaster-test-ports ${CMAKE_CURRENT_LIST_DIR}/test_ports.c)
target_include_directories(cogmaster-test-ports PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(cogmaster-test-ports PRIVATE cogmaster::cogmaster)
set_target_properties(cogmaster-test-ports PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
cogmaster_warnings(cogmaster-test-ports)

# Fault prevented: a refactor that moves the H-counter latch or a pad pin, so every
# light-gun, H-counter and pad game changes behaviour without a test noticing.
foreach(case IN ITEMS latch latch_control pins)
  # Timeout: 20 s floor; provisional until a hosted run measures it.
  cogmaster_add_test(NAME ports.${case} COMMAND cogmaster-test-ports ${case}
    LABELS ports portable TIMEOUT 20)
endforeach()
