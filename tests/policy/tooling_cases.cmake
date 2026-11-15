# Owned fixtures and registrations for the download retries, the library dirty-tree refusal and the milestone-close
# check; no policy logic lives here. Included by the ci lane from tests/policy/tests.cmake, so every build leg and the
# wasm32 run register these controls and none downloads anything real: a refused local port, missing or tampered file://
# URLs, a fake curl, a fake gh and throwaway repositories. Every byte a shell reads is written with
# file(CONFIGURE ... NEWLINE_STYLE UNIX) or by the shell, never file(WRITE), which writes CRLF on Windows. Scripts
# handed to a shell through -c hold no semicolon, which the command list would split.
find_program(tooling_sh sh REQUIRED)
set(tooling ${PROJECT_BINARY_DIR}/policy/tooling)
file(REMOVE_RECURSE ${tooling})
file(MAKE_DIRECTORY ${tooling})
set(tooling_expect ${PROJECT_SOURCE_DIR}/tests/cmake/expect_fail.cmake)
set(tooling_mail fixture@users.noreply.github.com)
set(tooling_id GIT_AUTHOR_NAME=fixture GIT_AUTHOR_EMAIL=${tooling_mail} GIT_COMMITTER_NAME=fixture GIT_COMMITTER_EMAIL=${tooling_mail})

# Fault prevented: an assembler download that fails on one dropped connection, that retries a permanent error, or that
# accepts a tarball whose digest differs. The URL is a refused local port (curl code 7, retried to three attempts), a
# file that does not exist (code 37, not retried) and a local file under a wrong digest (checked once, after the loop).
# Timeout: 30 s, provisional; a refused connection to 127.0.0.1 takes about 2 s per attempt on Windows, so the transient
# case needs about 6 s there; local measurement, no hosted run yet.
string(REPEAT 0 64 tooling_zeros)
function(tooling_wla name url regex)
  set(cmd ${CMAKE_COMMAND} -DURL=${url} -DSHA256=${tooling_zeros} -DOUT=${tooling}/wla/${name}.tgz -DRETRY_DELAY=0
    -P ${PROJECT_SOURCE_DIR}/cmake/WlaDx.cmake)
  string(REPLACE ";" "$<SEMICOLON>" cmd "${cmd}")
  cogmaster_add_test(NAME policy.wla_fetch.${name}
    COMMAND ${CMAKE_COMMAND} "-DCMD=${cmd}" -DCODE=1 "-DREGEX=${regex}" -P ${tooling_expect}
    LABELS policy TIMEOUT 30)
endfunction()
# file:// plus the path, or file:/// plus the path when it does not start with a slash (a drive letter).
function(tooling_file_url out path)
  if(path MATCHES "^/")
    set(${out} "file://${path}" PARENT_SCOPE)
  else()
    set(${out} "file:///${path}" PARENT_SCOPE)
  endif()
