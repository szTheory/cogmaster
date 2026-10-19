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
