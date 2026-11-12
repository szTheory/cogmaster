# Hygiene tests, registered only by the hygiene preset (COGMASTER_HYGIENE).
set(hygiene_scripts ${CMAKE_CURRENT_LIST_DIR}/../cmake)
include(${CMAKE_CURRENT_LIST_DIR}/../policy/budget_cases.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/../policy/firmware_window_cases.cmake)

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

# Fault prevented: a tree scan that misses a game image by content because the
# read-once scan dropped an offset or read the wrong copy, and a pre-commit scan
# that rereads every file six times. An untracked data.txt holds the Sega header
# marker at byte 32,752 only (no ROM-like name, no manifest row).
# Timeout: 20 s floor; 2 x 0.05 s (hygiene lane), slowest of 4 runs, run 43117518796.
set(tree ${CMAKE_BINARY_DIR}/hygiene/header_offset)
file(REMOVE_RECURSE ${tree})
file(MAKE_DIRECTORY ${tree}/scripts)
file(COPY ${CMAKE_SOURCE_DIR}/scripts/hygiene.sh DESTINATION ${tree}/scripts)
execute_process(COMMAND ${GIT_EXECUTABLE} init -q ${tree} COMMAND_ERROR_IS_FATAL ANY)
execute_process(COMMAND ${COGMASTER_SH} -c [=[dd if=/dev/zero of="$1" bs=32768 count=1 2>/dev/null && printf 'TMR SEGA' | dd of="$1" bs=1 seek=32752 conv=notrunc 2>/dev/null]=]
  sh ${tree}/data.txt COMMAND_ERROR_IS_FATAL ANY)
set(command ${COGMASTER_SH} -c "cd \"$1\" && sh scripts/hygiene.sh --tree" sh ${tree})
string(REPLACE ";" "$<SEMICOLON>" command "${command}")
cogmaster_add_test(NAME hygiene.tree.header_offset
  COMMAND ${CMAKE_COMMAND} "-DCMD=${command}" -DCODE=1 "-DREGEX=hygiene: game image by content"
    -P ${hygiene_scripts}/expect_fail.cmake
  LABELS hygiene TIMEOUT 20)

