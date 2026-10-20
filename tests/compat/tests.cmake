# Fault prevented: quoted titles are split, reordered or silently accepted malformed.
if(CMAKE_SCRIPT_MODE_FILE)
  file(MAKE_DIRECTORY "${OUT}")
  set(header "id,title,machine,region,mapper,predicate,status,evidence,date\n")
  set(row "fixture/a,\"A, \"\"quoted\"\"; title\",sms,ntsc-u,sega,boots@20,unknown,-,2026-10-20\n")
  file(WRITE "${OUT}/quoted.csv" "${header}${row}")
  execute_process(COMMAND sh -c "if test -f \"$1\"; then exec sh \"$1\" normalize \"$2\"; else printf 'normalizer=unavailable\\n'; fi" sh
    "${ROOT}/scripts/compat-common.sh" "${OUT}/quoted.csv" RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
  if(NOT rc STREQUAL "0" OR NOT out MATCHES "\"fixture/a\",\"A, \"\"quoted\"\"; title\"")
    message(FATAL_ERROR "accepted quoted CSV must normalize nine fields without changing title: exit ${rc}, ${out}${err}")
  endif()
  return()
endif()
cogmaster_add_test(NAME compat.csv
  COMMAND ${CMAKE_COMMAND} -DROOT=${PROJECT_SOURCE_DIR} -DOUT=${PROJECT_BINARY_DIR}/tests/compat
    -P ${CMAKE_CURRENT_LIST_FILE} LABELS compat TIMEOUT 30)
