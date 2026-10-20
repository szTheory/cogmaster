# Fault prevented: incorrect tile flags, wrap coordinates, scroll locks or
# raster writes changing dots already emitted in this field.
add_executable(cogmaster-test-background ${CMAKE_CURRENT_LIST_DIR}/test_background.c)
target_include_directories(cogmaster-test-background PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(cogmaster-test-background PRIVATE cogmaster::cogmaster)
set_target_properties(cogmaster-test-background PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
cogmaster_warnings(cogmaster-test-background)
foreach(case IN ITEMS tiles scroll locks intervals latches empty)
  cogmaster_add_test(NAME vdp.background.${case} COMMAND cogmaster-test-background ${case}
    LABELS portable vdp TIMEOUT 10)
endforeach()
