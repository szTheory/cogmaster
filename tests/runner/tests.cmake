# cogmaster-run tests.
#
# Run as a script (cmake -DCASE=<case> ... -P tests.cmake), this file is one
# runner check; include()d by tests/tests.cmake, it registers the tests.
if(CMAKE_SCRIPT_MODE_FILE)
  # Runs cogmaster-run with ARGN and fails unless it exits with code and its
  # merged output matches regex.
  function(expect_exit code regex)
    execute_process(COMMAND ${EMULATOR} ${RUN} ${ARGN} RESULT_VARIABLE rc
      OUTPUT_VARIABLE out ERROR_VARIABLE out)
    if(NOT rc STREQUAL code OR NOT out MATCHES "${regex}")
      message(FATAL_ERROR "cogmaster-run ${ARGN}: expected exit ${code} and output matching\n"
        "  ${regex}\ngot exit ${rc} and output\n${out}")
    endif()
  endfunction()

  if(CASE STREQUAL "png_file")
    # The decode check runs first, so the pinned digest is only ever
    # compared for a file whose IHDR and every pixel are already known good.
    file(REMOVE "${OUT}")
    expect_exit(0 "^$" --frames 60 --png "60:${OUT}")
    execute_process(COMMAND ${EMULATOR} ${DECODE} png_file "${OUT}" RESULT_VARIABLE rc
      OUTPUT_VARIABLE out ERROR_VARIABLE out)
    if(NOT rc STREQUAL "0")
      message(FATAL_ERROR "the PNG does not decode to the card (exit ${rc})\n${out}")
    endif()
    file(SHA256 "${OUT}" sha256)
    if(NOT sha256 STREQUAL EXPECTED)
      message(FATAL_ERROR "PNG SHA-256 mismatch: expected ${EXPECTED} got ${sha256}")
    endif()
    # A file the runner cannot write is a failure, not a success.
    expect_exit(1 "cannot write" --frames 1 --png "1:${OUT}.missing/x.png")
  elseif(CASE STREQUAL "png_syntax")
    # Each malformed or out-of-range --png is a usage error, whichever order
    # the options come in, and writes nothing.
    file(REMOVE "${OUT}")
    expect_exit(2 "usage" --frames 60 --png)
    expect_exit(2 "usage" --frames 60 --png "${OUT}")
    expect_exit(2 "usage" --frames 60 --png "60:")
    expect_exit(2 "usage" --frames 60 --png ":${OUT}")
    expect_exit(2 "usage" --frames 60 --png "0:${OUT}")
    expect_exit(2 "usage" --frames 60 --png "x:${OUT}")
    expect_exit(2 "usage" --frames 5 --png "6:${OUT}")
    expect_exit(2 "usage" --png "61:${OUT}" --frames 60)
    if(EXISTS "${OUT}")
      message(FATAL_ERROR "a refused --png wrote ${OUT}")
    endif()
  else()
    message(FATAL_ERROR "unknown CASE '${CASE}'")
  endif()
  return()
endif()

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

# The PNG encoder and its checksums, checked by an independent parse. The
# same program decodes the runner's PNG for runner.png_file.
add_executable(cogmaster-test-runner
  ${CMAKE_CURRENT_LIST_DIR}/test_png.c
  ${PROJECT_SOURCE_DIR}/runner/png.c
  ${PROJECT_SOURCE_DIR}/runner/crc32.c)
target_include_directories(cogmaster-test-runner PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(cogmaster-test-runner PRIVATE cogmaster::cogmaster)
set_target_properties(cogmaster-test-runner PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
cogmaster_warnings(cogmaster-test-runner)

# Fault prevented: an encoder whose chunk CRCs, Adler-32 or stored blocks are
# wrong, so a viewer or the RetroArch compare rejects or misreads the file.
cogmaster_add_test(NAME runner.png
  COMMAND cogmaster-test-runner png
  LABELS runner portable TIMEOUT 10)

string(REPLACE ";" "$<SEMICOLON>" runner_emulator "${CMAKE_CROSSCOMPILING_EMULATOR}")
set(runner_check ${CMAKE_COMMAND} -DRUN=$<TARGET_FILE:cogmaster-run>
  "-DEMULATOR=${runner_emulator}")

# Fault prevented: the reference PNG the RetroArch screenshot is compared
# with changes bytes (palette, layout or encoder) while the frame hash still
# passes, or no longer shows the card.
cogmaster_add_test(NAME runner.png_file
  COMMAND ${runner_check} -DCASE=png_file -DOUT=${PROJECT_BINARY_DIR}/tests/card.png
    -DDECODE=$<TARGET_FILE:cogmaster-test-runner> -DEXPECTED=${COGMASTER_TESTCARD_PNG_SHA256_SMS}
    -P ${CMAKE_CURRENT_LIST_FILE}
  LABELS runner portable TIMEOUT 30)

# Fault prevented: a malformed or out-of-range --png that is ignored or
# half-applied instead of refused as a usage error.
cogmaster_add_test(NAME runner.usage.png_syntax
  COMMAND ${runner_check} -DCASE=png_syntax -DOUT=${PROJECT_BINARY_DIR}/tests/refused.png
    -P ${CMAKE_CURRENT_LIST_FILE}
  LABELS runner portable TIMEOUT 30)
