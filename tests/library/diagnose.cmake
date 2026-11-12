# Fault prevented: a diagnose line outside the closed grammar, a pass row printed, a cause group missing, or the ledger or page rewritten.
string(REPLACE ";" "\\;" emulator "${EMULATOR}")
set(diag_env "COGMASTER_LIBRARY_RUN=${RUN}" "COGMASTER_LIBRARY_EMULATOR=${emulator}" "COGMASTER_LIBRARY_OUT=${OUT}/diag"
  "COGMASTER_CATALOG=${OUT}/catalog.tsv" "COGMASTER_LEDGER=${OUT}/standin/ledger.csv" "COGMASTER_PAGE=${OUT}/standin/page.md")
macro(diag_call dir)  # extra NAME=value settings in ARGN win; CR is removed so the ci-msvc lanes read the same text as the others
  execute_process(COMMAND "${CMAKE_COMMAND}" -E env ${diag_env} "COGMASTER_ROM_DIR=${dir}" ${ARGN}
    sh "${ROOT}/scripts/library.sh" diagnose WORKING_DIRECTORY "${ROOT}" RESULT_VARIABLE diag_rc OUTPUT_VARIABLE diag_out ERROR_VARIABLE diag_err)
  string(REPLACE "\r" "" diag_out "${diag_out}")
  string(REPLACE "\r" "" diag_err "${diag_err}")
endmacro()
function(diag_expect text)  # the first listed token the text lacks fails by name
  foreach(token IN LISTS ARGN)
    string(FIND "${text}" "${token}" at)
    if(at EQUAL -1)
      message(FATAL_ERROR "diagnose output lacks: ${token}\n${text}")
    endif()
  endforeach()
