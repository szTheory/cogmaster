# cogmaster_probe through the public header alone, over synthetic images.
add_executable(cogmaster-test-cartdb ${CMAKE_CURRENT_LIST_DIR}/test_cartdb.c)
target_link_libraries(cogmaster-test-cartdb PRIVATE cogmaster::cogmaster)
cogmaster_warnings(cogmaster-test-cartdb)
cogmaster_add_test(NAME cart.database COMMAND cogmaster-test-cartdb LABELS cart portable TIMEOUT 10)
cogmaster_add_test(NAME cart.fixture_identity COMMAND cogmaster-test-cartdb
  ${PROJECT_BINARY_DIR}/fixtures/battery.sms LABELS cart portable TIMEOUT 10)
target_include_directories(cogmaster-test-cartdb PRIVATE ${PROJECT_SOURCE_DIR}/src)
add_executable(cogmaster-cartdb ${PROJECT_SOURCE_DIR}/tools/cartdb.c)
cogmaster_warnings(cogmaster-cartdb)
string(REPLACE ";" "$<SEMICOLON>" cartdb_emulator "${CMAKE_CROSSCOMPILING_EMULATOR}")
# Fault prevented: filename classification and a changed assembler output losing battery metadata.
cogmaster_add_test(NAME cart.fixture_hash COMMAND ${CMAKE_COMMAND}
  -DIMAGE=${PROJECT_BINARY_DIR}/fixtures/battery.sms
  -DFACT=${CMAKE_CURRENT_LIST_DIR}/fixture_facts.txt
  -DOUT=${PROJECT_BINARY_DIR}/tests/cart-fixture -DORACLE=$<TARGET_FILE:cogmaster-test-cartdb>
  "-DEMULATOR=${cartdb_emulator}" -P ${CMAKE_CURRENT_LIST_DIR}/fixture.cmake
  LABELS cart portable TIMEOUT 10)
set(cmd ${CMAKE_COMMAND} -DWRONG=1 -DIMAGE=${PROJECT_BINARY_DIR}/fixtures/battery.sms
  -DFACT=${CMAKE_CURRENT_LIST_DIR}/fixture_facts.txt -P ${CMAKE_CURRENT_LIST_DIR}/fixture.cmake)
string(REPLACE ";" "$<SEMICOLON>" cmd "${cmd}")
cogmaster_add_test(NAME cart.fixture_wrong_hash COMMAND ${CMAKE_COMMAND}
  "-DCMD=${cmd}" -DCODE=1 "-DREGEX=SHA-1.*mismatch"
  -P ${PROJECT_SOURCE_DIR}/tests/cmake/expect_fail.cmake LABELS cart portable TIMEOUT 10)
# Fault prevented: guessed whole-image identities, conflicting facts and metadata parser drift.
cogmaster_add_test(NAME cart.generator COMMAND ${CMAKE_COMMAND}
  -DGEN=$<TARGET_FILE:cogmaster-cartdb> -DOUT=${PROJECT_BINARY_DIR}/tests/cartdb
  "-DEMULATOR=${cartdb_emulator}" -P ${CMAKE_CURRENT_LIST_DIR}/generator.cmake
  LABELS cart portable TIMEOUT 10)
add_executable(cogmaster-test-cart ${CMAKE_CURRENT_LIST_DIR}/test_cart.c)
target_link_libraries(cogmaster-test-cart PRIVATE cogmaster::cogmaster)
set_target_properties(cogmaster-test-cart PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
cogmaster_warnings(cogmaster-test-cart)

# Fault prevented: an image guessed onto the wrong board, a header read from
# the wrong place or past the image, or a probe whose answer depends on
# anything but the bytes, so a host's probe and the load disagree.
foreach(case IN ITEMS probe.branches probe.pure probe.sizes)
  # Timeout: 10 s floor; 2 x 0.18 s, slowest of 2 runs, run 39376644025, provisional.
  cogmaster_add_test(NAME cart.${case} COMMAND cogmaster-test-cart ${case}
    LABELS cart portable TIMEOUT 10)
endforeach()
