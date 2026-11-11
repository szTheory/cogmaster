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

include(${CMAKE_CURRENT_LIST_DIR}/budget_cases.cmake)

# quality.json holds the ceilings and floors; the real file is checked against
# the two inventories' line counts. Each control is a fixture limit one step
# past the measured value, an absent key or a malformed file, and expects the
# key or the file named.
cogmaster_add_test(NAME policy.quality
  COMMAND ${CMAKE_COMMAND} -DQUALITY=${PROJECT_SOURCE_DIR}/quality.json
    "-DMEASURED=tests.ci=lines:${PROJECT_SOURCE_DIR}/tests/inventory/ci.txt$<SEMICOLON>tests.wasm32=lines:${PROJECT_SOURCE_DIR}/tests/inventory/wasm32.txt"
    -P ${policy_scripts}/quality.cmake
  LABELS policy TIMEOUT 10)
set(quality_dir ${PROJECT_BINARY_DIR}/policy/quality)
file(REMOVE_RECURSE ${quality_dir})
file(WRITE ${quality_dir}/three.txt "# header\na\nb\nc\n")
function(quality_control name json measured regex)
  file(WRITE ${quality_dir}/${name}.json "${json}")
  set(command ${CMAKE_COMMAND} -DQUALITY=${quality_dir}/${name}.json "-DMEASURED=${measured}"
    -P ${policy_scripts}/quality.cmake)
  string(REPLACE ";" "$<SEMICOLON>" command "${command}")
  cogmaster_add_test(NAME policy.quality.${name}
    COMMAND ${CMAKE_COMMAND} "-DCMD=${command}" -DCODE=1 "-DREGEX=${regex}" -P ${policy_scripts}/expect_fail.cmake
    LABELS policy TIMEOUT 10)
endfunction()
quality_control(ceiling "{\"ceilings\": {\"size.fixture\": 14}}\n"
  "size.fixture=bytes:${quality_dir}/three.txt" "quality:[ \n]+size\\.fixture")
quality_control(floor "{\"floors\": {\"tests.fixture\": 4}}\n"
  "tests.fixture=lines:${quality_dir}/three.txt" "quality:[ \n]+tests\\.fixture")
quality_control(missing_key "{\"ceilings\": {}, \"floors\": {}}\n"
  "tests.absent=lines:${quality_dir}/three.txt" "quality:[ \n]+tests\\.absent.*3")
quality_control(malformed "{\"ceilings\": "
  "tests.fixture=lines:${quality_dir}/three.txt" "malformed\\.json[ \n]+is[ \n]+not[ \n]+valid")

# The ratchet compares the file with the one on the base branch: a limit that
# loosens needs a reasons entry for its key that the base does not already
# hold. Each control writes a head and a base fixture; an absent base passes.
function(quality_ratchet name code head base regex)
  file(WRITE ${quality_dir}/${name}.json "${head}")
  if(base STREQUAL "ABSENT")
    set(base_file ${quality_dir}/${name}.absent.json)
  else()
    set(base_file ${quality_dir}/${name}.base.json)
    file(WRITE ${base_file} "${base}")
  endif()
  set(command ${CMAKE_COMMAND} -DQUALITY=${quality_dir}/${name}.json -DBASE=${base_file}
    "-DMEASURED=tests.fixture=lines:${quality_dir}/three.txt" -P ${policy_scripts}/quality.cmake)
  if(code EQUAL 0)
    cogmaster_add_test(NAME policy.quality.${name} COMMAND ${command} LABELS policy TIMEOUT 10)
    set_tests_properties(policy.quality.${name} PROPERTIES PASS_REGULAR_EXPRESSION "${regex}")
  else()
    string(REPLACE ";" "$<SEMICOLON>" command "${command}")
    cogmaster_add_test(NAME policy.quality.${name}
      COMMAND ${CMAKE_COMMAND} "-DCMD=${command}" -DCODE=1 "-DREGEX=${regex}" -P ${policy_scripts}/expect_fail.cmake
      LABELS policy TIMEOUT 10)
  endif()
