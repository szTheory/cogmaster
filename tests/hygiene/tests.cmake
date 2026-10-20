# Hygiene tests, registered only by the hygiene preset (COGMASTER_HYGIENE).
set(hygiene_scripts ${CMAKE_CURRENT_LIST_DIR}/../cmake)
include(${CMAKE_CURRENT_LIST_DIR}/../policy/budget_cases.cmake)

# Fault prevented: a personal path or address, an unlisted binary, a game
# image or GPL text in any file git would track, ignored build output aside.
cogmaster_add_test(NAME hygiene.tree
  COMMAND ${COGMASTER_SH} ${CMAKE_SOURCE_DIR}/scripts/hygiene.sh --tree
  LABELS hygiene TIMEOUT 60)

# Fault prevented: a tree scan that passes whatever the tree holds. A
# throwaway repository holds a copy of scripts/hygiene.sh and one untracked,
# not ignored file whose text is a home directory path, which must be named.
# The path is built in pieces, so this file never holds one and
# hygiene.tree stays green.
set(hygiene_tree ${CMAKE_BINARY_DIR}/hygiene/tree_fixture)
file(REMOVE_RECURSE ${hygiene_tree})
file(MAKE_DIRECTORY ${hygiene_tree}/scripts)
file(COPY ${CMAKE_SOURCE_DIR}/scripts/hygiene.sh DESTINATION ${hygiene_tree}/scripts)
set(hygiene_home "/Use")
string(APPEND hygiene_home "rs/someone/projects/cogmaster")
file(WRITE ${hygiene_tree}/notes.txt "built in ${hygiene_home}\n")
execute_process(COMMAND ${GIT_EXECUTABLE} init -q ${hygiene_tree} RESULT_VARIABLE hygiene_rc)
if(NOT hygiene_rc EQUAL 0)
  message(FATAL_ERROR "hygiene: git init ${hygiene_tree} failed")
endif()
set(hygiene_cmd ${COGMASTER_SH} -c "cd \"$1\" && sh scripts/hygiene.sh --tree" sh ${hygiene_tree})
string(REPLACE ";" "$<SEMICOLON>" hygiene_cmd "${hygiene_cmd}")
cogmaster_add_test(NAME hygiene.tree.self_test
  COMMAND ${CMAKE_COMMAND} "-DCMD=${hygiene_cmd}" -DCODE=1 "-DREGEX=hygiene: home directory path"
    -P ${hygiene_scripts}/expect_fail.cmake
  LABELS hygiene TIMEOUT 60)

# Fault prevented: arbitrary licensed binary bytes mistaken for private text,
# while an unlisted binary must still fail before any text scanning.
set(hygiene_address "binary")
string(APPEND hygiene_address "@" "invalid.test")
foreach(case manifest unlisted)
  set(tree ${CMAKE_BINARY_DIR}/hygiene/binary_${case})
  file(REMOVE_RECURSE ${tree})
  file(MAKE_DIRECTORY ${tree}/scripts ${tree}/tests/roms)
  file(COPY ${CMAKE_SOURCE_DIR}/scripts/hygiene.sh DESTINATION ${tree}/scripts)
  execute_process(COMMAND ${GIT_EXECUTABLE} init -q ${tree} COMMAND_ERROR_IS_FATAL ANY)
  execute_process(COMMAND ${COGMASTER_SH} -c "printf '\\000%s\\n' \"$1\" > \"$2\""
    sh "${hygiene_address}" ${tree}/tests/roms/synthetic.bin COMMAND_ERROR_IS_FATAL ANY)
  if(case STREQUAL "manifest")
    file(WRITE ${tree}/tests/roms/manifest.txt "tests/roms/synthetic.bin MIT synthetic fixture\n")
  endif()
  set(command ${COGMASTER_SH} -c "cd \"$1\" && sh scripts/hygiene.sh --tree" sh ${tree})
  if(case STREQUAL "manifest")
    cogmaster_add_test(NAME hygiene.tree.binary_manifest COMMAND ${command} LABELS hygiene TIMEOUT 60)
  else()
    string(REPLACE ";" "$<SEMICOLON>" command "${command}")
    cogmaster_add_test(NAME hygiene.tree.binary_unlisted
      COMMAND ${CMAKE_COMMAND} "-DCMD=${command}" -DCODE=1 "-DREGEX=binary file not in manifest"
        -P ${hygiene_scripts}/expect_fail.cmake
      LABELS hygiene TIMEOUT 60)
  endif()
