# libretro .info claims check, run as
#   cmake -DINFO=<file.info> -DINVENTORY=<inventory.txt> -P info_claims.cmake
# Fault prevented: the .info promises a capability no test exercises.
# RetroArch turns features on from these keys, so every key set to "true"
# must map to a test, and that test must be in the inventory.
foreach(var INFO INVENTORY)
  if(NOT DEFINED ${var} OR NOT EXISTS "${${var}}")
    message(FATAL_ERROR "info_claims: -D${var} must name an existing file")
  endif()
endforeach()

# Claim to the test that proves it. is_experimental is a warning label, not
# a capability, so it needs no test.
set(claim_supports_no_game libretro.host.xrgb8888)
set(exempt is_experimental)

file(STRINGS "${INVENTORY}" inventory)
file(STRINGS "${INFO}" lines REGEX "^[a-z_0-9]+ = \"true\"$")
foreach(line IN LISTS lines)
  string(REGEX REPLACE " = .*" "" key "${line}")
  if(key IN_LIST exempt)
    continue()
  endif()
  if(NOT DEFINED claim_${key})
    message(FATAL_ERROR "info_claims: ${key} = \"true\" maps to no test")
  endif()
  string(REPLACE "." "\\." name "${claim_${key}}")
  set(found ${inventory})
  list(FILTER found INCLUDE REGEX "^${name}\t")
  if(NOT found)
    message(FATAL_ERROR "info_claims: ${key} needs ${claim_${key}}, which is not in ${INVENTORY}")
  endif()
endforeach()
list(LENGTH lines count)
message(STATUS "info_claims: ${count} \"true\" line(s), each backed by a test or exempt")
