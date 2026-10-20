# Public names against the embedding guide, run as
#   cmake -DHEADER=<include/cogmaster.h> -DDOC=<docs/embedding.md>
#         [-DOTHER=<file;...>] -P api_names.cmake
# Fault prevented: a public name that ships with no word about it in the
# guide, or a guide that names something the header no longer declares. Every
# cogmaster_ and COGMASTER_ name in the header must appear in the guide, and every
# one the guide writes must be declared; one ending in _ (COGMASTER_STATUS_*)
# must begin a declared name. A header name ending in _ is a comment's
# wildcard, not a name. OTHER lists files whose names the guide may also cite
# without the header declaring them: the generated version header's template
# and the build's options.
foreach(var HEADER DOC)
  if(NOT DEFINED ${var} OR NOT EXISTS "${${var}}")
    message(FATAL_ERROR "api_names: -D${var} must name an existing file")
  endif()
endforeach()

set(name_regex "cogmaster_[a-z0-9_]+|COGMASTER_[A-Z0-9_]+")
file(READ "${HEADER}" header)
file(READ "${DOC}" doc)
string(REGEX MATCHALL "${name_regex}" declared "${header}")
string(REGEX MATCHALL "${name_regex}" written "${doc}")
list(REMOVE_DUPLICATES declared)
list(REMOVE_DUPLICATES written)
# The include guard is not part of the interface.
list(REMOVE_ITEM declared COGMASTER_H)
list(FILTER declared EXCLUDE REGEX "_$")
set(cited "")
foreach(other IN LISTS OTHER)
  file(READ "${other}" other_text)
  string(REGEX MATCHALL "${name_regex}" other_names "${other_text}")
  list(APPEND cited ${other_names})
endforeach()

set(problems "")
foreach(name IN LISTS declared)
  if(NOT name IN_LIST written)
    string(APPEND problems "\n  ${name} is declared in the header but the guide never names it")
  endif()
endforeach()
foreach(name IN LISTS written)
  set(known FALSE)
  if(name IN_LIST declared OR name IN_LIST cited)
    set(known TRUE)
  elseif(name MATCHES "_$")
    foreach(candidate IN LISTS declared)
      string(FIND "${candidate}" "${name}" at)
      if(at EQUAL 0)
        set(known TRUE)
      endif()
    endforeach()
  endif()
  if(NOT known)
    string(APPEND problems "\n  ${name} is named in the guide but the header does not declare it")
  endif()
endforeach()
if(problems)
  message(FATAL_ERROR "api_names: ${HEADER} against ${DOC}:${problems}")
endif()
list(LENGTH declared count)
message(STATUS "api_names: all ${count} public names appear in ${DOC}, and it names nothing undeclared")
