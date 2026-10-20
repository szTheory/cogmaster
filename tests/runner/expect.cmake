# Fault prevented: an accepted expectation is ignored or becomes its own golden.
if(CMAKE_SCRIPT_MODE_FILE)
  file(MAKE_DIRECTORY "${OUT}")
  function(check code regex body)
    file(WRITE "${OUT}/case.expect" "${body}")
    execute_process(COMMAND ${EMULATOR} "${RUN}" --frames 20 --expect "${OUT}/case.expect" ${ARGN}
      RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
    if(NOT rc STREQUAL "${code}" OR NOT "${out}${err}" MATCHES "${regex}")
      message(FATAL_ERROR "predicate expected exit ${code} / ${regex}, got ${rc}: ${out}${err}")
    endif()
  endfunction()
  file(WRITE "${OUT}/card.expect" "frame@1=e14d77ce564f0b1498f703134e82e6c1fc718a27c94e49b5a35b95a981857ae7\nram@2:$C200=00\n")
  execute_process(COMMAND ${EMULATOR} "${RUN}" --frames 2 --expect "${OUT}/card.expect"
    RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
  if(NOT rc STREQUAL "0" OR NOT out MATCHES "predicate.*pass")
    message(FATAL_ERROR "accepted independent frame/RAM predicates: expected exit 0 and pass, got ${rc}: ${out}${err}")
  endif()
  string(REGEX MATCHALL "predicate kind=" judged "${out}")
  list(LENGTH judged count)
  if(NOT count EQUAL 2 OR NOT out MATCHES "execution machine=1 region=1 tv=1.*script_sha256=")
    message(FATAL_ERROR "multiple requested frames were not judged once with resolved facts")
  endif()
  check(3 "predicate.*fail" "frame@2=014d77ce564f0b1498f703134e82e6c1fc718a27c94e49b5a35b95a981857ae7\n")
  check(3 "predicate.*fail" "ram@2:$C200=01\n")
  check(3 "display=0 distinct_at_least=1 first_fault=0" "boots@20\n")
  check(3 "predicate.*fail" "mailbox@2\n")
  foreach(body IN ITEMS "" "\n" "audio@1=00\n" "boots@0\n" "boots@21\n" "boots@4294967296\n"
      "boots@-1\n" "boots@1x\n" "frame@1=0\n" "ram@1:$10000=00\n" "ram@1:$BFFF=00\n"
      "ram@1:$C000=100\n" "ram@1:$C000=-1\n" "ram@1:$C000=GG\n" "boots@1")
    check(2 "expectation file error" "${body}")
  endforeach()
  string(REPEAT "0" 256 huge)
  check(2 "expectation file error" "${huge}\n")
  string(REPEAT "boots@1\n" 65 huge)
  check(2 "expectation file error" "${huge}")
  foreach(args IN ITEMS "--trace;first-fault" "--mailbox" "--probe")
    execute_process(COMMAND ${EMULATOR} "${RUN}" --frames 20 --expect "${OUT}/card.expect" ${args}
      RESULT_VARIABLE rc OUTPUT_QUIET ERROR_QUIET)
    if(NOT rc STREQUAL "2")
      message(FATAL_ERROR "conflicting run options must exit 2")
    endif()
  endforeach()
  execute_process(COMMAND ${EMULATOR} "${RUN}" --frames 1000001 --expect "${OUT}/card.expect"
    RESULT_VARIABLE rc OUTPUT_QUIET ERROR_QUIET)
  if(NOT rc STREQUAL "2")
    message(FATAL_ERROR "expectation run must enforce frame cap before running")
  endif()
  execute_process(COMMAND ${EMULATOR} "${RUN}" --frames 1 --expect "${OUT}/absent.expect"
    RESULT_VARIABLE rc OUTPUT_QUIET ERROR_QUIET)
  if(NOT rc STREQUAL "1")
    message(FATAL_ERROR "missing expectation file is infrastructure failure")
  endif()
  check(77 "unsupported" "boots@20\n" --machine gg --region pal)
  check(3 "first_fault=1.*fault frame=.*kind=unmapped_read" "boots@20\n" "${SEEDS}/in-a-00.sms")
  check(3 "display=1 distinct_at_least=1 first_fault=0" "boots@20\n" "${SEEDS}/static-display.sms")
  if(EXISTS "${FIXTURES}/vdp_lines_192.sms")
    check(0 "display=1 distinct_at_least=2 first_fault=0" "boots@20\nmailbox@20\n"
      "${FIXTURES}/vdp_lines_192.sms")
    check(3 "predicate.*fail" "mailbox@1\n" "${FIXTURES}/vdp_lines_192.sms")
    file(READ "${INPUT}" script)
    string(REPEAT "p1=00 p2=00 pause=0 reset=0 start=0\n" 6 extra)
    file(WRITE "${OUT}/play.input" "${script}${extra}")
    check(0 "predicate.*pass" "ram@14:$C200=02\n" --input "${OUT}/play.input" "${FIXTURES}/boot_to_play.sms")
  endif()
  message(STATUS "predicate pass/mismatch/usage/refusal/infrastructure and HALT smoke controls passed")
  return()
endif()
string(REPLACE ";" "$<SEMICOLON>" expect_emulator "${CMAKE_CROSSCOMPILING_EMULATOR}")
cogmaster_add_test(NAME runner.expect.predicates
  COMMAND ${CMAKE_COMMAND} -DRUN=$<TARGET_FILE:cogmaster-run> "-DEMULATOR=${expect_emulator}"
    -DOUT=${PROJECT_BINARY_DIR}/tests/expect -DFIXTURES=${PROJECT_BINARY_DIR}/fixtures
    -DSEEDS=${PROJECT_BINARY_DIR}/tests/seeds
    -DINPUT=${PROJECT_BINARY_DIR}/tests/play.input -P ${CMAKE_CURRENT_LIST_FILE}
  LABELS portable runner TIMEOUT 30)
