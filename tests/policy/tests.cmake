# Repository policy tests: rules about the tests, the documents and the
# notices themselves.
set(policy_scripts ${CMAKE_CURRENT_LIST_DIR}/../cmake)

cogmaster_add_test(NAME policy.no_skip
  COMMAND ${CMAKE_COMMAND} -DROOT=${PROJECT_SOURCE_DIR} -DCHECK=no_skip -P ${policy_scripts}/repository.cmake
  LABELS policy TIMEOUT 10)

# The self test scans a tree holding one forbidden line and expects that line
# named. The property name is built in pieces so policy.no_skip passes here.
set(policy_rule SKIP_)
string(APPEND policy_rule RETURN_CODE)
set(policy_fixture ${PROJECT_BINARY_DIR}/policy/no_skip_fixture)
file(WRITE ${policy_fixture}/CMakeLists.txt
  "set_tests_properties(t PROPERTIES ${policy_rule} 77)\n")
set(policy_cmd ${CMAKE_COMMAND} -DROOT=${policy_fixture} -DCHECK=no_skip -P ${policy_scripts}/repository.cmake)
string(REPLACE ";" "$<SEMICOLON>" policy_cmd "${policy_cmd}")
cogmaster_add_test(NAME policy.no_skip.self_test
  COMMAND ${CMAKE_COMMAND} "-DCMD=${policy_cmd}" -DCODE=1
    "-DREGEX=CMakeLists.txt:1: ${policy_rule}" -P ${policy_scripts}/expect_fail.cmake
  LABELS policy TIMEOUT 10)

# One version source. The tree is checked at the current version and at the
# next minor and next patch, the versions a release pull request writes into
# version.txt; the self test is a fixture repository holding its version in
# a second tracked file and expects that file named.
cogmaster_add_test(NAME release.single_version_source
  COMMAND ${CMAKE_COMMAND} -DROOT=${PROJECT_SOURCE_DIR} -DCHECK=single_version -P ${policy_scripts}/repository.cmake
  LABELS policy TIMEOUT 30)
cogmaster_add_test(NAME release.single_version_source.next
  COMMAND ${CMAKE_COMMAND} -DROOT=${PROJECT_SOURCE_DIR} -DNEXT=ON
    -DCHECK=single_version -P ${policy_scripts}/repository.cmake
  LABELS policy TIMEOUT 30)

find_package(Git REQUIRED)
set(policy_repo ${PROJECT_BINARY_DIR}/policy/version_fixture)
file(REMOVE_RECURSE ${policy_repo})
file(WRITE ${policy_repo}/version.txt "1.2.3\n")
file(WRITE ${policy_repo}/notes.md "Released as 1.2.3 today.\n")
execute_process(COMMAND ${GIT_EXECUTABLE} init -q WORKING_DIRECTORY ${policy_repo}
  COMMAND_ERROR_IS_FATAL ANY)
execute_process(COMMAND ${GIT_EXECUTABLE} add version.txt notes.md
  WORKING_DIRECTORY ${policy_repo} COMMAND_ERROR_IS_FATAL ANY)
set(policy_cmd ${CMAKE_COMMAND} -DROOT=${policy_repo} -DCHECK=single_version -P ${policy_scripts}/repository.cmake)
string(REPLACE ";" "$<SEMICOLON>" policy_cmd "${policy_cmd}")
cogmaster_add_test(NAME release.single_version_source.self_test
  COMMAND ${CMAKE_COMMAND} "-DCMD=${policy_cmd}" -DCODE=1
    "-DREGEX=notes.md holds 1.2.3" -P ${policy_scripts}/expect_fail.cmake
  LABELS policy TIMEOUT 30)

cogmaster_add_test(NAME docs.budgets
  COMMAND ${CMAKE_COMMAND} -DROOT=${PROJECT_SOURCE_DIR} -P ${policy_scripts}/budgets.cmake
  LABELS policy TIMEOUT 10)