endforeach()

# Fault prevented: code that drifts from .clang-format. Every tracked C file
# but the vendored libretro.h, which is never edited.
cogmaster_add_test(NAME hygiene.format
  COMMAND ${COGMASTER_SH} -c "cd \"$1\" && \"$2\" ls-files -z '*.c' '*.h' \
':!:libretro/libretro.h' | xargs -0 \"$3\" --dry-run --Werror" sh
    ${CMAKE_SOURCE_DIR} ${GIT_EXECUTABLE} ${COGMASTER_CLANG_FORMAT}
  LABELS hygiene TIMEOUT 60)

# Fault prevented: floating point in the core's text. The self test scans a
# fixture with six float lines beside two lines that only look like them.
cogmaster_add_test(NAME hygiene.no_float
  COMMAND ${CMAKE_COMMAND} "-DROOTS=${CMAKE_SOURCE_DIR}/src$<SEMICOLON>${CMAKE_SOURCE_DIR}/include"
    -DCHECK=no_float -P ${hygiene_scripts}/repository.cmake
  LABELS hygiene TIMEOUT 30)
set(hygiene_fixture ${CMAKE_BINARY_DIR}/hygiene/no_float_fixture)
file(WRITE ${hygiene_fixture}/sample.c "int y = 0x1E5;\nint double_free = 2;\nx = 1.5;\n"
  "z = 0x1p3;\nw = 1e3;\ndouble d;\n#include <math.h>\n#include <float.h>\n")
set(hygiene_cmd ${CMAKE_COMMAND} -DROOTS=${hygiene_fixture} -DCHECK=no_float -P ${hygiene_scripts}/repository.cmake)
string(REPLACE ";" "$<SEMICOLON>" hygiene_cmd "${hygiene_cmd}")
cogmaster_add_test(NAME hygiene.no_float.self_test
  COMMAND ${CMAKE_COMMAND} "-DCMD=${hygiene_cmd}" -DCODE=1
    "-DREGEX=sample.c:3: x = 1.5.*sample.c:8: #include <float.h>.*no_float: 6 lines"
    -P ${hygiene_scripts}/expect_fail.cmake
  LABELS hygiene TIMEOUT 30)

# Fault prevented: a defect the clang static analyzer can see in the core.
# The Annex K check is off: the core may call only the C memory functions
# (abi.undefined), and the _s variants it asks for are optional in C11 and
# absent from glibc.
cogmaster_add_test(NAME hygiene.analyzer
  COMMAND ${COGMASTER_SH} -c "cd \"$1\" && \"$2\" -p \"$3\" --quiet \
-checks='-*,clang-analyzer-*,-clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling' \
--warnings-as-errors='*' src/*.c" sh
    ${CMAKE_SOURCE_DIR} ${COGMASTER_CLANG_TIDY} ${CMAKE_BINARY_DIR}
  LABELS hygiene TIMEOUT 120)

# Fault prevented: a pins.txt row that scripts/fetch-pin.sh would misread,
# or that downloads over plain HTTP or with a truncated digest. The self test
# runs scripts/fetch-pin.sh on a row whose digest is wrong for its file,
# which must stop before DEST is touched.
cogmaster_add_test(NAME policy.pins
  COMMAND ${CMAKE_COMMAND} -DPINS=${CMAKE_SOURCE_DIR}/pins.txt -DCHECK=pins -P ${hygiene_scripts}/repository.cmake
  LABELS hygiene policy TIMEOUT 10)
set(hygiene_pins ${CMAKE_BINARY_DIR}/hygiene/pins_fixture)
file(REMOVE_RECURSE ${hygiene_pins})
file(WRITE ${hygiene_pins}/payload.txt "not the pinned bytes\n")
string(REPEAT "0" 64 hygiene_zeros)
file(WRITE ${hygiene_pins}/pins.txt "payload 1 file://${hygiene_pins}/payload.txt ${hygiene_zeros}\n")
set(hygiene_check [=[
PINS=$1 sh "$2" payload "$3"
rc=$?
if [ -e "$3" ]
then
  echo "policy.pins.self_test: $3 was created before the digest matched"
  exit 3
fi
exit "$rc"
]=])
set(hygiene_cmd ${COGMASTER_SH} -c "${hygiene_check}" sh ${hygiene_pins}/pins.txt
  ${CMAKE_SOURCE_DIR}/scripts/fetch-pin.sh ${hygiene_pins}/dest)
