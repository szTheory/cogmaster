# Document size budgets, run as
#   cmake -DROOT=<source dir> -P budgets.cmake
# Fault prevented: the README, the public header or a page in docs/ growing
# past its budget, as manuals and design prose drift into them. Each file
# must be strictly under its budget. Prints every file's size and budget, and
# fails naming each file at or over budget, in bytes.
if(NOT IS_DIRECTORY "${ROOT}")
  message(FATAL_ERROR "budgets: -DROOT=<source dir> is required")
endif()

set(checks "README.md=12288" "include/cogmaster.h=24576")
file(GLOB docs RELATIVE "${ROOT}" "${ROOT}/docs/*.md")
foreach(doc IN LISTS docs)
  list(APPEND checks "${doc}=40960")
endforeach()

set(over "")
foreach(check IN LISTS checks)
  string(REGEX MATCH "^(.*)=([0-9]+)$" unused "${check}")
  set(file "${CMAKE_MATCH_1}")
  set(limit "${CMAKE_MATCH_2}")
  file(SIZE "${ROOT}/${file}" size)
  message(STATUS "budgets: ${file}: ${size} bytes, budget ${limit}")
  if(NOT size LESS limit)
    string(APPEND over "\n  ${file}: ${size} bytes, budget ${limit}")
  endif()
endforeach()
if(over)
  message(FATAL_ERROR "budgets: over budget:${over}")
endif()
list(LENGTH checks count)
message(STATUS "budgets: ${count} files under budget")
