# Binary checks on the core library: its undefined symbols, its writable data
# and its public names, each run by the shared repository checker on the built
# archive. The two fixtures are negative controls built with the core's own
# flags, so each check fails on every lane where it runs.
set(abi_scripts ${CMAKE_CURRENT_LIST_DIR}/../cmake)
# MSVC sets no CMAKE_NM; llvm-nm reads its COFF archives.
set(abi_nm ${CMAKE_NM})
if(NOT abi_nm)
  find_program(COGMASTER_NM NAMES llvm-nm nm REQUIRED)
  set(abi_nm ${COGMASTER_NM})
endif()
set(abi_args -DNM=${abi_nm} "-DSYSTEM=${CMAKE_SYSTEM_NAME}/${CMAKE_SYSTEM_PROCESSOR}")

get_target_property(abi_options cogmaster COMPILE_OPTIONS)
foreach(fixture IN ITEMS libc global)
  add_library(cogmaster-abi-${fixture} STATIC ${CMAKE_CURRENT_LIST_DIR}/${fixture}_fixture.c)
  target_compile_options(cogmaster-abi-${fixture} PRIVATE ${abi_options})
  set_target_properties(cogmaster-abi-${fixture} PROPERTIES
    C_VISIBILITY_PRESET hidden
    POSITION_INDEPENDENT_CODE ON
    ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests/abi)
  cogmaster_warnings(cogmaster-abi-${fixture})
endforeach()

foreach(mode IN ITEMS undefined globals symbols)
  cogmaster_add_test(NAME abi.${mode}
    COMMAND ${CMAKE_COMMAND} -DMODE=${mode} -DLIB=$<TARGET_FILE:cogmaster> ${abi_args}
      -DHEADER=${PROJECT_SOURCE_DIR}/include/cogmaster.h -DCHECK=abi -P ${abi_scripts}/repository.cmake
    LABELS abi TIMEOUT 30)
endforeach()

# CMake wraps a long message at a column that depends on the build path, so
# the regexes allow a line break anywhere in the finding's title.
foreach(case IN ITEMS "undefined;libc;unexpected[^:]*symbol:.*abort"
                      "globals;global;writable[^:]*symbol:.*counter")
  list(GET case 0 mode)
  list(GET case 1 fixture)
  list(GET case 2 regex)
  set(abi_cmd ${CMAKE_COMMAND} -DMODE=${mode} -DLIB=$<TARGET_FILE:cogmaster-abi-${fixture}>
    ${abi_args} -DCHECK=abi -P ${abi_scripts}/repository.cmake)
  string(REPLACE ";" "$<SEMICOLON>" abi_cmd "${abi_cmd}")
  cogmaster_add_test(NAME abi.${mode}.self_test
    COMMAND ${CMAKE_COMMAND} "-DCMD=${abi_cmd}" -DCODE=1 "-DREGEX=${regex}"
      -P ${abi_scripts}/expect_fail.cmake
    LABELS abi TIMEOUT 30)
endforeach()

# Every header name is in a versioned API test, so a declaration added after
# a release needs tests/api/v2.c and with it COGMASTER_API_VERSION 2. The self
# test reads a copy of the header with one extra function and no bump, and
# expects that function named.
cogmaster_add_test(NAME abi.api_version
  COMMAND ${CMAKE_COMMAND} -DHEADER=${PROJECT_SOURCE_DIR}/include/cogmaster.h
    -DAPI_DIR=${PROJECT_SOURCE_DIR}/tests/api -DCHECK=api_version -P ${abi_scripts}/repository.cmake
  LABELS abi TIMEOUT 10)
file(READ ${PROJECT_SOURCE_DIR}/include/cogmaster.h abi_header)
string(REPLACE "void cogmaster_destroy(cogmaster *m);"
  "void cogmaster_destroy(cogmaster *m);\ncogmaster_status cogmaster_added(cogmaster *m);"
  abi_header "${abi_header}")
file(WRITE ${PROJECT_BINARY_DIR}/tests/abi/added_function.h "${abi_header}")
set(abi_cmd ${CMAKE_COMMAND} -DHEADER=${PROJECT_BINARY_DIR}/tests/abi/added_function.h
  -DAPI_DIR=${PROJECT_SOURCE_DIR}/tests/api -DCHECK=api_version -P ${abi_scripts}/repository.cmake)
string(REPLACE ";" "$<SEMICOLON>" abi_cmd "${abi_cmd}")
cogmaster_add_test(NAME abi.api_version.self_test
  COMMAND ${CMAKE_COMMAND} "-DCMD=${abi_cmd}" -DCODE=1
    "-DREGEX=in no tests/api/vK.c with K <= [0-9]+: cogmaster_added" -P ${abi_scripts}/expect_fail.cmake
  LABELS abi TIMEOUT 10)

