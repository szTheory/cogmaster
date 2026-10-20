# cogmaster-run tests.
#
# Run as a script (cmake -DCASE=<case> ... -P tests.cmake), this file is one
# runner check; include()d by tests/tests.cmake, it registers the tests.
if(CMAKE_SCRIPT_MODE_FILE)
  # Runs cogmaster-run with ARGN and fails unless it exits with code and its
  # merged output matches regex.
  function(expect_exit code regex)
    execute_process(COMMAND ${EMULATOR} ${RUN} ${ARGN} RESULT_VARIABLE rc
      OUTPUT_VARIABLE out ERROR_VARIABLE out)
    if(NOT rc STREQUAL code OR NOT out MATCHES "${regex}")
      message(FATAL_ERROR "cogmaster-run ${ARGN}: expected exit ${code} and output matching\n"
        "  ${regex}\ngot exit ${rc} and output\n${out}")
    endif()
  endfunction()

  # Runs one case of the bench and stop unit tests.
  function(unit case)
    execute_process(COMMAND ${EMULATOR} ${UNIT} ${case} RESULT_VARIABLE rc
      OUTPUT_VARIABLE out ERROR_VARIABLE out)
    if(NOT rc STREQUAL "0")
      message(FATAL_ERROR "cogmaster-test-bench ${case}: exit ${rc}\n${out}")
    endif()
  endfunction()

  # Runs cogmaster-run --probe FILE and sets out and rc in the caller.
  macro(probe file)
    execute_process(COMMAND ${EMULATOR} ${RUN} --probe ${file} RESULT_VARIABLE rc
      OUTPUT_VARIABLE out ERROR_VARIABLE err)
  endmacro()

  # Fails unless out holds every probe key once, in order, each value in its
  # documented form. CMake regexes have no {n}, so digests are built by
  # repetition.
  function(expect_probe_form out)
    string(REPEAT "[0-9a-f]" 40 h40)
    string(REPEAT "[0-9a-f]" 64 h64)
    string(REPEAT "[0-9a-f]" 8 h8)
    string(REPEAT "[0-9a-f]" 32 h32)
    set(num "([0-9]+|none)")
    set(form "^size=[0-9]+\ncopier_header=[01]\nheader_at=([$][0-9A-F][0-9A-F][0-9A-F][0-9A-F]|none)\n"
      "header_magic=[01]\nchecksum=${num}\nchecksum_ok=([01]|none)\nproduct_code=${num}\n"
      "version=${num}\nregion_code=${num}\nrom_size_code=${num}\nmachine_guess=(sms|gg)\n"
      "region_guess=(ntsc-u|ntsc-j|pal)\nmapper_guess=(none|sega|codemasters|unsupported)\n"
      "sha1=${h40}\nsha256=${h64}\ncrc32=${h8}\nmd5=${h32}\n"
      "sha1_headerless=(${h40}|none)\nsha256_headerless=(${h64}|none)\n"
      "crc32_headerless=(${h8}|none)\nmd5_headerless=(${h32}|none)\n$")
    string(CONCAT form ${form})
    if(NOT out MATCHES "${form}")
      message(FATAL_ERROR "--probe output is not the documented 21 keys in order:\n${out}")
    endif()
  endfunction()

  # Fails unless the probe line key has the value.
  function(expect_key out key value)
    if(NOT out MATCHES "(^|\n)${key}=${value}\n")
      message(FATAL_ERROR "--probe: expected ${key}=${value} in\n${out}")
    endif()
  endfunction()

  if(CASE STREQUAL "probe_keys")
    # A 128 KiB export image with a good header and no mapper stores: the
    # keys in order, the header read at $7FF0 and the Sega board guessed.
    probe(${SEEDS}/header-export-128k.sms)
    if(NOT rc STREQUAL "0")
      message(FATAL_ERROR "--probe: expected exit 0, got ${rc}\n${out}${err}")
    endif()
    expect_probe_form("${out}")
    expect_key("${out}" header_at "[$]7FF0")
    expect_key("${out}" header_magic 1)
    expect_key("${out}" checksum_ok 1)
    expect_key("${out}" region_code 4)
    expect_key("${out}" rom_size_code 15)
    expect_key("${out}" machine_guess sms)
    expect_key("${out}" mapper_guess sega)
    expect_key("${out}" md5_headerless none)
    # A bad checksum is reported, not refused.
    probe(${SEEDS}/header-export-128k-bad.sms)
    expect_key("${out}" checksum_ok 0)
  elseif(CASE STREQUAL "probe_digests")
    # The digest code against the RFC vectors, then the runner's lines
    # against CMake's own digests of the same file.
    unit(digests)
    set(file ${SEEDS}/header-export-32k.sms)
    probe(${file})
    foreach(algo IN ITEMS MD5 SHA1 SHA256)
      file(${algo} ${file} want)
      string(TOLOWER ${algo} key)
      expect_key("${out}" ${key} ${want})
    endforeach()
  elseif(CASE STREQUAL "probe_headerless")
    # A copier header: the *_headerless digests cover the body after it,
    # which is size-16k.sms byte for byte, so they differ from the
    # whole-file ones and equal CMake's digests of that file. With no copier
    # header they are none.
    probe(${SEEDS}/size-16k-512.sms)
    expect_probe_form("${out}")
    expect_key("${out}" copier_header 1)
    foreach(algo IN ITEMS MD5 SHA1 SHA256)
      string(TOLOWER ${algo} key)
      file(${algo} ${SEEDS}/size-16k.sms body)
      file(${algo} ${SEEDS}/size-16k-512.sms whole)
      expect_key("${out}" ${key} ${whole})
      expect_key("${out}" ${key}_headerless ${body})
    endforeach()
    probe(${SEEDS}/header-export-32k.sms)
    expect_key("${out}" copier_header 0)
    foreach(key IN ITEMS sha1 sha256 crc32 md5)
      expect_key("${out}" ${key}_headerless none)
    endforeach()
  elseif(CASE STREQUAL "probe_unsupported")
    # Another board's mapper: every key is still printed, then exit 77.
    probe(${SEEDS}/unsupported-board.sms)
    if(NOT rc STREQUAL "77")
      message(FATAL_ERROR "--probe of another board: expected exit 77, got ${rc}\n${out}${err}")
    endif()
    expect_probe_form("${out}")
    expect_key("${out}" mapper_guess unsupported)
  elseif(CASE STREQUAL "refuse_size")
    # Below 1 KiB and above 1 MiB + 512 the core refuses the image; above
    # 64 MiB the runner refuses the file before reading it.
    expect_exit(77 "unsupported" --frames 1 ${SEEDS}/size-1023.sms)
    expect_exit(77 "unsupported" --frames 1 ${SEEDS}/size-1m-513.sms)
    expect_exit(0 "^$" --frames 1 ${SEEDS}/size-1024.sms)
    expect_exit(0 "^$" --frames 1 ${SEEDS}/size-1m-512.sms)
    expect_exit(77 "64 MiB" --frames 1 ${OVERSIZE})
    expect_exit(77 "64 MiB" --probe ${OVERSIZE})
  elseif(CASE STREQUAL "refuse_sg1000")
    # SG-1000 and SC-3000 images are refused by their file name, unread.
    foreach(name IN ITEMS game.sg game.sc GAME.SG)
      file(WRITE ${OUT}/${name} "")
      expect_exit(77 "unsupported: SG-1000 [(]planned[)]" --frames 1 ${OUT}/${name})
      expect_exit(77 "unsupported: SG-1000 [(]planned[)]" --probe ${OUT}/${name})
    endforeach()
  elseif(CASE STREQUAL "refuse_profile")
    # No Game Gear and no Japanese console was made for PAL.
    expect_exit(77 "unsupported" --frames 1 --machine gg --region pal)
    expect_exit(77 "unsupported" --frames 1 --machine gg --region pal ${SEEDS}/di-halt.sms)
    expect_exit(77 "unsupported" --frames 1 --region pal ${SEEDS}/di-halt.gg)
    expect_exit(77 "unsupported" --frames 1 --machine gg ${SEEDS}/gg-8k.gg)
    expect_exit(77 "unsupported" --frames 1 ${SEEDS}/unsupported-board.sms)
  elseif(CASE STREQUAL "machine_auto")
    # --machine auto takes the header's region code, then a .gg name, then
    # the Master System. The two crops differ, so the hash tells them apart.
    function(hash_of var)
      execute_process(COMMAND ${EMULATOR} ${RUN} --frames 1 --hash ${ARGN} RESULT_VARIABLE rc
        OUTPUT_VARIABLE out ERROR_VARIABLE out)
      if(NOT rc STREQUAL "0" OR NOT out MATCHES "^frame 1 sha256 [0-9a-f]+\n$")
        message(FATAL_ERROR "--frames 1 --hash ${ARGN}: exit ${rc}\n${out}")
      endif()
      set(${var} "${out}" PARENT_SCOPE)
    endfunction()
    foreach(file header-gg-export-32k.sms di-halt.gg)
      hash_of(auto ${SEEDS}/${file})
      hash_of(gg --machine gg ${SEEDS}/${file})
      hash_of(sms --machine sms ${SEEDS}/${file})
      if(NOT auto STREQUAL gg OR auto STREQUAL sms)
        message(FATAL_ERROR "${file}: auto is not the Game Gear\nauto ${auto}gg ${gg}sms ${sms}")
      endif()
    endforeach()
    hash_of(auto ${SEEDS}/di-halt.sms)
    hash_of(sms --machine sms ${SEEDS}/di-halt.sms)
    if(NOT auto STREQUAL sms)
      message(FATAL_ERROR "di-halt.sms: auto is not the Master System\nauto ${auto}sms ${sms}")
    endif()
  elseif(CASE STREQUAL "region")
    foreach(region auto ntsc-u ntsc-j pal)
      expect_exit(0 "^$" --frames 2 --region ${region} ${SEEDS}/di-halt.sms)
    endforeach()
    expect_exit(2 "usage" --frames 2 --region eu ${SEEDS}/di-halt.sms)
  elseif(CASE STREQUAL "png_file")
    # The decode check runs first, so the pinned digest is only ever
    # compared for a file whose IHDR and every pixel are already known good.
    file(REMOVE "${OUT}")
    expect_exit(0 "^$" --frames 60 --png "60:${OUT}")
    execute_process(COMMAND ${EMULATOR} ${DECODE} png_file "${OUT}" RESULT_VARIABLE rc
      OUTPUT_VARIABLE out ERROR_VARIABLE out)
    if(NOT rc STREQUAL "0")
      message(FATAL_ERROR "the PNG does not decode to the card (exit ${rc})\n${out}")
    endif()
    file(SHA256 "${OUT}" sha256)
    if(NOT sha256 STREQUAL EXPECTED)
      message(FATAL_ERROR "PNG SHA-256 mismatch: expected ${EXPECTED} got ${sha256}")
    endif()
    # A file the runner cannot write is a failure, not a success.
    expect_exit(1 "cannot write" --frames 1 --png "1:${OUT}.missing/x.png")
  elseif(CASE STREQUAL "png_syntax")
    # Each malformed or out-of-range --png is a usage error, whichever order
    # the options come in, and writes nothing.
    file(REMOVE "${OUT}")
    expect_exit(2 "usage" --frames 60 --png)
    expect_exit(2 "usage" --frames 60 --png "${OUT}")
    expect_exit(2 "usage" --frames 60 --png "60:")
    expect_exit(2 "usage" --frames 60 --png ":${OUT}")
    expect_exit(2 "usage" --frames 60 --png "0:${OUT}")
    expect_exit(2 "usage" --frames 60 --png "x:${OUT}")
    expect_exit(2 "usage" --frames 5 --png "6:${OUT}")
    expect_exit(2 "usage" --png "61:${OUT}" --frames 60)
    if(EXISTS "${OUT}")
      message(FATAL_ERROR "a refused --png wrote ${OUT}")
    endif()
  elseif(CASE STREQUAL "bench_format")
    # The report's text from fixed samples, then the runner's own report:
    # every line present, in order, as a whole number. Times are never
    # compared, as they vary from run to run.
    unit(format)
    set(num "[0-9]+")
    set(report "frames 30\np50_us ${num}\np90_us ${num}\np99_us ${num}\np99[.]9_us ${num}\n"
      "p99[.]99_us ${num}\np99[.]999_us ${num}\nmax_us ${num}\nmax_rss_kib ${num}\n")
    string(CONCAT report ${report})
    expect_exit(0 "^${report}$" --bench 30)
    # One frame: every percentile is the one sample, so all seven times are
    # equal.
    execute_process(COMMAND ${EMULATOR} ${RUN} --bench 1 RESULT_VARIABLE rc OUTPUT_VARIABLE out)
    string(REGEX MATCHALL "_us [0-9]+" times "${out}")
    list(REMOVE_DUPLICATES times)
    list(LENGTH times n)
    if(NOT rc STREQUAL "0" OR NOT out MATCHES "^frames 1\n" OR NOT n EQUAL 1)
      message(FATAL_ERROR "--bench 1: expected exit 0 and seven equal times, got exit ${rc}\n${out}")
    endif()
  elseif(CASE STREQUAL "bench_bytes")
    # The sample buffer is sized exactly up to the cap; a count above the cap
    # is refused before anything runs, never a wrapped allocation.
    unit(bytes)
    expect_exit(2 "usage" --bench 1073741824)
    expect_exit(2 "usage" --bench 4294967295)
    expect_exit(2 "usage" --bench 0)
  elseif(CASE STREQUAL "stop")
    # The flag, then a runner built to raise SIGTERM inside frame 3: the run
    # ends after that frame, still prints what was asked, and exits 0.
    unit(stop)
    set(RUN ${HOOKED})
    expect_exit(0 "^frame 3 sha256 ${EXPECTED}\n$" --frames 60 --hash)
    expect_exit(0 "^frames 3\n.*max_rss_kib [0-9]+\n$" --bench 60)
  elseif(CASE STREQUAL "exit_failure")
    # A --png path that cannot be opened is a failure, not a usage error.
    expect_exit(1 "cannot write" --frames 1 --png "1:${OUT}.missing/x.png")
  elseif(CASE STREQUAL "no_args")
    expect_exit(2 "usage")
  elseif(CASE STREQUAL "unknown")
    # Options a later release adds are refused now, and --help does not
    # mention them; --help and --version take nothing else; --trace takes
    # only first-fault and --region only its four names.
    foreach(opt --expect --port1 --port2 --input --hash-audio --capabilities --boot --bios
        --mailbox)
      expect_exit(2 "unknown option" --frames 1 ${opt})
      expect_exit(2 "unknown option" --frames 1 ${opt} x)
    endforeach()
    expect_exit(2 "usage" --help --version)
    expect_exit(2 "usage" --frames 1 --help)
    expect_exit(2 "usage" --version x)
    expect_exit(2 "usage" --frames 1 --trace)
    expect_exit(2 "usage" --frames 1 --trace full)
    expect_exit(2 "usage" --frames 1 --region)
    expect_exit(2 "usage" --frames 1 --region eu)
    expect_exit(2 "usage" --probe)
    expect_exit(2 "usage" --probe --frames 1 ${SEEDS}/di-halt.sms)
    execute_process(COMMAND ${EMULATOR} ${RUN} --help RESULT_VARIABLE rc OUTPUT_VARIABLE out)
    if(NOT rc STREQUAL "0" OR out MATCHES "--(expect|port|input|hash-audio|capabilities|boot|bios|mailbox)")
      message(FATAL_ERROR "--help: expected exit 0 and no later option, got exit ${rc}\n${out}")
    endif()
  elseif(CASE STREQUAL "positional")
    # One file, positional or after --rom, runs; a second file, a file with
    # no frame count, and an empty --rom are usage errors. A file that
    # cannot be read is a failure.
    expect_exit(0 "^$" --frames 1 ${SEEDS}/di-halt.sms)
    expect_exit(0 "^$" --rom ${SEEDS}/di-halt.sms --frames 1)
    expect_exit(2 "give one file" --frames 1 ${SEEDS}/di-halt.sms ${SEEDS}/di-halt.sms)
    expect_exit(2 "give one file" --frames 1 --rom ${SEEDS}/di-halt.sms ${SEEDS}/di-halt.sms)
    expect_exit(2 "usage" ${SEEDS}/di-halt.sms)
    expect_exit(2 "usage" --frames 1 --rom)
    expect_exit(1 "cannot read" --frames 1 ${SEEDS}/missing.sms)
  elseif(CASE STREQUAL "frame_range")
    expect_exit(2 "usage" --frames 0)
    expect_exit(2 "usage" --frames -1)
    expect_exit(2 "usage" --frames 1x)
    expect_exit(2 "usage" --frames 4294967296)
    expect_exit(2 "usage" --frames)
    expect_exit(2 "usage" --bench 0)
    expect_exit(2 "usage" --frames 1 --bench 1)
    expect_exit(2 "usage" --frames 1 --frames 1)
  elseif(CASE STREQUAL "hash_repeat")
    # The card does not change between frames, so the first and the 60th
    # frame hash alike, and the line names the frame.
    execute_process(COMMAND ${EMULATOR} ${RUN} --frames 1 --hash RESULT_VARIABLE rc1 OUTPUT_VARIABLE out1)
    execute_process(COMMAND ${EMULATOR} ${RUN} --frames 60 --hash RESULT_VARIABLE rc60 OUTPUT_VARIABLE out60)
    if(NOT rc1 STREQUAL "0" OR NOT rc60 STREQUAL "0"
        OR NOT out1 MATCHES "^frame 1 sha256 ([0-9a-f]+)\n$")
      message(FATAL_ERROR "--frames 1 --hash: exit ${rc1}\n${out1}--frames 60 --hash: exit ${rc60}\n${out60}")
    endif()
    set(first ${CMAKE_MATCH_1})
    if(NOT out60 STREQUAL "frame 60 sha256 ${first}\n")
      message(FATAL_ERROR "frame 1 and frame 60 differ:\n${out1}${out60}")
    endif()
  elseif(CASE STREQUAL "version")
    expect_exit(0 "^cogmaster-run [0-9]+\\.[0-9]+\\.[0-9]+\n$" --version)
  else()
    message(FATAL_ERROR "unknown CASE '${CASE}'")
  endif()
  return()
