# Owned fixtures and registrations for the firmware window scan and its hook; no scanning logic lives here, and this file
# is included by the hygiene lane only. Nothing here is a real firmware image: the dump is byte i = (i * 37 + 11) mod 256
# over 8 KiB, so every 32-byte window holds 32 distinct values (the step is odd), and it repeats every 256 bytes.
set(fwin ${PROJECT_BINARY_DIR}/policy/fwin)
file(REMOVE_RECURSE ${fwin})
file(MAKE_DIRECTORY ${fwin}/dump ${fwin}/dump_end ${fwin}/dump_low)
# The directory variable's name is built in pieces so no file under tests holds it.
set(fwin_var COGMASTER_BIOS)
string(APPEND fwin_var _DIR)
set(fwin_mail fixture@users.noreply.github.com)
set(fwin_id GIT_AUTHOR_NAME=fixture GIT_AUTHOR_EMAIL=${fwin_mail} GIT_COMMITTER_NAME=fixture GIT_COMMITTER_EMAIL=${fwin_mail})
# Bytes are made with the shell (never file(WRITE), which would write CRLF on Windows). $1 is the output, $2 the dump.
function(fwin_sh script)
  execute_process(COMMAND ${CMAKE_COMMAND} -E env ${fwin_id} ${COGMASTER_SH} -c "${script}" sh ${ARGN} COMMAND_ERROR_IS_FATAL ANY)
endfunction()
function(fwin_dump path split) # from byte split on, step 91 instead of 37, so the last window is unique when split < 8192
  fwin_sh([=[printf "$(awk -v k="$2" 'BEGIN{for(i=0;i<8192;i++)printf "\\%03o",(i<k?i*37+11:i*91+3)%256}')" > "$1"]=] ${path} ${split})
endfunction()
function(fwin_make name script)
  fwin_sh("${script}" ${fwin}/${name} ${fwin}/dump/own.bin)
endfunction()
fwin_dump(${fwin}/dump/own.bin 8192)
fwin_dump(${fwin}/dump_end/end.bin 8160)
fwin_sh([=[head -c 64 /dev/zero | tr '\000' A > "$1"]=] ${fwin}/dump_low/low.bin)
# Slices of the dump start at byte 101: its neighbours 0x7f (before) and 0x1f (after 31 bytes) are not 'x', so the
# 31-byte control cannot grow into a 32-byte window through its bracketing bytes.
fwin_make(hit.bin [=[{ printf odd; tail -c +102 "$2" | head -c 64; printf x; } > "$1"]=])
fwin_make(array.txt [=[{ echo 'static const unsigned char t[] = {'; tail -c +102 "$2" | head -c 64 | od -An -v -tx1 | sed 's/ \([0-9a-f][0-9a-f]\)/0x\1,/g'; echo '};'; } > "$1"]=])
fwin_make(w32.bin [=[{ printf x; tail -c +102 "$2" | head -c 32; printf x; } > "$1"]=])
fwin_make(w31.bin [=[{ printf x; tail -c +102 "$2" | head -c 31; printf x; } > "$1"]=])
fwin_make(clean.txt [=[printf 'owned clean text\n' > "$1"]=])
fwin_sh([=[head -c 40 /dev/zero | tr '\000' A > "$1"]=] ${fwin}/low.bin)
fwin_sh([=[tail -c 32 "$2" > "$1"]=] ${fwin}/end.bin ${fwin}/dump_end/end.bin)
function(fwin_repo name mode) # a throwaway repository with both scripts; staged or commit holds array.txt
  file(COPY ${PROJECT_SOURCE_DIR}/scripts/hygiene.sh ${PROJECT_SOURCE_DIR}/scripts/firmware-window.sh DESTINATION ${fwin}/${name}/scripts)
  file(COPY ${fwin}/array.txt DESTINATION ${fwin}/${name})
  fwin_sh([=[cd "$1" && git init -q && { [ "$2" = none ] || git add array.txt; } && { [ "$2" != commit ] || git -c commit.gpgsign=false commit -qm window; }]=] ${fwin}/${name} ${mode})
endfunction()
fwin_repo(unset none)
fwin_repo(staged staged)
fwin_repo(history commit)
# Fault prevented: a control that passes on an unrelated error, or a scan that reports more than a target's name. The
# failing controls match the whole output, so any extra byte, offset, window or dump path fails them.
function(fwin_case name mode regex)
  string(REPLACE ";" "$<SEMICOLON>" cmd "${ARGN}")
  # Timeout: 20 s floor; 2 x 0.16 s, slowest of 30 runs, run 43049044281.
  if(mode STREQUAL FAIL)
    cogmaster_add_test(NAME policy.firmware_window.${name} LABELS hygiene policy TIMEOUT 20
      COMMAND ${CMAKE_COMMAND} "-DCMD=${cmd}" -DCODE=1 "-DREGEX=${regex}" -P ${policy_scripts}/expect_fail.cmake)
  else()
    cogmaster_add_test(NAME policy.firmware_window.${name} COMMAND ${ARGN} LABELS hygiene policy TIMEOUT 20)
    set_tests_properties(policy.firmware_window.${name} PROPERTIES PASS_REGULAR_EXPRESSION "${regex}")
  endif()
endfunction()
set(fwin_line "^firmware window: dump 1 \\(8192 bytes\\) matches [^ \n]+\n$")
set(fwin_hook "firmware window: dump 1 \\(8192 bytes\\) matches")
set(fwin_scan ${COGMASTER_SH} ${PROJECT_SOURCE_DIR}/scripts/firmware-window.sh)
fwin_case(hit FAIL "${fwin_line}" ${fwin_scan} ${fwin}/dump ${fwin}/hit.bin)
fwin_case(array FAIL "${fwin_line}" ${fwin_scan} ${fwin}/dump ${fwin}/array.txt)
fwin_case(w32 FAIL "${fwin_line}" ${fwin_scan} ${fwin}/dump ${fwin}/w32.bin)
fwin_case(end FAIL "${fwin_line}" ${fwin_scan} ${fwin}/dump_end ${fwin}/end.bin)
fwin_case(w31 PASS "firmware window: 1 compared" ${fwin_scan} ${fwin}/dump ${fwin}/w31.bin)
fwin_case(low_entropy PASS "firmware window: 1 compared" ${fwin_scan} ${fwin}/dump_low ${fwin}/low.bin)
fwin_case(clean PASS "firmware window: 1 compared" ${fwin_scan} ${fwin}/dump ${fwin}/clean.txt)
# The scan derives the directory from the environment or the repository's .env.local, so the unset control runs in a
# throwaway repository with the variable removed.
fwin_case(unset PASS "no dump configured, 0 compared" ${CMAKE_COMMAND} -E env --unset=${fwin_var} ${fwin_id}
  ${COGMASTER_SH} -c [=[cd "$1" && sh scripts/firmware-window.sh]=] sh ${fwin}/unset)
# Fault prevented: the pre-commit and pre-push hooks not running the window scan. Each branch of hygiene.sh runs in a
# throwaway repository whose array.txt holds a window, staged for the first and committed for the second.
set(fwin_hygiene ${CMAKE_COMMAND} -E env ${fwin_id} ${fwin_var}=${fwin}/dump ${COGMASTER_SH} -c [=[cd "$1" && shift && sh scripts/hygiene.sh "$@"]=] sh)
fwin_case(staged FAIL "${fwin_hook}" ${fwin_hygiene} ${fwin}/staged --staged)
fwin_case(history FAIL "${fwin_hook}" ${fwin_hygiene} ${fwin}/history --history HEAD)