# Fault prevented: rounding a repository check at exactly one tenth down to a pass.
set(policy_ratio ${PROJECT_BINARY_DIR}/policy/ratio_fixture)
file(WRITE ${policy_ratio}/README.md "fixture\n")
file(WRITE ${policy_ratio}/include/cogmaster.h "fixture\n")
file(WRITE ${policy_ratio}/src/core.c "1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n")
file(WRITE ${policy_ratio}/tests/cmake/check.cmake "# counted\n")
file(WRITE ${policy_ratio}/tests/policy/repository-checks.txt
  "check|tests/cmake/check.cmake|fixture check\n")
set(policy_cmd ${CMAKE_COMMAND} -DROOT=${policy_ratio} -P ${policy_scripts}/budgets.cmake)
string(REPLACE ";" "$<SEMICOLON>" policy_cmd "${policy_cmd}")
cogmaster_add_test(NAME docs.budgets.ratio_equal
  COMMAND ${CMAKE_COMMAND} "-DCMD=${policy_cmd}" -DCODE=1
    "-DREGEX=repository.*1.*10.*strict" -P ${policy_scripts}/expect_fail.cmake
  LABELS policy TIMEOUT 10)
set(policy_missing ${PROJECT_BINARY_DIR}/policy/unclassified_fixture)
file(COPY ${policy_ratio}/ DESTINATION ${policy_missing})
file(WRITE ${policy_missing}/tests/policy/repository-checks.txt "")
set(policy_cmd ${CMAKE_COMMAND} -DROOT=${policy_missing} -P ${policy_scripts}/budgets.cmake)
string(REPLACE ";" "$<SEMICOLON>" policy_cmd "${policy_cmd}")
cogmaster_add_test(NAME docs.budgets.unclassified
  COMMAND ${CMAKE_COMMAND} "-DCMD=${policy_cmd}" -DCODE=1
    "-DREGEX=unclassified applicable code.*check.cmake" -P ${policy_scripts}/expect_fail.cmake
  LABELS policy TIMEOUT 10)

# Every public name in the header appears in the embedding guide, and the
# guide names nothing the header, the version header or the build lacks. The
# self test is a header declaring a call its guide omits beside a guide naming
# a call its header lacks, and expects both named.
cogmaster_add_test(NAME docs.api_names
  COMMAND ${CMAKE_COMMAND} -DHEADER=${PROJECT_SOURCE_DIR}/include/cogmaster.h
    -DDOC=${PROJECT_SOURCE_DIR}/docs/embedding.md
    "-DOTHER=${PROJECT_SOURCE_DIR}/cmake/version.h.in$<SEMICOLON>${PROJECT_SOURCE_DIR}/CMakeLists.txt"
    -DCHECK=api_names -P ${policy_scripts}/repository.cmake
  LABELS policy TIMEOUT 10)
set(policy_names ${PROJECT_BINARY_DIR}/policy/api_names_fixture)
file(WRITE ${policy_names}/cogmaster.h "#define COGMASTER_KEPT 1\nint cogmaster_added(void);\n")
file(WRITE ${policy_names}/embedding.md "Use `COGMASTER_KEPT` and `cogmaster_removed`.\n")
set(policy_cmd ${CMAKE_COMMAND} -DHEADER=${policy_names}/cogmaster.h
  -DDOC=${policy_names}/embedding.md -DCHECK=api_names -P ${policy_scripts}/repository.cmake)
string(REPLACE ";" "$<SEMICOLON>" policy_cmd "${policy_cmd}")
cogmaster_add_test(NAME docs.api_names.self_test
  COMMAND ${CMAKE_COMMAND} "-DCMD=${policy_cmd}" -DCODE=1
    "-DREGEX=cogmaster_added is declared in the header but the guide never names it.*cogmaster_removed is named in the guide but the header does not declare it"
    -P ${policy_scripts}/expect_fail.cmake
  LABELS policy TIMEOUT 10)

# Each derived file has a notice, each notice has a file, and the libretro
# .info licence field agrees with both.
cogmaster_add_test(NAME policy.notices
  COMMAND ${CMAKE_COMMAND} -DROOT=${PROJECT_SOURCE_DIR} -DCHECK=notices -P ${policy_scripts}/repository.cmake
  LABELS policy TIMEOUT 10)