endfunction()
if(CASE STREQUAL main)
  file(REMOVE_RECURSE "${OUT}")
  file(MAKE_DIRECTORY "${OUT}/images" "${OUT}/standin")
  string(REPEAT v 32768 halt)
  file(WRITE "${OUT}/halt.sms" "${halt}x")  # one byte past a power of two is not a copier-header size
  set(images "never.sms|${OUT}/halt.sms|unknown|-|Synthetic never draws" "pass.sms|${FIXTURES}/boot_to_play.sms|unknown|-|Synthetic pass"
    "port.sms|${SEEDS}/in-a-00.sms|unknown|-|Synthetic port fault" "static.sms|${SEEDS}/static-display.sms|unknown|-|Synthetic static"
    "skipped.sms|${SEEDS}/jr-self.sms|skipped|mame_no|Synthetic skipped")  # name|source|catalog status|reason|title
  foreach(row IN LISTS images)
    string(REPLACE "|" ";" row "${row}")
    list(POP_FRONT row name source status reason title)
    file(COPY_FILE "${source}" "${OUT}/images/${name}")
    execute_process(COMMAND ${EMULATOR} "${RUN}" --probe "${OUT}/images/${name}" OUTPUT_VARIABLE probe ERROR_QUIET)
    if(NOT probe MATCHES "mapper_guess=none")  # a probe change fails by name here, not as a board mismatch in the output
      message(FATAL_ERROR "diagnose fixture board: ${name}")
    endif()
    file(SHA1 "${OUT}/images/${name}" sha)
    file(APPEND "${OUT}/catalog.tsv" "${sha}\tsms\tnone\t-\t${title}\t${status}\t${reason}\texport\tpad\tpad\n")
  endforeach()
  file(COPY_FILE "${SEEDS}/ed-00.sms" "${OUT}/images/cpu.sms")  # not in the catalog, so never run
  file(WRITE "${OUT}/header.csv" "id,title,machine,region,tv,mapper,predicate,status,evidence,date\n")
  execute_process(COMMAND sh "${ROOT}/scripts/compat-seed.sh" "${OUT}/header.csv" "${OUT}/catalog.tsv" 2026-11-10
    OUTPUT_FILE "${OUT}/standin/ledger.csv" ERROR_QUIET COMMAND_ERROR_IS_FATAL ANY)
  file(WRITE "${OUT}/standin/page.md" "stand-in page\n")  # the ledger is the default rows for the catalog; the page is any text
  file(SHA256 "${OUT}/standin/ledger.csv" ledger0)
  file(SHA256 "${OUT}/standin/page.md" page0)
  diag_call("${OUT}/images")
  file(READ "${OUT}/diag/diagnose.txt" kept)
  string(REPLACE "\r" "" kept "${kept}")
  if(NOT diag_rc STREQUAL "0" OR NOT diag_out STREQUAL kept)
    message(FATAL_ERROR "diagnose must exit 0 and retain exactly what it prints: ${diag_rc}\n${diag_out}${diag_err}")
  endif()
  string(CONCAT shape "^(library-diagnose 1 core=[0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f](\\+dirty)? behaviour=[0-9]+"  # four anchored shapes with distinct prefixes: a line matches at most one, and must match one
    "|diag: title=[^\n]* group=[a-z_.]+ first_display=([0-9]+|none) first_nonblank=([0-9]+|none) distinct600=[1-8] distinct1800=[1-8] fault=[a-z_]+"
    " dev=[a-z]+ pc=(none|[0-9A-F]+) mapper=[a-z0-9_]+ first_change=([0-9]+|none) fault_frame=(0|1-9|10-99|100-599|600\\+|none)"
    " order=(before|after|nopicture|none) where=[a-z0-9]+ ram=(live|still) source=[a-z]+"
    "|diag-group: leaf=[a-z_.]+ (pcslot=[0-4] |where=[a-z0-9]+ |pc=[0-9A-F]+ )?rows=[0-9]+|diag-other: [a-z_0-9]+=[0-9]+( [a-z_0-9]+=[0-9]+)*)$")
  string(REGEX MATCHALL "[^\n]+" lines "${diag_out}")
  list(FILTER lines EXCLUDE REGEX "${shape}")
  if(lines)
    message(FATAL_ERROR "diagnose line outside the closed grammar: ${lines}")
  endif()
  diag_expect("${diag_out}" "diag: title=Synthetic never draws group=dark_no_fault.never_draws "
    "diag: title=Synthetic port fault group=fault_no_display.unmapped_read.ctrl " "diag: title=Synthetic static group=static_no_fault.frozen "
    "diag-group: leaf=dark_no_fault.never_draws rows=1" "diag-group: leaf=fault_no_display.unmapped_read.ctrl rows=1"
    "diag-group: leaf=static_no_fault.frozen rows=1" "diag-group: leaf=input_mismatch rows=0"
    "leaf=fault_no_display.unmapped_read.ctrl pcslot=0 rows=1" "leaf=fault_no_display.unmapped_read.ctrl where=port rows=1"
    "candidates=6 attempted=4 evaluated=4 malformed=0 malformed_io=0 uncatalogued=1 skipped_excluded=1 noprofile=0 unsupported=0 infrastructure_error=0 title_withheld=0"
    "judged=4 run_fail=3 " "groups_total=3 fail_total=3")
  if(diag_out MATCHES "Synthetic (pass|skipped)")
    message(FATAL_ERROR "a pass row and an excluded row must not be printed")
  endif()
  diag_call("${OUT}/missing")  # no existing directory: a fixed text, exit 2, nothing named
  if(NOT diag_rc STREQUAL "2" OR NOT diag_out STREQUAL "" OR NOT diag_err MATCHES "diagnose needs COGMASTER_ROM_DIR" OR diag_err MATCHES "missing")
    message(FATAL_ERROR "a missing directory must exit 2 with a fixed text: ${diag_rc} ${diag_out}${diag_err}")
  endif()
  diag_call("${OUT}/images")  # a second call repeats itself and leaves the ledger and page alone; run over the same inputs changes them
  file(SHA256 "${OUT}/standin/ledger.csv" ledger1)
  file(SHA256 "${OUT}/standin/page.md" page1)
  if(NOT diag_out STREQUAL kept OR NOT ledger1 STREQUAL ledger0 OR NOT page1 STREQUAL page0)
    message(FATAL_ERROR "diagnose must repeat itself and leave the ledger and page unchanged")
  endif()
  execute_process(COMMAND "${CMAKE_COMMAND}" -E env ${diag_env} "COGMASTER_ROM_DIR=${OUT}/images" "COGMASTER_LIBRARY_OUT=${OUT}/run-results"
    sh "${ROOT}/scripts/library.sh" run WORKING_DIRECTORY "${ROOT}" RESULT_VARIABLE rc OUTPUT_QUIET ERROR_QUIET)
  file(SHA256 "${OUT}/standin/ledger.csv" ledger2)
  file(SHA256 "${OUT}/standin/page.md" page2)
  if(NOT rc STREQUAL "0" OR (ledger2 STREQUAL ledger0 AND page2 STREQUAL page0))
    message(FATAL_ERROR "the control run must change the stand-in ledger or page: ${rc}")
  endif()