string(REPLACE ";" "$<SEMICOLON>" hygiene_cmd "${hygiene_cmd}")
cogmaster_add_test(NAME policy.pins.self_test
  COMMAND ${CMAKE_COMMAND} "-DCMD=${hygiene_cmd}" -DCODE=1 "-DREGEX=payload: sha256 mismatch"
    -P ${hygiene_scripts}/expect_fail.cmake
  LABELS hygiene policy TIMEOUT 30)

# Fault prevented: a pull-request title or body that cuts the wrong release
# or none, or starts no CI run on main. Each fixture line is want|title|body,
# the body with \n escapes; scripts/check-title.sh must give every want, and
# the fixtures must hold both kinds, so a check that passes or fails
# everything fails here.
set(hygiene_titles [=[
script=$1
passes=0
fails=0
while IFS='|' read -r want title body
do
  if [ -z "$want" ]
  then continue
  fi
  if printf '%b\n' "$body" | sh "$script" "$title" > /dev/null 2>&1
  then got=pass
  else got=fail
  fi
  if [ "$got" != "$want" ]
  then
    echo "policy.title: want $want, got $got: $title|$body"
    exit 1
  fi
  if [ "$want" = pass ]
  then passes=$((passes + 1))
  else fails=$((fails + 1))
  fi
done < "$2"
echo "policy.title: $passes pass and $fails fail fixtures"
[ "$passes" -gt 0 ] && [ "$fails" -gt 0 ]
]=])
cogmaster_add_test(NAME policy.title
  COMMAND ${COGMASTER_SH} -c "${hygiene_titles}" sh ${CMAKE_SOURCE_DIR}/scripts/check-title.sh
    ${CMAKE_CURRENT_LIST_DIR}/fixtures/titles.txt
  LABELS hygiene policy TIMEOUT 30)
# Fault prevented: a title check that accepts a "Phase N: Name" title, or
# fails it for some other reason than the one it names.
set(hygiene_cmd ${COGMASTER_SH} -c "sh \"$1\" \"$2\" < /dev/null" sh
  ${CMAKE_SOURCE_DIR}/scripts/check-title.sh "Phase 1: A test frame in RetroArch")
string(REPLACE ";" "$<SEMICOLON>" hygiene_cmd "${hygiene_cmd}")
cogmaster_add_test(NAME policy.title.self_test
  COMMAND ${CMAKE_COMMAND} "-DCMD=${hygiene_cmd}" -DCODE=1 "-DREGEX=not a Conventional Commit title"
    -P ${hygiene_scripts}/expect_fail.cmake
  LABELS hygiene policy TIMEOUT 30)

# Fault prevented: a code change skipping CI as planning only, or a change to
# the release pipeline merging unrehearsed. heavy=false only for a non-empty
# list wholly under .planning/, both names of a rename included, from a pull
# request of at most 3000 files, the most the files API lists;
# release_inputs=true for any of the five release inputs and for any list
# that cannot be trusted.
set(hygiene_classify [=[
script=$1
n=0
check() {
  got=$(printf '%b' "$3" | sh "$script" "${4-2}" | tr '\n' ' ')
  if [ "$got" != "heavy=$1 release_inputs=$2 " ]
  then
    echo "ci.classify: want heavy=$1 release_inputs=$2, got $got for: $3"
    exit 1
  fi
  n=$((n + 1))
}
check true false '.planning/STATE.md\nsrc/core.c\n'
check true false 'src/core.c\n.planning/core.c\n'
check true true ''
check true true '\n\n'
check true false '(listing failed)\n'
check true false '.planning-notes/a.md\n'
check true false 'docs/.planning/a.md\n'
check false false '.planning/STATE.md\n.planning/phases/01/a.md\n'
check false false '.planning/STATE.md'
check false false '.planning/STATE.md\n' 3000
check true true '.planning/STATE.md\n' 3001
check true true '.planning/STATE.md\n' ''
check true true '.planning/STATE.md\n' 'x'
check true true '.github/workflows/release.yml\n'
check true true '.github/workflows/ci.yml\n'
check true true 'src/core.c\nscripts/publish.sh\n'
check true true 'release-please-config.json\n'
check true true 'cmake/packaging.cmake\n'
check true true 'scripts/publish.sh\nscripts/release.sh\n'
check true false 'scripts/publish.sh.orig\ndocs/release.yml\n'
check false false '.planning/STATE.md\n' 1
echo "ci.classify: $n lists"
]=])
cogmaster_add_test(NAME ci.classify
  COMMAND ${COGMASTER_SH} -c "${hygiene_classify}" sh ${CMAKE_SOURCE_DIR}/scripts/classify-changes.sh
  LABELS hygiene TIMEOUT 30)

