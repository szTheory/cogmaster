# Roll-up check, run as
#   cmake -DCI_YML=<ci.yml> -DJQ=<jq> [-DREPORT_ONLY=<jobs>] -P ci_required.cmake
# Fault prevented: a CI required roll-up that passes a failed or wrongly
# skipped job, or a job missing from its needs. It runs the jq program of the
# CI required step, with the step's own arguments, on synthetic needs, and
# requires every job but the report-only ones among those needs.
cmake_minimum_required(VERSION 3.25)
foreach(var CI_YML JQ)
  if("${${var}}" STREQUAL "")
    message(FATAL_ERROR "ci_required: -D${var} is required")
  endif()
endforeach()
file(READ "${CI_YML}" yml)
string(FIND "${yml}" "\njobs:\n" at)
string(FIND "${yml}" "name: CI required" job_at)
if(at EQUAL -1 OR job_at EQUAL -1)
  message(FATAL_ERROR "ci_required: ${CI_YML} has no jobs or no CI required job")
endif()
string(SUBSTRING "${yml}" ${at} -1 jobs)
string(SUBSTRING "${yml}" ${job_at} -1 job)
if(NOT job MATCHES "needs: \\[([^]]*)\\]")
  message(FATAL_ERROR "ci_required: CI required has no needs list")
endif()
string(REPLACE " " "" needs "${CMAKE_MATCH_1}")
string(REPLACE "," ";" needs "${needs}")
if(NOT job MATCHES "jq -e ([^']*)'([^']*)'")
  message(FATAL_ERROR "ci_required: CI required runs no jq -e program")
endif()
set(program "${CMAKE_MATCH_2}")
string(REGEX MATCHALL "--arg [a-z]+" args "${CMAKE_MATCH_1}")
if(NOT args STREQUAL "--arg event;--arg heavy;--arg called")
  message(FATAL_ERROR "ci_required: the step passes '${args}', not event, heavy and called")
endif()

set(bad "")
string(REGEX MATCHALL "\n  [a-z0-9-]+:\n" names "${jobs}")
foreach(name IN LISTS names)
  string(STRIP "${name}" name)
  string(REPLACE ":" "" name "${name}")
  if(NOT name STREQUAL "ci-required" AND NOT name IN_LIST needs AND NOT name IN_LIST REPORT_ONLY)
    list(APPEND bad "job ${name} is not in CI required's needs")
  endif()
endforeach()

# name|event|heavy|called|job=result,...|want. Unlisted jobs succeeded. The
# synthetic needs hold every job in CI required's needs; a light pull request
# skips every one but changes, hygiene and title.
set(all ${needs})
set(light "")
foreach(j IN LISTS all)
  if(NOT j MATCHES "^(changes|hygiene|title)$")
    string(APPEND light ",${j}=skipped")
  endif()
endforeach()
string(SUBSTRING "${light}" 1 -1 light)
set(cases
  "all succeeded|pull_request|true|false||pass"
  "light pull request|pull_request|false|false|${light}|pass"
  "build skipped when heavy=true|pull_request|true|false|build=skipped|fail"
  "build skipped when changes failed|pull_request||false|changes=failure,${light}|fail"
  "hygiene skipped when heavy=false|pull_request|false|false|hygiene=skipped|fail"
  "title skipped on push|push|true|false|title=skipped|pass"
  "title skipped on a pull request|pull_request|true|false|title=skipped|fail"
  "perf skipped on push|push|true|false|perf=skipped|pass"
  "perf skipped in a nested call|pull_request|true|true|perf=skipped|pass"
  "perf skipped on a pull request|pull_request|true|false|perf=skipped|fail"
  "asan failed|pull_request|true|false|asan=failure|fail"
  "build cancelled|push|true|false|build=cancelled|fail")
foreach(case IN LISTS cases)
  string(REPLACE "|" ";" f "${case}")
  list(GET f 0 name)
  list(GET f 1 event)
  list(GET f 2 heavy)
  list(GET f 3 called)
  list(GET f 4 results)
  list(GET f 5 want)
  set(json "")
  foreach(j IN LISTS all)
    set(result success)
    if(",${results}," MATCHES ",${j}=([a-z]+),")
      set(result ${CMAKE_MATCH_1})
    endif()
    string(APPEND json ",\"${j}\":{\"result\":\"${result}\"}")
  endforeach()
  string(SUBSTRING "${json}" 1 -1 json)
  execute_process(COMMAND ${JQ} -n -e --arg event "${event}" --arg heavy "${heavy}"
      --arg called "${called}" --argjson needs "{${json}}" "$needs | ${program}"
    RESULT_VARIABLE rc OUTPUT_QUIET ERROR_VARIABLE err)
  if(NOT (want STREQUAL "pass" AND rc STREQUAL "0") AND NOT (want STREQUAL "fail" AND rc STREQUAL "1"))
    list(APPEND bad "${name}: want ${want}, got exit ${rc} ${err}")
  endif()
endforeach()
list(LENGTH cases n)
if(bad)
  list(JOIN bad "\n  " bad)
  message(FATAL_ERROR "ci_required:\n  ${bad}")
endif()
message(STATUS "ci_required: ${n} cases")
