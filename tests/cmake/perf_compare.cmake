# Instructions-per-frame gate, run as
#   cmake -DROM=<name> -DHEAD=<n> -DBASE1=<n> -DBASE2=<n> -DALLOW=<allow file>
#     -DMODE=<gate|report> -DTODAY=<YYYY-MM-DD> [-DNO_BASE=true] -P perf_compare.cmake
# Fault prevented: a pull request making the core slower unnoticed. HEAD is
# the Cachegrind Ir/frame of the merge commit, (Ir at 360 frames - Ir at 60) /
# 300, and BASE1 and BASE2 two measurements of its first parent. Prints one
# line,
#   perf <rom> head=<n> base=<n> delta_bp=<n> verdict=<PASS|FAIL|UNSTABLE>
# or, with NO_BASE true (the first parent has no tree to build),
#   perf <rom> head=<n> base=none delta_bp=none verdict=NO_BASE
# The limit is +100 basis points, exactly +1.00 % passing; a dated line in the
# allow file raises it for one ROM until its expiry. PASS exits 0; FAIL,
# UNSTABLE and NO_BASE exit 1 only when MODE is gate, and MODE report prints
# the verdict and exits 0. An input error prints "perf: error <reason>" and
# exits 1 in either mode: a missing measurement is never a pass. Integer
# arithmetic only.
cmake_minimum_required(VERSION 3.25)

function(perf_error reason)
  message(FATAL_ERROR "perf: error ${reason}")
endfunction()

# Days since 1970-01-01 of a YYYY-MM-DD date (the civil-from-days inverse).
function(perf_days out date)
  if(NOT date MATCHES "^([0-9][0-9][0-9][0-9])-([0-9][0-9])-([0-9][0-9])$")
    perf_error("date '${date}' is not YYYY-MM-DD")
  endif()
  set(y ${CMAKE_MATCH_1})
  set(m ${CMAKE_MATCH_2})
  set(d ${CMAKE_MATCH_3})
  # A leading zero would read as octal.
  string(REGEX REPLACE "^0" "" m "${m}")
  string(REGEX REPLACE "^0" "" d "${d}")
  if(m LESS 1 OR m GREATER 12 OR d LESS 1 OR d GREATER 31)
    perf_error("date '${date}' is out of range")
  endif()
  if(m LESS_EQUAL 2)
    math(EXPR y "${y} - 1")
    math(EXPR m "${m} + 9")
  else()
    math(EXPR m "${m} - 3")
  endif()
  math(EXPR era "${y} / 400")
  math(EXPR yoe "${y} - ${era} * 400")
  math(EXPR doy "(153 * ${m} + 2) / 5 + ${d} - 1")
  math(EXPR days "${era} * 146097 + ${yoe} * 365 + ${yoe} / 4 - ${yoe} / 100 + ${doy} - 719468")
  set(${out} ${days} PARENT_SCOPE)
endfunction()

# A measurement is a decimal integer without a leading zero; 0 is no
# measurement either.
function(perf_number name)
  if(NOT "${${name}}" MATCHES "^[1-9][0-9]*$")
    perf_error("${name} '${${name}}' is not a measurement")
  endif()
endfunction()

if(NOT ROM MATCHES "^[A-Za-z0-9_.-]+$")
  perf_error("ROM '${ROM}' is not a ROM name")
endif()
if(NOT MODE MATCHES "^(gate|report)$")
  perf_error("MODE '${MODE}' is not gate or report")
endif()
perf_number(HEAD)
if(NO_BASE STREQUAL "true")
  if(NOT "${BASE1}${BASE2}" STREQUAL "")
    perf_error("NO_BASE is true but a base was measured")
  endif()
else()
  perf_number(BASE1)
  perf_number(BASE2)
endif()
if(NOT EXISTS "${ALLOW}")
  perf_error("allow file '${ALLOW}' does not exist")
endif()
perf_days(today "${TODAY}")

# Allow lines: <rom> <max % x 100> <expiry YYYY-MM-DD> <reason>. The reason is
# never printed, so a semicolon or bracket in it is replaced before the text is
# split into lines.
set(limit 100)
file(READ "${ALLOW}" text)
string(REGEX REPLACE "[][;\r]" "_" text "${text}")
string(REPLACE "\n" ";" lines "${text}")
set(n 0)
foreach(line IN LISTS lines)
  math(EXPR n "${n} + 1")
  if(line MATCHES "^[ \t_]*$" OR line MATCHES "^#")
    continue()
  endif()
  if(NOT line MATCHES "^([A-Za-z0-9_.-]+) (0|[1-9][0-9]*) ([0-9-]+) [^ ]")
    perf_error("malformed allow line ${n} of ${ALLOW}")
  endif()
  set(rom ${CMAKE_MATCH_1})
  set(cap ${CMAKE_MATCH_2})
  perf_days(expiry "${CMAKE_MATCH_3}")
  math(EXPR ahead "${expiry} - ${today}")
  # Applies from today through its expiry, at most 60 days away.
  if(rom STREQUAL ROM AND ahead GREATER_EQUAL 0 AND ahead LESS_EQUAL 60 AND cap GREATER limit)
    set(limit ${cap})
  endif()
endforeach()

if(NO_BASE STREQUAL "true")
  set(verdict NO_BASE)
  set(line "perf ${ROM} head=${HEAD} base=none delta_bp=none verdict=NO_BASE")
else()
  math(EXPR delta "${HEAD} - ${BASE1}")
  # Truncated toward zero for the printed value; the decision below is exact.
  math(EXPR delta_bp "${delta} * 10000 / ${BASE1}")
  math(EXPR scaled "${delta} * 10000")
  math(EXPR allowed "${limit} * ${BASE1}")
  if(NOT BASE1 EQUAL BASE2)
    set(verdict UNSTABLE)
  elseif(scaled GREATER allowed)
    set(verdict FAIL)
  else()
    set(verdict PASS)
  endif()
  set(line "perf ${ROM} head=${HEAD} base=${BASE1} delta_bp=${delta_bp} verdict=${verdict}")
endif()

message(NOTICE "${line}")
if(NOT verdict STREQUAL "PASS" AND MODE STREQUAL "gate")
  message(FATAL_ERROR "perf: verdict ${verdict} in gate mode")
endif()
