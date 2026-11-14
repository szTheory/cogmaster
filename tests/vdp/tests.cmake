# Fault prevented: incorrect tile flags, wrap coordinates, scroll locks or
# raster writes changing dots already emitted in this field.
add_executable(cogmaster-test-gg ${CMAKE_CURRENT_LIST_DIR}/test_gg.c)
target_include_directories(cogmaster-test-gg PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(cogmaster-test-gg PRIVATE cogmaster::cogmaster)
set_target_properties(cogmaster-test-gg PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
cogmaster_warnings(cogmaster-test-gg)
foreach(case IN ITEMS palette colours intervals coordinates freeze profiles ports state)
  cogmaster_add_test(NAME vdp.gg.${case} COMMAND cogmaster-test-gg ${case} LABELS portable vdp TIMEOUT 10)
endforeach()

add_executable(cogmaster-test-geometry ${CMAKE_CURRENT_LIST_DIR}/test_geometry.c)
target_include_directories(cogmaster-test-geometry PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(cogmaster-test-geometry PRIVATE cogmaster::cogmaster)
set_target_properties(cogmaster-test-geometry PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
cogmaster_warnings(cogmaster-test-geometry)
foreach(case IN ITEMS heights coordinates transitions buffers state hostile)
  cogmaster_add_test(NAME vdp.geometry.${case} COMMAND cogmaster-test-geometry ${case}
    LABELS portable vdp TIMEOUT 10)
endforeach()

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

# Fault prevented: missed IRQ edges, lost pending sources or premature reloads.
add_executable(cogmaster-test-timing ${CMAKE_CURRENT_LIST_DIR}/test_timing.c)
target_include_directories(cogmaster-test-timing PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(cogmaster-test-timing PRIVATE cogmaster::cogmaster)
set_target_properties(cogmaster-test-timing PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
cogmaster_warnings(cogmaster-test-timing)
foreach(case IN ITEMS irq_edges irq_reload irq_pending irq_cpu irq_state hcounter th_capture
                      mode_edges deadlines counter_state th_edges
                      v_ntsc_192 v_ntsc_224 v_ntsc_240 v_pal_192 v_pal_224 v_pal_240)
  cogmaster_add_test(NAME vdp.timing.${case} COMMAND cogmaster-test-timing ${case}
    LABELS portable vdp TIMEOUT 10)
endforeach()

# Fault prevented: incorrect sprite shapes, SAT ordering and zoom slots.
add_executable(cogmaster-test-sprites ${CMAKE_CURRENT_LIST_DIR}/test_sprites.c)
target_include_directories(cogmaster-test-sprites PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(cogmaster-test-sprites PRIVATE cogmaster::cogmaster)
set_target_properties(cogmaster-test-sprites PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
cogmaster_warnings(cogmaster-test-sprites)
foreach(case IN ITEMS shapes wrap_clip zoom sms1_zoom priority gg_slots intervals bounds_state
                      collision overflow inactive sms1_collision)
  cogmaster_add_test(NAME vdp.sprites.${case} COMMAND cogmaster-test-sprites ${case}
    LABELS portable vdp TIMEOUT 10)
endforeach()

# Fault prevented: an oracle for the Graphics I and II draw that disagrees with hand-computed dots, status
# inputs or sprite rows, so that every later renderer comparison would be a comparison with a wrong answer.
# The renderer cases compare the in-process picture with it.
# Timeout: 10 s floor; the oracle case scans the field eleven times and runs in about a second, provisional.
add_executable(cogmaster-test-legacy ${CMAKE_CURRENT_LIST_DIR}/test_legacy.c)
target_include_directories(cogmaster-test-legacy PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(cogmaster-test-legacy PRIVATE cogmaster::cogmaster)
set_target_properties(cogmaster-test-legacy PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
cogmaster_warnings(cogmaster-test-legacy)
foreach(case IN ITEMS oracle
    # Fault prevented: a Graphics I or II dot, palette entry or backdrop that differs from the oracle,
    # which a picture that merely looks plausible would hide.
    g1 g2 palette backdrop
    # Fault prevented: a mask, row limit, interval split, mode switch, bring-up or hostile input that
    # moves a dot, repaints an emitted one or makes a call unbounded.
    masks rows intervals switch bringup hostile)
  cogmaster_add_test(NAME vdp.legacy.${case} COMMAND cogmaster-test-legacy ${case}
    LABELS portable vdp TIMEOUT 10)
endforeach()
# Fault prevented: a mode that should still fault drawing silently, or recording the wrong fault.
foreach(case IN ITEMS text multicolour mixed invalid_text native_gg)
  cogmaster_add_test(NAME vdp.unsupported.${case} COMMAND cogmaster-test-legacy ${case}
    LABELS portable vdp TIMEOUT 10)
endforeach()