# Fault prevented: a release with a missing archive, or a SHA256SUMS whose
# order differs from host to host. scripts/publish.sh prepare runs on 18
# empty placeholder zips and six build legs' commit.txt, and SHA256SUMS must
# list the 18 in C-locale name order. The self test leaves one zip out,
# which must fail naming it.
set(hygiene_publish ${CMAKE_BINARY_DIR}/hygiene/publish_set)
file(REMOVE_RECURSE ${hygiene_publish})
foreach(c library runner libretro)
  foreach(o linux macos windows)
    foreach(a x64 arm64)
      file(WRITE ${hygiene_publish}/full/cogmaster-${c}-${o}-${a}.zip "")
      if(NOT "${c}-${o}-${a}" STREQUAL "runner-windows-x64")
        file(WRITE ${hygiene_publish}/short/cogmaster-${c}-${o}-${a}.zip "")
      endif()
    endforeach()
  endforeach()
endforeach()
foreach(leg RANGE 1 6)
  file(WRITE ${hygiene_publish}/full/legs/${leg}/commit.txt "fixture\n")
  file(WRITE ${hygiene_publish}/short/legs/${leg}/commit.txt "fixture\n")
endforeach()
set(hygiene_publish_check [=[
EXPECTED_SHA=fixture sh "$1" prepare "$2" || exit 1
names=$(awk '{ print $2 }' "$2/SHA256SUMS")
count=$(printf '%s\n' "$names" | grep -c '^cogmaster-[a-z]*-[a-z]*-[a-z0-9]*\.zip$')
if [ "$count" -ne 18 ]
then
  echo "policy.publish_set: $count archives in SHA256SUMS, want 18"
  exit 1
fi
if ! printf '%s\n' "$names" | LC_ALL=C sort -c -u
then
  echo "policy.publish_set: SHA256SUMS is not in C-locale name order"
  exit 1
fi
echo "policy.publish_set: 18 archives in C-locale name order"
]=])
cogmaster_add_test(NAME policy.publish_set
  COMMAND ${COGMASTER_SH} -c "${hygiene_publish_check}" sh ${CMAKE_SOURCE_DIR}/scripts/publish.sh
    ${hygiene_publish}/full
  LABELS hygiene policy TIMEOUT 30)
set(hygiene_cmd ${CMAKE_COMMAND} -E env EXPECTED_SHA=fixture
  ${COGMASTER_SH} ${CMAKE_SOURCE_DIR}/scripts/publish.sh prepare ${hygiene_publish}/short)
string(REPLACE ";" "$<SEMICOLON>" hygiene_cmd "${hygiene_cmd}")
cogmaster_add_test(NAME policy.publish_set.self_test
  COMMAND ${CMAKE_COMMAND} "-DCMD=${hygiene_cmd}" -DCODE=1
    "-DREGEX=publish: missing cogmaster-runner-windows-x64.zip"
    -P ${hygiene_scripts}/expect_fail.cmake
  LABELS hygiene policy TIMEOUT 30)

# Fault prevented: a release that verifies against a pull request's
# attestation. verify on a release tag with a pull request's ref must refuse
# before any network call.
set(hygiene_cmd ${CMAKE_COMMAND} -E env GITHUB_REPOSITORY=fixture/fixture SOURCE_REF=refs/pull/1/merge
  ${COGMASTER_SH} ${CMAKE_SOURCE_DIR}/scripts/publish.sh verify fixture-release ${hygiene_publish}/full)
string(REPLACE ";" "$<SEMICOLON>" hygiene_cmd "${hygiene_cmd}")
cogmaster_add_test(NAME policy.publish_ref
  COMMAND ${CMAKE_COMMAND} "-DCMD=${hygiene_cmd}" -DCODE=1
    "-DREGEX=publish: fixture-release verifies only from refs/heads/main, not refs/pull/1/merge"
    -P ${hygiene_scripts}/expect_fail.cmake
  LABELS hygiene policy TIMEOUT 30)