# The frozen API v1 program: every function, field and constant of version 1.
add_executable(cogmaster-test-api-v1 ${PROJECT_SOURCE_DIR}/tests/api/v1.c)
target_link_libraries(cogmaster-test-api-v1 PRIVATE cogmaster::cogmaster)
set_target_properties(cogmaster-test-api-v1 PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
cogmaster_warnings(cogmaster-test-api-v1)
cogmaster_add_test(NAME abi.api_v1 COMMAND cogmaster-test-api-v1
  LABELS abi portable TIMEOUT 10)

# The API v2 program: every name version 2 adds.
add_executable(cogmaster-test-api-v2 ${PROJECT_SOURCE_DIR}/tests/api/v2.c)
target_link_libraries(cogmaster-test-api-v2 PRIVATE cogmaster::cogmaster)
set_target_properties(cogmaster-test-api-v2 PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
cogmaster_warnings(cogmaster-test-api-v2)
# Timeout: 10 s floor; 2 x 0.00 s, slowest of 2 runs, run 39376644025, provisional.
cogmaster_add_test(NAME abi.api_v2 COMMAND cogmaster-test-api-v2
  LABELS abi portable TIMEOUT 10)

# The public header compiles alone as C17 and as C++17 with warnings fatal.
add_executable(cogmaster-test-api-v3 ${PROJECT_SOURCE_DIR}/tests/api/v3.c)
target_link_libraries(cogmaster-test-api-v3 PRIVATE cogmaster::cogmaster)
cogmaster_warnings(cogmaster-test-api-v3)
# Fault prevented: absolute input or the appended result breaking legacy ABI calls.
cogmaster_add_test(NAME abi.api_v3 COMMAND cogmaster-test-api-v3
  LABELS abi portable TIMEOUT 10)

# Fault prevented: appending SMS1 or VDP diagnostics renumbering existing values.
add_executable(cogmaster-test-api-v4 ${PROJECT_SOURCE_DIR}/tests/api/v4.c)
target_link_libraries(cogmaster-test-api-v4 PRIVATE cogmaster::cogmaster)
cogmaster_warnings(cogmaster-test-api-v4)
cogmaster_add_test(NAME abi.api_v4 COMMAND cogmaster-test-api-v4
  LABELS abi portable TIMEOUT 10)

# Fault prevented: a header that only the core's own build, with its own
# flags and language, can compile. C++ is enabled here only; an embedding
# project never meets it. The targets are built only by these tests, which
# share the build tree and so take turns.
enable_language(CXX)
add_library(cogmaster_header_c17 OBJECT EXCLUDE_FROM_ALL
  ${CMAKE_CURRENT_LIST_DIR}/../header/header.c)
add_library(cogmaster_header_cxx17 OBJECT EXCLUDE_FROM_ALL
  ${CMAKE_CURRENT_LIST_DIR}/../header/header.cpp)
set_target_properties(cogmaster_header_cxx17 PROPERTIES
  CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
cogmaster_warnings(cogmaster_header_c17)
foreach(abi_target IN ITEMS cogmaster_header_c17 cogmaster_header_cxx17)
  target_include_directories(${abi_target} PRIVATE
    ${PROJECT_SOURCE_DIR}/include ${PROJECT_BINARY_DIR}/generated)
  target_compile_options(${abi_target} PRIVATE
    $<$<COMPILE_LANG_AND_ID:C,GNU,Clang,AppleClang>:-Wall -Wextra -Wpedantic -Werror>
    $<$<COMPILE_LANG_AND_ID:CXX,GNU,Clang,AppleClang>:-Wall -Wextra -Wpedantic -Werror>
    $<$<COMPILE_LANG_AND_ID:C,MSVC>:/W4 /WX /TC>
    $<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/W4 /WX /TP>)
endforeach()
foreach(abi_lang IN ITEMS c17 cxx17)
  cogmaster_add_test(NAME abi.header.${abi_lang}
    COMMAND ${CMAKE_COMMAND} --build ${PROJECT_BINARY_DIR}
      --target cogmaster_header_${abi_lang} --config $<CONFIG>
    LABELS abi TIMEOUT 120)
  set_tests_properties(abi.header.${abi_lang} PROPERTIES RESOURCE_LOCK build_tree)
endforeach()

# On the nofp lane, a fixture holding one double division must fail to build
# under -mgeneral-regs-only with GCC's own diagnostic (AArch64 names the
# option; x86-64 refuses the SSE register a double travels in). Fault
# prevented: a flag that stopped rejecting floating point, so the lane passes
# on a core that uses it. The fixture is built only inside this test.
if(COGMASTER_NOFP)
  set(abi_cmd ${CMAKE_C_COMPILER} -std=c17 -mgeneral-regs-only
    -c ${CMAKE_CURRENT_LIST_DIR}/nofp_fixture.c -o ${PROJECT_BINARY_DIR}/tests/abi/nofp_fixture.o)
  string(REPLACE ";" "$<SEMICOLON>" abi_cmd "${abi_cmd}")
  cogmaster_add_test(NAME abi.nofp_fixture
    COMMAND ${CMAKE_COMMAND} "-DCMD=${abi_cmd}" -DCODE=1
      "-DREGEX=general-regs-only[^ ]* is incompatible with the use of floating-point.types|SSE register (return|argument) with SSE disabled"
      -P ${abi_scripts}/expect_fail.cmake
    LABELS abi TIMEOUT 30)
endif()
