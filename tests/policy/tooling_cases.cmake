# Owned fixtures and registrations for the download retries and the library dirty-tree refusal; no policy logic lives
# here. Included by the ci lane from tests/policy/tests.cmake, so every build leg and the wasm32 run register these
# controls and none downloads anything real: a refused local port, missing or tampered file:// URLs, a fake curl and
# throwaway repositories. Every byte a shell reads is written with file(CONFIGURE ... NEWLINE_STYLE UNIX) or by the
# shell, never file(WRITE), which writes CRLF on Windows. Scripts handed to a shell through -c hold no semicolon, which
# the command list would split.
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