elseif(CASE STREQUAL taxonomy)  # canned runner lines through the real library.sh and library.cmake: no emulation
  file(REMOVE_RECURSE "${OUT}")
  file(MAKE_DIRECTORY "${OUT}/standin")
  file(WRITE "${OUT}/wrapper.sh" [[for a do last=$a; done
case $1 in --probe) cat "$last.probe" ;; *) cat "$last.line" ;; esac
]])
  file(WRITE "${OUT}/standin/ledger.csv" "id,title,machine,region,tv,mapper,predicate,status,evidence,date\n")
  set(f1 "first_display=none first_nonblank=none first_change=none distinct600=1 distinct1800=1 ram=live")
  set(f2 "first_display=100 first_nonblank=100 first_change=100")
  set(rows  # sub dir|rows|id digit|catalog board|title|canned line
    "main|5|1|none|Shared title|diagnose 1 verdict600=fail ${f1} fault=unmapped_read dev=ctrl pc=0002 pcslot=0 where=port fault_frame=1-9 order=nopicture mapper=none source=db"
    "main|4|2|none|T2|diagnose 1 verdict600=fail ${f2} distinct600=1 distinct1800=1 ram=live fault=rom_write dev=cart pc=0100 pcslot=1 where=rom fault_frame=10-99 order=after mapper=none source=db"
    "main|1|3|none|T3|diagnose 1 verdict600=fail ${f2} distinct600=3 distinct1800=3 ram=live fault=mapper_range dev=mapper pc=FFFC pcslot=4 where=mapper fault_frame=100-599 order=after mapper=none source=default"
    "main|1|4|none|T4|diagnose 1 verdict600=fail first_display=none first_nonblank=300 first_change=300 distinct600=1 distinct1800=1 ram=live fault=none dev=none pc=none pcslot=none where=none fault_frame=none order=none mapper=none source=heuristic"
    "main|1|5|none|T5|diagnose 1 verdict600=fail first_display=none first_nonblank=900 first_change=900 distinct600=1 distinct1800=1 ram=live fault=none dev=none pc=none pcslot=none where=none fault_frame=none order=none mapper=none source=db"
    "main|1|6|none|T6|diagnose 1 verdict600=fail ${f1} fault=none dev=none pc=none pcslot=none where=none fault_frame=none order=none mapper=none source=db"
    "main|1|7|none|T7|diagnose 1 verdict600=fail ${f1} fault=unmapped_write dev=ram pc=none pcslot=none where=none fault_frame=600+ order=after mapper=none source=db"
    "main|1|8|none|T8|diagnose 1 verdict600=fail ${f2} distinct600=1 distinct1800=2 ram=live fault=none dev=none pc=none pcslot=none where=none fault_frame=none order=none mapper=none source=db"
    "main|1|9|none|T9|diagnose 1 verdict600=fail ${f2} distinct600=1 distinct1800=1 ram=still fault=none dev=none pc=none pcslot=none where=none fault_frame=none order=none mapper=none source=db"
    "main|1|a|none|Ta|diagnose 1 verdict600=fail ${f2} distinct600=1 distinct1800=1 ram=live fault=none dev=none pc=none pcslot=none where=none fault_frame=none order=none mapper=none source=db"
    "main|1|b|sega|Tb|diagnose 1 verdict600=fail ${f2} distinct600=2 distinct1800=2 ram=live fault=none dev=none pc=none pcslot=none where=none fault_frame=none order=none mapper=none source=db"
    "main|1|c|none|Tc|diagnose 1 verdict600=pass first_display=100 first_nonblank=100 first_change=75 distinct600=2 distinct1800=2 ram=live fault=none dev=none pc=none pcslot=none where=none fault_frame=none order=none mapper=none source=db")
  foreach(row IN LISTS rows)
    string(REPLACE "|" ";" row "${row}")
    list(POP_FRONT row sub n digit board title line)
    string(REPEAT ${digit} 39 prefix)
    file(MAKE_DIRECTORY "${OUT}/${sub}")
    foreach(i RANGE 1 ${n})
      file(WRITE "${OUT}/${sub}/${digit}${i}.sms" "x")
      file(WRITE "${OUT}/${sub}/${digit}${i}.sms.probe" "identity_sha1=${prefix}${i}\nregion_guess=ntsc-u\n")
      file(WRITE "${OUT}/${sub}/${digit}${i}.sms.line" "${line}\n")
      file(APPEND "${OUT}/catalog.tsv" "${prefix}${i}\tsms\t${board}\t-\t${title}\tunknown\t-\texport\tpad\tpad\n")
    endforeach()
  endforeach()
  diag_call("${OUT}/main" "COGMASTER_LIBRARY_RUN=${OUT}/wrapper.sh" COGMASTER_LIBRARY_EMULATOR=sh)
  if(NOT diag_rc STREQUAL "0")
    message(FATAL_ERROR "canned lines must group completely: ${diag_rc}\n${diag_out}${diag_err}")
  endif()
  set(l1 "diag-group: leaf=fault_no_display.unmapped_read.ctrl")
  set(l2 "diag-group: leaf=fault_static.rom_write.cart")
  diag_expect("${diag_out}" "${l1} rows=5\n${l1} pcslot=0 rows=5\n${l1} where=port rows=5\n${l1} pc=0002 rows=5\n"
    "${l2} rows=4\n${l2} pcslot=1 rows=4\n${l2} where=rom rows=4\ndiag-group: leaf=dark_no_fault.never_draws rows=2\n")
  if(diag_out MATCHES "diag-group: [^\n]*pc=0100")
    message(FATAL_ERROR "an exact pc shared by 4 rows must not print")
  endif()
  string(REGEX MATCHALL "diag-group: leaf=[a-z_.]+ rows=[0-9]+" leaves "${diag_out}")
  list(TRANSFORM leaves REPLACE "^diag-group: leaf=([a-z_.]+) rows=.*$" "\\1")
  set(order fault_no_display.unmapped_read.ctrl fault_static.rom_write.cart dark_no_fault.never_draws input_mismatch
    fault_alive.mapper_range.mapper dark_no_fault.latch_miss dark_no_fault.late static_no_fault.animates_late static_no_fault.frozen
    static_no_fault.alive_still)
  if(NOT leaves STREQUAL order)
    message(FATAL_ERROR "leaf order: ${leaves}")
  endif()
endif()
