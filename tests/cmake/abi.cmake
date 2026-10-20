# Link-surface checks on a static library, run as
#   cmake -DMODE=<undefined|globals|symbols> -DLIB=<library> -DNM=<nm>
#         -DSYSTEM=<system>/<processor> [-DHEADER=<cogmaster.h>] -P abi.cmake
# Faults prevented, one per mode:
#   undefined  the core calling libc, libm or a floating-point helper. Every
#              undefined symbol that no member defines must be one of the C
#              memory functions or a compiler helper listed for SYSTEM below.
#   globals    mutable state outside the instance: no writable data symbol
#              (nm types B b D d C), so const integer tables only.
#   symbols    a public name the header does not declare, or a declared
#              function nothing defines. Defined external cogmaster_* names
#              equal HEADER's functions both ways; every other defined
#              external name starts cogmaster__.
cmake_minimum_required(VERSION 3.25)
foreach(var MODE LIB NM SYSTEM)
  if("${${var}}" STREQUAL "")
    message(FATAL_ERROR "abi: -D${var} is required")
  endif()
endforeach()
if(NOT EXISTS "${LIB}" OR IS_DIRECTORY "${LIB}")
  message(FATAL_ERROR "abi: library ${LIB} does not exist")
endif()

# Per-target compiler helpers, "system regex:names". Nothing else is allowed:
# the core builds with -fno-stack-protector -U_FORTIFY_SOURCE, so no
# __stack_chk_* or __*_chk name ever appears.
set(helpers
  # Apple clang lowers a zeroing memset to bzero (measured: nm -u on the core,
  # Apple clang 21, -O3).
  "^Darwin/:bzero __bzero"
  # x86-64 ELF position-independent code names the linker-defined GOT symbol
  # (System V x86-64 psABI, section 3.5).
  "^Linux/(x86_64|amd64|AMD64)$:_GLOBAL_OFFSET_TABLE_"
  # 32-bit PowerPC has no 64-bit divide or shift instruction; GCC calls the
  # libgcc integer routines (GCC internals manual, "Integer library routines").
  "^Linux/(ppc|powerpc)$:__udivdi3 __umoddi3 __ashldi3 __lshrdi3"
  # MSVC x64's auto-vectorizer reads the C runtime's CPU level (measured: llvm-nm
  # on the core, ci-msvc on windows-2025).
  "^Windows/(AMD64|x86_64)$:__isa_available")
# Names a compiler gives its own constants, "system regex:name regex". They
# are left out of the globals and symbols checks. MSVC puts each string
# literal in a read-only COMDAT named ??_C@_ and each 128-bit vector constant
# in one named __xmm@, and on x64 emits __isa_available_default beside the
# vectorizer's CPU check; BoringSSL's util/audit_symbols.go allows the same
# three for MSVC builds.
set(generated
  "^Windows/:^(\\?\\?_C@_|__xmm@)"
  "^Windows/(AMD64|x86_64)$:^__isa_available_default$")
set(allowed memcpy memmove memset memcmp)
foreach(row IN LISTS helpers)
  string(REGEX MATCH "^([^:]*):(.*)$" unused "${row}")
  set(names "${CMAKE_MATCH_2}")
  if(SYSTEM MATCHES "${CMAKE_MATCH_1}")
    separate_arguments(names UNIX_COMMAND "${names}")
    list(APPEND allowed ${names})
  endif()
endforeach()

set(generated_regex "^$")
foreach(row IN LISTS generated)
  string(REGEX MATCH "^([^:]*):(.*)$" unused "${row}")
  set(names "${CMAKE_MATCH_2}")
  if(SYSTEM MATCHES "${CMAKE_MATCH_1}")
    string(APPEND generated_regex "|${names}")
  endif()
endforeach()

# nm -P prints "name type [value size]" per symbol and a header per member.
execute_process(COMMAND "${NM}" -P "${LIB}" RESULT_VARIABLE rc
  OUTPUT_VARIABLE text ERROR_VARIABLE err)
if(NOT rc EQUAL 0)
  message(FATAL_ERROR "abi: ${NM} -P ${LIB} failed (${rc}): ${err}")
endif()
string(REPLACE "\n" ";" lines "${text}")
foreach(line IN LISTS lines)
  if(NOT line MATCHES "^([^ ]+) ([A-Za-z])( |$)")
    continue()
  endif()
  set(name "${CMAKE_MATCH_1}")
  set(type "${CMAKE_MATCH_2}")
  if(SYSTEM MATCHES "^Darwin/")
    string(REGEX REPLACE "^_" "" name "${name}") # Mach-O prefixes C names with _
  endif()
  if(type STREQUAL "U")
    list(APPEND undefined "${name}")
    continue()
  endif()
  list(APPEND defined "${name}")
  if(name MATCHES "${generated_regex}")
    continue()
  endif()
  if(type MATCHES "^[BbDdC]$")
    list(APPEND writable "${type} ${name}")
  endif()
  if(type MATCHES "^[A-Z]$")
    list(APPEND external "${name}")
  endif()
endforeach()

set(bad "")
if(MODE STREQUAL "undefined")
  list(REMOVE_ITEM undefined ${defined} ${allowed})
  set(bad ${undefined})
  set(what "unexpected undefined symbol")
elseif(MODE STREQUAL "globals")
  set(bad ${writable})
  set(what "writable data symbol")
elseif(MODE STREQUAL "symbols")
  file(READ "${HEADER}" header)
  string(REGEX MATCHALL "cogmaster_[a-z0-9_]*\\(" declared "${header}")
  string(REPLACE "(" "" declared "${declared}")
  set(public ${external})
  list(FILTER public INCLUDE REGEX "^cogmaster_[^_]")
  list(FILTER external EXCLUDE REGEX "^cogmaster_")
  set(missing ${declared})
  list(REMOVE_ITEM missing ${public})
  list(REMOVE_ITEM public ${declared})
  list(TRANSFORM missing PREPEND "declared, not defined: ")
  list(TRANSFORM public PREPEND "defined, not declared: ")
  list(TRANSFORM external PREPEND "outside the cogmaster_ namespace: ")
  set(bad ${missing} ${public} ${external})
  set(what "public name mismatch")
else()
  message(FATAL_ERROR "abi: unknown -DMODE=${MODE}")
endif()
list(REMOVE_DUPLICATES bad)
if(bad)
  list(JOIN bad "\n  " bad)
  message(FATAL_ERROR "abi ${MODE}: ${LIB}: ${what}:\n  ${bad}")
endif()
message(STATUS "abi ${MODE}: ${LIB} ok")
