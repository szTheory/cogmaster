# Test registration and the exact test inventory.
#
# Fault prevented: a test that is silently not registered, or registered on
# one lane only, lets CI pass without running it. Configure fails unless the
# tests this build registers, with their labels, equal the lines of
# tests/inventory/<COGMASTER_INVENTORY>.txt.
#
# Inventory format: one test per line, "name<TAB>labels", labels sorted and
# separated by commas, lines sorted; lines starting with # are comments.

# cogmaster_add_test(NAME n COMMAND cmd... LABELS l... TIMEOUT seconds [COST c])
# TIMEOUT is multiplied by COGMASTER_TIMEOUT_SCALE, which slow lanes raise.
function(cogmaster_add_test)
  cmake_parse_arguments(PARSE_ARGV 0 T "" "NAME;TIMEOUT;COST" "COMMAND;LABELS")
  if(NOT T_NAME OR NOT T_COMMAND OR NOT T_LABELS OR NOT T_TIMEOUT)
    message(FATAL_ERROR "cogmaster_add_test: NAME, COMMAND, LABELS and TIMEOUT are required")
  endif()
  if(NOT DEFINED T_COST)
    set(T_COST 1)
  endif()
  math(EXPR timeout "${T_TIMEOUT} * ${COGMASTER_TIMEOUT_SCALE}")
  add_test(NAME ${T_NAME} COMMAND ${T_COMMAND})
  set_tests_properties(${T_NAME} PROPERTIES LABELS "${T_LABELS}" TIMEOUT ${timeout} COST ${T_COST})
endfunction()

# Runs once the top-level directory has been processed, so every include()d
# subsystem file has registered its tests.
function(cogmaster__check_inventory)
  set(file "${CMAKE_SOURCE_DIR}/tests/inventory/${COGMASTER_INVENTORY}.txt")
  if(NOT EXISTS "${file}")
    message(FATAL_ERROR "test inventory ${file} does not exist")
  endif()

  get_property(names DIRECTORY "${CMAKE_SOURCE_DIR}" PROPERTY TESTS)
  set(registered "")
  foreach(name IN LISTS names)
    get_test_property(${name} LABELS labels)
    list(SORT labels)
    list(JOIN labels "," labels)
    list(APPEND registered "${name}\t${labels}")
  endforeach()

  file(STRINGS "${file}" lines)
  set(listed "")
  foreach(line IN LISTS lines)
    if(NOT line STREQUAL "" AND NOT line MATCHES "^#")
      list(APPEND listed "${line}")
    endif()
  endforeach()
  set(sorted ${listed})
  list(SORT sorted)
  if(NOT sorted STREQUAL listed)
    message(FATAL_ERROR "${file} is not sorted")
  endif()

  set(missing ${listed})
  list(REMOVE_ITEM missing ${registered})
  set(unexpected ${registered})
  list(REMOVE_ITEM unexpected ${listed})
  if(missing OR unexpected)
    list(JOIN missing "\n  " missing)
    list(JOIN unexpected "\n  " unexpected)
    message(FATAL_ERROR "registered tests differ from ${file}\n"
      "listed but not registered:\n  ${missing}\n"
      "registered but not listed:\n  ${unexpected}")
  endif()
endfunction()

cmake_language(DEFER DIRECTORY "${CMAKE_SOURCE_DIR}" CALL cogmaster__check_inventory)
