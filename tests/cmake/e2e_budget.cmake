# Real-host end-to-end line budget, run as
#   cmake -DDIR=<tests/retroarch> -P e2e_budget.cmake
# Fault prevented: real-host test code growing without bound. Every line of
# every file under DIR counts, its negative controls included, and the sum
# may not exceed 300.
cmake_minimum_required(VERSION 3.25)
if("${DIR}" STREQUAL "" OR NOT IS_DIRECTORY "${DIR}")
  message(FATAL_ERROR "e2e_budget: -DDIR must name a directory")
endif()
file(GLOB_RECURSE files "${DIR}/*")
set(total 0)
foreach(file IN LISTS files)
  # Counts newline bytes, as wc -l does, plus a last line without one.
  file(READ "${file}" hex HEX)
  string(REGEX MATCHALL ".." bytes "${hex}")
  list(FILTER bytes INCLUDE REGEX "^0a$")
  list(LENGTH bytes n)
  string(LENGTH "${hex}" len)
  if(len GREATER 0 AND NOT hex MATCHES "0a$")
    math(EXPR n "${n} + 1")
  endif()
  math(EXPR total "${total} + ${n}")
endforeach()
if(total GREATER 300)
  message(FATAL_ERROR "e2e_budget: ${total} lines under tests/retroarch exceeds 300")
endif()
message(STATUS "e2e_budget: ${total} lines under tests/retroarch, at most 300")
