# Fault prevented: a derived file without a notice, or a notice for nothing
# (PROVENANCE.md section 5). Every SPDX line in a tracked C file is MIT, or
# MIT AND a licence PROVENANCE.md section 1 admits; each file with more than
# MIT is named on an "Applies to" line of THIRD-PARTY-NOTICES.md; every path
# those lines name exists; and the libretro .info licence field lists exactly
# the licences of the SPDX lines and the notices.
#
#   cmake -DROOT=<checkout> -P notices.cmake
cmake_minimum_required(VERSION 3.25)
find_program(GIT git REQUIRED)

file(READ "${ROOT}/THIRD-PARTY-NOTICES.md" notices)
string(REGEX MATCHALL "\\*\\*Applies to:\\*\\*[^\n]*" applies "${notices}")
string(REGEX MATCHALL "`[^`]+`" named "${applies}")
string(REGEX MATCHALL "\\*\\*Licence:\\*\\* [^\n]*" licences "${notices}")
list(TRANSFORM licences REPLACE "^\\*\\*Licence:\\*\\* " "")
set(errors "")
foreach(path IN LISTS named)
  string(REPLACE "`" "" path "${path}")
  if(NOT EXISTS "${ROOT}/${path}")
    string(APPEND errors "\n  THIRD-PARTY-NOTICES.md names ${path}, which does not exist")
  endif()
endforeach()

execute_process(COMMAND ${GIT} -C ${ROOT} -c core.quotePath=false ls-files -- *.c *.h
  OUTPUT_VARIABLE files OUTPUT_STRIP_TRAILING_WHITESPACE COMMAND_ERROR_IS_FATAL ANY)
string(REPLACE "\n" ";" files "${files}")
set(admitted "BSD-2-Clause|BSD-3-Clause|Zlib|BSL-1\\.0|ISC|CC0-1\\.0")
foreach(file IN LISTS files)
  file(STRINGS "${ROOT}/${file}" spdx REGEX "SPDX-License-Identifier:" LIMIT_COUNT 1)
  if(NOT spdx)
    continue()
  elseif(NOT spdx MATCHES "SPDX-License-Identifier: MIT( AND (${admitted}))?( \\*/)?$")
    string(APPEND errors "\n  ${file}: SPDX is not MIT or MIT AND a PROVENANCE.md licence")
  elseif(CMAKE_MATCH_2)
    list(APPEND licences ${CMAKE_MATCH_2})
    if(NOT "`${file}`" IN_LIST named)
      string(APPEND errors "\n  ${file} is ${CMAKE_MATCH_2} but has no notices section")
    endif()
  endif()
endforeach()

file(STRINGS "${ROOT}/libretro/cogmaster_libretro.info.in" info REGEX "^license = ")
string(REGEX REPLACE "^license = \"(.*)\"$" "\\1" info "${info}")
string(REPLACE ", " ";" info "${info}")
list(APPEND licences MIT)
foreach(var licences info)
  list(REMOVE_DUPLICATES ${var})
  list(SORT ${var})
endforeach()
if(NOT licences STREQUAL info)
  string(APPEND errors "\n  .info licence is '${info}', the sources and notices say '${licences}'")
endif()
if(errors)
  message(FATAL_ERROR "notices:${errors}")
endif()
message(STATUS "notices: ${licences}")