# Fault prevented: firmware retyped into source as a byte array. 300 consecutive two-digit literals fail, 255 pass,
# and 255 split by one prose line pass, so only a long unbroken run trips the tripwire.
foreach(case over under split)
  set(tree ${CMAKE_BINARY_DIR}/hygiene/byte_run_${case})
  file(REMOVE_RECURSE ${tree})
  file(MAKE_DIRECTORY ${tree}/scripts)
  file(COPY ${CMAKE_SOURCE_DIR}/scripts/hygiene.sh DESTINATION ${tree}/scripts)
  execute_process(COMMAND ${GIT_EXECUTABLE} init -q ${tree} COMMAND_ERROR_IS_FATAL ANY)
  set(literals 255)
  set(prose -1)
  if(case STREQUAL "over")
    set(literals 300)
  elseif(case STREQUAL "split")
    set(prose 128)
  endif()
  execute_process(COMMAND ${COGMASTER_SH} -c [=[awk -v n="$2" -v s="$3" 'BEGIN { for (i = 0; i < n; i++) { if (i == s) print "prose"; printf "0x%02x,%s", i % 256, (i % 16 == 15 ? "\n" : " ") } print "" }' > "$1"]=]
    sh ${tree}/data.txt ${literals} ${prose} COMMAND_ERROR_IS_FATAL ANY)
  set(command ${COGMASTER_SH} -c "cd \"$1\" && sh scripts/hygiene.sh --tree" sh ${tree})
  # Timeout: 20 s floor; 2 x 0.10 s, slowest of 9 runs, run 43049044281.
  if(case STREQUAL "over")
    string(REPLACE ";" "$<SEMICOLON>" command "${command}")
    cogmaster_add_test(NAME hygiene.byte_run.over
      COMMAND ${CMAKE_COMMAND} "-DCMD=${command}" -DCODE=1 "-DREGEX=hygiene: retyped bytes" -P ${hygiene_scripts}/expect_fail.cmake
      LABELS hygiene TIMEOUT 20)
  else()
    cogmaster_add_test(NAME hygiene.byte_run.${case} COMMAND ${command} LABELS hygiene TIMEOUT 20)
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
# absent from glibc. The run is parallel, nine files per process, four at a
# time; a failing run makes xargs exit non-zero (123 with GNU xargs).
cogmaster_add_test(NAME hygiene.analyzer
  COMMAND ${COGMASTER_SH} -c "cd \"$1\" && printf '%s\\n' src/*.c | xargs -n 9 -P 4 \"$2\" -p \"$3\" --quiet \
-checks='-*,clang-analyzer-*,-clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling' \
--warnings-as-errors='*'" sh
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

# Fault prevented: a timeout guessed from one local run instead of twice the
# slowest hosted time, and a duration report that counts a failed or timed-out
# test as a measurement. The fixture log has a test on two lanes at different
# times, one on a single lane, a Failed and a Timeout line, and the report must
# list the first two slowest first and never name the others.
set(hygiene_durations ${CMAKE_BINARY_DIR}/hygiene/durations_fixture)
file(REMOVE_RECURSE ${hygiene_durations})
file(CONFIGURE OUTPUT ${hygiene_durations}/log.txt NEWLINE_STYLE UNIX CONTENT [=[
lane-a  Run ctest  2026-11-11T22:24:54.6Z  1/9 Test  #1: z_slow ........   Passed    0.50 sec
lane-a  Run ctest  2026-11-11T22:24:55.1Z  2/9 Test  #2: a_fast ........   Passed    1.00 sec
lane-a  Run ctest  2026-11-11T22:24:55.2Z  3/9 Test  #3: c_failed .....***Failed    0.30 sec
lane-b  Run ctest  2026-11-11T22:25:01.0Z  1/9 Test  #1: z_slow ........   Passed    2.10 sec
lane-b  Run ctest  2026-11-11T22:25:02.0Z  4/9 Test  #4: d_timeout ..***Timeout   20.00 sec
]=])
set(hygiene_durations_test [=[
sh "$1" tests-summarise < "$2" > "$3" || exit 1
want=$(printf 'test\tlanes\tslowest_s\nz_slow\t2\t2.10\na_fast\t1\t1.00\n')
[ "$(cat "$3")" = "$want" ] || { echo "ci.durations_tests: unexpected table"; cat "$3"; exit 1; }
sh "$1" tests abc > /dev/null 2>&1 && { echo "ci.durations_tests: a run id that is not a number was accepted"; exit 1; }
echo "ci.durations_tests: two tests listed, the failed and the timed-out one left out"
]=])
# Timeout: 20 s floor; 2 x 0.01 s (hygiene lane), slowest of 1 run, run 43117518796.
cogmaster_add_test(NAME ci.durations_tests
  COMMAND ${COGMASTER_SH} -c "${hygiene_durations_test}" sh ${CMAKE_SOURCE_DIR}/scripts/ci-durations.sh
    ${hygiene_durations}/log.txt ${hygiene_durations}/table.tsv
  LABELS hygiene TIMEOUT 20)

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

# Fault prevented: a required gate that fails because a deleted tag still reads back for a few seconds. With a
# gh that keeps answering "exists" for LAG reads of the tag and a no-op sleep, rehearsal-delete must succeed for
# a short lag, and for a tag that never goes must read exactly six times, name the tag and exit 1.
# Timeout: 30 s floor; 2 x 0.20 s, slowest of 1 run, run 43126677381.
set(hygiene_delete ${CMAKE_BINARY_DIR}/hygiene/publish_delete)
file(REMOVE_RECURSE ${hygiene_delete})
file(WRITE ${hygiene_delete}/bin/gh [=[
#!/bin/sh
case "$*" in
  "api -X DELETE "* | "release delete "* | "api --paginate "*) exit 0 ;;
  "api "*/git/ref/tags/*)
    echo read >> "$STUB_DIR/reads"
    [ "$(wc -l < "$STUB_DIR/reads")" -le "$LAG" ] ;;
  *) exit 1 ;;
esac
]=])
file(WRITE ${hygiene_delete}/bin/sleep "#!/bin/sh\nexit 0\n")
file(CHMOD ${hygiene_delete}/bin/gh ${hygiene_delete}/bin/sleep
  FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
set(hygiene_delete_check [=[
script=$1
export STUB_DIR=$2 PATH="$2/bin:$PATH" GITHUB_REPOSITORY=o/r
for lag in 2 99
do
  : > "$STUB_DIR/reads"
  rc=0
  out=$(LAG=$lag sh "$script" rehearsal-delete rehearsal-1-1 42 2>&1) || rc=$?
  reads=$(wc -l < "$STUB_DIR/reads" | tr -d ' ')
  case $lag in
    2) want_rc=0 want_reads=3 ;;
    *) want_rc=1 want_reads=6 ;;
  esac
  if [ "$rc" -ne "$want_rc" ] || [ "$reads" -ne "$want_reads" ]
  then
    echo "policy.publish_delete: lag $lag exited $rc after $reads reads, want $want_rc after $want_reads: $out"
    exit 1
  fi
done
case $out in
  *"tag rehearsal-1-1 still exists"*) ;;
  *) echo "policy.publish_delete: a tag that never goes was not named: $out"; exit 1 ;;
esac
echo "policy.publish_delete: a lag of two reads passes, a tag that stays fails after six"
]=])
cogmaster_add_test(NAME policy.publish_delete
  COMMAND ${COGMASTER_SH} -c "${hygiene_delete_check}" sh ${CMAKE_SOURCE_DIR}/scripts/publish.sh ${hygiene_delete}
  LABELS hygiene policy TIMEOUT 30)

# Fault prevented: a CI required roll-up that passes a failed or wrongly
# skipped job, or a job missing from its needs. The self test runs it on a
# copy of ci.yml whose program ignores heavy, which must let a skipped build
# through and so fail the case that names it.
set(hygiene_ci ${CMAKE_SOURCE_DIR}/.github/workflows/ci.yml)
cogmaster_add_test(NAME policy.ci_required
  COMMAND ${CMAKE_COMMAND} -DCI_YML=${hygiene_ci} -DJQ=${COGMASTER_JQ}
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
  -DJQ=${COGMASTER_JQ} -P ${hygiene_scripts}/ci_required.cmake)
string(REPLACE ";" "$<SEMICOLON>" hygiene_cmd "${hygiene_cmd}")
cogmaster_add_test(NAME policy.ci_required.self_test
  COMMAND ${CMAKE_COMMAND} "-DCMD=${hygiene_cmd}" -DCODE=1
    "-DREGEX=build skipped when heavy=true: want fail, got exit 0"
    -P ${hygiene_scripts}/expect_fail.cmake
  LABELS hygiene policy TIMEOUT 30)

# Fault prevented: the Dependabot exception in CI required widened to every
# author. The self test runs on a copy of ci.yml whose author clause accepts
# any author, which must let a person's skipped rehearsal through and so fail
# the case that names it.
string(REPLACE "$actor == \"dependabot[bot]\"" "true" hygiene_any_actor "${hygiene_yml}")
if(hygiene_any_actor STREQUAL hygiene_yml)
  message(FATAL_ERROR "policy.ci_required.actor_self_test: ci.yml's program no longer tests $actor == \"dependabot[bot]\"")
endif()
file(WRITE ${CMAKE_BINARY_DIR}/hygiene/ci_required_actor_fixture/ci.yml "${hygiene_any_actor}")
set(hygiene_cmd ${CMAKE_COMMAND} -DCI_YML=${CMAKE_BINARY_DIR}/hygiene/ci_required_actor_fixture/ci.yml
  -DJQ=${COGMASTER_JQ} -P ${hygiene_scripts}/ci_required.cmake)
string(REPLACE ";" "$<SEMICOLON>" hygiene_cmd "${hygiene_cmd}")
cogmaster_add_test(NAME policy.ci_required.actor_self_test
  COMMAND ${CMAKE_COMMAND} "-DCMD=${hygiene_cmd}" -DCODE=1
    "-DREGEX=rehearsal skipped when a release input changed: want fail"
    -P ${hygiene_scripts}/expect_fail.cmake
  LABELS hygiene policy TIMEOUT 30)

# Fault prevented: a job timeout guessed rather than measured. Every
# timeout-minutes line carries twice its slowest measured run and that run's
# ID, or the five-minute floor for a workflow with no runs yet. The self test
# scans a fixture with two good lines and five bad ones.
cogmaster_add_test(NAME hygiene.timeout_comments
  COMMAND ${CMAKE_COMMAND} -DDIR=${CMAKE_SOURCE_DIR}/.github/workflows -DCHECK=timeouts -P ${hygiene_scripts}/repository.cmake
  LABELS hygiene TIMEOUT 30)
set(hygiene_timeouts ${CMAKE_BINARY_DIR}/hygiene/timeouts_fixture)
file(REMOVE_RECURSE ${hygiene_timeouts})
file(WRITE ${hygiene_timeouts}/sample.yml "jobs:\n  a:\n    timeout-minutes: 5  # 5-minute floor; no runs yet\n"
  "  b:\n    timeout-minutes: 8  # 2 x 180 s, slowest of 40 runs, run 38935647258\n"
  "  c:\n    timeout-minutes: 30\n  d:\n    timeout-minutes: 15  # a guess\n"
  "  e:\n    timeout-minutes: 6  # 2 x 3 s, slowest of 2 runs, run 38935647258, provisional\n"
  "  f:\n    timeout-minutes: 1  # 2 x 40 s, slowest of 12 runs, run 39336389305\n"
  "  g:\n    timeout-minutes: 3  # 5-minute floor; no runs yet\n")
set(hygiene_cmd ${CMAKE_COMMAND} -DDIR=${hygiene_timeouts} -DCHECK=timeouts -P ${hygiene_scripts}/repository.cmake)
string(REPLACE ";" "$<SEMICOLON>" hygiene_cmd "${hygiene_cmd}")
cogmaster_add_test(NAME hygiene.timeout_comments.self_test
  COMMAND ${CMAKE_COMMAND} "-DCMD=${hygiene_cmd}" -DCODE=1
    "-DREGEX=sample.yml: timeout-minutes: 30.*sample.yml: timeout-minutes: 15 .*a guess.*sample.yml: timeout-minutes: 6 .*provisional.*sample.yml: timeout-minutes: 1 .*sample.yml: timeout-minutes: 3 .*timeout_comments: 5 lines"
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

# Fault prevented: a weekly digest that names the wrong slowest job, misses a
# flaky run, or never checks a job's timeout margin because its YAML key
# differs from the name ci-durations prints (ci-required against CI required).
# The timeouts fixture holds that key-differs-from-name job and a matrix job.
set(hygiene_digest ${CMAKE_BINARY_DIR}/hygiene/digest_fixture)
file(REMOVE_RECURSE ${hygiene_digest})
file(WRITE ${hygiene_digest}/sample.yml [=[
name: Sample
on: push
jobs:
  ci-required:
    name: CI required
    timeout-minutes: 4  # 5-minute floor; no runs yet
    steps:
      - name: not a job name
        run: true
  build:
    name: build (${{ matrix.os }})
    timeout-minutes: 7
  plain:
    timeout-minutes: 9
]=])
set(hygiene_digest_timeouts [=[
sh "$1" timeouts "$2" > "$3" || exit 1
"$4" -e '."CI required" == 4 and .build == 7 and .plain == 9 and (has("ci-required") | not)' "$3"
]=])
cogmaster_add_test(NAME digest.timeouts
  COMMAND ${COGMASTER_SH} -c "${hygiene_digest_timeouts}" sh ${CMAKE_SOURCE_DIR}/scripts/digest.sh
    ${hygiene_digest}/sample.yml ${hygiene_digest}/timeouts.json ${COGMASTER_JQ}
  LABELS hygiene TIMEOUT 30)
set(hygiene_digest_format [=[
sh "$1" < "$2" > "$3" || exit 1
for want in '^Slowest job: build, 200 s, run 2' '^Flaky: build' '^- timeout margin: CI required' '^- nightly issue #12' '^attention: [1-9]'
do
  grep -Eq "$want" "$3" || { echo "digest.format: no line matching $want"; cat "$3"; exit 1; }
done
grep -Fq '#9 is red: `fix: name see http://evil.test/x x`' "$3" && ! grep -q '@name' "$3" || { echo "digest.format: a pull request title was not neutralised"; cat "$3"; exit 1; }
if grep -q 'timeout margin: build' "$3"
then
  echo "digest.format: build is inside its timeout margin but was flagged"
  exit 1
fi
]=])
cogmaster_add_test(NAME digest.format
  COMMAND ${COGMASTER_SH} -c "${hygiene_digest_format}" sh ${CMAKE_SOURCE_DIR}/scripts/digest.sh
    ${CMAKE_CURRENT_LIST_DIR}/fixtures/digest.json ${hygiene_digest}/digest.md
  LABELS hygiene TIMEOUT 30)

# Fault prevented: the digest reporter gaining write access or a schedule
# nobody asked for. The self test scans a fixture with a write-all workflow
# permission, a daily cron and a job permission outside the four allowed.
cogmaster_add_test(NAME policy.digest
  COMMAND ${CMAKE_COMMAND} -DDOC=${CMAKE_SOURCE_DIR}/.github/workflows/digest.yml -DCHECK=digest
    -P ${hygiene_scripts}/repository.cmake
  LABELS hygiene policy TIMEOUT 30)
file(WRITE ${hygiene_digest}/digest.yml [=[
on:
  schedule:
    - cron: "0 6 * * *"
permissions:
  contents: write
jobs:
  digest:
    permissions:
      actions: read
      checks: read
]=])
set(hygiene_cmd ${CMAKE_COMMAND} -DDOC=${hygiene_digest}/digest.yml -DCHECK=digest -P ${hygiene_scripts}/repository.cmake)
string(REPLACE ";" "$<SEMICOLON>" hygiene_cmd "${hygiene_cmd}")
cogmaster_add_test(NAME policy.digest.self_test
  COMMAND ${CMAKE_COMMAND} "-DCMD=${hygiene_cmd}" -DCODE=1
    "-DREGEX=digest: workflow permission contents: write.*digest: job permission checks: read.*digest: cron"
    -P ${hygiene_scripts}/expect_fail.cmake
  LABELS hygiene policy TIMEOUT 30)

include(${PROJECT_SOURCE_DIR}/tests/policy/e2e_cases.cmake)
include(${PROJECT_SOURCE_DIR}/tests/compat/tests.cmake)
