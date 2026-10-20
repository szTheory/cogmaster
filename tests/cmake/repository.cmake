# Shared repository checks. Faults and negative controls retain their original check names.
cmake_minimum_required(VERSION 3.25)
get_filename_component(top "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
if(NOT DEFINED ROOT)
  set(ROOT "${top}")
endif()
find_program(repository_sh sh REQUIRED)
if(CHECK STREQUAL "exit_table")
  set(RUN_HELP "${CMAKE_CURRENT_BINARY_DIR}/exit-help.txt")
  execute_process(COMMAND ${EMULATOR} "${RUN}" --help OUTPUT_FILE "${RUN_HELP}" COMMAND_ERROR_IS_FATAL ANY)
endif()
foreach(key ROOT HEADER DOC OTHER API_DIR DIR PINS INFO INVENTORY ROOTS VERSION NEXT MODE LIB NM SYSTEM JUNIT LABELS EXCLUDE CI_YML JQ REPORT_ONLY PROVENANCE RUN_HELP)
  string(REPLACE ";" "\n" value "${${key}}")
  list(APPEND environment "${key}=${value}")
endforeach()
execute_process(COMMAND ${CMAKE_COMMAND} -E env ${environment}
  ${repository_sh} ${CMAKE_CURRENT_LIST_DIR}/repository.sh "${CHECK}"
  COMMAND_ERROR_IS_FATAL ANY)
