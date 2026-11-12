# Fault prevented: local-library coverage silently skips images absent from metadata.
if(CMAKE_SCRIPT_MODE_FILE)
  execute_process(COMMAND "${CMAKE_COMMAND}" --list-presets=workflow
    WORKING_DIRECTORY "${ROOT}" RESULT_VARIABLE rc OUTPUT_VARIABLE text)
  if(NOT rc STREQUAL "0" OR NOT text MATCHES "\"library\"")
    message(FATAL_ERROR "local library workflow must be available")
  endif()
  file(REMOVE_RECURSE "${OUT}")
  file(MAKE_DIRECTORY "${OUT}/images/nested.sms" "${OUT}/empty")
  file(COPY_FILE "${FIXTURES}/boot_to_play.sms" "${OUT}/images/known 'quote'; name.sms")
  string(REPEAT v 32768 halt)
  file(WRITE "${OUT}/images/.unlisted.ROM" "${halt}")
  string(REPEAT v 8192 small)
  file(WRITE "${OUT}/images/unsupported.gg" "${small}")
  file(WRITE "${OUT}/images/malformed.bin" "bad")
  file(WRITE "${OUT}/images/unrelated.txt" "ignore")
  file(COPY_FILE "${FIXTURES}/boot_to_play.sms" "${OUT}/images/nested.sms/hidden.sms")
  execute_process(COMMAND "${CMAKE_COMMAND}" -E create_symlink "${OUT}/images/.unlisted.ROM" "${OUT}/images/link.sms"
    RESULT_VARIABLE symlink_rc ERROR_QUIET)
  # Fault prevented: a stand-in that is only a copy of the live ledger and page changes silently with them, and a test whose cost
  # grows with the rows the whole list adds. The stand-in is the own fixture rows of the ledger plus the default rows the seed script
  # writes for the three catalog lines typed here, and its page is generated from it; every run edits fresh copies, never a tracked file.
  set(ENV{COMPAT_REASONS} "${ROOT}/tests/compat/reasons.txt")
  file(MAKE_DIRECTORY "${OUT}/standin")
  file(SHA1 "${FIXTURES}/vdp_lines_192.sms" lines_sha)
  file(SHA1 "${FIXTURES}/fm_sweep_jp.sms" japan_sha)
  file(SHA1 "${FIXTURES}/boot_to_play.gg" gg_sha)
  file(SHA1 "${FIXTURES}/boot_to_play.sms" skipped_sha)
  file(SHA1 "${FIXTURES}/psg_sweep.sms" device_sha)
  file(WRITE "${OUT}/catalog.tsv" "${lines_sha}\tsms\tnone\t-\tSynthetic lines\tunknown\t-\texport\tpad\tpad\n${japan_sha}\tsms\tnone\t-\tSynthetic japan\tunknown\t-\tjapan\tpad\tpad\n${gg_sha}\tgg\tnone\t-\tSynthetic gg\tunknown\t-\texport\tpad\tpad\n")
  file(APPEND "${OUT}/catalog.tsv" "${skipped_sha}\tsms\tnone\t-\tSynthetic skipped\tskipped\tmame_no\texport\tpad\tpad\n${device_sha}\tsms\tnone\t-\tSynthetic device\tunknown\t-\texport\tlphaser\tpad\n")
  set(empty e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855)
  file(STRINGS "${ROOT}/tests/compat/ledger.csv" fixture_rows REGEX "^(id,|\"fixture/)")
  set(standin_fixtures "")
  foreach(line IN LISTS fixture_rows)
    string(APPEND standin_fixtures "${line}\n")
  endforeach()
  file(WRITE "${OUT}/standin-fixtures.csv" "${standin_fixtures}")
  execute_process(COMMAND sh "${ROOT}/scripts/compat-seed.sh" "${OUT}/standin-fixtures.csv" "${OUT}/catalog.tsv" 2026-11-10
    RESULT_VARIABLE rc OUTPUT_VARIABLE standin_ledger ERROR_QUIET COMMAND_ERROR_IS_FATAL ANY)
  file(WRITE "${OUT}/standin-initial.csv" "${standin_ledger}")
  execute_process(COMMAND "${CMAKE_COMMAND}" -E env "COMPAT_CATALOG=${OUT}/catalog.tsv" "COMPAT_REASONS=${ROOT}/tests/compat/reasons.txt" sh "${ROOT}/scripts/compat-common.sh" docs "${OUT}/standin-initial.csv"
    RESULT_VARIABLE rc OUTPUT_VARIABLE standin_page COMMAND_ERROR_IS_FATAL ANY)
  set(library_directory "${ROOT}")
  function(library directory)
    file(WRITE "${OUT}/standin/ledger.csv" "${standin_ledger}")
    file(WRITE "${OUT}/standin/page.md" "${standin_page}")
    if(seed)
      file(APPEND "${OUT}/standin/ledger.csv" "${seed}")
    endif()
    execute_process(COMMAND "${CMAKE_COMMAND}" -E env "COGMASTER_ROM_DIR=${directory}"
      "COGMASTER_LIBRARY_RUN=${RUN}" "COGMASTER_LIBRARY_EMULATOR=${EMULATOR}" "COGMASTER_LIBRARY_OUT=${OUT}/results"
      "COGMASTER_LEDGER=${OUT}/standin/ledger.csv" "COGMASTER_PAGE=${OUT}/standin/page.md" ${ARGN}
      sh "${ROOT}/scripts/library.sh" run WORKING_DIRECTORY "${library_directory}"
      RESULT_VARIABLE rc OUTPUT_VARIABLE text ERROR_VARIABLE err)
    if(NOT rc STREQUAL "0")
      message(FATAL_ERROR "synthetic library operation failed: ${rc}")
    endif()
    set(text "${text}" PARENT_SCOPE)
  endfunction()
  library("${OUT}/images")
  if(NOT text MATCHES "candidates=4 attempted=3 evaluated=1 coverage=1 predicates=3")
    message(FATAL_ERROR "every candidate must run/classify once with accepted coverage separated: ${text}")
  endif()
  foreach(status IN ITEMS pass unknown unsupported malformed_io)
    if(NOT text MATCHES "${status}=1")
      message(FATAL_ERROR "missing distinct library status ${status}: ${text}")
    endif()
  endforeach()
  file(READ "${OUT}/results/results.csv" rows)
  if(NOT rows MATCHES "candidate,status,evaluated,attempted")
    message(FATAL_ERROR "library result schema missing")
  endif()
  if(NOT text MATCHES "library-judge 1" OR NOT text MATCHES "pal: skipped: not configured" OR NOT text MATCHES "fm: skipped: not configured"
      OR NOT text MATCHES "merge: rows_added=0 ratchet=ok")
    message(FATAL_ERROR "a library with no catalog cart must report skipped sections and no added rows: ${text}")
  endif()
  # Fault prevented: a judge that takes a profile from a file name or a title gives a mislabelled or hostile file an arbitrary
  # row; a merge that rewrites tracked files when the page cannot hold the rows; a path or address leaking to public output.
  # The catalog lists owned images by SHA-1. The Game Gear image is named .sms and the Japan image .gg, so a judge that
  # reads the machine from the extension produces other keys and fails the exact-key assertions below.
  set(home_path "Users" "." "jane" "." "Documents")
  string(CONCAT home_path ${home_path})
  set(address "jane" "@" "example" ".org")
  string(CONCAT address ${address})
  file(MAKE_DIRECTORY "${OUT}/judge-images")
  file(COPY_FILE "${FIXTURES}/vdp_lines_192.sms" "${OUT}/judge-images/${home_path} lines.sms")
  file(COPY_FILE "${FIXTURES}/fm_sweep_jp.sms" "${OUT}/judge-images/${address} japan.gg")
  file(COPY_FILE "${FIXTURES}/boot_to_play.gg" "${OUT}/judge-images/${home_path} ${address}.sms")
  file(COPY_FILE "${FIXTURES}/boot_to_play.sms" "${OUT}/judge-images/${home_path} skipped.sms")
  file(COPY_FILE "${FIXTURES}/psg_sweep.sms" "${OUT}/judge-images/${address} device.sms")
  library("${OUT}/judge-images" "COGMASTER_CATALOG=${OUT}/catalog.tsv")
  set(applied "${text}")
  if(NOT applied MATCHES "rows_added=1 ratchet=ok page_bytes=[0-9]+" OR NOT applied MATCHES "pal: carts=2 ran_pal=2 boots_pass=[0-2] boots_fail=[0-2]"
      OR NOT applied MATCHES "fm: carts=1 boots_pass=[01] boots_fail=[01]" OR NOT applied MATCHES "library-judge 1")
    message(FATAL_ERROR "the synthetic catalog run must add the PAL row and print the count block: ${applied}")
  endif()
  # Fault prevented: an owner's run whose merge prints no floors verdict. The printed block comes from the command, and the same
  # command over the merged stand-in ledger must find the verdict unmet (a floor silently at zero shows). The real .info carries no
  # flag now, so the block also says failed: the stand-in is below the floors while the flag is gone.
  if(NOT applied MATCHES "exit: cart_pass=[0-9]+/20" OR NOT applied MATCHES "judged_cart_floors=unmet")
    message(FATAL_ERROR "the run must print the exit block with an unmet verdict: ${applied}")
  endif()
  execute_process(COMMAND "${CMAKE_COMMAND}" "-DLEDGER=${OUT}/standin/ledger.csv" "-DCATALOG=${OUT}/catalog.tsv" -DCOUNTS=none
    "-DOUT=${OUT}/exit-standin" -P "${ROOT}/tests/compat/exit.cmake" RESULT_VARIABLE rc OUTPUT_VARIABLE exit_text ERROR_VARIABLE exit_err)
  if(NOT rc STREQUAL "0" OR NOT exit_text MATCHES "judged_cart_floors=unmet flag=unchecked")
    message(FATAL_ERROR "the command over the merged stand-in ledger must exit 0 with an unmet verdict: ${rc} ${exit_text}${exit_err}")
  endif()
  file(READ "${OUT}/standin/ledger.csv" merged)
  string(REGEX MATCHALL "\n" merged_lines "${merged}")
  string(REGEX MATCHALL "\n" standin_lines "${standin_ledger}")
  list(LENGTH merged_lines merged_count)
  list(LENGTH standin_lines standin_count)
  math(EXPR added "${merged_count}-${standin_count}")
  set(cfg_off "cfg:video=sms;fm=off;port1=pad;port2=pad;script=none;sha256=${empty}/boots@600")
  set(cfg_on "cfg:video=sms;fm=on;port1=pad;port2=pad;script=none;sha256=${empty}/boots@600")
  set(cfg_gg "cfg:video=gg;fm=off;port1=pad;port2=pad;script=none;sha256=${empty}/boots@600")
  function(expect_row sha title machine region tv cfg)
    if(NOT merged MATCHES "\"${sha}\",\"${title}\",\"${machine}\",\"${region}\",\"${tv}\",\"none\",\"${cfg}\",\"(pass|fail)\",\"core=[0-9a-f]+\"")
      message(FATAL_ERROR "the merged ledger lacks the judged ${machine} ${region} ${tv} row of ${title}")
    endif()
    if(merged MATCHES "\"${sha}\",[^\n]*\"unknown\"")
      message(FATAL_ERROR "the seeded unknown row of ${title} must be flipped, not kept beside a new row")
    endif()
  endfunction()
  expect_row("${lines_sha}" "Synthetic lines" sms export ntsc "${cfg_off}")
  expect_row("${lines_sha}" "Synthetic lines" sms export pal "${cfg_off}")
  expect_row("${japan_sha}" "Synthetic japan" sms japan ntsc "${cfg_on}")
  expect_row("${gg_sha}" "Synthetic gg" gg export ntsc "${cfg_gg}")
  # Fault prevented: a library run turning a dump MAME marks as not working into pass (its runs happen and their facts are discarded by
  # the update guard), and a title that needs a light phaser getting a pad row a pad run could satisfy (its image is never run as a pad).
  string(REGEX MATCHALL "\n\"${skipped_sha}\"," skipped_rows "${merged}")
  string(REGEX MATCHALL "\n\"${device_sha}\"," device_rows "${merged}")
  list(LENGTH skipped_rows skipped_count)
  list(LENGTH device_rows device_count)
  file(READ "${OUT}/results/public-facts.csv" produced)
  if(NOT merged MATCHES "\"${skipped_sha}\",\"Synthetic skipped\",\"sms\",\"export\",\"ntsc\",\"none\",\"[^\n]*\"skipped\",\"reason=mame_no\"" OR NOT skipped_count EQUAL 1
      OR NOT produced MATCHES "${skipped_sha},-,sms,export,ntsc,none,[^\n]*,(pass|fail)," OR NOT produced MATCHES "${skipped_sha},-,sms,export,pal,none,")
    message(FATAL_ERROR "a skipped row must keep its status and reason while its runs are produced and discarded")
  endif()
  if(NOT device_count EQUAL 1 OR NOT merged MATCHES "\"${device_sha}\",[^\n]*port1=lphaser;[^\n]*\"unknown\"" OR produced MATCHES "${device_sha}")
    message(FATAL_ERROR "a device title must keep its one unknown row and be given no pad run")
  endif()
  if(NOT added EQUAL 1)
    message(FATAL_ERROR "the merge must add exactly the PAL row, not ${added}")
  endif()
  execute_process(COMMAND "${CMAKE_COMMAND}" -E env "COMPAT_CATALOG=${OUT}/catalog.tsv" "COMPAT_REASONS=${ROOT}/tests/compat/reasons.txt" sh "${ROOT}/scripts/compat-common.sh" docs "${OUT}/standin/ledger.csv"
    RESULT_VARIABLE rc OUTPUT_VARIABLE fresh_page COMMAND_ERROR_IS_FATAL ANY)
  file(READ "${OUT}/standin/page.md" merged_page)
  file(SIZE "${OUT}/standin/page.md" page_size)
  file(READ "${ROOT}/quality.json" quality)
  string(JSON docs_cap GET "${quality}" ceilings docs.max_bytes)
  if(NOT merged_page STREQUAL fresh_page OR merged_page STREQUAL standin_page OR NOT page_size LESS docs_cap)
    message(FATAL_ERROR "the merged page must equal a fresh generation, differ from the initial page and stay under the cap")
  endif()
  file(WRITE "${OUT}/allow.empty" "")
  execute_process(COMMAND "${CMAKE_COMMAND}" -E env "COMPAT_CATALOG=${OUT}/catalog.tsv" "COMPAT_REASONS=${ROOT}/tests/compat/reasons.txt" sh "${ROOT}/scripts/compat-ratchet.sh" "${OUT}/standin-initial.csv" "${OUT}/standin/ledger.csv"
    "${OUT}/allow.empty" 2026-11-11 16 16 RESULT_VARIABLE rc OUTPUT_QUIET)
  if(NOT rc STREQUAL "0")
    message(FATAL_ERROR "the merged ledger must pass the ratchet: ${rc}")
  endif()
  # A cap below the page holds the merge: both stand-ins stay byte-identical to their initial text.
  library("${OUT}/judge-images" "COGMASTER_CATALOG=${OUT}/catalog.tsv" "COGMASTER_DOCS_CAP=100")
  set(held "${text}")
  file(READ "${OUT}/standin/ledger.csv" held_ledger)
  file(READ "${OUT}/standin/page.md" held_page)
  if(NOT held MATCHES "merge: held" OR NOT held_ledger STREQUAL standin_ledger OR NOT held_page STREQUAL standin_page)
    message(FATAL_ERROR "a page over the cap must hold the merge and leave the ledger and page untouched: ${held}")
  endif()
  # Nothing derived from a file name or address reaches a public or printed output.
  file(READ "${OUT}/results/public-facts.csv" facts)
  file(READ "${OUT}/results/results.csv" results)
  foreach(output IN ITEMS "${applied}" "${held}" "${facts}" "${results}" "${merged}" "${merged_page}")
    if(output MATCHES "Users|jane|example")
      message(FATAL_ERROR "a planted name reached an output")
    endif()
  endforeach()
  if(facts MATCHES "(^|\n)[^\n,]*[^0-9a-f,\n][^\n,]*,-," OR NOT facts MATCHES "${lines_sha},-,sms,export,ntsc,none,")
    message(FATAL_ERROR "public facts must carry catalog identities only")
  endif()
  message(STATUS "applied run:\n${applied}\nheld run:\n${held}")
  execute_process(COMMAND git check-ignore -q build/library/results/1.path WORKING_DIRECTORY "${ROOT}" RESULT_VARIABLE rc)
  if(NOT rc STREQUAL "0")
    message(FATAL_ERROR "the library results directory must be ignored by git: ${rc}")
  endif()
  # Fault prevented: one run that gives no verdict (a timeout on a loaded host) for a catalog title the ledger already holds as
  # a pass refusing the whole merge, and with it every row adopted in the same run. The runner is a wrapper that answers the
  # probe with a catalog identity (no owned image is in the catalog) and fails every case, so each case ends as an
  # infrastructure error; only pass and fail leave the judge as facts.
  file(MAKE_DIRECTORY "${OUT}/flaky-images")
  file(WRITE "${OUT}/flaky-images/held.sms" "${halt}x")
  file(SHA1 "${OUT}/flaky-images/held.sms" held_sha)
  set(flaky_emulator "${EMULATOR}")
  string(REPLACE ";" " " flaky_emulator "${flaky_emulator}")
  set(flaky_run "${RUN}")
  file(CONFIGURE OUTPUT "${OUT}/flaky-runner.sh" NEWLINE_STYLE UNIX @ONLY CONTENT [[case "$1" in
  --probe) @flaky_emulator@ "@flaky_run@" "$@" | sed 's/^identity_sha1=.*/identity_sha1=@held_sha@/' ;;
  *) exit 1 ;;
esac
]])
  # A second line carries a skipped status: the judge refuses a catalog that holds none, as an empty or reshaped one would be.
  string(REPEAT d 40 held_skipped)
  file(WRITE "${OUT}/flaky-catalog.tsv" "${held_sha}\tsms\tnone\t-\tSynthetic held\n${held_skipped}\tsms\tnone\t-\tSynthetic skipped\tskipped\tmame_no\texport\tpad\tpad\n")
  set(seed "\"${held_sha}\",\"Synthetic held\",\"sms\",\"export\",\"ntsc\",\"none\",\"${cfg_off}\",\"pass\",\"-\",\"2026-11-11\"\n")
  set(RUN "${OUT}/flaky-runner.sh")
  library("${OUT}/flaky-images" "COGMASTER_CATALOG=${OUT}/flaky-catalog.tsv" "COGMASTER_LIBRARY_EMULATOR=sh")
  set(RUN "${flaky_run}")
  file(READ "${OUT}/standin/ledger.csv" flaky_ledger)
  if(NOT text MATCHES "merge: rows_added=0 ratchet=ok" OR NOT flaky_ledger MATCHES "\"${held_sha}\",[^\n]*\"pass\"")
    message(FATAL_ERROR "a case without a verdict must leave a passing catalog row as it was: ${text}")
  endif()
  set(seed "")
  library("${OUT}/empty")
  if(NOT text MATCHES "candidates=0 attempted=0 evaluated=0 coverage=0")
    message(FATAL_ERROR "empty library must report zero evaluated coverage")
  endif()
  # Fault prevented: a printed failure of the exit command mistaken for a release gate. A judge malfunction recorded in the ledger fails
  # the command's second leg; the block says so and the script still exits 0 (library() requires status 0).
  set(seed "\"${lines_sha}\",\"Synthetic lines\",\"sms\",\"export\",\"pal\",\"none\",\"${cfg_off}\",\"infrastructure_error\",\"-\",\"2026-11-11\"\n")
  library("${OUT}/empty")
  set(seed "")
  if(NOT text MATCHES "exit: failed \\([0-9]+\\)" OR NOT text MATCHES "exit: leg 3: 1 rows have status infrastructure_error" OR text MATCHES "judged_cart_floors")
    message(FATAL_ERROR "a failing exit command must be printed with its leg and never change the script status: ${text}")
  endif()
  # Fault prevented: a real cart (its probe names the ledger id, unlike the owned fixtures) whose seeded default row the loop over the
  # ledger already judges being judged a second time, or whose missing PAL profile is skipped because the default row exists. The
  # wrapper gives an owned image a catalog identity and no fixture hash; the default row flips once and only the PAL run is derived.
  file(MAKE_DIRECTORY "${OUT}/known-images")
  file(COPY_FILE "${FIXTURES}/vdp_lines_192.sms" "${OUT}/known-images/cart.sms")
  file(CONFIGURE OUTPUT "${OUT}/known-runner.sh" NEWLINE_STYLE UNIX @ONLY CONTENT [[case "$1" in
  --probe) @flaky_emulator@ "@flaky_run@" "$@" | sed -e 's/^identity_sha1=.*/identity_sha1=@lines_sha@/' -e 's/^\(sha256[a-z_]*\)=.*/\1=00/' ;;
  *) @flaky_emulator@ "@flaky_run@" "$@" ;;
esac
]])
  set(RUN "${OUT}/known-runner.sh")
  library("${OUT}/known-images" "COGMASTER_CATALOG=${OUT}/catalog.tsv" "COGMASTER_LIBRARY_EMULATOR=sh")
  set(RUN "${flaky_run}")
  file(READ "${OUT}/standin/ledger.csv" known_ledger)
  if(NOT text MATCHES "pal: carts=0 ran_pal=1" OR NOT text MATCHES "rows_added=1 ratchet=ok" OR NOT known_ledger MATCHES "\"${lines_sha}\",[^\n]*\"pal\"[^\n]*\"(pass|fail)\"")
    message(FATAL_ERROR "a cart judged by its ledger row must still get its PAL profile and no second default run: ${text}")
  endif()
  # Fault prevented: a script that reads its catalog or reasons from the working directory, which the judge now refuses (it fails closed).
  set(library_directory "${ROOT}/docs")
  library("${OUT}/empty")
  set(library_directory "${ROOT}")
  if(NOT text MATCHES "merge: rows_added=0 ratchet=ok")
    message(FATAL_ERROR "a run from a subdirectory of the repository must merge as from the root: ${text}")
  endif()
  execute_process(COMMAND "${CMAKE_COMMAND}" -E env "COGMASTER_ROM_DIR=${OUT}/missing"
    "${CMAKE_COMMAND}" --preset library -B "${OUT}/configure-missing" WORKING_DIRECTORY "${ROOT}"
    RESULT_VARIABLE rc OUTPUT_VARIABLE text ERROR_VARIABLE err)
  if(rc STREQUAL "0" OR NOT "${text}${err}" MATCHES "COGMASTER_ROM_DIR.*existing directory")
    message(FATAL_ERROR "missing library directory must fail configure")
  endif()
  if(NOT CMAKE_CROSSCOMPILING)
    execute_process(COMMAND "${CMAKE_COMMAND}" -E env "COGMASTER_ROM_DIR=${OUT}/empty"
      "COGMASTER_LIBRARY_OUT=${OUT}/workflow-results" "${CMAKE_COMMAND}" --workflow --preset library
      WORKING_DIRECTORY "${ROOT}" RESULT_VARIABLE rc OUTPUT_VARIABLE text ERROR_VARIABLE err)
    file(READ "${OUT}/workflow-results/results.csv" results)
    if(NOT rc STREQUAL "0" OR NOT results STREQUAL "candidate,status,evaluated,attempted\n")
      message(FATAL_ERROR "local preset must configure/build/test an empty synthetic library: ${rc}")
    endif()
  endif()
  set(RUN "${OUT}/missing-runner")
  library("${OUT}/images")
  if(NOT text MATCHES "infrastructure_error=4" OR NOT text MATCHES "evaluated=0 coverage=0")
    message(FATAL_ERROR "missing runner must remain infrastructure failure with zero coverage")
  endif()
  # Exercise literal dotenv parsing in an isolated repository, never the owner's .env.local.
  set(dotenv "${OUT}/dotenv")
  file(MAKE_DIRECTORY "${dotenv}/empty 'quote'; directory")
  file(WRITE "${dotenv}/.env.local" "COGMASTER_ROM_DIR=${dotenv}/empty 'quote'; directory\n")
  file(WRITE "${dotenv}/CMakeLists.txt" [=[
cmake_minimum_required(VERSION 3.25)
project(dotenv NONE)
if(NOT IS_DIRECTORY "$ENV{COGMASTER_ROM_DIR}")
  message(FATAL_ERROR "literal dotenv path did not reach configure")
endif()
]=])
  file(WRITE "${dotenv}/CMakePresets.json" [=[
{"version":6,"configurePresets":[{"name":"library","generator":"Ninja","binaryDir":"${sourceDir}/build/library"}],"workflowPresets":[{"name":"library","steps":[{"type":"configure","name":"library"}]}]}
]=])
  execute_process(COMMAND git init -q WORKING_DIRECTORY "${dotenv}" COMMAND_ERROR_IS_FATAL ANY)
  execute_process(COMMAND "${CMAKE_COMMAND}" -E env --unset=COGMASTER_ROM_DIR
    sh "${ROOT}/scripts/library.sh" WORKING_DIRECTORY "${dotenv}"
    RESULT_VARIABLE rc OUTPUT_VARIABLE text ERROR_VARIABLE err)
  if(NOT rc STREQUAL "0")
    message(FATAL_ERROR "local wrapper must read literal dotenv directory without executing it")
  endif()
  return()
endif()
if(COGMASTER_FIXTURES)
  string(REPLACE ";" "$<SEMICOLON>" library_emulator "${CMAKE_CROSSCOMPILING_EMULATOR}")
  # Timeout: 160 s; 2 x 79.87 s (windows-arm64), slowest of 1 run, run 43158408243.
  cogmaster_add_test(NAME library.synthetic
    COMMAND ${CMAKE_COMMAND} -DROOT=${PROJECT_SOURCE_DIR} -DOUT=${PROJECT_BINARY_DIR}/tests/library
      -DFIXTURES=${PROJECT_BINARY_DIR}/fixtures -DRUN=$<TARGET_FILE:cogmaster-run>
      "-DEMULATOR=${library_emulator}" -P ${CMAKE_CURRENT_LIST_FILE}
    LABELS library TIMEOUT 160)
endif()
