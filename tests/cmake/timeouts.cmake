# Workflow timeout comments, run as
#   cmake -DDIR=<workflows directory> -P timeouts.cmake
# Fault prevented: a job timeout guessed rather than measured. Every
# timeout-minutes line must carry a comment naming the run it was measured
# from ("run" and nine or more digits) or the word provisional, which marks a
# job with too few runs yet.
cmake_minimum_required(VERSION 3.25)
if("${DIR}" STREQUAL "")
  message(FATAL_ERROR "timeout_comments: -DDIR is required")
endif()
file(GLOB files "${DIR}/*.yml" "${DIR}/*.yaml")
set(bad "")
set(seen 0)
set(id "runs? [0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9]")
foreach(file IN LISTS files)
  get_filename_component(base "${file}" NAME)
  file(STRINGS "${file}" lines REGEX "^[ \t-]*timeout-minutes:")
  foreach(line IN LISTS lines)
    math(EXPR seen "${seen} + 1")
    if(NOT line MATCHES "timeout-minutes:[^#]*#.*(provisional|${id})")
      string(STRIP "${line}" line)
      list(APPEND bad "${base}: ${line}")
    endif()
  endforeach()
endforeach()
list(LENGTH bad count)
if(count GREATER 0)
  list(JOIN bad "\n" bad)
  message(FATAL_ERROR "${bad}\ntimeout_comments: ${count} lines without a run ID or provisional")
endif()
if(seen EQUAL 0)
  message(FATAL_ERROR "timeout_comments: no timeout-minutes line under ${DIR}")
endif()
message(STATUS "timeout_comments: ${seen} timeouts, each with a run ID or provisional")