endif()

# Every command that starts cogmaster-run goes through the build's
# CMAKE_CROSSCOMPILING_EMULATOR, so an emulated lane runs its own guest binary.
set(runner_cmd ${CMAKE_CROSSCOMPILING_EMULATOR} $<TARGET_FILE:cogmaster-run> --frames 60 --hash)
set(runner_check_hash ${PROJECT_SOURCE_DIR}/tests/cmake/check_hash.cmake)

# Fault prevented: the runner's printed SMS card hash, the line hosts and CI
# compare, drifts from the pinned card or differs on another platform.
string(REPLACE ";" "$<SEMICOLON>" cmd "${runner_cmd}")
cogmaster_add_test(NAME runner.hash.sms
  COMMAND ${CMAKE_COMMAND} "-DCMD=${cmd}" -DEXPECTED=${COGMASTER_TESTCARD_SHA256_SMS}
    -P ${runner_check_hash}
  LABELS runner portable TIMEOUT 30)

# The same for the Game Gear card's 160x144 crop.
string(REPLACE ";" "$<SEMICOLON>" cmd "${runner_cmd};--machine;gg")
cogmaster_add_test(NAME runner.hash.gg
  COMMAND ${CMAKE_COMMAND} "-DCMD=${cmd}" -DEXPECTED=${COGMASTER_TESTCARD_SHA256_GG}
    -P ${runner_check_hash}
  LABELS runner portable TIMEOUT 30)

