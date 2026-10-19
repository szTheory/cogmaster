# Workflow action pins, run as
#   cmake -DDIR=<workflows directory> -P action_pins.cmake
# Fault prevented: an action referenced by tag or branch, which whoever moves
# that tag can change under the release. Every uses: line must name a local
# ./path, or owner/repo[/path] at a full 40-hex commit followed by a
# "# v..." comment naming the release the commit is.
cmake_minimum_required(VERSION 3.25)
if("${DIR}" STREQUAL "")
  message(FATAL_ERROR "action_pins: -DDIR is required")
endif()
file(GLOB files "${DIR}/*.yml" "${DIR}/*.yaml")
set(bad "")
set(seen 0)
string(REPEAT "[0-9a-f]" 40 sha)
foreach(file IN LISTS files)
  get_filename_component(base "${file}" NAME)
  file(STRINGS "${file}" lines REGEX "^[ \t-]*uses:")
  foreach(line IN LISTS lines)
    math(EXPR seen "${seen} + 1")
    string(REGEX REPLACE "^[ \t-]*" "" line "${line}")
    if(NOT line MATCHES "uses: \\./"
       AND NOT line MATCHES "uses: [A-Za-z0-9_.-]+/[A-Za-z0-9_./-]+@${sha} +# v[0-9]")
      list(APPEND bad "action_pins: ${base}: ${line}")
    endif()
  endforeach()
endforeach()
list(LENGTH bad count)
if(count GREATER 0)
  list(JOIN bad "\n" bad)
  message(FATAL_ERROR "${bad}\naction_pins: ${count} uses: lines not pinned to a commit with a # v comment")
endif()
message(STATUS "action_pins: ${seen} uses: lines, each pinned to a commit")
