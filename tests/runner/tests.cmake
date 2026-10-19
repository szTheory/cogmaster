# cogmaster-run tests.
#
# Every command that starts cogmaster-run goes through the build's
# CMAKE_CROSSCOMPILING_EMULATOR, so an emulated lane runs its own guest binary.
set(runner_cmd ${CMAKE_CROSSCOMPILING_EMULATOR} $<TARGET_FILE:cogmaster-run> --frames 60 --hash)
set(runner_check_hash ${PROJECT_SOURCE_DIR}/tests/cmake/check_hash.cmake)

# Fault prevented: the runner's printed SMS card hash, the line hosts and CI
# compare, drifts from the pinned card or differs on another platform.
string(REPLACE ";" "$<SEMICOLON>" cmd "${runner_cmd}")
cogmaster_add_test(NAME runner.hash.sms
  COMMAND ${CMAKE_COMMAND} "-DCMD=${cmd}" -DEXPECTED=${COGMASTER_TESTCARD_SHA256_SMS}
    -P ${runner_check_hash}
  LABELS runner portable TIMEOUT 30)

# The same for the Game Gear card's 160x144 crop.
string(REPLACE ";" "$<SEMICOLON>" cmd "${runner_cmd};--machine;gg")
cogmaster_add_test(NAME runner.hash.gg
  COMMAND ${CMAKE_COMMAND} "-DCMD=${cmd}" -DEXPECTED=${COGMASTER_TESTCARD_SHA256_GG}
    -P ${runner_check_hash}
  LABELS runner portable TIMEOUT 30)

# Fault prevented: a hash check that passes whatever the runner prints. The
# SMS card's hash checked against the Game Gear pin must fail with exit 1 and
# the mismatch message.
string(REPLACE ";" "\;" inner "${runner_cmd}")
set(cmd ${CMAKE_COMMAND} "-DCMD=${inner}" -DEXPECTED=${COGMASTER_TESTCARD_SHA256_GG}
  -P ${runner_check_hash})
string(REPLACE ";" "$<SEMICOLON>" cmd "${cmd}")
cogmaster_add_test(NAME runner.hash.self_test
  COMMAND ${CMAKE_COMMAND} "-DCMD=${cmd}" -DCODE=1 "-DREGEX=hash mismatch"
    -P ${PROJECT_SOURCE_DIR}/tests/cmake/expect_fail.cmake
  LABELS runner TIMEOUT 30)
