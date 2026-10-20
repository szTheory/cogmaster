# The cartridge boards at register level: synthetic images built in the test,
# read and written through the bus in src/internal.h.
add_executable(cogmaster-test-mapper ${CMAKE_CURRENT_LIST_DIR}/test_mapper.c)
target_include_directories(cogmaster-test-mapper PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(cogmaster-test-mapper PRIVATE cogmaster::cogmaster)
set_target_properties(cogmaster-test-mapper PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
cogmaster_warnings(cogmaster-test-mapper)

# Fault prevented: a bank register that selects the wrong 16 KiB page, or a
# slot 0 switch that moves the interrupt vectors, so a paged game runs code
# or reads data from the wrong place.
foreach(case IN ITEMS sega.banks)
  cogmaster_add_test(NAME mapper.${case} COMMAND cogmaster-test-mapper ${case}
    LABELS mapper portable TIMEOUT 10)
endforeach()