endfunction()
file(MAKE_DIRECTORY ${tooling}/wla)
file(CONFIGURE OUTPUT ${tooling}/wla/tampered.txt CONTENT "not the pinned tarball\n" @ONLY NEWLINE_STYLE UNIX)
tooling_file_url(tooling_missing_url ${tooling}/wla/missing.tgz)
tooling_file_url(tooling_tampered_url ${tooling}/wla/tampered.txt)
tooling_wla(transient http://127.0.0.1:1/x "attempt[ \n]+3[ \n]+of[ \n]+3")
tooling_wla(permanent ${tooling_missing_url} "after[ \n]+attempt[ \n]+1[ \n]+of[ \n]+3")
tooling_wla(hash ${tooling_tampered_url} "hash[ \n]+mismatch[ \n]+after[ \n]+attempt[ \n]+1[ \n]+of[ \n]+3")

# Fault prevented: scripts/fetch-pin.sh failing a job on one dropped connection (curl exit 6 twice, then success), or
# retrying an error that cannot pass (exit 22, an HTTP error status). A fake curl earlier on PATH counts its calls in a
# file beside itself; the wrapper clears the count first, so a second ctest run without reconfiguring counts afresh, and
# prints it with fetch-pin's own exit status.
# Timeout: 30 s, provisional; the fake curl and a zero retry delay make a run take well under a second; local
# measurement, no hosted run yet.
set(tooling_payload "tool payload\n")
string(SHA256 tooling_digest "${tooling_payload}")
set(tooling_curl_head [=[
#!/bin/sh
d=$(dirname "$0")
while [ $# -gt 0 ]
do
  [ "$1" != -o ] || { out=$2 ; shift ; }
  shift
done
echo call >> "$d/calls"
]=])
set(tooling_curl_retry [=[
[ "$(($(wc -l < "$d/calls")))" -gt 2 ] || exit 6
printf 'tool payload\n' > "$out"
]=])
set(tooling_curl_permanent "exit 22\n")
set(tooling_pins_run [=[
cd "$1" && [ -d shim ] && rm -rf shim/calls dest || exit 1
PATH="$PWD/shim:$PATH" PINS=pins.txt FETCH_PIN_RETRY_DELAY=0 sh "$2" payload dest
rc=$?
echo "calls=$(($(wc -l < shim/calls)))"
exit "$rc"
]=])
function(tooling_pins variant)
  set(dir ${tooling}/pins_${variant})
  file(MAKE_DIRECTORY ${dir}/shim)
  file(CONFIGURE OUTPUT ${dir}/shim/curl CONTENT "${tooling_curl_head}${tooling_curl_${variant}}" @ONLY NEWLINE_STYLE UNIX)
  file(CHMOD ${dir}/shim/curl PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE
    WORLD_READ WORLD_EXECUTE)
  file(CONFIGURE OUTPUT ${dir}/pins.txt CONTENT "payload 1 https://example.invalid/payload.txt ${tooling_digest}\n"
    @ONLY NEWLINE_STYLE UNIX)
  set(cmd ${tooling_sh} -c "${tooling_pins_run}" sh ${dir} ${PROJECT_SOURCE_DIR}/scripts/fetch-pin.sh)
  string(REPLACE ";" "$<SEMICOLON>" cmd "${cmd}")
  set(code 0)
  set(regex "calls=[ ]*3")
  if(variant STREQUAL permanent)
    set(code 1)
    set(regex "calls=[ ]*1")
  endif()
  # The retry control also requires exit 0, which a PASS_REGULAR_EXPRESSION registration would ignore.
  cogmaster_add_test(NAME policy.pins.${variant}
    COMMAND ${CMAKE_COMMAND} "-DCMD=${cmd}" -DCODE=${code} "-DREGEX=${regex}" -P ${tooling_expect}
    LABELS policy TIMEOUT 30)
endfunction()
tooling_pins(retry)
tooling_pins(permanent)

# Fault prevented: a library run that stamps core= from a tree that is not a commit, and a refusal that fires on a
# .planning edit, an untracked file or a clean tree. Four throwaway repositories, one per case so parallel runs share
# nothing, each committed clean and holding the script, its helper, a header-only ledger at the tracked path, a tracked
# file and a tracked .planning file. The change is applied at test time after a reset, so a rerun is not polluted by what
# an earlier run wrote. The controls pass when the run was not refused; it may fail later for lack of a real library,
# which is not what they assert.
# Timeout: 60 s, provisional; a run over an empty directory starts a handful of shell and cmake processes, which take
# seconds on Windows; local measurement, no hosted run yet.
set(tooling_empty ${tooling}/empty)
file(MAKE_DIRECTORY ${tooling_empty})
file(STRINGS ${PROJECT_SOURCE_DIR}/tests/compat/ledger.csv tooling_header LIMIT_COUNT 1)
set(tooling_dirty_run [=[
cd "$1" && [ -d .git ] || exit 1
git checkout -q -- .
rm -rf build new.txt
if [ "$2" = refuses ]
then echo changed >> notes.txt
fi
if [ "$2" = planning ]
then echo changed >> .planning/x
fi
if [ "$2" = untracked ]
then echo new > new.txt
fi
sh scripts/library.sh run
rc=$?
echo "library exit $rc"
exit "$rc"
]=])
# The repositories hold copies, so an edit to any of these scripts must reconfigure before the controls run.
set_property(DIRECTORY ${PROJECT_SOURCE_DIR} APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
  ${PROJECT_SOURCE_DIR}/scripts/library.sh ${PROJECT_SOURCE_DIR}/scripts/compat-common.sh
  ${PROJECT_SOURCE_DIR}/scripts/milestone-close.sh)
function(tooling_dirty case)
  set(repo ${tooling}/dirty_${case})
  file(COPY ${PROJECT_SOURCE_DIR}/scripts/library.sh ${PROJECT_SOURCE_DIR}/scripts/compat-common.sh DESTINATION ${repo}/scripts)
  file(CONFIGURE OUTPUT ${repo}/tests/compat/ledger.csv CONTENT "${tooling_header}\n" @ONLY NEWLINE_STYLE UNIX)
  file(CONFIGURE OUTPUT ${repo}/notes.txt CONTENT "notes\n" @ONLY NEWLINE_STYLE UNIX)
  file(CONFIGURE OUTPUT ${repo}/.planning/x CONTENT "plan\n" @ONLY NEWLINE_STYLE UNIX)
  execute_process(COMMAND ${GIT_EXECUTABLE} init -q WORKING_DIRECTORY ${repo} COMMAND_ERROR_IS_FATAL ANY)
  execute_process(COMMAND ${GIT_EXECUTABLE} config core.autocrlf false WORKING_DIRECTORY ${repo} COMMAND_ERROR_IS_FATAL ANY)
  execute_process(COMMAND ${GIT_EXECUTABLE} add -A WORKING_DIRECTORY ${repo} COMMAND_ERROR_IS_FATAL ANY)
  execute_process(COMMAND ${CMAKE_COMMAND} -E env ${tooling_id} ${GIT_EXECUTABLE} -c commit.gpgsign=false commit -qm base
    WORKING_DIRECTORY ${repo} COMMAND_ERROR_IS_FATAL ANY)
  # The default, tracked ledger path must be the one in use, and results stay under the repository's own build directory.
  set(cmd ${CMAKE_COMMAND} -E env --unset=COGMASTER_LEDGER --unset=COGMASTER_PAGE --unset=COGMASTER_CATALOG
    --unset=COGMASTER_LIBRARY_OUT COGMASTER_ROM_DIR=${tooling_empty} ${tooling_sh} -c "${tooling_dirty_run}" sh ${repo} ${case})
  if(case STREQUAL refuses)
    string(REPLACE ";" "$<SEMICOLON>" cmd "${cmd}")
    cogmaster_add_test(NAME policy.library_dirty.refuses
      COMMAND ${CMAKE_COMMAND} "-DCMD=${cmd}" -DCODE=2 "-DREGEX=tracked changes outside[ \n]+[.]planning" -P ${tooling_expect}
      LABELS policy TIMEOUT 60)
  else()
    cogmaster_add_test(NAME policy.library_dirty.${case} COMMAND ${cmd} LABELS policy TIMEOUT 60)
    set_tests_properties(policy.library_dirty.${case} PROPERTIES
      PASS_REGULAR_EXPRESSION "library exit [0-9]+" FAIL_REGULAR_EXPRESSION "tracked changes outside")
  endif()
endfunction()
foreach(tooling_case IN ITEMS refuses planning untracked clean)
  tooling_dirty(${tooling_case})
endforeach()

# Fault prevented: a milestone-close check that reports all ok over a dirty tree or an off remote state, or that turns a
# failed gh call into ok. One throwaway repository holds a copy of the script, a notes file and the release manifest, with
# a remote-tracking main at its commit. A fake gh outside it answers every request already filtered, so the filters the
# real gh applies are not run here, and refuses a request it does not know, so a request the script adds without an
# answer turns the baseline into unknown. The driver resets the tree, applies one change, runs the script with the fake
# first on PATH and asserts the exit status and the row by tolerant pattern.
# Timeout: 60 s; eleven cases of a few git and sh processes each, seconds on Windows; local measurement, no hosted run yet.
set(tooling_close ${tooling}/close)
set(tooling_close_gh [=[
#!/bin/sh
c=${CLOSE_CASE-}
[ "$c" != offline ] || exit 1
[ "$1" != repo ] || { echo o/r ; exit 0 ; }
p=
for a
do
  case $a in repos/* | search/*) p=$a ;; esac
done
case $p in
  repos/o/r/commits/main) git rev-parse HEAD ;;
  repos/o/r/commits/v*) echo success ;;
  repos/o/r/commits/*) case $c in red_ci) echo failure ;; pending) echo in_progress ;; *) echo success ;; esac ;;
  repos/o/r/pulls?*) [ "$c" = open_pr ] && echo 1 || echo 0 ;;
  repos/o/r/releases/latest) [ "$c" = release_tag ] && echo v0.0.9 || echo v0.1.0 ;;
  repos/o/r/releases) [ "$c" != draft ] || echo 123 ;;
  repos/o/r/compare/*) [ "$c" = unreleased ] && echo 2 || echo 0 ;;
  search/issues*) [ "$c" = release_pr ] && echo 1 || echo 0 ;;
  *) exit 1 ;;
esac
exit 0
]=])
set(tooling_close_run [=[
cd "$1" && [ -d .git ] || exit 1
PATH=$(cd "$2" && pwd):$PATH
export PATH
fail=0
# check <case> <exit status> <row pattern> [second pattern]
check() {
  git checkout -q -- .
  rm -f new.txt
  case $1 in
    tracked) echo changed >> notes.txt ;;
    untracked) echo new > new.txt ;;
  esac
  rc=0
  out=$(CLOSE_CASE=$1 sh scripts/milestone-close.sh 2>&1) || rc=$?
  if [ "$rc" -eq "$2" ] && printf '%s\n' "$out" | grep -Eq "$3" && printf '%s\n' "$out" | grep -Eq "${4:-.}"
  then
    echo "case $1 ok"
  else
    echo "case $1 FAILED $rc"
    printf '%s\n' "$out"
    fail=1
  fi
}
check baseline 0 '^close-check: all ok$'
check tracked 1 '^tree +off'
check untracked 1 '^tree +off'
check open_pr 1 '^open-prs +off'
check red_ci 1 '^main-ci +off'
check release_tag 1 '^release +off'
check draft 1 '^release +off'
check release_pr 1 '^release-pr +off'
check unreleased 1 '^unreleased +off'
check pending 2 '^main-ci +unknown' 'close-check: 0 off, 1 unknown'
check offline 2 '^open-prs +unknown' 'close-check: 0 off,'
exit "$fail"
]=])
set(tooling_close_repo ${tooling_close}/repo)
file(COPY ${PROJECT_SOURCE_DIR}/scripts/milestone-close.sh DESTINATION ${tooling_close_repo}/scripts)
file(CONFIGURE OUTPUT ${tooling_close_repo}/notes.txt CONTENT "notes\n" @ONLY NEWLINE_STYLE UNIX)
file(CONFIGURE OUTPUT ${tooling_close_repo}/.release-please-manifest.json CONTENT "{\n  \".\": \"0.1.0\"\n}\n"
  @ONLY NEWLINE_STYLE UNIX)
execute_process(COMMAND ${GIT_EXECUTABLE} init -q WORKING_DIRECTORY ${tooling_close_repo} COMMAND_ERROR_IS_FATAL ANY)
execute_process(COMMAND ${GIT_EXECUTABLE} symbolic-ref HEAD refs/heads/main
  WORKING_DIRECTORY ${tooling_close_repo} COMMAND_ERROR_IS_FATAL ANY)
execute_process(COMMAND ${GIT_EXECUTABLE} config core.autocrlf false WORKING_DIRECTORY ${tooling_close_repo} COMMAND_ERROR_IS_FATAL ANY)
execute_process(COMMAND ${GIT_EXECUTABLE} add -A WORKING_DIRECTORY ${tooling_close_repo} COMMAND_ERROR_IS_FATAL ANY)
execute_process(COMMAND ${CMAKE_COMMAND} -E env ${tooling_id} ${GIT_EXECUTABLE} -c commit.gpgsign=false commit -qm base
  WORKING_DIRECTORY ${tooling_close_repo} COMMAND_ERROR_IS_FATAL ANY)
# A remote-tracking main at the commit lets git show origin/main:... answer without a remote.
execute_process(COMMAND ${GIT_EXECUTABLE} update-ref refs/remotes/origin/main HEAD
  WORKING_DIRECTORY ${tooling_close_repo} COMMAND_ERROR_IS_FATAL ANY)
file(CONFIGURE OUTPUT ${tooling_close}/shim/gh CONTENT "${tooling_close_gh}" @ONLY NEWLINE_STYLE UNIX)
file(CHMOD ${tooling_close}/shim/gh PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE
  WORLD_READ WORLD_EXECUTE)
file(CONFIGURE OUTPUT ${tooling_close}/run.sh CONTENT "${tooling_close_run}" @ONLY NEWLINE_STYLE UNIX)
cogmaster_add_test(NAME policy.milestone_close
  COMMAND ${tooling_sh} ${tooling_close}/run.sh ${tooling_close_repo} ${tooling_close}/shim
  LABELS policy TIMEOUT 60)
