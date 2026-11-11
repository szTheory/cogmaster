# cogmaster-run tests.
if(NOT CMAKE_SCRIPT_MODE_FILE)
  include(${CMAKE_CURRENT_LIST_DIR}/expect.cmake)
endif()
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

  # file(WRITE) turns each LF into CRLF on Windows, and these files carry exact bytes: file(CONFIGURE) with
  # a newline style writes the same bytes on every platform. STYLE is UNIX or CRLF; CONTENT has LF endings and
  # always ends in a newline in the file.
  function(write_bytes path style content)
    file(CONFIGURE OUTPUT "${path}" CONTENT "${content}" NEWLINE_STYLE ${style} @ONLY)
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
      "region_guess=(ntsc-u|ntsc-j|pal)\nmapper_guess=(none|sega|codemasters|korean|korean_nb|hicom|zemina|nemesis|hes_4pak|unsupported)\n"
      "sha1=${h40}\nsha256=${h64}\ncrc32=${h8}\nmd5=${h32}\n"
      "sha1_headerless=(${h40}|none)\nsha256_headerless=(${h64}|none)\n"
      "crc32_headerless=(${h8}|none)\nmd5_headerless=(${h32}|none)\n"
      "identified=[01]\npublic_identity=[01]\npersistence=[012]\npersistent_size=[0-9]+\n"
      "persistence_geometry=[012]\nvideo_mode=[012]\nidentity_sha1=(${h40}|)\ntitle=[^\n]*\n"
      "board_source=(db|header|heuristic|default|none)\nboard_candidate=[a-z0-9_]+\nscan_sega=[0-9]+\n"
      "scan_candidate=[0-9]+\nscan_runner_up=[0-9]+\nunsupported_id=[0-9]+\nunsupported_reason=[^\n]*\n"
      "port1_default=(pad|paddle|sports|lphaser)\nport2_default=(pad|paddle|sports|lphaser)\n"
      "$")
    string(CONCAT form ${form})
    if(NOT out MATCHES "${form}")
      message(FATAL_ERROR "--probe output keys or values differ from the documented form:\n${out}")
    endif()
  endfunction()

  # Fails unless the probe line key has the value.
  function(expect_key out key value)
    if(NOT out MATCHES "(^|\n)${key}=${value}\n")
      message(FATAL_ERROR "--probe: expected ${key}=${value} in\n${out}")
    endif()
  endfunction()

  if(CASE STREQUAL "hash_matrix")
    execute_process(COMMAND ${CMAKE_COMMAND} -DMODE=write -DRUN=${RUN}
      "-DEMULATOR=${EMULATOR}" -DOUT=${OUT} -DFIXTURES=${FIXTURES} -P ${HASHES}
      COMMAND_ERROR_IS_FATAL ANY)
    file(READ "${OUT}/hashes.txt" hashes)
    if(NOT FIXTURES)
      return()
    endif()
    foreach(row IN ITEMS "playing boot_to_play.gg native ntsc-u input"
        "playing boot_to_play.sms sms1 pal input" "playing boot_to_play.sms gg-sms ntsc-j input"
        "playing vdp_lines_240.sms sms pal baseline"
        "playing vdp_lines_224.sms native ntsc-j baseline"
        "playing vdp_lines_240.sms native ntsc-u baseline")
      if(NOT hashes MATCHES "${row} frame 14 sha256 [0-9a-f]+")
        message(FATAL_ERROR "playing matrix missing ${row}")
      endif()
    endforeach()
    # The playable fixture has all four sound rows, each with its two hashes.
    foreach(row IN ITEMS "boot_to_play.sms sms ntsc-u baseline 60" "boot_to_play.sms sms ntsc-u input 14"
        "boot_to_play.gg native ntsc-u baseline 60" "boot_to_play.gg native ntsc-u input 14")
      foreach(kind IN ITEMS transitions pcm)
        if(NOT hashes MATCHES "audio-${kind} ${row} ")
          message(FATAL_ERROR "sound matrix missing audio-${kind} ${row}")
        endif()
      endforeach()
    endforeach()
    # The sound rows equal the frozen pins, line for line; the first line that differs is named.
    foreach(kind IN ITEMS transitions pcm)
      file(STRINGS "${OUT}/hashes.txt" got REGEX "^audio-${kind} ")
      file(STRINGS "${PINS}/${kind}.txt" want)
      list(LENGTH got got_n)
      list(LENGTH want want_n)
      if(want_n EQUAL 0 OR NOT got_n EQUAL want_n)
        message(FATAL_ERROR "audio-${kind}: ${got_n} lines against ${want_n} in ${PINS}/${kind}.txt")
      endif()
      math(EXPR last "${got_n} - 1")
      foreach(i RANGE ${last})
        list(GET got ${i} got_line)
        list(GET want ${i} want_line)
        if(NOT got_line STREQUAL want_line)
          message(FATAL_ERROR "audio-${kind} line ${i} differs from ${PINS}/${kind}.txt:\n  got  ${got_line}\n  want ${want_line}")
        endif()
      endforeach()
    endforeach()
    file(MAKE_DIRECTORY "${OUT}/card-only")
    execute_process(COMMAND ${CMAKE_COMMAND} -DMODE=write -DRUN=${RUN}
      "-DEMULATOR=${EMULATOR}" -DOUT=${OUT}/card-only -DFIXTURES= -P ${HASHES}
      COMMAND_ERROR_IS_FATAL ANY)
  elseif(CASE STREQUAL "save_unit")
    file(MAKE_DIRECTORY "${OUT}")
    file(REMOVE "${OUT}/round.trip.srm")
    file(WRITE "${OUT}/unrelated.tmp" "unrelated")
    file(WRITE "${OUT}/other.srm.tmp-0" "other")
    file(WRITE "${OUT}/round.trip.srm.tmp-0" "stale")
    execute_process(COMMAND ${EMULATOR} ${UNIT} ${SAVE_CASE} "${BATTERY}" "${OUT}"
      "${OUT}/round.trip.srm" RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE out)
    if(NOT rc STREQUAL "0")
      message(FATAL_ERROR "save ${SAVE_CASE}: exit ${rc}\n${out}")
    endif()
    # A load removes this save's stale temporary, but only where the cartridge has a save at all.
    file(GLOB temps "${OUT}/*.tmp-*")
    list(REMOVE_ITEM temps "${OUT}/other.srm.tmp-0")
    if(SAVE_CASE STREQUAL "excluded")
      list(REMOVE_ITEM temps "${OUT}/round.trip.srm.tmp-0")
    endif()
    file(READ "${OUT}/unrelated.tmp" unrelated)
    file(READ "${OUT}/other.srm.tmp-0" other)
    if(temps OR NOT unrelated STREQUAL "unrelated" OR NOT other STREQUAL "other")
      message(FATAL_ERROR "save temporary cleanup affected unrelated data or leaked its temporary: ${temps}")
    endif()
  elseif(CASE STREQUAL "save_sweep")
    # Temporaries 0 to 1023 of this save go at load; index 1024, a name that is no number, and
    # another save's temporary stay.
    file(MAKE_DIRECTORY "${OUT}")
    file(REMOVE "${OUT}/battery.srm")
    foreach(name IN ITEMS battery.srm.tmp-0 battery.srm.tmp-1023 battery.srm.tmp-1024
        battery.srm.tmp-unrelated other.srm.tmp-0)
      file(WRITE "${OUT}/${name}" "${name}")
    endforeach()
    expect_exit(0 "^$" --frames 1 --save-dir "${OUT}" "${BATTERY}")
    foreach(name IN ITEMS battery.srm.tmp-0 battery.srm.tmp-1023)
      if(EXISTS "${OUT}/${name}")
        message(FATAL_ERROR "the load left the stale ${name}")
      endif()
    endforeach()
    foreach(name IN ITEMS battery.srm.tmp-1024 battery.srm.tmp-unrelated other.srm.tmp-0)
      if(NOT EXISTS "${OUT}/${name}")
        message(FATAL_ERROR "the sweep removed ${name}")
      endif()
      file(READ "${OUT}/${name}" data)
      if(NOT data STREQUAL name)
        message(FATAL_ERROR "the sweep or the flush changed ${name}")
      endif()
    endforeach()
    if(NOT EXISTS "${OUT}/battery.srm")
      message(FATAL_ERROR "the run after the sweep wrote no save")
    endif()
  elseif(CASE STREQUAL "save_process")
    file(MAKE_DIRECTORY "${OUT}")
    file(WRITE "${OUT}/battery.srm.tmp-unrelated" "unrelated")
    foreach(existing 0 1)
      file(REMOVE "${OUT}/battery.srm")
      if(existing)
        file(WRITE "${OUT}/battery.srm" "old")
      endif()
      expect_exit(${CODE} "${REGEX}" --frames 60 --hash --save-dir "${OUT}" "${BATTERY}")
      file(GLOB temps "${OUT}/battery.srm.tmp-*")
      list(REMOVE_ITEM temps "${OUT}/battery.srm.tmp-unrelated")
      if(CODE EQUAL 0 OR RENAMED)
        file(READ "${OUT}/battery.srm" data LIMIT 3 HEX)
        file(SIZE "${OUT}/battery.srm" n)
        if(NOT data STREQUAL "a55a01" OR NOT n EQUAL 32768)
          message(FATAL_ERROR "signal save differs from a completed frame")
        endif()
      elseif(existing)
        file(READ "${OUT}/battery.srm" data)
        if(NOT data STREQUAL "old")
          message(FATAL_ERROR "failed or interrupted write changed previous target")
        endif()
      elseif(EXISTS "${OUT}/battery.srm")
        message(FATAL_ERROR "failed or interrupted first creation published a target")
      endif()
      if(CODE EQUAL 93)
        list(LENGTH temps n)
        if(NOT n EQUAL 1)
          message(FATAL_ERROR "interrupted writer did not leave exactly its own temporary")
        endif()
        list(GET temps 0 temp)
        file(READ "${temp}" data LIMIT 3 HEX)
        file(SIZE "${temp}" n)
        if(NOT data STREQUAL "a55a01" OR NOT n EQUAL 32768)
          message(FATAL_ERROR "interrupted writer's temporary is not the complete new data")
        endif()
        file(REMOVE "${temp}")
      elseif(temps)
        message(FATAL_ERROR "writer leaked a failed temporary")
      endif()
      file(READ "${OUT}/battery.srm.tmp-unrelated" data)
      if(NOT data STREQUAL "unrelated")
        message(FATAL_ERROR "writer touched another process's temporary")
      endif()
    endforeach()
  elseif(CASE STREQUAL "save_kill")
    # RUN is a runner that ends the process at the first flush whose temporary is complete and
    # before it replaces the save. Save A has counter 01; a run that goes on from A must show 02.
    # A killed process does not flush standard output, so a frame line cannot tell where the kill
    # fell. A --png for frame 2 can: it is written inside the frame loop, so it exists only if the
    # run got past frame 1 before the kill.
    file(REMOVE_RECURSE "${OUT}")
    file(MAKE_DIRECTORY "${OUT}")
    execute_process(COMMAND ${EMULATOR} ${PRODUCTION} --frames 60 --save-dir "${OUT}" "${BATTERY}"
      RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
    file(READ "${OUT}/battery.srm" first LIMIT 3 HEX)
    if(NOT rc STREQUAL "0" OR NOT first STREQUAL "a55a01")
      message(FATAL_ERROR "save A: exit ${rc}, holds ${first}, expected a55a01\n${out}${err}")
    endif()
    configure_file("${OUT}/battery.srm" "${OUT}.A" COPYONLY)
    set(png "${OUT}.frame2.png")
    function(kill_run)
      file(REMOVE "${png}")
      execute_process(COMMAND ${EMULATOR} ${RUN} ${ARGN} --frames 10 --hash --png "2:${png}"
        --save-dir "${OUT}" "${BATTERY}" RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
      set(rc "${rc}" PARENT_SCOPE)
      set(out "${out}" PARENT_SCOPE)
      set(err "${err}" PARENT_SCOPE)
    endfunction()
    # Control: with no interval the process is killed only at the end, after frame 2 was drawn.
    kill_run()
    if(NOT rc STREQUAL "93" OR NOT EXISTS "${png}")
      message(FATAL_ERROR "control: exit ${rc}, frame 2 file ${png} missing\n${out}${err}")
    endif()
    file(GLOB leftover "${OUT}/battery.srm.tmp-*")
    file(REMOVE ${leftover})
    kill_run(--save-interval 1)
    if(NOT rc STREQUAL "93" OR out MATCHES "frame [0-9]+ sha256" OR EXISTS "${png}")
      message(FATAL_ERROR "kill at the first periodic flush: exit ${rc}, expected 93 before frame 2\n${out}${err}")
    endif()
    file(GLOB leftover "${OUT}/battery.srm.tmp-*")
    list(LENGTH leftover n)
    if(NOT n EQUAL 1)
      message(FATAL_ERROR "expected exactly one leftover temporary, found: ${leftover}")
    endif()
    file(SIZE "${leftover}" size)
    file(READ "${leftover}" head LIMIT 3 HEX)
    if(NOT size EQUAL 32768 OR NOT head STREQUAL "a55a02")
      message(FATAL_ERROR "the leftover temporary is ${size} bytes starting ${head}, not the complete next save")
    endif()
    execute_process(COMMAND ${CMAKE_COMMAND} -E compare_files "${OUT}.A" "${OUT}/battery.srm"
      RESULT_VARIABLE same)
    if(NOT same EQUAL 0)
      message(FATAL_ERROR "the kill changed the previous save")
    endif()
    # The next production run loads that save, removes the temporary and goes on from it.
    execute_process(COMMAND ${EMULATOR} ${PRODUCTION} --frames 1 --save-dir "${OUT}" "${BATTERY}"
      RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
    file(GLOB leftover "${OUT}/battery.srm.tmp-*")
    file(READ "${OUT}/battery.srm" head LIMIT 3 HEX)
    if(NOT rc STREQUAL "0" OR leftover OR NOT head STREQUAL "a55a02")
      message(FATAL_ERROR "after the kill: exit ${rc}, leftover ${leftover}, save ${head}\n${out}${err}")
    endif()
  elseif(CASE STREQUAL "save_continuation")
    file(MAKE_DIRECTORY "${OUT}")
    file(REMOVE "${OUT}/battery.srm")
    # Each execute_process starts a new runner; the second always uses the
    # installed production path, including after a signalled first child.
    function(continue_save runner old next prefix)
      execute_process(COMMAND ${EMULATOR} ${runner} --frames 60 --hash --mailbox
        --save-dir "${OUT}" "${BATTERY}" RESULT_VARIABLE rc
        OUTPUT_VARIABLE out ERROR_VARIABLE err)
      if(NOT rc STREQUAL "0" OR NOT out MATCHES "status=00\nrom=07\nmagic=ok\ncrc=ok\n"
          OR NOT out MATCHES "01=${next}${old} flags=1\n"
          OR NOT out MATCHES "frame 3 sha256 [0-9a-f]+")
        message(FATAL_ERROR "fresh-process continuation: exit ${rc}\n${out}${err}")
      endif()
      file(READ "${OUT}/battery.srm" data HEX)
      string(REPEAT ff 32765 tail)
      if(NOT data STREQUAL "a55a${prefix}${tail}")
        message(FATAL_ERROR "fresh-process save bytes or exact length differ")
      endif()
      file(GLOB temps "${OUT}/battery.srm.tmp-*")
      if(temps)
        message(FATAL_ERROR "fresh-process writer leaked temporary files")
      endif()
    endfunction()
    continue_save("${RUN}" 00 01 01)
    continue_save("${CONTINUE}" 01 02 02)
    file(REMOVE "${OUT}/battery.srm")
    continue_save("${CONTINUE}" 00 01 01)
    # Wrong save keeps the signature but supplies counter 41, so the guest
    # must report 41 -> 42 rather than either 0 -> 1 or 1 -> 2.
    string(ASCII 165 90 41 signature)
    string(ASCII 255 erased)
    string(REPEAT "${erased}" 32765 tail)
    file(WRITE "${OUT}/battery.srm" "${signature}${tail}")
    continue_save("${CONTINUE}" 29 2A 2a)
    file(APPEND "${OUT}/battery.srm" x)
    file(SHA256 "${OUT}/battery.srm" before)
    execute_process(COMMAND ${EMULATOR} ${CONTINUE} --frames 60 --save-dir "${OUT}" "${BATTERY}"
      RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
    file(SHA256 "${OUT}/battery.srm" after)
    if(NOT rc STREQUAL "4" OR NOT err MATCHES "oversized" OR NOT before STREQUAL after)
      message(FATAL_ERROR "oversize control: exit ${rc}; target changed or refusal missing: ${err}")
    endif()
  elseif(CASE STREQUAL "save_eeprom")
    # Two fresh processes continue one 128-byte EEPROM save. The first four
    # bytes are the signature word 34 12 and the run counter, low byte first;
    # the mailbox must also say the word read back was the one written
    # (record 02) and the write after EWDS changed nothing (record 03).
    # SECOND overrides the expected second prefix, for the negative control.
    if(NOT DEFINED SECOND)
      set(SECOND 34120200)
    endif()
    file(MAKE_DIRECTORY "${OUT}")
    file(REMOVE "${OUT}/eeprom.srm")
    function(continue_eeprom prefix counter)
      execute_process(COMMAND ${EMULATOR} ${RUN} --frames 60 --mailbox
        --save-dir "${OUT}" "${EEPROM}" RESULT_VARIABLE rc
        OUTPUT_VARIABLE out ERROR_VARIABLE err)
      if(NOT rc STREQUAL "0" OR NOT out MATCHES "status=00\nrom=0A\nmagic=ok\ncrc=ok\n"
          OR NOT out MATCHES "01=${counter} flags=1\n"
          OR NOT out MATCHES "02=0001 flags=1\n" OR NOT out MATCHES "03=0001 flags=1\n")
        message(FATAL_ERROR "eeprom continuation: exit ${rc}\n${out}${err}")
      endif()
      file(SIZE "${OUT}/eeprom.srm" size)
      file(READ "${OUT}/eeprom.srm" head HEX LIMIT 4)
      if(NOT size EQUAL 128 OR NOT head STREQUAL "${prefix}")
        message(FATAL_ERROR "eeprom save is ${size} bytes starting ${head}, expected 128 starting ${prefix}")
      endif()
      file(GLOB temps "${OUT}/eeprom.srm.tmp-*")
      if(temps)
        message(FATAL_ERROR "eeprom writer leaked temporary files")
      endif()
    endfunction()
    continue_eeprom(34120100 0001)
    continue_eeprom(${SECOND} 0002)
  elseif(CASE STREQUAL "save_import")
    file(MAKE_DIRECTORY "${OUT}")
    file(REMOVE "${OUT}/battery.srm")
    expect_exit(0 "^$" --frames 1 --save-dir "${OUT}" "${BATTERY}")
    if(NOT EXISTS "${OUT}/battery.srm")
      message(FATAL_ERROR "dirty battery was not persisted")
    endif()
    file(READ "${OUT}/battery.srm" first LIMIT 3 HEX)
    expect_exit(0 "^$" --frames 1 --save-dir "${OUT}" "${BATTERY}")
    file(READ "${OUT}/battery.srm" second LIMIT 3 HEX)
    file(SIZE "${OUT}/battery.srm" n)
    if(NOT first STREQUAL "a55a01" OR NOT second STREQUAL "a55a02" OR NOT n EQUAL 32768)
      message(FATAL_ERROR "first-create/overwrite continuation bytes differ")
    endif()
    expect_exit(2 "requires content" --frames 1 --save-dir "${OUT}")
    expect_exit(2 "usage" --frames 1 --save-dir)
    expect_exit(2 "usage" --probe "${BATTERY}" --save-dir "${OUT}")
    expect_exit(1 "existing directory" --frames 1 --save-dir "${OUT}/missing" "${BATTERY}")
    expect_exit(1 "existing directory" --frames 1 --save-dir "${BATTERY}" "${BATTERY}")
    expect_exit(77 "unknown persistence" --frames 1 --save-dir "${OUT}" "${SEEDS}/di-halt.sms")
    string(REPEAT x 32769 large)
    file(WRITE "${OUT}/battery.srm" "${large}")
    file(SHA256 "${OUT}/battery.srm" before)
    expect_exit(4 "oversized" --frames 1 --save-dir "${OUT}" "${BATTERY}")
    file(SHA256 "${OUT}/battery.srm" after)
    if(NOT before STREQUAL after)
      message(FATAL_ERROR "oversize save was changed")
    endif()
    # An EEPROM save is 0 or 128 bytes: the refusal says which length it wanted, and the file stays.
    file(MAKE_DIRECTORY "${OUT}/eeprom")
    foreach(size 100 129)
      string(REPEAT x ${size} wrong)
      file(WRITE "${OUT}/eeprom/eeprom.srm" "${wrong}")
      file(SHA256 "${OUT}/eeprom/eeprom.srm" before)
      if(size EQUAL 100)
        expect_exit(4 "(length|size)[^\n]*128" --frames 1 --save-dir "${OUT}/eeprom" "${EEPROM}")
      else()
        expect_exit(4 "oversized" --frames 1 --save-dir "${OUT}/eeprom" "${EEPROM}")
      endif()
      file(SHA256 "${OUT}/eeprom/eeprom.srm" after)
      if(NOT before STREQUAL after)
        message(FATAL_ERROR "a refused ${size}-byte EEPROM save was changed")
      endif()
    endforeach()
    string(REPEAT x 128 whole)
    file(WRITE "${OUT}/eeprom/eeprom.srm" "${whole}")
    expect_exit(0 "^$" --frames 1 --save-dir "${OUT}/eeprom" "${EEPROM}")
    file(REMOVE "${OUT}/battery.srm")
    file(MAKE_DIRECTORY "${OUT}/battery.srm")
    expect_exit(1 "cannot read" --frames 1 --save-dir "${OUT}" "${BATTERY}")
    file(REMOVE_RECURSE "${OUT}/battery.srm")
    file(MAKE_DIRECTORY "${OUT}/save café 游戏")
    configure_file("${BATTERY}" "${OUT}/round.trip café 游戏.sms" COPYONLY)
    expect_exit(0 "^$" --frames 1 --save-dir "${OUT}/save café 游戏" "${OUT}/round.trip café 游戏.sms")
    if(NOT EXISTS "${OUT}/save café 游戏/round.trip café 游戏.srm")
      message(FATAL_ERROR "Unicode save target missing")
    endif()
  elseif(CASE STREQUAL "save_interval_usage")
    expect_exit(2 "usage" --frames 1 --save-dir "${OUT}" "${OUT}/x.sms" --save-interval abc)
    expect_exit(2 "usage" --frames 1 --save-dir "${OUT}" "${OUT}/x.sms" --save-interval 1x)
    expect_exit(2 "usage" --frames 1 --save-dir "${OUT}" "${OUT}/x.sms" --save-interval 4294967296)
    expect_exit(2 "usage" --frames 1 --save-dir "${OUT}" "${OUT}/x.sms" --save-interval)
    expect_exit(2 "usage" --frames 1 "${OUT}/x.sms" --save-interval 5)
    expect_exit(2 "usage" --probe "${OUT}/x.sms" --save-interval 5)
  elseif(CASE STREQUAL "save_interval")
    # Each run starts from an empty directory, so the saves compare byte for byte.
    function(interval_run name)
      file(REMOVE_RECURSE "${OUT}/${name}")
      file(MAKE_DIRECTORY "${OUT}/${name}")
      execute_process(COMMAND ${EMULATOR} ${RUN} --frames 10 --hash --save-dir "${OUT}/${name}"
        ${ARGN} "${BATTERY}" RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
      if(NOT rc STREQUAL "0" OR NOT out MATCHES "^frame 10 sha256 [0-9a-f]+\n$")
        message(FATAL_ERROR "interval run ${name}: exit ${rc}\n${out}${err}")
      endif()
      file(SHA256 "${OUT}/${name}/battery.srm" sum)
      set(${name}_out "${out}" PARENT_SCOPE)
      set(${name}_sum "${sum}" PARENT_SCOPE)
    endfunction()
    interval_run(plain)
    interval_run(off --save-interval 0)
    interval_run(every --save-interval 1)
    interval_run(over --save-interval 20)
    foreach(name off every over)
      if(NOT ${name}_out STREQUAL plain_out OR NOT ${name}_sum STREQUAL plain_sum)
        message(FATAL_ERROR "--save-interval changed the hash or the save bytes (${name})")
      endif()
    endforeach()
    file(GLOB temps "${OUT}/*/battery.srm.tmp-*")
    if(temps)
      message(FATAL_ERROR "an interval run leaked a temporary: ${temps}")
    endif()
  elseif(CASE STREQUAL "save_interval_fault")
    # RUN fails every write of the save, so a flush that happens is a failure that shows.
    foreach(name every over)
      file(REMOVE_RECURSE "${OUT}/${name}")
      file(MAKE_DIRECTORY "${OUT}/${name}")
    endforeach()
    expect_exit(1 "save: cannot" --save-interval 1 --frames 10 --hash --save-dir "${OUT}/every" "${BATTERY}")
    execute_process(COMMAND ${EMULATOR} ${RUN} --save-interval 1 --frames 10 --hash
      --save-dir "${OUT}/every" "${BATTERY}" OUTPUT_VARIABLE out ERROR_VARIABLE err)
    if(out MATCHES "frame 10 sha256")
      message(FATAL_ERROR "a periodic flush failure was deferred past frame 10:\n${out}")
    endif()
    # Standard output is flushed after the exit flush's message, so the two are checked apart.
    execute_process(COMMAND ${EMULATOR} ${RUN} --save-interval 20 --frames 10 --hash
      --save-dir "${OUT}/over" "${BATTERY}" RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
    if(NOT rc STREQUAL "1" OR NOT out MATCHES "frame 10 sha256 [0-9a-f]+\n" OR NOT err MATCHES "save: cannot")
      message(FATAL_ERROR "interval over the run: exit ${rc}, want 1 with the frame 10 line then the exit flush failing\n${out}${err}")
    endif()
  elseif(CASE STREQUAL "video_override")
    expect_exit(0 "^$" --frames 1 --machine sms1)
    expect_exit(0 "^$" --frames 1 --machine sms --video-mode sms)
    expect_exit(0 "^$" --frames 1 --machine gg --video-mode gg)
    expect_exit(0 "^$" --frames 1 --machine gg --video-mode sms)
    expect_exit(0 "^$" --frames 1 --machine gg --video-mode auto)
    expect_exit(77 "unsupported" --frames 1 --machine sms --video-mode gg)
    expect_exit(77 "unsupported" --frames 1 --machine sms1 --video-mode gg)
    expect_exit(2 "usage" --frames 1 --video-mode invalid)
    expect_exit(2 "usage" --frames 1 --video-mode)
  elseif(CASE STREQUAL "lag")
    string(REPEAT "p1=00 p2=00 pause=0 reset=0 start=0\n" 14 body)
    file(WRITE "${OUT}" "${body}")
    expect_exit(0 "(^|\n)lag_frames=14\n" ${SEEDS}/di-halt.sms --frames 14 --input ${OUT})
    expect_exit(0 "(^|\n)lag_frames=4\n" ${READ_ROM} --frames 14 --input ${OUT})
    expect_exit(0 "(^|\n)lag_frames=4\n" ${READ_ROM} --frames 14 --input ${OUT}
      --port1 none --port2 none)
  elseif(CASE STREQUAL "input")
    set(record "p1=00 p2=00 pause=0 reset=0 start=0")
    foreach(ending IN ITEMS lf crlf none)
      set(eol "\n")
      if(ending STREQUAL "crlf")
        set(eol "\r\n")
      elseif(ending STREQUAL "none")
        set(eol "")
      endif()
      set(style LF)
      if(ending STREQUAL "crlf")
        set(style CRLF)
      endif()
      if(ending STREQUAL "none")
        file(WRITE "${OUT}" "${record}")
      else()
        file(CONFIGURE OUTPUT "${OUT}" CONTENT "${record}${eol}" NEWLINE_STYLE ${style})
      endif()
      file(READ "${OUT}" actual HEX)
      string(HEX "${record}${eol}" expected)
      if(NOT actual STREQUAL expected)
        message(FATAL_ERROR "input fixture ${ending}: byte mismatch ${actual}")
      endif()
      message(STATUS "input fixture ${ending}: exact bytes verified")
      expect_exit(0 "frame 1 sha256" --frames 1 --input ${OUT} --hash)
    endforeach()
    set(bad "" "${record}\n${record}\n" "p1=40 p2=00 pause=0 reset=0 start=0\n"
      "p1=00 p2=80 pause=0 reset=0 start=0\n" "p1=0G p2=00 pause=0 reset=0 start=0\n"
      "p1=00 p1=00 pause=0 reset=0 start=0\n" "p1=00 p2=00 pause=2 reset=0 start=0\n"
      "p1=00 p2=00 pause=0 reset=2 start=0\n" "p1=00 p2=00 pause=0 reset=0 start=2\n"
      "${record} unknown=0\n" "\n" "#comment\n" "p1=00\n" "${record}\r")
    string(REPEAT x 100 long)
    list(APPEND bad "${long}\n")
    foreach(body IN LISTS bad)
      file(WRITE "${OUT}" "${body}")
      expect_exit(2 "input file error" --frames 1 --input ${OUT} --hash)
    endforeach()
    file(WRITE "${OUT}" "${record}\n")
    expect_exit(2 "input file error" --frames 2 --input ${OUT} --hash)
    expect_exit(1 "input file error" --frames 1 --input ${OUT}.missing)
    expect_exit(2 "usage" --frames 1 --port1 other)
    expect_exit(2 "usage" --frames 1 --port2)
    expect_exit(0 "frame 1 sha256" --frames 1 --port1 none --port2 none --hash)
  elseif(CASE STREQUAL "device_usage")
    expect_exit(0 "frame 1 sha256" --frames 1 --port1 auto --hash)
    expect_exit(0 "frame 1 sha256" --frames 1 --port1 paddle --region ntsc-j --hash)
    expect_exit(0 "frame 1 sha256" --frames 1 --port2 sports --hash)
    expect_exit(0 "frame 1 sha256" --frames 1 --port1 lphaser --hash)
    expect_exit(0 "frame 1 sha256" --frames 1 --port2 lphaser --hash)
    expect_exit(2 "usage" --frames 1 --port1 phaser)
    expect_exit(2 "usage" --frames 1 --port2 Lphaser)
    expect_exit(2 "usage" --frames 1 --port1 lightgun)
    expect_exit(2 "usage" --frames 1 --port1 AUTO)
    expect_exit(2 "usage" --frames 1 --port2 Paddle)
  elseif(CASE STREQUAL "axes_grammar")
    # The 75-byte line: every malformed form is exit 2 with one message and no frame run.
    set(base "p1=00 p2=00 pause=0 reset=0 start=0")
    set(good "${base} x1=+00000 y1=+00000 x2=+00000 y2=+00000")
    set(bad "")
    foreach(len RANGE 36 74)
      string(SUBSTRING "${good}" 0 ${len} cut)
      list(APPEND bad "${cut}\n")
    endforeach()
    list(APPEND bad "${good}0\n" "${good}\n${good}\n" "${good} \n"
      "${base} y1=+00000 x1=+00000 x2=+00000 y2=+00000\n"
      "${base} x1=+00000 y1=+00000 x2=+00000 x1=+00000\n"
      "${base} x1=+00000 y1=+00000 x2=+00000 z2=+00000\n"
      "${base} x1=+00000 y1=+00000 x2=+00000 y2=+00000 \n"
      "${base} x1=+0000 y1=+000000 x2=+00000 y2=+00000\n"
      "${base} x1=000000 y1=+00000 x2=+00000 y2=+00000\n"
      "${base} x1=+00000 y1=+00000 x2=+00000 y2=*00000\n"
      "${base} x1=+00000 y1=+00000 x2=+00000 y2=+0000a\n"
      "${base} x1=+32768 y1=+00000 x2=+00000 y2=+00000\n"
      "${base} x1=+00000 y1=-32768 x2=+00000 y2=+00000\n"
      "${base} x1=+00000 y1=+00000 x2=+99999 y2=+00000\n"
      "${base} x1=+00000 y1=+00000 x2=+00000 y2=-99999\n"
      "${base}  x1=+00000 y1=+00000 x2=+00000 y2=+0000\n")
    foreach(body IN LISTS bad)
      write_bytes("${OUT}" UNIX "${body}")
      expect_exit(2 "^cogmaster-run: input file error[^\n]*\n$" --frames 1 --port1 sports
        --port2 sports --input ${OUT} --hash)
    endforeach()
    # A refusal on the last record still stops before frame 1.
    write_bytes("${OUT}" UNIX "${good}\n${base} x1=+32768 y1=+00000 x2=+00000 y2=+00000\n")
    expect_exit(2 "^cogmaster-run: input file error[^\n]*\n$" --frames 2 --port1 sports
      --input ${OUT} --hash)
    # The edges of the range, a CRLF ending and a missing final newline are accepted.
    write_bytes("${OUT}" CRLF "${base} x1=+32767 y1=-32767 x2=-32767 y2=+32767\n")
    file(APPEND "${OUT}" "${good}")
    expect_exit(0 "frame 2 sha256" --frames 2 --port1 sports --port2 sports --input ${OUT} --hash)
  elseif(CASE STREQUAL "axes_device")
    # Motion on a port whose resolved device takes none is exit 2 before frame 1, naming both.
    set(base "p1=00 p2=00 pause=0 reset=0 start=0")
    set(refused "^cogmaster-run: input file error: port ([12]) is ([a-z]+), which takes no motion\n$")
    foreach(axes IN ITEMS "x1=+00001 y1=+00000 x2=+00000 y2=+00000"
        "x1=+00000 y1=-00001 x2=+00000 y2=+00000")
      write_bytes("${OUT}" UNIX "${base}\n${base} ${axes}\n")
      expect_exit(2 "port 1 is pad, which takes no motion" --frames 2 --input ${OUT} --hash)
      expect_exit(2 "port 1 is none, which takes no motion" --frames 2 --port1 none --input ${OUT}
        --hash)
      expect_exit(2 "${refused}" --frames 2 --port1 none --input ${OUT} --hash)
    endforeach()
    foreach(axes IN ITEMS "x1=+00000 y1=+00000 x2=+00001 y2=+00000"
        "x1=+00000 y1=+00000 x2=+00000 y2=-00001")
      write_bytes("${OUT}" UNIX "${base} ${axes}\n")
      expect_exit(2 "port 2 is pad, which takes no motion" --frames 1 --port1 sports --input ${OUT}
        --hash)
      expect_exit(2 "port 2 is none, which takes no motion" --frames 1 --port1 sports
        --port2 none --input ${OUT} --hash)
    endforeach()
    # A Sports Pad takes both axes; a Paddle takes x and accepts y, which the core ignores.
    write_bytes("${OUT}" UNIX "${base} x1=+00005 y1=-00003 x2=-00007 y2=+00009\n")
    expect_exit(0 "frame 1 sha256" --frames 1 --port1 sports --port2 sports --input ${OUT} --hash)
    expect_exit(0 "frame 1 sha256" --frames 1 --region ntsc-j --port1 paddle --port2 paddle
      --input ${OUT} --hash)
    # An all-zero 75-byte line is accepted for any device.
    write_bytes("${OUT}" UNIX "${base} x1=+00000 y1=+00000 x2=+00000 y2=+00000\n")
    expect_exit(0 "frame 1 sha256" --frames 1 --port1 none --port2 pad --input ${OUT} --hash)
  elseif(CASE STREQUAL "axes_equivalent")
    # The 35-byte line means all four axes are 0; a script that differs in one digit is told apart
    # by the digest in the execution line.
    set(base "p1=10 p2=00 pause=0 reset=0 start=0")
    string(REPEAT "${base}\n" 3 short)
    string(REPEAT "${base} x1=+00000 y1=+00000 x2=+00000 y2=+00000\n" 3 long)
    write_bytes("${OUT}.short" UNIX "${short}")
    write_bytes("${OUT}.long" UNIX "${long}")
    write_bytes("${OUT}.a" UNIX "${base} x1=+00004 y1=+00000 x2=+00000 y2=+00000\n${base}\n${base}\n")
    write_bytes("${OUT}.b" UNIX "${base} x1=+00005 y1=+00000 x2=+00000 y2=+00000\n${base}\n${base}\n")
    write_bytes("${OUT}.expect" UNIX "ram@2:$C200=00\n")
    set(digest "script_sha256=([0-9a-f]+) ")
    foreach(device IN ITEMS pad sports)
      foreach(form IN ITEMS short long)
        execute_process(COMMAND ${EMULATOR} ${RUN} --frames 3 --port1 ${device} --input ${OUT}.${form}
          --hash RESULT_VARIABLE rc OUTPUT_VARIABLE run_${form} ERROR_VARIABLE run_${form})
        if(NOT rc STREQUAL "0" OR NOT run_${form} MATCHES "frame 3 sha256 [0-9a-f]+\nlag_frames=[0-9]+\n$")
          message(FATAL_ERROR "${device} ${form}: exit ${rc}\n${run_${form}}")
        endif()
      endforeach()
      if(NOT run_short STREQUAL run_long)
        message(FATAL_ERROR "${device}: the 35-byte and the all-zero 75-byte forms differ:\n"
          "${run_short}\n${run_long}")
      endif()
    endforeach()
    foreach(form IN ITEMS short long a b)
      execute_process(COMMAND ${EMULATOR} ${RUN} --frames 3 --port1 sports --input ${OUT}.${form}
        --expect ${OUT}.expect RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE out)
      if(NOT rc STREQUAL "0" OR NOT out MATCHES "${digest}")
        message(FATAL_ERROR "${form}: exit ${rc}\n${out}")
      endif()
      set(digest_${form} "${CMAKE_MATCH_1}")
    endforeach()
    if(digest_a STREQUAL digest_b OR digest_a STREQUAL digest_short OR digest_short STREQUAL digest_long)
      message(FATAL_ERROR "the script digests do not tell the scripts apart: "
        "${digest_short} ${digest_long} ${digest_a} ${digest_b}")
    endif()
    # The test card never reads a controller, so the two scripts differing in one digit give the
    # same frame hash here; what they differ in is the digest above.
    foreach(form IN ITEMS a b)
      execute_process(COMMAND ${EMULATOR} ${RUN} --frames 3 --port1 sports --input ${OUT}.${form}
        --hash RESULT_VARIABLE rc OUTPUT_VARIABLE hash_${form} ERROR_VARIABLE hash_${form})
    endforeach()
    if(NOT hash_a STREQUAL hash_b)
      message(FATAL_ERROR "a pad-only image changed its hash with the delta:\n${hash_a}\n${hash_b}")
    endif()
  elseif(CASE STREQUAL "eyes_line")
    # The test card writes no eye: all frames count as none, and the report changes no other line.
    set(eyes "eyes frames=([0-9]+) none=([0-9]+) left=([0-9]+) right=([0-9]+) flips=([0-9]+) last=(left|right|none)\n$")
    execute_process(COMMAND ${EMULATOR} ${RUN} --frames 5 --hash --eyes RESULT_VARIABLE rc
      OUTPUT_VARIABLE with ERROR_VARIABLE with)
    execute_process(COMMAND ${EMULATOR} ${RUN} --frames 5 --hash RESULT_VARIABLE rc2
      OUTPUT_VARIABLE without ERROR_VARIABLE without)
    if(NOT rc STREQUAL "0" OR NOT rc2 STREQUAL "0" OR NOT with MATCHES "${eyes}")
      message(FATAL_ERROR "--eyes: exit ${rc} ${rc2}, output\n${with}")
    endif()
    math(EXPR total "${CMAKE_MATCH_2} + ${CMAKE_MATCH_3} + ${CMAKE_MATCH_4}")
    if(NOT CMAKE_MATCH_1 STREQUAL "5" OR NOT total STREQUAL "5" OR NOT CMAKE_MATCH_5 STREQUAL "0"
        OR NOT CMAKE_MATCH_6 STREQUAL "none")
      message(FATAL_ERROR "--eyes: the counts do not add up to the frames run:\n${with}")
    endif()
    string(REGEX REPLACE "eyes [^\n]*\n$" "" stripped "${with}")
    if(NOT stripped STREQUAL without)
      message(FATAL_ERROR "--eyes changed the other output:\n${with}\n${without}")
    endif()
    # The report follows the lag line of a scripted run and an early stop counts frames run.
    write_bytes("${OUT}" UNIX "p1=00 p2=00 pause=0 reset=0 start=0\n")
    expect_exit(0 "lag_frames=[0-9]+\neyes frames=1 none=1 left=0 right=0 flips=0 last=none\n$"
      --frames 1 --input ${OUT} --eyes)
  elseif(CASE STREQUAL "eyes_probe")
    expect_exit(2 "usage.*--probe" --eyes --probe ${SEEDS}/di-halt.sms)
    expect_exit(2 "usage.*--probe" --probe ${SEEDS}/di-halt.sms --eyes)
    expect_exit(2 "usage" --eyes)
  elseif(CASE STREQUAL "gun_crop")
    # The runner's script aim is the field dot: every machine that can hold a phaser has its crop at 0,0, so a
    # script aim at the square's picture position gives the record inside the row of tests/diag/expected.csv
    # and the same script 16 dots to the left gives one outside it.
    file(STRINGS "${CSV}" row REGEX "^light_phaser,0C:")
    if(NOT row MATCHES "^light_phaser,0C:[^,]*,[*],([0-9A-F]+)-([0-9A-F]+),")
      message(FATAL_ERROR "${CSV}: no light_phaser row 0C with a range")
    endif()
    math(EXPR low "0x${CMAKE_MATCH_1}")
    math(EXPR high "0x${CMAKE_MATCH_2}")
    foreach(aim IN ITEMS aimed left)
      set(script ${PHASER})
      if(aim STREQUAL "left")
        set(script ${LEFT})
      endif()
      execute_process(COMMAND ${EMULATOR} ${RUN} ${IMAGE} --frames ${FRAMES} --port1 lphaser --input ${script}
        --mailbox RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
      if(NOT rc STREQUAL "0" OR NOT out MATCHES "(^|\n)status=00\n(.*\n)?0C=([0-9A-F]+) flags=")
        message(FATAL_ERROR "gun_crop (${aim}): exit ${rc}, no record 0C\n${out}${err}")
      endif()
      math(EXPR record "0x${CMAKE_MATCH_3}")
      if(aim STREQUAL "aimed" AND (record LESS low OR record GREATER high))
        message(FATAL_ERROR "gun_crop: record 0C is ${record}, expected ${low}-${high}\n${out}")
      elseif(aim STREQUAL "left" AND record GREATER_EQUAL low AND record LESS_EQUAL high)
        message(FATAL_ERROR "gun_crop: the aim 16 dots left still gives record 0C ${record} in ${low}-${high}\n${out}")
      endif()
    endforeach()
  elseif(CASE STREQUAL "gun_noshot")
    # The host half of the no-shot invariant: the fixture run with the phaser parked, and with its aim on the
    # dark square, prints the frame hash, the lag line and the mailbox records of the run with a pad and the
    # same script's aim columns zeroed (the execution line names the device and the script, so it is dropped).
    # The aim on the lit square must change the records. The picture the fixture draws is fixed, so its frame
    # hash cannot differ: the records are what keep the equality from being vacuous.
    set(runs pad parked dark aimed)
    foreach(run IN LISTS runs)
      set(device lphaser)
      set(script ${PHASER})
      if(run STREQUAL "pad")
        set(device pad)
        set(script ${PARKED})
      elseif(run STREQUAL "parked")
        set(script ${PARKED})
      elseif(run STREQUAL "dark")
        set(script ${DARK})
      endif()
      execute_process(COMMAND ${EMULATOR} ${RUN} ${IMAGE} --frames ${FRAMES} --port1 ${device} --input ${script}
        --hash --mailbox RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
      if(NOT rc STREQUAL "0" OR NOT out MATCHES "(^|\n)frame [0-9]+ sha256 [0-9a-f]+\n")
        message(FATAL_ERROR "gun_noshot (${run}): exit ${rc}\n${out}${err}")
      endif()
      string(REGEX REPLACE "(^|\n)execution [^\n]*" "\\1" out "${out}")
      set(out_${run} "${out}")
    endforeach()
    foreach(run IN ITEMS parked dark)
      if(NOT out_${run} STREQUAL out_pad)
        message(FATAL_ERROR "gun_noshot: the ${run} phaser run differs from the pad run\n${out_${run}}\n${out_pad}")
      endif()
    endforeach()
    if(out_aimed STREQUAL out_pad OR NOT out_aimed MATCHES "(^|\n)status=00\n")
      message(FATAL_ERROR "gun_noshot: the aim on the lit square changed nothing\n${out_aimed}")
    endif()
  elseif(CASE STREQUAL "gun_args")
    # A phaser's aim is a 1-based field dot, 0 0 or the short line is no aim, and every other pair is exit 2
    # before frame 1 naming the port and the device; the same file on a pad port is exit 2 too.
    set(base "p1=00 p2=00 pause=0 reset=0 start=0")
    set(refused "^cogmaster-run: input file error: port ([12]) is lphaser, which takes[^\n]*\n$")
    foreach(axes IN ITEMS "+00257 y1=+00001" "+00001 y1=+00241" "-00001 y1=+00001" "+00001 y1=-00001"
        "+00005 y1=+00000" "+00000 y1=+00005")
      write_bytes("${OUT}" UNIX "${base} x1=${axes} x2=+00000 y2=+00000\n${base}\n")
      expect_exit(2 "${refused}" --frames 2 --port1 lphaser --input ${OUT} --hash)
      string(REPLACE "y1=" "y2=" swapped "${axes}")
      write_bytes("${OUT}" UNIX "${base} x1=+00000 y1=+00000 x2=${swapped}\n${base}\n")
      expect_exit(2 "${refused}" --frames 2 --port2 lphaser --input ${OUT} --hash)
    endforeach()
    # A refusal on the last record still stops before frame 1.
    write_bytes("${OUT}" UNIX "${base}\n${base} x1=+00257 y1=+00001 x2=+00000 y2=+00000\n")
    expect_exit(2 "port 1 is lphaser" --frames 2 --port1 lphaser --input ${OUT} --hash)
    foreach(axes IN ITEMS "x1=+00001 y1=+00001" "x1=+00256 y1=+00240" "x1=+00000 y1=+00000")
      write_bytes("${OUT}" UNIX "${base} ${axes} x2=+00000 y2=+00000\n${base}\n")
      expect_exit(0 "frame 2 sha256" --frames 2 --port1 lphaser --input ${OUT} --hash)
      string(REPLACE "x1=" "x2=" moved "${axes}")
      string(REPLACE "y1=" "y2=" moved "${moved}")
      write_bytes("${OUT}" UNIX "${base} x1=+00000 y1=+00000 ${moved}\n${base}\n")
      expect_exit(0 "frame 2 sha256" --frames 2 --port2 lphaser --input ${OUT} --hash)
    endforeach()
    # The short line and a held trigger (bit 4 of p1) are accepted.
    write_bytes("${OUT}" UNIX "${base}\np1=10 p2=00 pause=0 reset=0 start=0\n")
    expect_exit(0 "frame 2 sha256" --frames 2 --port1 lphaser --input ${OUT} --hash)
    # Control: a valid aim sent to a pad is exit 2, so the strict check is not why a pad refuses.
    write_bytes("${OUT}" UNIX "${base} x1=+00001 y1=+00001 x2=+00000 y2=+00000\n${base}\n")
    expect_exit(2 "port 1 is pad, which takes no motion" --frames 2 --port1 pad --input ${OUT} --hash)
    expect_exit(0 "frame 2 sha256" --frames 2 --port1 lphaser --input ${OUT} --hash)
    write_bytes("${OUT}" UNIX "${base} x1=+00000 y1=+00000 x2=+00001 y2=+00001\n${base}\n")
    expect_exit(2 "port 2 is pad, which takes no motion" --frames 2 --port1 lphaser --input ${OUT}
      --hash)
  elseif(CASE STREQUAL "gun_gg_refused")
    # The Game Gear has no controller ports, so a phaser there is the shipped refusal without a region hint.
    set(refused "^cogmaster-run: unsupported: lphaser on port ([12]) is not available here; the Game Gear[^\n]*\n$")
    expect_exit(77 "${refused}" --frames 1 --port1 lphaser --machine gg)
    expect_exit(77 "unsupported: lphaser on port 1" --frames 1 --port1 lphaser --machine gg)
    expect_exit(77 "unsupported: lphaser on port 2" --frames 1 --port2 lphaser --machine gg)
    expect_exit(0 "frame 1 sha256" --frames 1 --port1 pad --machine gg --hash)
  elseif(CASE STREQUAL "device_names")
    # The execution line names each device and keeps the prefix older checks match.
    write_bytes("${OUT}" UNIX "ram@2:$C200=00\n")
    set(line "execution machine=[0-9]+ region=[0-9]+ tv=[0-9]+ video=[0-9]+ mapper=[0-9]+ port1=")
    expect_exit(0 "${line}pad port2=pad " --frames 2 --expect ${OUT})
    expect_exit(0 "${line}sports port2=none " --frames 2 --expect ${OUT}
      --port1 sports --port2 none)
    expect_exit(0 "${line}paddle port2=pad " --frames 2 --expect ${OUT}
      --port1 paddle --region ntsc-j)
  elseif(CASE STREQUAL "device_refused")
    expect_exit(77 "unsupported.*--region" --frames 1 --port1 paddle)
    expect_exit(77 "unsupported.*--region" --frames 1 --port2 paddle --region ntsc-u)
    expect_exit(77 "unsupported.*Game Gear" --frames 1 --port1 sports --machine gg)
    expect_exit(77 "unsupported.*Game Gear" --frames 1 --port2 paddle --machine gg
      --region ntsc-j)
    expect_exit(77 "paddle on port 2" --frames 1 --port1 sports --port2 paddle)
    expect_exit(0 "frame 1 sha256" --frames 1 --port1 paddle --region ntsc-j --hash)
    expect_exit(0 "frame 1 sha256" --frames 1 --port1 sports --hash)
  elseif(CASE STREQUAL "device_auto")
    # A pad-only image resolves auto to a pad, whether the option is absent or spelled out,
    # and its probe reports pad defaults after unsupported_reason.
    write_bytes("${OUT}" UNIX "ram@2:$C200=00\n")
    set(line "mapper=[0-9]+ port1=pad port2=pad ")
    expect_exit(0 "${line}" ${SEEDS}/di-halt.sms --frames 2 --expect ${OUT})
    expect_exit(0 "${line}" ${SEEDS}/di-halt.sms --frames 2 --expect ${OUT}
      --port1 auto --port2 auto)
    expect_exit(0 "mapper=[0-9]+ port1=pad port2=sports " ${SEEDS}/di-halt.sms --frames 2
      --expect ${OUT} --port2 sports)
    probe(${SEEDS}/di-halt.sms)
    if(NOT out MATCHES "unsupported_reason=[^\n]*\nport1_default=pad\nport2_default=pad\n$")
      message(FATAL_ERROR "--probe: pad defaults missing after unsupported_reason in\n${out}")
    endif()
  elseif(CASE STREQUAL "device_default")
    # An owned device image names its own controllers and region in the table, so with no option the
    # execution line carries them; an explicit region the profile cannot honour degrades the default to a
    # pad, and an explicit pad overrides the table.
    write_bytes("${OUT}" UNIX "mailbox@120\n")
    set(line "execution machine=[0-9]+ region=")
    set(play --frames 120 --expect ${OUT})
    expect_exit(0 "${line}1 [^\n]*port1=sports port2=sports " ${SPORTS} ${play})
    expect_exit(0 "${line}1 [^\n]*port1=sports port2=sports " ${SPORTS} ${play} --port1 auto)
    expect_exit(0 "${line}1 [^\n]*port1=pad port2=sports " ${SPORTS} ${play} --port1 pad --region ntsc-u)
    expect_exit(0 "${line}2 [^\n]*port1=paddle port2=pad " ${PADDLE} ${play})
    expect_exit(0 "${line}2 [^\n]*port1=paddle port2=pad " ${PADDLE} ${play} --region auto)
    expect_exit(0 "${line}1 [^\n]*port1=pad port2=pad " ${PADDLE} ${play} --region ntsc-u)
  elseif(CASE STREQUAL "device_probe_default")
    probe(${SPORTS})
    expect_key("${out}" identified 1)
    expect_key("${out}" region_guess ntsc-u)
    expect_key("${out}" port1_default sports)
    expect_key("${out}" port2_default sports)
    probe(${PADDLE})
    expect_key("${out}" identified 1)
    expect_key("${out}" region_guess ntsc-j)
    expect_key("${out}" port1_default paddle)
    expect_key("${out}" port2_default pad)
  elseif(CASE STREQUAL "device_paddle_export_refused")
    # A Paddle dump on an export console is refused, never run as a pad; a pad asked for by name still plays.
    expect_exit(77 "unsupported.*--region" ${PADDLE} --frames 1 --port1 paddle --region ntsc-u)
    expect_exit(77 "unsupported.*--region" ${PADDLE} --frames 1 --port1 paddle --region pal)
    expect_exit(0 "frame 1 sha256" ${PADDLE} --frames 1 --port1 pad --region ntsc-u --hash)
  elseif(CASE STREQUAL "probe_keys")
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
    expect_key("${out}" board_source header)
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
    # Another board's mapper: every key is still printed, then exit 77. Two stores to
    # a Korean register are too few to trust, so the scan names korean and refuses; four
    # stores to Janggun's register name a board the core does not play, with its reason.
    # A load says which board and why.
    probe(${SEEDS}/unsupported-board.sms)
    if(NOT rc STREQUAL "77")
      message(FATAL_ERROR "--probe of another board: expected exit 77, got ${rc}\n${out}${err}")
    endif()
    expect_probe_form("${out}")
    expect_key("${out}" mapper_guess unsupported)
    expect_key("${out}" board_source heuristic)
    expect_key("${out}" board_candidate korean)
    expect_exit(77 "unsupported: [^\n]*korean" --frames 1 ${SEEDS}/unsupported-board.sms)
    probe(${SEEDS}/janggun-board.sms)
    if(NOT rc STREQUAL "77")
      message(FATAL_ERROR "--probe of a Janggun-looking image: expected exit 77, got ${rc}\n${out}${err}")
    endif()
    expect_probe_form("${out}")
    expect_key("${out}" unsupported_reason "janggun: reverse bit[^\n]*")
    expect_exit(77 "reverse bit" --frames 1 ${SEEDS}/janggun-board.sms)
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
  elseif(CASE STREQUAL "trace_fault")
    # LD A,$00 then IN A,($00): port $00 is undriven on the export console,
    # so the first fault is that read, with A * 256 + n on the bus.
    set(hex "[0-9A-F]")
    expect_exit(0 "^fault frame=[0-9]+ line=[0-9]+ dot=[0-9]+ pc=[$]${hex}${hex}${hex}${hex} op=[$]DB addr=[$]0000 width=8 dev=ctrl kind=unmapped_read\n$"
      ${SEEDS}/in-a-00.sms --frames 10 --trace first-fault)
    # Enable display without Mode 4: the VDP fault must keep its public names.
    string(ASCII 243 62 64 211 191 62 129 211 191 118 code)
    string(REPEAT "v" 1014 padding)
    set(rom "${SEEDS}/../unsupported-video.sms")
    file(WRITE "${rom}" "${code}${padding}")
    expect_exit(0 "^fault frame=[0-9]+ [^\n]*dev=vdp kind=unsupported_video_mode\n$"
      "${rom}" --frames 2 --trace first-fault)
    file(REMOVE "${rom}")
  elseif(CASE STREQUAL "mailbox_stop")
    # The stop rule frame by frame, then an image that never writes a
    # mailbox: its status reads $00 from power-on, which is not done, so
    # every frame runs.
    unit(mailbox_stop)
    expect_exit(0 "^status=00\nrom=00\nmagic=bad\ncrc=bad\nframe 3 sha256 [0-9a-f]+\n$"
      ${SEEDS}/di-halt.sms --frames 3 --mailbox --hash)
  elseif(CASE STREQUAL "png_unreached")
    # --trace first-fault stops after frame 1 of this image, so a --png for
    # frame 10 is not written: a line on standard error names it, standard
    # output holds only the fault, and the exit status stays 0. A --png for
    # a frame that ran is still written.
    file(REMOVE "${OUT}.10.png" "${OUT}.1.png")
    execute_process(COMMAND ${EMULATOR} ${RUN} ${SEEDS}/in-a-00.sms --frames 10
      --trace first-fault --png "1:${OUT}.1.png" --png "10:${OUT}.10.png"
      RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
    if(NOT rc STREQUAL "0" OR NOT out MATCHES "^fault frame=[0-9]+ [^\n]*\n$"
        OR NOT err MATCHES "^cogmaster-run: --png 10:[^\n]* not written[^\n]*\n$"
        OR EXISTS "${OUT}.10.png" OR NOT EXISTS "${OUT}.1.png")
      message(FATAL_ERROR "--png past a first-fault stop: exit ${rc}\nstdout:\n${out}stderr:\n${err}")
    endif()
  elseif(CASE STREQUAL "trace_none")
    # DI; HALT touches nothing undriven; with --hash the hash line follows.
    expect_exit(0 "^fault none\n$" ${SEEDS}/di-halt.sms --frames 10 --trace first-fault)
    expect_exit(0 "^fault none\nframe 10 sha256 [0-9a-f]+\n$"
      ${SEEDS}/di-halt.sms --frames 10 --trace first-fault --hash)
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
    # A failed close removes the wide-opened target and preserves its CP1252 alias.
    set(target "${OUT}.café.png")
    set(alias "${OUT}.cafÃ©.png")
    file(WRITE "${alias}" "unrelated")
    execute_process(COMMAND ${EMULATOR} ${FAIL_CLOSE} --frames 1 --png "1:${target}"
      RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
    if(NOT rc STREQUAL "1" OR NOT err MATCHES "cannot write" OR EXISTS "${target}"
        OR NOT EXISTS "${alias}")
      message(FATAL_ERROR "PNG close cleanup changed the wrong path: exit ${rc}\n${out}${err}")
    endif()
    file(READ "${alias}" data)
    if(NOT data STREQUAL "unrelated")
      message(FATAL_ERROR "PNG cleanup changed unrelated file bytes")
    endif()
    file(REMOVE "${alias}")
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
    string(REPEAT "p1=00 p2=00 pause=0 reset=0 start=0\n" 60 body)
    file(WRITE "${SEEDS}/stop.input" "${body}")
    expect_exit(0 "^frame 3 sha256 ${EXPECTED}\nlag_frames=3\n$"
      --frames 60 --input ${SEEDS}/stop.input --hash)
  elseif(CASE STREQUAL "exit_failure")
    # A --png path that cannot be opened is a failure, not a usage error.
    expect_exit(1 "cannot write" --frames 1 --png "1:${OUT}.missing/x.png")
  elseif(CASE STREQUAL "no_args")
    expect_exit(2 "usage")
  elseif(CASE STREQUAL "unknown")
    # Options a later release adds are refused now, and --help does not
    # mention them; --help and --version take nothing else; --trace takes
    # only first-fault and --region only its four names.
    foreach(opt --capabilities)
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
    if(NOT rc STREQUAL "0" OR out MATCHES "--capabilities")
      message(FATAL_ERROR "--help: expected exit 0 and no later option, got exit ${rc}\n${out}")
    endif()
  elseif(CASE STREQUAL "hash_audio" OR CASE STREQUAL "hash_audio_fm")
    # Each run is made twice and must print the same bytes; the sample total
    # is the cumulative cadence formula in integer math, and nothing clips.
    # IMAGE, when set, is a fixture that sounds.
    function(audio_run frames)
      execute_process(COMMAND ${EMULATOR} ${RUN} --frames ${frames} --hash-audio ${ARGN}
        RESULT_VARIABLE rc1 OUTPUT_VARIABLE out1 ERROR_VARIABLE err1)
      execute_process(COMMAND ${EMULATOR} ${RUN} --frames ${frames} --hash-audio ${ARGN}
        RESULT_VARIABLE rc2 OUTPUT_VARIABLE out2 ERROR_VARIABLE err2)
      if(NOT rc1 STREQUAL "0" OR NOT rc2 STREQUAL "0")
        message(FATAL_ERROR "--hash-audio ${ARGN}: exit ${rc1} and ${rc2}\n${out1}${err1}${err2}")
      endif()
      if(NOT out1 STREQUAL out2)
        message(FATAL_ERROR "--hash-audio ${ARGN}: two runs differ:\n${out1}${out2}")
      endif()
      string(REPEAT "[0-9a-f]" 64 h64)
      if(NOT out1 MATCHES "(^|\n)audio-transitions ${frames} count [0-9]+ sha256 ${h64}\n"
          OR NOT out1 MATCHES "(^|\n)audio-pcm ${frames} rate 48000 channels ([12]) samples ([0-9]+) peak ([0-9]+) clipped ([0-9]+) sha256 ${h64}\n")
        message(FATAL_ERROR "--hash-audio ${ARGN}: the two lines are missing or malformed:\n${out1}")
      endif()
      math(EXPR want "${frames} * 3504512 / 4375")
      # Group 1 is the line start.
      if(NOT CMAKE_MATCH_3 STREQUAL want OR NOT CMAKE_MATCH_5 STREQUAL "0")
        message(FATAL_ERROR "--hash-audio ${ARGN}: samples ${CMAKE_MATCH_3} (want ${want}) clipped ${CMAKE_MATCH_5}")
      endif()
      set(channels ${CMAKE_MATCH_2} PARENT_SCOPE)
      set(peak ${CMAKE_MATCH_4} PARENT_SCOPE)
      set(sound "${out1}" PARENT_SCOPE)
    endfunction()

    if(CASE STREQUAL "hash_audio_fm")
      # The FM fixture on the Japanese console, whose built-in unit sounds, and on the export
      # console, which has none: both obey the checks above, each equals its frozen lines, the
      # Japanese run sounds, and the two differ, so the unit is heard and does not leak.
      foreach(region IN ITEMS ntsc-j ntsc-u)
        audio_run(300 ${IMAGE} --machine sms --region ${region})
        if(NOT channels STREQUAL "1" OR peak GREATER 29204)
          message(FATAL_ERROR "--hash-audio fm ${region}: channels ${channels} peak ${peak}, want 1 and at most 29204")
        endif()
        if(region STREQUAL "ntsc-j" AND peak LESS 1)
          message(FATAL_ERROR "--hash-audio fm ${region}: the unit made no sound (peak ${peak})")
        endif()
        foreach(kind IN ITEMS transitions pcm)
          file(READ "${PINS}/${kind}.txt" pins)
          string(REGEX MATCH "(^|\n)audio-${kind} ([0-9]+ [^\n]*)\n" run_line "${sound}")
          set(want_line "\naudio-${kind} fm_sweep.sms sms ${region} baseline ${CMAKE_MATCH_2}\n")
          string(FIND "\n${pins}" "${want_line}" at)
          if(at EQUAL -1)
            message(FATAL_ERROR "--hash-audio fm ${region}: the ${kind} line is not in ${PINS}/${kind}.txt:\n${run_line}")
          endif()
          string(REPLACE "-" "_" id "${region}")
          set(${kind}_${id} "${run_line}")
        endforeach()
      endforeach()
      if(pcm_ntsc_j STREQUAL pcm_ntsc_u OR transitions_ntsc_j STREQUAL transitions_ntsc_u)
        message(FATAL_ERROR "--hash-audio fm: the Japanese and export consoles sound alike")
      endif()
      return()
    endif()

    # The card is silent: one channel on the Master System, two on the Game Gear, no peak.
    audio_run(60)
    if(NOT channels STREQUAL "1" OR NOT peak STREQUAL "0")
      message(FATAL_ERROR "--hash-audio card: channels ${channels} peak ${peak}")
    endif()
    audio_run(60 --machine gg)
    if(NOT channels STREQUAL "2" OR NOT peak STREQUAL "0")
      message(FATAL_ERROR "--hash-audio card on gg: channels ${channels} peak ${peak}")
    endif()
    if(IMAGE)
      audio_run(300 ${IMAGE})
      if(NOT channels STREQUAL "1" OR peak LESS 1 OR peak GREATER 29204)
        message(FATAL_ERROR "--hash-audio ${IMAGE}: channels ${channels} peak ${peak}, want 1 to 29204")
      endif()
      # The run's own lines, tagged as hashes.txt tags them, are lines of the frozen pins.
      foreach(kind IN ITEMS transitions pcm)
        file(READ "${PINS}/${kind}.txt" pins)
        string(REGEX MATCH "(^|\n)audio-${kind} ([0-9]+ [^\n]*)\n" run_line "${sound}")
        set(want_line "\naudio-${kind} psg_sweep.sms sms ntsc-u baseline ${CMAKE_MATCH_2}\n")
        string(FIND "\n${pins}" "${want_line}" at)
        if(at EQUAL -1)
          message(FATAL_ERROR "--hash-audio ${IMAGE}: the ${kind} line is not in ${PINS}/${kind}.txt:\n${run_line}")
        endif()
      endforeach()
    endif()

    # A runner built to offer the core 800 samples a frame meets a frame of 801
    # or 802: that is a failure, not a short hash.
    execute_process(COMMAND ${EMULATOR} ${SHORT} --frames 2 --hash-audio
      RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
    if(NOT rc STREQUAL "1" OR NOT err MATCHES "frame 1 holds 800 samples" OR out MATCHES "audio-")
      message(FATAL_ERROR "the 800 sample runner: expected exit 1 and no hash lines, got exit ${rc}\n${out}${err}")
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
  elseif(CASE STREQUAL "bios_grammar" OR CASE STREQUAL "bios_refusals" OR CASE STREQUAL "bios_no_path_echo")
    # Writes a file of exactly size bytes of "a". file(CONFIGURE) ends a body in one LF on every platform, so
    # the body is size-1 characters; the files of 0 and 1 byte hold no LF and are written whole. The size
    # is read back, so a file of the wrong size fails here by name and not as a refusal that was never
    # about its size.
    function(owned_file path size)
      if(size LESS 2)
        string(REPEAT "a" ${size} text)
        file(WRITE "${path}" "${text}")
      else()
        math(EXPR body "${size} - 1")
        string(REPEAT "a" ${body} text)
        write_bytes("${path}" UNIX "${text}")
      endif()
      file(SIZE "${path}" got)
      if(NOT got EQUAL size)
        message(FATAL_ERROR "owned_file ${path}: wanted ${size} bytes, got ${got}")
      endif()
    endfunction()
    # A BIOS error holding the marker in every path it was given must not print it: the decision
    # is that no text of the BIOS options echoes a path.
    function(expect_no_echo code regex)
      execute_process(COMMAND ${EMULATOR} ${RUN} ${ARGN} RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
      if(NOT rc STREQUAL code OR NOT "${out}${err}" MATCHES "${regex}")
        message(FATAL_ERROR "cogmaster-run ${ARGN}: expected exit ${code} and output matching ${regex}\ngot exit ${rc}\n${out}${err}")
      endif()
      string(FIND "${out}${err}" "NOECHO-MARKER-7f3a" at)
      if(NOT at EQUAL -1)
        message(FATAL_ERROR "cogmaster-run ${ARGN}: the output holds the path marker\n${out}${err}")
      endif()
    endfunction()
    file(MAKE_DIRECTORY "${OUT}")
    set(cart ${SEEDS}/di-halt.sms)
    owned_file("${OUT}/bios.bin" 8192)
    set(bios ${OUT}/bios.bin)
    if(CASE STREQUAL "bios_grammar")
      # Fault prevented: a BIOS option that silently changes a hash world (a path given with fast boot) or
      # runs on a misspelt word. Every misuse is exit 2 whose first line names the option, and the valid
      # spellings, in either order, run.
      set(first "^cogmaster-run: [^\n]*--(boot|bios|expect|probe)")
      expect_exit(0 "BIOS not recognised" --frames 1 --boot bios --bios ${bios} ${cart})
      expect_exit(0 "BIOS not recognised" ${cart} --bios ${bios} --frames 1 --boot bios)
      expect_exit(0 "^$" --frames 1 --boot fast ${cart})
      expect_exit(2 "${first}" --frames 1 ${cart} --boot)
      expect_exit(2 "${first}" --frames 1 ${cart} --boot slow)
      expect_exit(2 "${first}" --frames 1 ${cart} --boot "")
      expect_exit(2 "${first}" --frames 1 ${cart} --boot fast --boot fast)
      expect_exit(2 "${first}" --frames 1 ${cart} --boot bios --boot bios --bios ${bios})
      expect_exit(2 "${first}" --frames 1 ${cart} --boot bios --bios ${bios} --bios ${bios})
      expect_exit(2 "${first}" --frames 1 ${cart} --boot bios --bios "")
      expect_exit(2 "${first}" --frames 1 ${cart} --boot bios --bios)
      expect_exit(2 "${first}" --frames 1 ${cart} --bios ${bios})
      expect_exit(2 "${first}" --frames 1 ${cart} --boot fast --bios ${bios})
      expect_exit(2 "${first}" --frames 1 --boot bios --bios ${bios})
      expect_exit(2 "${first}" --probe ${cart} --boot bios --bios ${bios})
      expect_exit(2 "${first}" --probe ${cart} --boot fast)
      expect_exit(2 "${first}" --probe ${cart} --bios ${bios})
      write_bytes("${OUT}/run.expect" UNIX "ram@2:$C200=00")
      expect_exit(2 "${first}" --frames 2 ${cart} --boot bios --bios ${bios} --expect ${OUT}/run.expect)
      # Control: the same expectation with fast boot is accepted and prints the execution line.
      expect_exit(0 "execution machine=" --frames 2 ${cart} --boot fast --expect ${OUT}/run.expect)
      # Nothing is read before a usage error: a missing BIOS and a missing cartridge are still exit 2.
      expect_exit(2 "${first}" --frames 1 --boot fast --bios ${OUT}/absent.bin ${SEEDS}/missing.sms)
    elseif(CASE STREQUAL "bios_refusals")
      # Fault prevented: a missing or unsupported BIOS sent to exit 1 like a crash, or a Game Gear cartridge
      # started on a Master System BIOS. Each refusal is exit 77 with a text naming the BIOS.
      set(why "unsupported: [^\n]*BIOS")
      expect_exit(0 "BIOS not recognised \\(sha1 " --frames 1 --boot bios --bios ${bios} ${cart})
      expect_exit(77 "${why}" --frames 1 --boot bios ${cart})
      expect_exit(77 "${why}" --frames 1 --boot bios --bios ${OUT}/absent.bin ${cart})
      if(NOT WIN32)
        # A path below a regular file is not a path that exists.
        expect_exit(77 "${why}" --frames 1 --boot bios --bios ${bios}/below ${cart})
      endif()
      foreach(size 0 1 8191 8193 16384)
        owned_file("${OUT}/size${size}.bin" ${size})
        expect_exit(77 "${why} file of (over )?[0-9]+ bytes" --frames 1 --boot bios --bios ${OUT}/size${size}.bin ${cart})
      endforeach()
      expect_exit(77 "BIOS file of 8191 bytes" --frames 1 --boot bios --bios ${OUT}/size8191.bin ${cart})
      expect_exit(77 "BIOS cannot start a Game Gear" --frames 1 --boot bios --bios ${bios} --machine gg ${cart})
      expect_exit(77 "BIOS cannot start a Game Gear" --frames 1 --boot bios --bios ${bios} ${SEEDS}/di-halt.gg)
      # Control: the Game Gear image does run without a BIOS, so it is the BIOS the refusal is about.
      expect_exit(0 "^$" --frames 1 ${SEEDS}/di-halt.gg)
      # Exit 1: a BIOS path that is a directory, and a missing cartridge, as for a cartridge today.
      expect_exit(1 "cannot read the BIOS file" --frames 1 --boot bios --bios ${OUT} ${cart})
      expect_exit(1 "cannot read" --frames 1 --boot bios --bios ${bios} ${SEEDS}/missing.sms)
      expect_exit(1 "cannot read" --frames 1 --boot bios --bios ${OUT}/absent.bin ${SEEDS}/missing.sms)
    else()
      # Fault prevented: a failure text that echoes the path of a BIOS, or a byte of a dump.
      set(marker ${OUT}/NOECHO-MARKER-7f3a)
      file(MAKE_DIRECTORY "${marker}")
      owned_file("${marker}/bios.bin" 8192)
      owned_file("${marker}/short.bin" 8191)
      file(COPY ${cart} DESTINATION "${marker}")
      set(here ${marker}/di-halt.sms)
      expect_no_echo(77 "BIOS" --frames 1 --boot bios --bios ${marker}/absent.bin ${here})
      expect_no_echo(77 "BIOS" --frames 1 --boot bios --bios ${marker}/short.bin ${here})
      expect_no_echo(77 "BIOS" --frames 1 --boot bios --bios ${marker}/bios.bin --machine gg ${here})
      expect_no_echo(77 "BIOS" --frames 1 --boot bios ${here})
      expect_no_echo(1 "BIOS" --frames 1 --boot bios --bios ${marker} ${here})
      expect_no_echo(0 "BIOS not recognised" --frames 1 --boot bios --bios ${marker}/bios.bin ${here})
      # Control: a cartridge path is echoed by its own error, so the marker check can fail.
      execute_process(COMMAND ${EMULATOR} ${RUN} --frames 1 ${marker}/missing.sms RESULT_VARIABLE rc
        OUTPUT_VARIABLE out ERROR_VARIABLE err)
      string(FIND "${out}${err}" "NOECHO-MARKER-7f3a" at)
      if(NOT rc STREQUAL "1" OR at EQUAL -1)
        message(FATAL_ERROR "control: a missing cartridge under the marker should exit 1 and print its path, got exit ${rc}\n${out}${err}")
      endif()
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
# Writes build/<preset>/hashes.txt with the SMS and Game Gear hash lines and
# the fixture image digests, the file CI collects from every lane and
# compares byte for byte.
string(REPLACE ";" "$<SEMICOLON>" runner_emulator "${CMAKE_CROSSCOMPILING_EMULATOR}")
set(runner_hashes ${PROJECT_SOURCE_DIR}/tests/cmake/hashes.cmake)
# With the fixtures built, their image digests join the file.
set(runner_fixtures "")
if(COGMASTER_FIXTURES)
  set(runner_fixtures -DFIXTURES=${PROJECT_BINARY_DIR}/fixtures)
endif()
# Timeout: 30 s floor; 2 x 2.3 s, slowest of 5 local runs with the sound rows (1.2 s alone to 2.3 s
# inside the parallel ci workflow), no hosted run yet, so provisional until the be32 (x8) and wasm32 (x5) timings of the first hosted run are read.
cogmaster_add_test(NAME runner.hashes_file
  COMMAND ${CMAKE_COMMAND} -DCASE=hash_matrix -DRUN=$<TARGET_FILE:cogmaster-run>
    "-DEMULATOR=${runner_emulator}" -DOUT=${PROJECT_BINARY_DIR} ${runner_fixtures}
    -DHASHES=${runner_hashes} -DPINS=${PROJECT_SOURCE_DIR}/tests/audio
    -P ${CMAKE_CURRENT_LIST_FILE}
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

if(COGMASTER_FIXTURES)
  add_executable(cogmaster-test-save ${CMAKE_CURRENT_LIST_DIR}/test_save.c
    ${PROJECT_SOURCE_DIR}/runner/save.c ${PROJECT_SOURCE_DIR}/runner/stop.c)
  target_include_directories(cogmaster-test-save PRIVATE ${PROJECT_SOURCE_DIR}/src)
  target_link_libraries(cogmaster-test-save PRIVATE cogmaster::cogmaster)
  target_compile_definitions(cogmaster-test-save PRIVATE _POSIX_C_SOURCE=200809L COGMASTER_SAVE_TEST)
  set_target_properties(cogmaster-test-save PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
  cogmaster_warnings(cogmaster-test-save)
  foreach(case imports replacement excluded failures next_index clean_sigint clean_sigterm native_error)
    # Fault prevented: raw import, clean flush or classified refusals corrupting host data.
    cogmaster_add_test(NAME runner.save.${case}
      COMMAND ${runner_check} -DCASE=save_unit -DSAVE_CASE=${case}
        -DUNIT=$<TARGET_FILE:cogmaster-test-save>
        -DOUT=${PROJECT_BINARY_DIR}/tests/save-${case}
        -DBATTERY=${PROJECT_BINARY_DIR}/fixtures/battery.sms -P ${CMAKE_CURRENT_LIST_FILE}
      LABELS runner portable TIMEOUT 10)
  endforeach()
  # Fault prevented: a stale temporary of this save left behind forever, or another file deleted by the
  # sweep.
  cogmaster_add_test(NAME runner.save.sweep
    COMMAND ${runner_check} -DCASE=save_sweep
      -DOUT=${PROJECT_BINARY_DIR}/tests/save-sweep
      -DBATTERY=${PROJECT_BINARY_DIR}/fixtures/battery.sms -P ${CMAKE_CURRENT_LIST_FILE}
    LABELS runner portable TIMEOUT 10)
  # Fault prevented: --save-dir silently dropping a guest battery write.
  cogmaster_add_test(NAME runner.save.import
    COMMAND ${runner_check} -DCASE=save_import
      -DOUT=${PROJECT_BINARY_DIR}/tests/save-import
      -DEEPROM=${PROJECT_BINARY_DIR}/fixtures/eeprom.gg
      -DBATTERY=${PROJECT_BINARY_DIR}/fixtures/battery.sms -P ${CMAKE_CURRENT_LIST_FILE}
    LABELS runner portable TIMEOUT 10)
endif()

if(COGMASTER_FIXTURES)
  # Fault prevented: host polling credited as guest reads, or lag output omitted.
  cogmaster_add_test(NAME runner.lag
    COMMAND ${runner_check} -DCASE=lag -DOUT=${PROJECT_BINARY_DIR}/tests/lag.input
      -DREAD_ROM=${PROJECT_BINARY_DIR}/fixtures/boot_to_play.sms
      -P ${CMAKE_CURRENT_LIST_FILE}
    LABELS runner portable TIMEOUT 10)
endif()

# Fault prevented: a controller choice the profile cannot honour that runs silently as a pad, or a default that never
# reaches the host, so a Paddle title plays with the wrong device.
# Timeout: 30 s floor; 2 x 0.34 s, slowest of 1 run, run 42973268415.
foreach(device_case IN ITEMS usage names refused auto)
  cogmaster_add_test(NAME runner.device.${device_case}
    COMMAND ${runner_check} -DCASE=device_${device_case}
      -DOUT=${PROJECT_BINARY_DIR}/tests/device-${device_case}.expect -P ${CMAKE_CURRENT_LIST_FILE}
    LABELS portable runner TIMEOUT 30)
endforeach()

# Fault prevented: a gun aim typo that runs as no aim and hides a broken script, an aim sent to a pad that is
# silently dropped, or a Game Gear that runs a gun it cannot have.
# Timeout: 30 s floor; 2 x 1.16 s (args on be32), slowest of 1 run, run 43010180943.
foreach(gun_case IN ITEMS args gg_refused)
  cogmaster_add_test(NAME runner.gun.${gun_case}
    COMMAND ${runner_check} -DCASE=gun_${gun_case}
      -DOUT=${PROJECT_BINARY_DIR}/tests/gun-${gun_case}.input -P ${CMAKE_CURRENT_LIST_FILE}
    LABELS portable runner TIMEOUT 30)
endforeach()

# Fault prevented: a script aim that is not the field dot, so a gun title is aimed at the wrong place by the same
# crop the hosts map through; a gun on the port that changes a run in which it sees no light, so a title that
# never aims is not the title that ran before. The fixture's aim is judged against its row and, 16 dots left, falls
# outside it; the parked and dark-aimed runs equal the pad's.
# Timeout: 30 s floor; 2 x 1.43 s (noshot on be32), slowest of 1 run, run 43010180943.
if(COGMASTER_FIXTURES)
  set(gun_run ${runner_check} -DIMAGE=${PROJECT_BINARY_DIR}/fixtures/light_phaser.sms -DFRAMES=40
    -DPHASER=${PROJECT_BINARY_DIR}/tests/phaser.input -DLEFT=${PROJECT_BINARY_DIR}/tests/phaser-left.input
    -DPARKED=${PROJECT_BINARY_DIR}/tests/phaser-parked.input -DDARK=${PROJECT_BINARY_DIR}/tests/phaser-dark.input
    -DCSV=${PROJECT_SOURCE_DIR}/tests/diag/expected.csv)
  foreach(gun_case IN ITEMS crop noshot)
    cogmaster_add_test(NAME runner.gun.${gun_case}
      COMMAND ${gun_run} -DCASE=gun_${gun_case} -P ${CMAKE_CURRENT_LIST_FILE}
      LABELS portable runner TIMEOUT 30)
  endforeach()
endif()

# Fault prevented: a table default that never reaches a host, or a Paddle dump that loads as export and plays as a
# pad, seen through the runner on the two owned device images.
# Timeout: 30 s floor; 2 x 13.41 s (default, be32 lane), slowest of 1 run, run 42978311844.
if(COGMASTER_FIXTURES)
  foreach(device_case IN ITEMS default probe_default paddle_export_refused)
    cogmaster_add_test(NAME runner.device.${device_case}
      COMMAND ${runner_check} -DCASE=device_${device_case}
        -DSPORTS=${PROJECT_BINARY_DIR}/fixtures/sports_pad.sms -DPADDLE=${PROJECT_BINARY_DIR}/fixtures/paddle.sms
        -DOUT=${PROJECT_BINARY_DIR}/tests/device-${device_case}.expect -P ${CMAKE_CURRENT_LIST_FILE}
      LABELS portable runner TIMEOUT 30)
  endforeach()
endif()

# Fault prevented: malformed, truncated or surplus absolute input being run as released input.
cogmaster_add_test(NAME runner.input
  COMMAND ${runner_check} -DCASE=input -DOUT=${PROJECT_BINARY_DIR}/tests/input.txt
    -P ${CMAKE_CURRENT_LIST_FILE}
  LABELS runner portable TIMEOUT 30)

# Fault prevented: a delta script that is half-parsed, accepted out of range or applied to a port that cannot move, so a replay differs from the recording without an error.
# Timeout: 30 s floor; 2 x 2.39 s, slowest of 1 run, run 42973268415.
foreach(axes_case IN ITEMS axes_grammar axes_device axes_equivalent)
  cogmaster_add_test(NAME runner.input.${axes_case}
    COMMAND ${runner_check} -DCASE=${axes_case}
      -DOUT=${PROJECT_BINARY_DIR}/tests/${axes_case}.expect -P ${CMAKE_CURRENT_LIST_FILE}
    LABELS portable runner TIMEOUT 30)
endforeach()
foreach(eyes_case IN ITEMS line probe)
  cogmaster_add_test(NAME runner.eyes.${eyes_case}
    COMMAND ${runner_check} -DCASE=eyes_${eyes_case}
      -DOUT=${PROJECT_BINARY_DIR}/tests/eyes_${eyes_case}.expect -P ${CMAKE_CURRENT_LIST_FILE}
    LABELS portable runner TIMEOUT 30)
endforeach()

# Fault prevented: the reference PNG the RetroArch screenshot is compared
# with changes bytes (palette, layout or encoder) while the frame hash still
# passes, or no longer shows the card.
set(png_failure_sources ${COGMASTER_RUNNER_SOURCES})
list(REMOVE_ITEM png_failure_sources ${PROJECT_SOURCE_DIR}/runner/main.c)
add_executable(cogmaster-test-png-close ${CMAKE_CURRENT_LIST_DIR}/png_close_failure.c
  ${png_failure_sources})
target_link_libraries(cogmaster-test-png-close PRIVATE cogmaster::cogmaster cogmaster-host)
target_compile_definitions(cogmaster-test-png-close PRIVATE _POSIX_C_SOURCE=200809L)
set_target_properties(cogmaster-test-png-close PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
cogmaster_warnings(cogmaster-test-png-close)
cogmaster_add_test(NAME runner.png_file
  COMMAND ${runner_check} -DCASE=png_file -DOUT=${PROJECT_BINARY_DIR}/tests/card.png
    -DDECODE=$<TARGET_FILE:cogmaster-test-runner> -DEXPECTED=${COGMASTER_TESTCARD_PNG_SHA256_SMS}
    -DFAIL_CLOSE=$<TARGET_FILE:cogmaster-test-png-close>
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
  ${PROJECT_SOURCE_DIR}/runner/mailbox.c
  ${PROJECT_SOURCE_DIR}/runner/md5.c
  ${PROJECT_SOURCE_DIR}/runner/sha1.c
  ${PROJECT_SOURCE_DIR}/runner/stop.c)
target_link_libraries(cogmaster-test-bench PRIVATE cogmaster::cogmaster)
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

# The same runner offering the core 800 samples a frame, for runner.hash_audio.
add_executable(cogmaster-test-run-audio-short ${COGMASTER_RUNNER_SOURCES})
target_link_libraries(cogmaster-test-run-audio-short PRIVATE cogmaster::cogmaster cogmaster-host)
target_compile_definitions(cogmaster-test-run-audio-short PRIVATE _POSIX_C_SOURCE=200809L
  COGMASTER_RUN_AUDIO_CAP=800)
set_target_properties(cogmaster-test-run-audio-short PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
cogmaster_warnings(cogmaster-test-run-audio-short)

if(COGMASTER_FIXTURES)
  # Share uninstrumented helpers; only main and save carry the test-only hooks.
  set(save_support ${COGMASTER_RUNNER_SOURCES})
  list(REMOVE_ITEM save_support ${PROJECT_SOURCE_DIR}/runner/main.c ${PROJECT_SOURCE_DIR}/runner/save.c)
  add_library(cogmaster-test-save-support OBJECT ${save_support})
  target_link_libraries(cogmaster-test-save-support PRIVATE cogmaster::cogmaster cogmaster-host)
  target_compile_definitions(cogmaster-test-save-support PRIVATE _POSIX_C_SOURCE=200809L)
  cogmaster_warnings(cogmaster-test-save-support)
  foreach(case term int create write flush sync close replace interrupt dirsync int_write normal_write)
    set(run $<TARGET_FILE:cogmaster-test-run-stop>)
    set(code 0)
    set(regex "frame 3 sha256")
    if(NOT case STREQUAL "term")
      set(target cogmaster-test-save-${case})
      add_executable(${target} ${PROJECT_SOURCE_DIR}/runner/main.c ${PROJECT_SOURCE_DIR}/runner/save.c
        $<TARGET_OBJECTS:cogmaster-test-save-support>)
      target_link_libraries(${target} PRIVATE cogmaster::cogmaster cogmaster-host)
      target_compile_definitions(${target} PRIVATE _POSIX_C_SOURCE=200809L
        COGMASTER_SAVE_TEST)
      if(NOT case STREQUAL "normal_write")
        target_compile_definitions(${target} PRIVATE COGMASTER_RUN_RAISE_AT=3)
      endif()
      if(case MATCHES "^int")
        target_compile_definitions(${target} PRIVATE COGMASTER_RUN_RAISE_SIGNAL=SIGINT)
      endif()
      if(NOT case STREQUAL "int")
        string(REGEX REPLACE "^(int_|normal_)" "" fault ${case})
        string(TOUPPER ${fault} fault)
        target_compile_definitions(${target} PRIVATE COGMASTER_SAVE_FAIL=COGMASTER_SAVE_${fault})
        set(code 1)
        set(regex "save: cannot")
      endif()
      set_target_properties(${target} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
      cogmaster_warnings(${target})
      set(run $<TARGET_FILE:${target}>)
    endif()
    if(case STREQUAL "interrupt")
      set(code 93)
      set(regex ".*")
    endif()
    # RENAMED: the fault comes after the rename, so the new bytes are in place whatever the exit.
    set(renamed 0)
    if(case STREQUAL "dirsync")
      if(WIN32 OR CMAKE_SYSTEM_NAME STREQUAL "WASI")
        # No directory sync exists on this platform: the fault name is accepted, the point is never
        # reached, and the save must still complete normally.
        set(code 0)
        set(regex "frame 3 sha256")
      else()
        set(renamed 1)
      endif()
    endif()
    # Fault prevented: signal/failure paths reporting success or damaging an old save.
    cogmaster_add_test(NAME runner.save.process.${case}
      COMMAND ${runner_check} -DRUN=${run} -DCASE=save_process -DCODE=${code} "-DREGEX=${regex}"
        -DRENAMED=${renamed}
        -DOUT=${PROJECT_BINARY_DIR}/tests/save-process-${case}
        -DBATTERY=${PROJECT_BINARY_DIR}/fixtures/battery.sms -P ${CMAKE_CURRENT_LIST_FILE}
      LABELS runner portable TIMEOUT 10)
    if(case STREQUAL "term" OR case STREQUAL "int")
      # Fault prevented: signal flush writes bytes that a fresh process cannot continue.
      cogmaster_add_test(NAME runner.save.continuation.${case}
        COMMAND ${runner_check} -DRUN=${run} -DCONTINUE=$<TARGET_FILE:cogmaster-run>
          -DCASE=save_continuation
          -DOUT=${PROJECT_BINARY_DIR}/tests/save-continuation-${case}
          -DBATTERY=${PROJECT_BINARY_DIR}/fixtures/battery.sms -P ${CMAKE_CURRENT_LIST_FILE}
        LABELS runner portable TIMEOUT 10)
    endif()
  endforeach()
  # Fault prevented: a periodic save failure deferred to the end of the run, or a flush on every frame
  # whatever the interval.
  cogmaster_add_test(NAME runner.save.interval_fault
    COMMAND ${runner_check} -DCASE=save_interval_fault -DRUN=$<TARGET_FILE:cogmaster-test-save-normal_write>
      -DOUT=${PROJECT_BINARY_DIR}/tests/save-interval-fault
      -DBATTERY=${PROJECT_BINARY_DIR}/fixtures/battery.sms -P ${CMAKE_CURRENT_LIST_FILE}
    LABELS runner portable TIMEOUT 30)
  # Fault prevented: a process killed between flushes losing the last complete save, or leaving a
  # temporary the next run trips on.
  cogmaster_add_test(NAME runner.save.kill
    COMMAND ${runner_check} -DCASE=save_kill -DRUN=$<TARGET_FILE:cogmaster-test-save-interrupt>
      -DPRODUCTION=$<TARGET_FILE:cogmaster-run>
      -DOUT=${PROJECT_BINARY_DIR}/tests/save-kill
      -DBATTERY=${PROJECT_BINARY_DIR}/fixtures/battery.sms -P ${CMAKE_CURRENT_LIST_FILE}
    LABELS runner portable TIMEOUT 10)
  # Fault prevented: successful normal exit loses the numeric guest continuation.
  cogmaster_add_test(NAME runner.save.continuation.normal
    COMMAND ${runner_check} -DCONTINUE=$<TARGET_FILE:cogmaster-run>
      -DCASE=save_continuation
      -DOUT=${PROJECT_BINARY_DIR}/tests/save-continuation-normal
      -DBATTERY=${PROJECT_BINARY_DIR}/fixtures/battery.sms -P ${CMAKE_CURRENT_LIST_FILE}
    LABELS runner portable TIMEOUT 10)
  # Fault prevented: an EEPROM save that a fresh process cannot continue, or a refused write that still lands.
  cogmaster_add_test(NAME runner.save.continuation.eeprom
    COMMAND ${runner_check} -DCASE=save_eeprom
      -DOUT=${PROJECT_BINARY_DIR}/tests/save-continuation-eeprom
      -DEEPROM=${PROJECT_BINARY_DIR}/fixtures/eeprom.gg -P ${CMAKE_CURRENT_LIST_FILE}
    LABELS runner portable TIMEOUT 10)
  # Fault prevented: a periodic flush that changes save bytes or hashes, accepts a malformed interval, or
  # runs when the interval exceeds the run.
  cogmaster_add_test(NAME runner.save.interval
    COMMAND ${runner_check} -DCASE=save_interval
      -DOUT=${PROJECT_BINARY_DIR}/tests/save-interval
      -DBATTERY=${PROJECT_BINARY_DIR}/fixtures/battery.sms -P ${CMAKE_CURRENT_LIST_FILE}
    LABELS runner portable TIMEOUT 30)
endif()

set(bench_check ${runner_check} -DUNIT=$<TARGET_FILE:cogmaster-test-bench>)

# Fault prevented: a mailbox CRC that differs from the ROM's CRC-16 CCITT,
# so every fixture reports crc=bad, or a wrong one that agrees with a wrong
# ROM routine.
# Timeout: 10 s floor; 2 x 0.00 s, slowest of 2 runs, run 39376644025, provisional.
cogmaster_add_test(NAME runner.mailbox.crc
  COMMAND cogmaster-test-bench mailbox_crc
  LABELS portable runner TIMEOUT 10)

# Fault prevented: --mailbox stopping on the $00 status work RAM holds from
# power-on, or on a mailbox caught mid-initialisation, so the measurement is
# taken before the ROM has run.
cogmaster_add_test(NAME runner.mailbox.stop
  COMMAND ${bench_check} -DCASE=mailbox_stop -P ${CMAKE_CURRENT_LIST_FILE}
  LABELS portable runner TIMEOUT 30)

# Fault prevented: a run that stops early on --trace or --mailbox and exits 0
# with a requested --png silently missing, so a script reads a stale file.
cogmaster_add_test(NAME runner.png_unreached
  COMMAND ${runner_check} -DCASE=png_unreached -DOUT=${PROJECT_BINARY_DIR}/tests/unreached
    -P ${CMAKE_CURRENT_LIST_FILE}
  LABELS portable runner TIMEOUT 30)

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
    -DCHECK=exit_table -P ${PROJECT_SOURCE_DIR}/tests/cmake/repository.cmake
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

# Fault prevented: a BIOS option that silently changes a hash world, a missing-firmware request that exits like a
# crash, and a failure text that echoes the path of a dump.
# Timeout: 20 s floor; no hosted run yet, so provisional.
foreach(bios_case IN ITEMS grammar refusals no_path_echo)
  cogmaster_add_test(NAME runner.bios.${bios_case}
    COMMAND ${runner_check} -DCASE=bios_${bios_case}
      -DOUT=${PROJECT_BINARY_DIR}/tests/bios-${bios_case} -P ${CMAKE_CURRENT_LIST_FILE}
    LABELS portable runner TIMEOUT 20)
endforeach()

# Fault prevented: a malformed or unusable --save-interval that is accepted instead of refused as a
# usage error.
cogmaster_add_test(NAME runner.usage.save_interval
  COMMAND ${runner_check} -DCASE=save_interval_usage -DOUT=${PROJECT_BINARY_DIR}/tests/save-interval-usage
    -P ${CMAKE_CURRENT_LIST_FILE}
  LABELS portable runner TIMEOUT 30)

# Fault prevented: a hash that depends on the frame count for a card that
# does not move, or a hash line that names the wrong frame.
cogmaster_add_test(NAME runner.hash.repeat
  COMMAND ${runner_check} -DCASE=hash_repeat -P ${CMAKE_CURRENT_LIST_FILE}
  LABELS portable runner TIMEOUT 30)

# Fault prevented: sound hash lines that differ between two runs of the same
# input, a sample total off the cadence formula, a sample that clips, a frame
# of the wrong length passed as a hash, or a trace log that overflows silently.
# Timeout: 30 s; the card and the fixture runs take well under a second each.
set(hash_audio_image "")
if(COGMASTER_FIXTURES)
  set(hash_audio_image -DIMAGE=${PROJECT_BINARY_DIR}/fixtures/psg_sweep.sms)
endif()
cogmaster_add_test(NAME runner.hash_audio
  COMMAND ${runner_check} -DCASE=hash_audio -DSHORT=$<TARGET_FILE:cogmaster-test-run-audio-short>
    ${hash_audio_image} -DPINS=${PROJECT_SOURCE_DIR}/tests/audio -P ${CMAKE_CURRENT_LIST_FILE}
  LABELS portable psg runner TIMEOUT 30)

# Fault prevented: an FM unit whose sound differs between two runs, between lanes or from the
# frozen lines, that stays silent on the Japanese console, or that sounds on the export console.
# Timeout: 30 s floor; four 300 frame runs of 0.1 s each, no hosted run yet, so provisional.
if(COGMASTER_FIXTURES)
  cogmaster_add_test(NAME runner.hash_audio.fm
    COMMAND ${runner_check} -DCASE=hash_audio_fm
      -DIMAGE=${PROJECT_BINARY_DIR}/fixtures/fm_sweep.sms -DPINS=${PROJECT_SOURCE_DIR}/tests/audio
      -P ${CMAKE_CURRENT_LIST_FILE}
    LABELS fm portable runner TIMEOUT 30)
endif()

# Fault prevented: --version printing something a script cannot parse.
cogmaster_add_test(NAME runner.version
  COMMAND ${runner_check} -DCASE=version -P ${CMAKE_CURRENT_LIST_FILE}
  LABELS portable runner TIMEOUT 30)

# Fault prevented: a --probe report that drops, reorders or reformats a key,
# misreads the header, or guesses the wrong board, so a script parsing it
# reads the wrong field.
# Timeout: 10 s floor; 2 x 0.02 s, slowest of 2 runs, run 39376644025, provisional.
cogmaster_add_test(NAME runner.probe.keys
  COMMAND ${runner_check} -DCASE=probe_keys -P ${CMAKE_CURRENT_LIST_FILE}
  LABELS portable runner TIMEOUT 10)

# Fault prevented: a SHA-1 or MD5 that is wrong for some lengths, so the
# printed md5 is not the RetroAchievements hash and the sha1 matches no
# database entry.
# Timeout: 10 s floor; 2 x 0.03 s, slowest of 2 runs, run 39376644025, provisional.
cogmaster_add_test(NAME runner.probe.digests
  COMMAND ${bench_check} -DCASE=probe_digests -P ${CMAKE_CURRENT_LIST_FILE}
  LABELS portable runner TIMEOUT 10)

# Fault prevented: *_headerless digests taken over the whole file, or
# printed for a file with no copier header.
# Timeout: 10 s floor; 2 x 0.01 s, slowest of 2 runs, run 39376644025, provisional.
cogmaster_add_test(NAME runner.probe.headerless
  COMMAND ${runner_check} -DCASE=probe_headerless -P ${CMAKE_CURRENT_LIST_FILE}
  LABELS portable runner TIMEOUT 10)

# Fault prevented: --probe hiding the report of an image it calls
# unsupported, or calling it supported; a refusal that does not say which board
# or why, or a probe that hides where its board came from.
# Timeout: 10 s floor; 2 x 0.01 s, slowest of 2 runs, run 39376644025, provisional.
cogmaster_add_test(NAME runner.probe.unsupported
  COMMAND ${runner_check} -DCASE=probe_unsupported -P ${CMAKE_CURRENT_LIST_FILE}
  LABELS portable runner TIMEOUT 10)

# Fault prevented: an image outside the accepted sizes run or reported as a
# failure, or a huge file read into memory before it is refused.
# Timeout: 10 s floor; 2 x 0.03 s, slowest of 2 runs, run 39376644025, provisional.
cogmaster_add_test(NAME runner.refuse.size
  COMMAND ${runner_check} -DCASE=refuse_size -DOVERSIZE=${COGMASTER_OVERSIZE_FILE}
    -P ${CMAKE_CURRENT_LIST_FILE}
  LABELS portable runner TIMEOUT 10)

# Fault prevented: an SG-1000 image run as a Master System cartridge.
# Timeout: 10 s floor; 2 x 0.01 s, slowest of 2 runs, run 39376644025, provisional.
cogmaster_add_test(NAME runner.refuse.sg1000
  COMMAND ${runner_check} -DCASE=refuse_sg1000 -DOUT=${PROJECT_BINARY_DIR}/tests
    -P ${CMAKE_CURRENT_LIST_FILE}
  LABELS portable runner TIMEOUT 10)

# Fault prevented: a machine and region no console had, or a board the core
# cannot run, reported as a failure instead of a refusal.
# Timeout: 10 s floor; 2 x 0.01 s, slowest of 2 runs, run 39376644025, provisional.
cogmaster_add_test(NAME runner.refuse.profile
  COMMAND ${runner_check} -DCASE=refuse_profile -P ${CMAKE_CURRENT_LIST_FILE}
  LABELS portable runner TIMEOUT 10)

# Fault prevented: --machine auto running a Game Gear cartridge as a Master
# System one, or the reverse.
# Timeout: 10 s floor; 2 x 0.03 s, slowest of 2 runs, run 39376644025, provisional.
cogmaster_add_test(NAME runner.load.machine_auto
  COMMAND ${runner_check} -DCASE=machine_auto -P ${CMAKE_CURRENT_LIST_FILE}
  LABELS portable runner TIMEOUT 10)

# Fault prevented: a --region name that is refused, ignored or accepts a
# misspelling.
# Timeout: 10 s floor; 2 x 0.02 s, slowest of 2 runs, run 39376644025, provisional.
cogmaster_add_test(NAME runner.load.region
  COMMAND ${runner_check} -DCASE=region -P ${CMAKE_CURRENT_LIST_FILE}
  LABELS portable runner TIMEOUT 10)
cogmaster_add_test(NAME runner.load.video_override
  COMMAND ${runner_check} -DCASE=video_override -P ${CMAKE_CURRENT_LIST_FILE}
  LABELS portable runner TIMEOUT 10)

# Fault prevented: --trace first-fault missing a fault, printing a later
# one, or printing it in a form a script cannot parse.
# Timeout: 10 s floor; 2 x 0.01 s, slowest of 2 runs, run 39376644025, provisional.
cogmaster_add_test(NAME runner.trace.fault
  COMMAND ${runner_check} -DCASE=trace_fault -P ${CMAKE_CURRENT_LIST_FILE}
  LABELS portable runner TIMEOUT 10)

# Fault prevented: --trace first-fault reporting a fault for a program that
# makes none, or printing nothing when there is none.
# Timeout: 10 s floor; 2 x 0.01 s, slowest of 2 runs, run 39376644025, provisional.
cogmaster_add_test(NAME runner.trace.none
  COMMAND ${runner_check} -DCASE=trace_none -P ${CMAKE_CURRENT_LIST_FILE}
  LABELS portable runner TIMEOUT 10)

# Fault prevented: README play steps that no longer run, print another line, promise a ROM extension the
# core does not load, or lose the legality sentence. The command runs against the built playable fixture,
# so it has no ROM bytes and runs on every lane that builds fixtures, emulated ones included. The control
# is a README copy that names .sg where it names .gg, which must be refused by name.
# Timeout: 10 s floor; one 600 frame run of the fixture, no hosted run yet, so provisional.
if(COGMASTER_FIXTURES)
  file(READ ${PROJECT_SOURCE_DIR}/README.md readme_text)
  string(REPLACE "`.gg`" "`.sg`" readme_sg "${readme_text}")
  if(readme_sg STREQUAL readme_text)
    message(FATAL_ERROR "docs.readme_play.self_test: README.md no longer names `.gg`")
  endif()
  file(WRITE ${PROJECT_BINARY_DIR}/tests/readme_sg/README.md "${readme_sg}")
  set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${PROJECT_SOURCE_DIR}/README.md)
  set(readme_args -DINFO=${PROJECT_SOURCE_DIR}/libretro/cogmaster_libretro.info.in
    -DRUN=$<TARGET_FILE:cogmaster-run> -DIMAGE=${PROJECT_BINARY_DIR}/fixtures/boot_to_play.sms)
  set(readme_script ${PROJECT_SOURCE_DIR}/tests/cmake/readme.cmake)
  cogmaster_add_test(NAME docs.readme_play
    COMMAND ${CMAKE_COMMAND} -DREADME=${PROJECT_SOURCE_DIR}/README.md ${readme_args}
      "-DEMULATOR=${runner_emulator}" -P ${readme_script}
    LABELS runner portable TIMEOUT 10)
  string(REPLACE ";" "\;" readme_inner "${CMAKE_CROSSCOMPILING_EMULATOR}")
  set(cmd ${CMAKE_COMMAND} -DREADME=${PROJECT_BINARY_DIR}/tests/readme_sg/README.md ${readme_args}
    "-DEMULATOR=${readme_inner}" -P ${readme_script})
  string(REPLACE ";" "$<SEMICOLON>" cmd "${cmd}")
  cogmaster_add_test(NAME docs.readme_play.self_test
    COMMAND ${CMAKE_COMMAND} "-DCMD=${cmd}" -DCODE=1 "-DREGEX=readme: [.]sg is not in supported_extensions"
      -P ${PROJECT_SOURCE_DIR}/tests/cmake/expect_fail.cmake
    LABELS runner portable TIMEOUT 10)
endif()
