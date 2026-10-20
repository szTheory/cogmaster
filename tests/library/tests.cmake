# Fault prevented: local-library coverage silently skips images absent from metadata.
if(CMAKE_SCRIPT_MODE_FILE)
  execute_process(COMMAND "${CMAKE_COMMAND}" --list-presets=workflow
    WORKING_DIRECTORY "${ROOT}" RESULT_VARIABLE rc OUTPUT_VARIABLE text)
  if(NOT rc STREQUAL "0" OR NOT text MATCHES "\"library\"")
    message(FATAL_ERROR "local library workflow must be available")
  endif()
  return()
endif()
if(COGMASTER_FIXTURES)
  string(REPLACE ";" "$<SEMICOLON>" library_emulator "${CMAKE_CROSSCOMPILING_EMULATOR}")
  cogmaster_add_test(NAME library.synthetic
    COMMAND ${CMAKE_COMMAND} -DROOT=${PROJECT_SOURCE_DIR} -DOUT=${PROJECT_BINARY_DIR}/tests/library
      -DFIXTURES=${PROJECT_BINARY_DIR}/fixtures -DRUN=$<TARGET_FILE:cogmaster-run>
      "-DEMULATOR=${library_emulator}" -P ${CMAKE_CURRENT_LIST_FILE}
    LABELS library TIMEOUT 60)
endif()