# Fault prevented: a hash check that passes whatever the runner prints. The
# SMS card's hash checked against the Game Gear pin must fail with exit 1 and
# the mismatch message.
string(REPLACE ";" "\;" inner "${runner_cmd}")
set(cmd ${CMAKE_COMMAND} "-DCMD=${inner}" -DEXPECTED=${COGMASTER_TESTCARD_SHA256_GG}
  -P ${runner_check_hash})
string(REPLACE ";" "$<SEMICOLON>" cmd "${cmd}")
cogmaster_add_test(NAME runner.hash.self_test
  COMMAND ${CMAKE_COMMAND} "-DCMD=${cmd}" -DCODE=1 "-DREGEX=hash mismatch"
    -P ${PROJECT_SOURCE_DIR}/tests/cmake/expect_fail.cmake
  LABELS runner TIMEOUT 30)

# Fault prevented: a lane whose hashes are never compared with the others.
# Writes build/<preset>/hashes.txt with the SMS and Game Gear hash lines,
# the file CI collects from every lane and compares byte for byte.
string(REPLACE ";" "$<SEMICOLON>" runner_emulator "${CMAKE_CROSSCOMPILING_EMULATOR}")
set(runner_hashes ${PROJECT_SOURCE_DIR}/tests/cmake/hashes.cmake)
cogmaster_add_test(NAME runner.hashes_file
  COMMAND ${CMAKE_COMMAND} -DMODE=write -DRUN=$<TARGET_FILE:cogmaster-run>
    "-DEMULATOR=${runner_emulator}" -DOUT=${PROJECT_BINARY_DIR} -P ${runner_hashes}
  LABELS runner portable TIMEOUT 30)

