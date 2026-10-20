# Document size budgets, run as
#   cmake -DROOT=<source dir> -P budgets.cmake
# Fault prevented: the README, the public header or a page in docs/ growing
# past its budget, as manuals and design prose drift into them. Each file
# must be strictly under its budget; tests/fuzz is checked as a whole below. Prints every file's size and budget, and
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
# Fault prevented: tests/fuzz turning into a corpus dump. Only minimised
# regressions are committed there, so the directory stays at most 512 KiB.
file(GLOB_RECURSE fuzz_files "${ROOT}/tests/fuzz/*")
set(fuzz_size 0)
foreach(file IN LISTS fuzz_files)
  file(SIZE "${file}" size)
  math(EXPR fuzz_size "${fuzz_size} + ${size}")
endforeach()
message(STATUS "budgets: tests/fuzz: ${fuzz_size} bytes, budget 524288 at most")
if(fuzz_size GREATER 524288)
  string(APPEND over "\n  tests/fuzz: ${fuzz_size} bytes, budget 524288 at most")
endif()

if(over)
  message(FATAL_ERROR "budgets: over budget:${over}")
endif()
list(LENGTH checks count)
message(STATUS "budgets: ${count} files under budget")
