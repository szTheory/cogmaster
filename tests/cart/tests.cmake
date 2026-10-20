# cogmaster_probe through the public header alone, over synthetic images.
add_executable(cogmaster-test-cart ${CMAKE_CURRENT_LIST_DIR}/test_cart.c)
target_link_libraries(cogmaster-test-cart PRIVATE cogmaster::cogmaster)
set_target_properties(cogmaster-test-cart PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
cogmaster_warnings(cogmaster-test-cart)

# Fault prevented: an image guessed onto the wrong board, a header read from
# the wrong place or past the image, or a probe whose answer depends on
# anything but the bytes, so a host's probe and the load disagree.
foreach(case IN ITEMS probe.branches probe.pure probe.sizes)
  cogmaster_add_test(NAME cart.${case} COMMAND cogmaster-test-cart ${case}
    LABELS cart portable TIMEOUT 10)
endforeach()
