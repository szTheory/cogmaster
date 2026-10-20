# Fault prevented: incorrect tile flags, wrap coordinates, scroll locks or
# raster writes changing dots already emitted in this field.
add_executable(cogmaster-test-background ${CMAKE_CURRENT_LIST_DIR}/test_background.c)
target_include_directories(cogmaster-test-background PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(cogmaster-test-background PRIVATE cogmaster::cogmaster)
set_target_properties(cogmaster-test-background PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
cogmaster_warnings(cogmaster-test-background)
foreach(case IN ITEMS tiles scroll locks intervals latches empty priority state_interval
                      sms1_masks sms1_state unsupported)
  cogmaster_add_test(NAME vdp.background.${case} COMMAND cogmaster-test-background ${case}
    LABELS portable vdp TIMEOUT 10)
endforeach()

# Fault prevented: incorrect sprite shapes, SAT ordering and zoom slots.
add_executable(cogmaster-test-sprites ${CMAKE_CURRENT_LIST_DIR}/test_sprites.c)
target_include_directories(cogmaster-test-sprites PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(cogmaster-test-sprites PRIVATE cogmaster::cogmaster)
set_target_properties(cogmaster-test-sprites PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
cogmaster_warnings(cogmaster-test-sprites)
foreach(case IN ITEMS shapes wrap_clip zoom sms1_zoom priority gg_slots intervals)
  cogmaster_add_test(NAME vdp.sprites.${case} COMMAND cogmaster-test-sprites ${case}
    LABELS portable vdp TIMEOUT 10)
endforeach()