endfunction()
set(quality_4 "{\"ceilings\": {\"tests.fixture\": 4}}\n")
set(quality_5 "{\"ceilings\": {\"tests.fixture\": 5}}\n")
set(quality_4_grew "{\"ceilings\": {\"tests.fixture\": 4}, \"reasons\": [{\"key\": \"tests.fixture\", \"reason\": \"grew\"}]}\n")
set(quality_5_grew "{\"ceilings\": {\"tests.fixture\": 5}, \"reasons\": [{\"key\": \"tests.fixture\", \"reason\": \"grew\"}]}\n")
quality_ratchet(base_absent 0 "${quality_5}" ABSENT "quality: no base file")
quality_ratchet(tightened 0 "${quality_4}" "${quality_5}" "tests\\.fixture measured=3")
quality_ratchet(new_reason 0 "${quality_5_grew}" "${quality_4}" "tests\\.fixture measured=3")
quality_ratchet(loosened 1 "${quality_5}" "${quality_4}"
  "quality:[ \n]+tests\\.fixture:[ \n]+loosened[ \n]+from[ \n]+4[ \n]+to[ \n]+5")
quality_ratchet(reused_reason 1 "${quality_5_grew}" "${quality_4_grew}" "quality:[ \n]+tests\\.fixture")

# Every public name in the header appears in the embedding guide, and the
# guide names nothing the header, the version header or the build lacks. The
# self test is a header declaring a call its guide omits beside a guide naming
# a call its header lacks, and expects both named.
cogmaster_add_test(NAME docs.api_names
  COMMAND ${CMAKE_COMMAND} -DHEADER=${PROJECT_SOURCE_DIR}/include/cogmaster.h
    "-DDOC=${PROJECT_SOURCE_DIR}/docs/embedding.md$<SEMICOLON>${PROJECT_SOURCE_DIR}/docs/saves.md$<SEMICOLON>${PROJECT_SOURCE_DIR}/docs/boards.md"
    "-DOTHER=${PROJECT_SOURCE_DIR}/cmake/version.h.in$<SEMICOLON>${PROJECT_SOURCE_DIR}/CMakeLists.txt"
    -DCHECK=api_names -P ${policy_scripts}/repository.cmake
  LABELS policy TIMEOUT 10)
set(policy_names ${PROJECT_BINARY_DIR}/policy/api_names_fixture)
file(WRITE ${policy_names}/cogmaster.h "#define COGMASTER_KEPT 1\nint cogmaster_added(void);\n")
file(WRITE ${policy_names}/guide_a.md "Use `COGMASTER_KEPT`.\n")
file(WRITE ${policy_names}/guide_b.md "Use `cogmaster_removed`.\n")
set(policy_cmd ${CMAKE_COMMAND} -DHEADER=${policy_names}/cogmaster.h
  "-DDOC=${policy_names}/guide_a.md\\$<SEMICOLON>${policy_names}/guide_b.md" -DCHECK=api_names -P ${policy_scripts}/repository.cmake)
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

# Faults prevented: a Dependabot group that holds majors, which never
# auto-merges; a renamed group, which silently stops auto-merging; and a
# privileged trigger or checkout in the auto-merge workflow, which would run
# untrusted code with the App token. The self test scans a tree holding both
# bad files and expects the named failure from each.
cogmaster_add_test(NAME policy.dependabot
  COMMAND ${CMAKE_COMMAND} -DDIR=${PROJECT_SOURCE_DIR}/.github -DCHECK=dependabot -P ${policy_scripts}/repository.cmake
  LABELS policy TIMEOUT 30)
set(policy_dependabot ${PROJECT_BINARY_DIR}/policy/dependabot_fixture)
file(REMOVE_RECURSE ${policy_dependabot})
file(WRITE ${policy_dependabot}/dependabot.yml
  "version: 2\nupdates:\n  - package-ecosystem: github-actions\n    directory: /\n    schedule:\n      interval: daily\n"
  "    commit-message:\n      prefix: ci\n    cooldown:\n      default-days: 7\n    groups:\n      all-actions:\n"
  "        update-types:\n          - minor\n          - patch\n          - major\n")
file(WRITE ${policy_dependabot}/workflows/dependabot-automerge.yml
  "on: pull_request_target\njobs:\n  merge:\n    if: github.event.pull_request.user.login == 'dependabot[bot]'\n"
  "    steps:\n      - uses: actions/checkout@v4\n      - run: echo \"$TITLE\" | grep -q 'the minor-patch group'\n")