# Fault prevented: a comparison that passes lanes which differ, or a lane
# that silently drops out. Two files one hex digit apart must fail, and two
# files where three are required must fail.
set(runner_fixture ${PROJECT_BINARY_DIR}/tests/hashes)
file(WRITE ${runner_fixture}/a.txt "frame 60 sha256 00\n")
file(WRITE ${runner_fixture}/b.txt "frame 60 sha256 01\n")
# CMake wraps long messages, so the regexes allow a break between words.
foreach(case IN ITEMS "count;3;a;expected 3 files,[^/]*got 2" "byte;2;b;b.txt[^/]*differs")
  list(GET case 0 name)
  list(GET case 1 count)
  list(GET case 2 second)
  list(GET case 3 regex)
  set(cmd ${CMAKE_COMMAND} "-DFILES=${runner_fixture}/a.txt\\;${runner_fixture}/${second}.txt"
    -DCOUNT=${count} -DMODE=compare -P ${runner_hashes})
  string(REPLACE ";" "$<SEMICOLON>" cmd "${cmd}")
  cogmaster_add_test(NAME runner.hashes_compare.${name}.self_test
    COMMAND ${CMAKE_COMMAND} "-DCMD=${cmd}" -DCODE=1 "-DREGEX=${regex}"
      -P ${PROJECT_SOURCE_DIR}/tests/cmake/expect_fail.cmake
    LABELS runner TIMEOUT 10)
