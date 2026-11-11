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
target_include_directories(cogmaster-cartdb PRIVATE ${PROJECT_SOURCE_DIR}/include ${PROJECT_BINARY_DIR}/generated)
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
# Fault prevented: an owned image edited without its table row, so the default controller or region no
# longer applies and nothing notices. A wrong declared hash for each image must fail the same check.
# Timeout: 10 s floor; 2 x 0.08 s, slowest of 1 run, run 42978311844.
foreach(row IN ITEMS 3:sports:sports_pad 4:paddle:paddle)
  string(REPLACE ":" ";" row "${row}")
  list(GET row 0 index)
  list(GET row 1 case)
  list(GET row 2 image)
  set(identity ${CMAKE_COMMAND} -DHASH_ONLY=1 -DROW=${index}
    -DIMAGE=${PROJECT_BINARY_DIR}/fixtures/${image}.sms
    -DFACT=${CMAKE_CURRENT_LIST_DIR}/fixture_facts.txt)
  cogmaster_add_test(NAME cart.fixture_identity.${case} COMMAND ${identity}
    -P ${CMAKE_CURRENT_LIST_DIR}/fixture.cmake LABELS cart portable TIMEOUT 10)
  set(cmd ${identity} -DWRONG=1 -P ${CMAKE_CURRENT_LIST_DIR}/fixture.cmake)
  string(REPLACE ";" "$<SEMICOLON>" cmd "${cmd}")
  cogmaster_add_test(NAME cart.fixture_wrong_hash.${case} COMMAND ${CMAKE_COMMAND}
    "-DCMD=${cmd}" -DCODE=1 "-DREGEX=SHA-1.*mismatch"
    -P ${PROJECT_SOURCE_DIR}/tests/cmake/expect_fail.cmake LABELS cart portable TIMEOUT 10)
endforeach()
# Fault prevented: guessed whole-image identities, conflicting facts and metadata parser drift.
cogmaster_add_test(NAME cart.generator COMMAND ${CMAKE_COMMAND}
  -DGEN=$<TARGET_FILE:cogmaster-cartdb> -DOUT=${PROJECT_BINARY_DIR}/tests/cartdb
  -DFACT=${CMAKE_CURRENT_LIST_DIR}/fixture_facts.txt
  -DPINS=${PROJECT_SOURCE_DIR}/pins.txt -DTABLE=${PROJECT_SOURCE_DIR}/src/cartdb.inc
  "-DEMULATOR=${cartdb_emulator}" -P ${CMAKE_CURRENT_LIST_DIR}/generator.cmake
  LABELS cart portable TIMEOUT 10)
add_executable(cogmaster-test-cart ${CMAKE_CURRENT_LIST_DIR}/test_cart.c)
target_link_libraries(cogmaster-test-cart PRIVATE cogmaster::cogmaster)
set_target_properties(cogmaster-test-cart PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
cogmaster_warnings(cogmaster-test-cart)

# Fault prevented: an image guessed onto the wrong board, a header read from
# the wrong place or past the image, a Sega game turned Korean by data bytes
# that merely look like register stores, or a probe whose answer depends on
# anything but the bytes, so a host's probe and the load disagree.
foreach(case IN ITEMS probe.branches probe.pure probe.scan probe.sizes)
  # Timeout: 10 s floor; 2 x 0.18 s, slowest of 2 runs, run 39376644025, provisional.
  cogmaster_add_test(NAME cart.${case} COMMAND cogmaster-test-cart ${case}
    LABELS cart portable TIMEOUT 10)
endforeach()

# The controller a dump defaults to, the region a paddle dump gets and the default a profile cannot
# take, over every row of the table against the codes and the availability rule written out again.
add_executable(cogmaster-test-cart-device ${CMAKE_CURRENT_LIST_DIR}/test_cart_device.c)
target_include_directories(cogmaster-test-cart-device PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(cogmaster-test-cart-device PRIVATE cogmaster::cogmaster)
set_target_properties(cogmaster-test-cart-device PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
cogmaster_warnings(cogmaster-test-cart-device)

# Fault prevented: a Paddle dump that loads as export and finds no paddle, or a default the profile
# cannot take that fails the load instead of degrading to a pad.
foreach(case IN ITEMS defaults region degrade none)
  # Timeout: 20 s floor; 2 x 0.05 s, slowest of 1 run, run 42973268415.
  cogmaster_add_test(NAME cart.device.${case} COMMAND cogmaster-test-cart-device ${case}
    LABELS cart portable TIMEOUT 20)
endforeach()