# Fault prevented: a release that accepts downloads signed by the rehearsal's
# workflow. verify runs end to end against a gh and a curl on PATH that log
# their arguments and serve a fixture release; with SIGNER_WORKFLOW=ci.yml a
# release tag must still verify as release.yml, and a rehearsal- tag as ci.yml.
set(hygiene_signer ${CMAKE_BINARY_DIR}/hygiene/publish_signer)
file(REMOVE_RECURSE ${hygiene_signer})
foreach(c library runner libretro)
  foreach(o linux macos windows)
    foreach(a x64 arm64)
      file(WRITE ${hygiene_signer}/dist/cogmaster-${c}-${o}-${a}.zip "${c}-${o}-${a}\n")
    endforeach()
  endforeach()
endforeach()
foreach(leg RANGE 1 6)
  file(WRITE ${hygiene_signer}/dist/legs/${leg}/commit.txt "fixture\n")
endforeach()
# The original verifies from its ref; the flipped copy, checked with no ref,
# does not.
file(WRITE ${hygiene_signer}/bin/gh [=[
#!/bin/sh
echo "gh $*" >> "$STUB_LOG"
case "$*" in
  "attestation verify "*--source-ref*) exit 0 ;;
  "attestation verify "*) exit 1 ;;
  *isDraft*) echo false ;;
  *assets*) cd "$STUB_DIR" && ls cogmaster-*.zip SHA256SUMS ;;
