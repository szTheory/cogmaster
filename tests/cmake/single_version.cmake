# Single version source check, run as
#   cmake [-DROOT=<repo>] [-DVERSION=<x.y.z>] [-DNEXT=ON] -P single_version.cmake
# Fault prevented: a version string edited by hand in a second place, which
# release-please, editing only version.txt, would leave stale. Fails naming
# every tracked file in scope that holds the version with no digit or dot on
# either side. NEXT=ON checks the next minor and next patch instead, the
# versions a release pull request writes. Out of scope: version.txt, the
# release-please manifest, configuration (its initial-version is an input)
# and changelog, and .planning/, whose records cite versions by design.
if(NOT DEFINED ROOT)
  get_filename_component(ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
endif()
if(NOT DEFINED VERSION)
  file(STRINGS "${ROOT}/version.txt" VERSION LIMIT_COUNT 1)
endif()
if(NOT VERSION MATCHES "^([0-9]+)\\.([0-9]+)\\.([0-9]+)$")
  message(FATAL_ERROR "single_version: '${VERSION}' is not MAJOR.MINOR.PATCH")
endif()
set(versions ${VERSION})
if(NEXT)
  math(EXPR minor "${CMAKE_MATCH_2} + 1")
  math(EXPR patch "${CMAKE_MATCH_3} + 1")
  set(versions "${CMAKE_MATCH_1}.${minor}.0" "${CMAKE_MATCH_1}.${CMAKE_MATCH_2}.${patch}")
endif()

list(JOIN versions "|" alternatives)
string(REPLACE "." "\\." alternatives "${alternatives}")
set(bounded "(^|[^0-9.])(${alternatives})([^0-9.]|$)")
set(skip "^(version\\.txt|\\.release-please-manifest\\.json|release-please-config\\.json|CHANGELOG\\.md|\\.planning/.*)$")
find_program(GIT git REQUIRED)
execute_process(COMMAND ${GIT} -C ${ROOT} -c core.quotePath=false ls-files
  OUTPUT_VARIABLE listing COMMAND_ERROR_IS_FATAL ANY)
string(REPLACE "\n" ";" files "${listing}")
set(bad "")
foreach(file IN LISTS files)
  if(file STREQUAL "" OR file MATCHES "${skip}" OR IS_DIRECTORY "${ROOT}/${file}"
      OR NOT EXISTS "${ROOT}/${file}")
    continue()
  endif()
  file(STRINGS "${ROOT}/${file}" hits REGEX "${bounded}")
  foreach(hit IN LISTS hits)
    string(REGEX MATCH "${bounded}" unused "${hit}")
    string(APPEND bad "\n  ${file} holds ${CMAKE_MATCH_2}")
  endforeach()
endforeach()
if(bad)
  message(FATAL_ERROR "single_version: a version is written outside version.txt:${bad}")
endif()
message(STATUS "single_version: ${versions} appear only in version.txt")
