# Fault prevented: a comparator accepting a changed pixel at (7,3).
cmake_minimum_required(VERSION 3.25)
execute_process(COMMAND "${COMPARE}" "${EXPECTED}" "${BMP}" --flip
  RESULT_VARIABLE rc OUTPUT_VARIABLE out)
message(STATUS "flip: compare exited ${rc}: ${out}")
if(NOT rc STREQUAL "1" OR NOT out MATCHES "first difference at 7,3:")
  message(FATAL_ERROR "flip: expected exit 1 naming pixel 7,3")
endif()