esac
]=])
file(WRITE ${hygiene_signer}/bin/curl [=[
#!/bin/sh
while [ $# -gt 1 ]
do
  if [ "$1" = -o ]
  then out=$2
  fi
  shift
done
cp "$STUB_DIR/${1##*/}" "$out"
]=])
file(CHMOD ${hygiene_signer}/bin/gh ${hygiene_signer}/bin/curl
  FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
set(hygiene_signer_check [=[
script=$1
export STUB_DIR=$2/dist STUB_LOG=$2/gh.log RUNNER_TEMP=$2/tmp PATH="$2/bin:$PATH"
export GITHUB_REPOSITORY=fixture/fixture SIGNER_WORKFLOW=ci.yml EXPECTED_SHA=fixture
sh "$script" prepare "$STUB_DIR" > /dev/null || exit 1
for case in fixture-release:refs/heads/main:release.yml rehearsal-1-1:refs/pull/1/merge:ci.yml
do
  tag=${case%%:*} rest=${case#*:}
  : > "$STUB_LOG"
  SOURCE_REF=${rest%%:*} sh "$script" verify "$tag" "$STUB_DIR" > /dev/null || exit 1
  want="--signer-workflow fixture/fixture/.github/workflows/${rest#*:} "
  got=$(grep -c -e "$want" "$STUB_LOG")
  other=$(grep -e '--signer-workflow' "$STUB_LOG" | grep -c -v -e "$want")
  if [ "$got" -ne 18 ] || [ "$other" -ne 0 ]
  then
    echo "policy.publish_signer: $tag verified $got downloads as ${rest#*:} and $other otherwise"
    exit 1
  fi
done
echo "policy.publish_signer: a release tag verifies as release.yml, a rehearsal tag as ci.yml"
]=])
cogmaster_add_test(NAME policy.publish_signer
  COMMAND ${COGMASTER_SH} -c "${hygiene_signer_check}" sh ${CMAKE_SOURCE_DIR}/scripts/publish.sh ${hygiene_signer}
  LABELS hygiene policy TIMEOUT 30)

# Fault prevented: a CI required roll-up that passes a failed or wrongly
# skipped job, or a job missing from its needs. The self test runs it on a
# copy of ci.yml whose program ignores heavy, which must let a skipped build
# through and so fail the case that names it. wasm32 reports on its own, so
# it may stay out of the needs.
set(hygiene_ci ${CMAKE_SOURCE_DIR}/.github/workflows/ci.yml)
cogmaster_add_test(NAME policy.ci_required
  COMMAND ${CMAKE_COMMAND} -DCI_YML=${hygiene_ci} -DJQ=${COGMASTER_JQ} -DREPORT_ONLY=wasm32
    -P ${hygiene_scripts}/ci_required.cmake
  LABELS hygiene policy TIMEOUT 30)
file(READ ${hygiene_ci} hygiene_yml)
string(REPLACE "$heavy == \"false\" and " "" hygiene_ignores_heavy "${hygiene_yml}")
if(hygiene_ignores_heavy STREQUAL hygiene_yml)
  message(FATAL_ERROR "policy.ci_required.self_test: ci.yml's program no longer tests $heavy == \"false\"")
endif()
file(WRITE ${CMAKE_BINARY_DIR}/hygiene/ci_required_fixture/ci.yml "${hygiene_ignores_heavy}")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${hygiene_ci})
set(hygiene_cmd ${CMAKE_COMMAND} -DCI_YML=${CMAKE_BINARY_DIR}/hygiene/ci_required_fixture/ci.yml
  -DJQ=${COGMASTER_JQ} -DREPORT_ONLY=wasm32 -P ${hygiene_scripts}/ci_required.cmake)
string(REPLACE ";" "$<SEMICOLON>" hygiene_cmd "${hygiene_cmd}")
cogmaster_add_test(NAME policy.ci_required.self_test
  COMMAND ${CMAKE_COMMAND} "-DCMD=${hygiene_cmd}" -DCODE=1
    "-DREGEX=build skipped when heavy=true: want fail, got exit 0"
    -P ${hygiene_scripts}/expect_fail.cmake
  LABELS hygiene policy TIMEOUT 30)

# Fault prevented: a job timeout guessed rather than measured. Every
# timeout-minutes line names a run ID or says provisional. The self test
# scans a fixture with two good lines and two bad ones.
cogmaster_add_test(NAME hygiene.timeout_comments
  COMMAND ${CMAKE_COMMAND} -DDIR=${CMAKE_SOURCE_DIR}/.github/workflows -DCHECK=timeouts -P ${hygiene_scripts}/repository.cmake
  LABELS hygiene TIMEOUT 30)
set(hygiene_timeouts ${CMAKE_BINARY_DIR}/hygiene/timeouts_fixture)
file(REMOVE_RECURSE ${hygiene_timeouts})
file(WRITE ${hygiene_timeouts}/sample.yml "jobs:\n  a:\n    timeout-minutes: 5  # provisional\n"
  "  b:\n    timeout-minutes: 30\n  c:\n    timeout-minutes: 8  # 2 x 3 min, run 38935647258\n"
  "  d:\n    timeout-minutes: 15  # a guess\n")
set(hygiene_cmd ${CMAKE_COMMAND} -DDIR=${hygiene_timeouts} -DCHECK=timeouts -P ${hygiene_scripts}/repository.cmake)
string(REPLACE ";" "$<SEMICOLON>" hygiene_cmd "${hygiene_cmd}")
cogmaster_add_test(NAME hygiene.timeout_comments.self_test
  COMMAND ${CMAKE_COMMAND} "-DCMD=${hygiene_cmd}" -DCODE=1
    "-DREGEX=sample.yml: timeout-minutes: 30.*sample.yml: timeout-minutes: 15 .*a guess.*timeout_comments: 2 lines"
    -P ${hygiene_scripts}/expect_fail.cmake
  LABELS hygiene TIMEOUT 30)

# Fault prevented: an action referenced by a tag that can be moved under the
# release. The self test scans a fixture that uses a tag.
cogmaster_add_test(NAME policy.action_pins
  COMMAND ${CMAKE_COMMAND} -DDIR=${CMAKE_SOURCE_DIR}/.github/workflows -DCHECK=action_pins -P ${hygiene_scripts}/repository.cmake
  LABELS hygiene policy TIMEOUT 30)
set(hygiene_actions ${CMAKE_BINARY_DIR}/hygiene/action_pins_fixture)
file(REMOVE_RECURSE ${hygiene_actions})
string(REPEAT "0" 40 hygiene_sha)
file(WRITE ${hygiene_actions}/sample.yml "steps:\n  - uses: ./.github/actions/local\n"
  "  - uses: actions/checkout@${hygiene_sha} # v7.0.1\n  - uses: actions/checkout@v4\n")
set(hygiene_cmd ${CMAKE_COMMAND} -DDIR=${hygiene_actions} -DCHECK=action_pins -P ${hygiene_scripts}/repository.cmake)
string(REPLACE ";" "$<SEMICOLON>" hygiene_cmd "${hygiene_cmd}")
cogmaster_add_test(NAME policy.action_pins.self_test
  COMMAND ${CMAKE_COMMAND} "-DCMD=${hygiene_cmd}" -DCODE=1
    "-DREGEX=action_pins: sample.yml: uses: actions/checkout@v4.*action_pins: 1 uses: lines not pinned"
    -P ${hygiene_scripts}/expect_fail.cmake
  LABELS hygiene policy TIMEOUT 30)

include(${PROJECT_SOURCE_DIR}/tests/policy/e2e_cases.cmake)
