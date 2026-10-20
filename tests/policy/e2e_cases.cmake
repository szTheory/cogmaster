# Shared hosted-helper boundary controls for CI and hygiene.
set(hygiene_scripts ${PROJECT_SOURCE_DIR}/tests/cmake)
# Fault prevented: real-host test code growing without bound. Every line
# under tests/retroarch and the shared oracle counts, at most 300.
# Controls cover nested files, missing final newlines and the shared helper.
cogmaster_add_test(NAME policy.e2e_budget
  COMMAND ${CMAKE_COMMAND} -DDIR=${CMAKE_SOURCE_DIR}/tests/retroarch -DCHECK=e2e_budget -P ${hygiene_scripts}/repository.cmake
  LABELS hygiene policy TIMEOUT 30)
set(hygiene_e2e ${CMAKE_BINARY_DIR}/hygiene/e2e_budget_fixture)
file(REMOVE_RECURSE ${hygiene_e2e})
string(REPEAT "line\n" 150 hygiene_lines)
file(WRITE ${hygiene_e2e}/sample.cmake "${hygiene_lines}")
file(WRITE ${hygiene_e2e}/helpers/parser.cmake "${hygiene_lines}last")
set(hygiene_cmd ${CMAKE_COMMAND} -DDIR=${hygiene_e2e} -DCHECK=e2e_budget -P ${hygiene_scripts}/repository.cmake)
string(REPLACE ";" "$<SEMICOLON>" hygiene_cmd "${hygiene_cmd}")
cogmaster_add_test(NAME policy.e2e_budget.self_test
  COMMAND ${CMAKE_COMMAND} "-DCMD=${hygiene_cmd}" -DCODE=1 "-DREGEX=e2e_budget: 301 lines"
    -P ${hygiene_scripts}/expect_fail.cmake
  LABELS hygiene policy TIMEOUT 30)
# Fault prevented: the shared independent oracle escaping hosted helper accounting.
set(hygiene_shared ${CMAKE_BINARY_DIR}/hygiene/e2e_shared)
file(REMOVE_RECURSE ${hygiene_shared})
string(REPEAT "line\n" 280 hygiene_lines)
file(WRITE ${hygiene_shared}/retroarch/driver.cmake "${hygiene_lines}")
string(REPEAT "line\n" 21 hygiene_lines)
file(WRITE ${hygiene_shared}/fixtures/play.h "${hygiene_lines}")
set(hygiene_cmd ${CMAKE_COMMAND} -DDIR=${hygiene_shared}/retroarch -DCHECK=e2e_budget -P ${hygiene_scripts}/repository.cmake)
string(REPLACE ";" "$<SEMICOLON>" hygiene_cmd "${hygiene_cmd}")
cogmaster_add_test(NAME policy.e2e_budget.shared_helper
  COMMAND ${CMAKE_COMMAND} "-DCMD=${hygiene_cmd}" -DCODE=1 "-DREGEX=e2e_budget: 301 lines"
    -P ${hygiene_scripts}/expect_fail.cmake
  LABELS hygiene policy TIMEOUT 30)