set(policy_cmd ${CMAKE_COMMAND} -DDIR=${policy_dependabot} -DCHECK=dependabot -P ${policy_scripts}/repository.cmake)
string(REPLACE ";" "$<SEMICOLON>" policy_cmd "${policy_cmd}")
cogmaster_add_test(NAME policy.dependabot.self_test
  COMMAND ${CMAKE_COMMAND} "-DCMD=${policy_cmd}" -DCODE=1
    "-DREGEX=dependabot: dependabot.yml: group all-actions.*dependabot: dependabot.yml: update-types lists major.*dependabot: dependabot-automerge.yml: trigger pull_request_target.*dependabot: dependabot-automerge.yml: .*actions/checkout.*dependabot: dependabot.yml: interval daily.*dependabot: dependabot-automerge.yml: .*the minor-and-patch group"
    -P ${policy_scripts}/expect_fail.cmake
  LABELS policy TIMEOUT 30)

# Fault prevented: a signature table keyed on the loaded title (a delay per
# header key), which a behaviour test on an owned fixture can never catch
# because the key never matches it. The check reads code only in the five
# files the sensor reaches. The self tests prove that it scans the hooks, that
# it fails on an initialised array in the sensor (braces or a string), that a
# // or /* inside a string literal does not blind it to the rest of the line or
# file, and that the hardware comments that name cartridges and titles do not
# trip it.
cogmaster_add_test(NAME policy.no_title_table
  COMMAND ${CMAKE_COMMAND} -DDIR=${PROJECT_SOURCE_DIR}/src -DCHECK=no_title_table -P ${policy_scripts}/repository.cmake
  LABELS policy TIMEOUT 10)
function(no_title_table_case name gun ports regex)
  set(dir ${PROJECT_BINARY_DIR}/policy/no_title_table_${name})
  file(REMOVE_RECURSE ${dir})
  file(WRITE ${dir}/gun.c "${gun}")
  file(WRITE ${dir}/ports.c "${ports}")
  foreach(file io.c vdp.c vdp_render.c)
    file(WRITE ${dir}/${file} "int ${name}_unit;\n")
  endforeach()
  set(command ${CMAKE_COMMAND} -DDIR=${dir} -DCHECK=no_title_table -P ${policy_scripts}/repository.cmake)
  if(regex STREQUAL "")
    cogmaster_add_test(NAME policy.no_title_table.self_test.${name} COMMAND ${command}
      LABELS policy TIMEOUT 10)
  else()
    string(REPLACE ";" "$<SEMICOLON>" command "${command}")
    cogmaster_add_test(NAME policy.no_title_table.self_test.${name}
      COMMAND ${CMAKE_COMMAND} "-DCMD=${command}" -DCODE=1 "-DREGEX=${regex}" -P ${policy_scripts}/expect_fail.cmake
      LABELS policy TIMEOUT 10)
  endif()
endfunction()
set(no_title_clean "int a;\nint b;\nint c;\nint d;\nint e;\n")
no_title_table_case(array "int a;\nint b;\nint c;\nint d;\nstatic const unsigned char offs[] = {9, 19, 15};\n" "${no_title_clean}" "gun.c:5")
no_title_table_case(strarray "int a;\nint b;\nint c;\nint d;\nstatic const char offs[] = \"\\x09\\x13\";\n" "${no_title_clean}" "gun.c:5")
no_title_table_case(hook "${no_title_clean}" "int a;\nint b;\nint x = m->cart.rom != 0;\n" "ports.c:3")
no_title_table_case(strings "${no_title_clean}"
  "int a;\nint b;\nconst char *u = \"a//b\\\"//\"; int x = m->cart.rom[0];\nconst char *v = \"/*\";\nint y = m->cart.rom[1];\nconst char *w = \"*/\"; /* */ int z;\n"
  "ports.c:3.*ports.c:5")
no_title_table_case(comments
  "/* Each cart carries a title, a sha1 and\n   a crc32 that no code reads. */\nint a; // the cart title\n"
  "/* the cart\n   title\n*/ int b; /* sha1 */ int c; // crc32 of the cart\n" "")

include(${CMAKE_CURRENT_LIST_DIR}/e2e_cases.cmake)
