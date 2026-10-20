# Fault prevented: an accepted expectation is ignored or becomes its own golden.
if(CMAKE_SCRIPT_MODE_FILE)
  file(MAKE_DIRECTORY "${OUT}")
  file(WRITE "${OUT}/card.expect" "frame@1=e14d77ce564f0b1498f703134e82e6c1fc718a27c94e49b5a35b95a981857ae7\nram@2:$C200=00\n")
  execute_process(COMMAND ${EMULATOR} "${RUN}" --frames 2 --expect "${OUT}/card.expect"
    RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
  if(NOT rc STREQUAL "0" OR NOT out MATCHES "predicate.*pass")
    message(FATAL_ERROR "accepted independent frame/RAM predicates: expected exit 0 and pass, got ${rc}: ${out}${err}")
  endif()
  return()
endif()
string(REPLACE ";" "$<SEMICOLON>" expect_emulator "${CMAKE_CROSSCOMPILING_EMULATOR}")
cogmaster_add_test(NAME runner.expect.predicates
  COMMAND ${CMAKE_COMMAND} -DRUN=$<TARGET_FILE:cogmaster-run> "-DEMULATOR=${expect_emulator}"
    -DOUT=${PROJECT_BINARY_DIR}/tests/expect -P ${CMAKE_CURRENT_LIST_FILE}
  LABELS portable runner TIMEOUT 30)
