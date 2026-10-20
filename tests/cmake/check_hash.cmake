# Pinned frame-hash check, used as
#   cmake -DCMD=<command list> -DEXPECTED=<64 hex digits> -P check_hash.cmake
# Fault prevented: a test card that changes, or hashes differently on another
# platform, while every other test still passes. CMD must exit 0 and print a
# "frame N sha256 X" line whose X equals EXPECTED.
foreach(var CMD EXPECTED)
  if(NOT DEFINED ${var} OR "${${var}}" STREQUAL "")
    message(FATAL_ERROR "check_hash: -D${var} is required")
  endif()
endforeach()

execute_process(COMMAND ${CMD} RESULT_VARIABLE rc
  OUTPUT_VARIABLE out ERROR_VARIABLE err)
if(NOT rc STREQUAL "0")
  message(FATAL_ERROR "check_hash: command exited ${rc}\n${out}${err}")
endif()
if(NOT out MATCHES "frame [0-9]+ sha256 ([0-9a-f]+)")
  message(FATAL_ERROR "check_hash: no 'frame N sha256' line in the output\n${out}")
endif()
set(got "${CMAKE_MATCH_1}")
if(NOT got STREQUAL EXPECTED)
  message(FATAL_ERROR "check_hash: hash mismatch: expected ${EXPECTED} got ${got}")
endif()
message(STATUS "check_hash: ${got}")