endforeach()

# The PNG encoder and its checksums, checked by an independent parse. The
# same program decodes the runner's PNG for runner.png_file.
add_executable(cogmaster-test-runner
  ${CMAKE_CURRENT_LIST_DIR}/test_png.c
  ${PROJECT_SOURCE_DIR}/runner/png.c
  ${PROJECT_SOURCE_DIR}/runner/crc32.c)
target_include_directories(cogmaster-test-runner PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(cogmaster-test-runner PRIVATE cogmaster::cogmaster)
set_target_properties(cogmaster-test-runner PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
cogmaster_warnings(cogmaster-test-runner)

# Fault prevented: an encoder whose chunk CRCs, Adler-32 or stored blocks are
# wrong, so a viewer or the RetroArch compare rejects or misreads the file.
cogmaster_add_test(NAME runner.png
  COMMAND cogmaster-test-runner png
  LABELS runner portable TIMEOUT 10)

set(runner_check ${CMAKE_COMMAND} -DRUN=$<TARGET_FILE:cogmaster-run>
  "-DEMULATOR=${runner_emulator}" -DSEEDS=${COGMASTER_SEEDS_DIR})

# Fault prevented: the reference PNG the RetroArch screenshot is compared
# with changes bytes (palette, layout or encoder) while the frame hash still
# passes, or no longer shows the card.
cogmaster_add_test(NAME runner.png_file
  COMMAND ${runner_check} -DCASE=png_file -DOUT=${PROJECT_BINARY_DIR}/tests/card.png
    -DDECODE=$<TARGET_FILE:cogmaster-test-runner> -DEXPECTED=${COGMASTER_TESTCARD_PNG_SHA256_SMS}
    -P ${CMAKE_CURRENT_LIST_FILE}
  LABELS runner portable TIMEOUT 30)

# Fault prevented: a malformed or out-of-range --png that is ignored or
# half-applied instead of refused as a usage error.
cogmaster_add_test(NAME runner.usage.png_syntax
  COMMAND ${runner_check} -DCASE=png_syntax -DOUT=${PROJECT_BINARY_DIR}/tests/refused.png
    -P ${CMAKE_CURRENT_LIST_FILE}
  LABELS runner portable TIMEOUT 30)

# The bench statistics and the stop flag, tested directly, and a runner built
# with a hook that raises SIGTERM from inside frame 3.
add_executable(cogmaster-test-bench
  ${CMAKE_CURRENT_LIST_DIR}/test_runner.c
  ${PROJECT_SOURCE_DIR}/runner/bench.c
  ${PROJECT_SOURCE_DIR}/runner/md5.c
  ${PROJECT_SOURCE_DIR}/runner/sha1.c
  ${PROJECT_SOURCE_DIR}/runner/stop.c)
set_target_properties(cogmaster-test-bench PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
cogmaster_warnings(cogmaster-test-bench)

add_executable(cogmaster-test-run-stop ${COGMASTER_RUNNER_SOURCES})
target_link_libraries(cogmaster-test-run-stop PRIVATE cogmaster::cogmaster cogmaster-host)
target_compile_definitions(cogmaster-test-run-stop PRIVATE _POSIX_C_SOURCE=200809L
  COGMASTER_RUN_RAISE_AT=3)
set_target_properties(cogmaster-test-run-stop PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
cogmaster_warnings(cogmaster-test-run-stop)

set(bench_check ${runner_check} -DUNIT=$<TARGET_FILE:cogmaster-test-bench>)

# Fault prevented: a percentile that interpolates, picks the wrong rank or
# reads unsorted samples, so the report names a time no frame took.
cogmaster_add_test(NAME runner.bench.percentile
  COMMAND cogmaster-test-bench percentile
  LABELS bench portable runner TIMEOUT 10)

# Fault prevented: a bench report missing a line, out of order or not in
# whole units, which the nightly trend parser would misread.
cogmaster_add_test(NAME runner.bench.format
  COMMAND ${bench_check} -DCASE=bench_format -P ${CMAKE_CURRENT_LIST_FILE}
  LABELS bench portable runner TIMEOUT 30)

# Fault prevented: a sample count whose buffer size wraps to a small
# allocation, so the frame loop writes past it.
cogmaster_add_test(NAME runner.bench.bytes
  COMMAND ${bench_check} -DCASE=bench_bytes -P ${CMAKE_CURRENT_LIST_FILE}
  LABELS bench portable runner TIMEOUT 30)

# Fault prevented: SIGINT or SIGTERM killing the run mid-frame or losing the
# output already asked for, so an interrupted CI job reports nothing.
cogmaster_add_test(NAME runner.stop
  COMMAND ${bench_check} -DHOOKED=$<TARGET_FILE:cogmaster-test-run-stop>
    -DEXPECTED=${COGMASTER_TESTCARD_SHA256_SMS} -DCASE=stop -P ${CMAKE_CURRENT_LIST_FILE}
  LABELS portable runner TIMEOUT 30)

# Fault prevented: the exit statuses in docs/runner.md drifting from the ones
# --help prints.
cogmaster_add_test(NAME runner.exit.table
  COMMAND ${runner_check} -DDOC=${PROJECT_SOURCE_DIR}/docs/runner.md
    -P ${PROJECT_SOURCE_DIR}/tests/cmake/exit_table.cmake
  LABELS runner TIMEOUT 30)

# Fault prevented: a runner that exits 0 after failing to write a file, or
# calls a failure a usage error.
cogmaster_add_test(NAME runner.exit.failure
  COMMAND ${runner_check} -DCASE=exit_failure -DOUT=${PROJECT_BINARY_DIR}/tests/failure.png
    -P ${CMAKE_CURRENT_LIST_FILE}
  LABELS portable runner TIMEOUT 30)

# Fault prevented: a command line that runs something it should refuse: no
# arguments, an option a later release adds, two files, or a frame count
# that is zero, negative, malformed, too large or given twice.
foreach(case no_args unknown positional frame_range)
  cogmaster_add_test(NAME runner.usage.${case}
    COMMAND ${runner_check} -DCASE=${case} -P ${CMAKE_CURRENT_LIST_FILE}
    LABELS portable runner TIMEOUT 30)
endforeach()

# Fault prevented: a hash that depends on the frame count for a card that
# does not move, or a hash line that names the wrong frame.
cogmaster_add_test(NAME runner.hash.repeat
  COMMAND ${runner_check} -DCASE=hash_repeat -P ${CMAKE_CURRENT_LIST_FILE}
  LABELS portable runner TIMEOUT 30)

# Fault prevented: --version printing something a script cannot parse.
cogmaster_add_test(NAME runner.version
  COMMAND ${runner_check} -DCASE=version -P ${CMAKE_CURRENT_LIST_FILE}
  LABELS portable runner TIMEOUT 30)

# Fault prevented: a --probe report that drops, reorders or reformats a key,
# misreads the header, or guesses the wrong board, so a script parsing it
# reads the wrong field.
cogmaster_add_test(NAME runner.probe.keys
  COMMAND ${runner_check} -DCASE=probe_keys -P ${CMAKE_CURRENT_LIST_FILE}
  LABELS portable runner TIMEOUT 30)

# Fault prevented: a SHA-1 or MD5 that is wrong for some lengths, so the
# printed md5 is not the RetroAchievements hash and the sha1 matches no
# database entry.
cogmaster_add_test(NAME runner.probe.digests
  COMMAND ${bench_check} -DCASE=probe_digests -P ${CMAKE_CURRENT_LIST_FILE}
  LABELS portable runner TIMEOUT 30)

# Fault prevented: *_headerless digests taken over the whole file, or
# printed for a file with no copier header.
cogmaster_add_test(NAME runner.probe.headerless
  COMMAND ${runner_check} -DCASE=probe_headerless -P ${CMAKE_CURRENT_LIST_FILE}
  LABELS portable runner TIMEOUT 30)

# Fault prevented: --probe hiding the report of an image it calls
# unsupported, or calling it supported.
cogmaster_add_test(NAME runner.probe.unsupported
  COMMAND ${runner_check} -DCASE=probe_unsupported -P ${CMAKE_CURRENT_LIST_FILE}
  LABELS portable runner TIMEOUT 30)

# Fault prevented: an image outside the accepted sizes run or reported as a
# failure, or a huge file read into memory before it is refused.
cogmaster_add_test(NAME runner.refuse.size
  COMMAND ${runner_check} -DCASE=refuse_size -DOVERSIZE=${COGMASTER_OVERSIZE_FILE}
    -P ${CMAKE_CURRENT_LIST_FILE}
  LABELS portable runner TIMEOUT 30)

# Fault prevented: an SG-1000 image run as a Master System cartridge.
cogmaster_add_test(NAME runner.refuse.sg1000
  COMMAND ${runner_check} -DCASE=refuse_sg1000 -DOUT=${PROJECT_BINARY_DIR}/tests
    -P ${CMAKE_CURRENT_LIST_FILE}
  LABELS portable runner TIMEOUT 30)

# Fault prevented: a machine and region no console had, or a board the core
# cannot run, reported as a failure instead of a refusal.
cogmaster_add_test(NAME runner.refuse.profile
  COMMAND ${runner_check} -DCASE=refuse_profile -P ${CMAKE_CURRENT_LIST_FILE}
  LABELS portable runner TIMEOUT 30)

# Fault prevented: --machine auto running a Game Gear cartridge as a Master
# System one, or the reverse.
cogmaster_add_test(NAME runner.load.machine_auto
  COMMAND ${runner_check} -DCASE=machine_auto -P ${CMAKE_CURRENT_LIST_FILE}
  LABELS portable runner TIMEOUT 30)

# Fault prevented: a --region name that is refused, ignored or accepts a
# misspelling.
cogmaster_add_test(NAME runner.load.region
  COMMAND ${runner_check} -DCASE=region -P ${CMAKE_CURRENT_LIST_FILE}
  LABELS portable runner TIMEOUT 30)
