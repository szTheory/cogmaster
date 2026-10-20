# Fault prevented: local records publish private strings or promote an observed output into a golden.
if(CMAKE_SCRIPT_MODE_FILE)
  file(MAKE_DIRECTORY "${OUT}")
  set(header "id,title,machine,region,mapper,predicate,status,evidence,date\n")
  set(id 00cfce623709b8d9bc9e6bbb96feb9faf9a81f84)
  file(WRITE "${OUT}/input.csv" "${header}${id},private-label,gg,ntsc-u,sega,boots@600,pass,private-trace,2026-10-20\n")
  execute_process(COMMAND sh "${ROOT}/scripts/compat-update.sh" "${OUT}/input.csv"
    RESULT_VARIABLE rc OUTPUT_VARIABLE text ERROR_VARIABLE err)
  if(NOT rc STREQUAL "0" OR NOT text MATCHES "Crazy Faces.*prototype.*pass")
    message(FATAL_ERROR "explicit accepted public case must update with its immutable CC0 title")
  endif()
  return()
endif()
