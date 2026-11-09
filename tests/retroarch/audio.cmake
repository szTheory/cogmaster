# Fault prevented: a silent recording passing as sound, or a sound check that cannot fail.
cmake_minimum_required(VERSION 3.25)
set(ENV{LC_ALL} C)
execute_process(COMMAND afinfo "${MOV}" RESULT_VARIABLE rc OUTPUT_VARIABLE info)
if(NOT EXISTS "${MOV}" OR NOT rc STREQUAL "0" OR NOT MODE MATCHES "^(control|sound)$"
   OR NOT info MATCHES "audio bytes: ([0-9]+).*audio data file offset: ([0-9]+)")
  message(FATAL_ERROR "audio: no readable ${MODE} recording at ${MOV} (afinfo exit ${rc})")
endif()
set(bytes ${CMAKE_MATCH_1})
math(EXPR from "${CMAKE_MATCH_2} + 1")
execute_process(COMMAND tail -c +${from} "${MOV}" COMMAND head -c ${bytes}
  COMMAND tr -d "\\000" COMMAND wc -c OUTPUT_VARIABLE loud)
string(STRIP "${loud}" loud)
message(STATUS "audio: ${MODE} has ${bytes} bytes, ${loud} not zero")
if(bytes LESS 188000 OR bytes GREATER 196000 OR (MODE STREQUAL "control" AND NOT loud EQUAL 0)
   OR (MODE STREQUAL "sound" AND loud LESS 8192))
  message(FATAL_ERROR "audio: the ${MODE} recording is out of bounds")
endif()
