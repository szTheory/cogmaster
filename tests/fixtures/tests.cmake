# The assembled fixtures: tests/roms/src/<name>.asm, linked by <name>.link,
# built with the pinned WLA-DX into build/<preset>/fixtures/<name>.sms. The
# images live only in the build tree: they are never committed and never
# packed into a release archive.
#
# Run as a script (cmake -DCASE=<case> ... -P tests.cmake), this file is one
# fixture check; include()d by tests/tests.cmake, it builds and registers them.
if(CMAKE_SCRIPT_MODE_FILE AND CASE STREQUAL "vtable_closed_form")
  # The V counter values one per line over a frame, from the jump intervals of the V counter table
  # (decimal forms of the hex intervals): PAL 192 $00-$F2 and $BA-$FF; PAL 224 $00-$FF, $00-$02 and
  # $CA-$FF; PAL 240 $00-$FF, $00-$0A and $D2-$FF; NTSC 192 $00-$DA and $D5-$FF; NTSC 224 $00-$EA
  # and $E5-$FF. Every constant is recomputed here and compared with the csv row.
  set(vt_192_pal "0-242;186-255")
  set(vt_224_pal "0-255;0-2;202-255")
  set(vt_240_pal "0-255;0-10;210-255")
  set(vt_192_ntsc "0-218;213-255")
  set(vt_224_ntsc "0-234;229-255")
  file(STRINGS "${CSV}" vt_rows)
  set(vt_checked 0)
  foreach(spec IN ITEMS 192:pal 224:pal 240:pal 192:ntsc 224:ntsc)
    string(REPLACE ":" ";" spec "${spec}")
    list(GET spec 0 height)
    list(GET spec 1 tv)
    set(samples "")
    foreach(interval IN LISTS vt_${height}_${tv})
      string(REPLACE "-" ";" ends "${interval}")
      list(GET ends 0 low)
      list(GET ends 1 high)
      foreach(value RANGE ${low} ${high})
        list(APPEND samples ${value})
      endforeach()
    endforeach()
    list(LENGTH samples count)
    set(sum 0)
    set(squares 0)
    set(descents 0)
    set(max 0)
    set(min 255)
    math(EXPR last "${count} - 1")
    foreach(index RANGE 0 ${last})
      math(EXPR next "(${index} + 1) % ${count}")
      list(GET samples ${index} value)
      list(GET samples ${next} after)
      math(EXPR sum "(${sum} + ${value}) % 65536")
      math(EXPR squares "(${squares} + ${value} * ${value}) % 65536")
      if(value GREATER after)
        math(EXPR descents "${descents} + 1")
      endif()
      if(value GREATER max)
        set(max ${value})
      endif()
      if(value LESS min)
        set(min ${value})
      endif()
    endforeach()
    set(want_01 ${count})
    set(want_02 ${sum})
    set(want_03 ${squares})
    set(want_04 ${descents})
    set(want_05 ${max})
    set(want_06 ${min})
    set(want_07 1)
    # The first model has no 224 or 240 height and runs 192 lines, so its rows hold the 192 values.
    set(pairs vdp_vtable_${height}:${tv})
    if(height STREQUAL "192")
      list(APPEND pairs vdp_vtable_224:${tv}-sms1)
    endif()
    foreach(pair IN LISTS pairs)
      string(REPLACE ":" ";" pair "${pair}")
      list(GET pair 0 rom)
      list(GET pair 1 profile)
      foreach(id IN ITEMS 01 02 03 04 05 06 07)
        set(found "")
        foreach(row IN LISTS vt_rows)
          string(REPLACE "," ";" f "${row}")
          list(GET f 0 row_rom)
          list(GET f 1 row_test)
          list(GET f 2 row_profile)
          list(GET f 3 row_hex)
          if(row_rom STREQUAL rom AND row_test MATCHES "^${id}:" AND
              (row_profile STREQUAL profile OR row_profile STREQUAL "*"))
            set(found ${row_hex})
            break()
          endif()
        endforeach()
        if(found STREQUAL "")
          message(FATAL_ERROR "vtable closed form: ${rom} ${profile} ${id} has no csv row")
        endif()
        math(EXPR csv_value "0x${found}")
        if(NOT csv_value EQUAL want_${id})
          math(EXPR computed "${want_${id}}" OUTPUT_FORMAT HEXADECIMAL)
          message(FATAL_ERROR "vtable closed form: ${rom} ${profile} ${id} csv 0x${found} computed ${computed}")
        endif()
        math(EXPR vt_checked "${vt_checked} + 1")
      endforeach()
    endforeach()
  endforeach()
  message(STATUS "vtable closed form: ${vt_checked} csv rows equal the recomputed values")
  return()
endif()
if(CMAKE_SCRIPT_MODE_FILE AND CASE STREQUAL "pause_closed_form")
  # The Pause NMI follows the frame interrupt by (lines - 2 - H) x 684 + 4 master ticks, H the screen
  # height (the frame interrupt is tick 607 of line H, the NMI tick 611 of line lines - 2). The
  # constants below are the cycle counts summed in the header of tests/roms/src/pause_nmi.asm; they
  # are read from that listing and never from a run. The count n lies in
  #   [floor((C - S_LO) / B), floor((C - S_HI) / B)], C = floor(ticks / 3).
  set(pz_body 78)         # the main loop body, in cycles
  set(pz_handler 76)      # the frame handler, from the end of the accept to its return
  set(pz_accept 13)       # the IM 1 accept
  set(pz_wait 59)         # the longest wait before the frame interrupt is taken (at most the body)
  set(pz_first_min 42)    # the first increment ends this long after the resume at the body's start
  set(pz_first_max 61)    # and this long after the resume after LD A,(nn)
  set(pz_longest 16)      # the longest instruction an NMI can wait for
  set(pz_round 1)         # one cycle of tick rounding
  if(pz_wait GREATER pz_body)
    message(FATAL_ERROR "pause closed form: the longest wait ${pz_wait} exceeds the body ${pz_body}")
  endif()
  math(EXPR pz_s_lo "(${pz_wait} + ${pz_round}) + ${pz_accept} + ${pz_handler} + ${pz_first_max} - ${pz_body}")
  math(EXPR pz_s_hi "${pz_accept} + ${pz_handler} + ${pz_first_min} - (${pz_longest} + ${pz_round}) - ${pz_body}")
  file(STRINGS "${CSV}" pz_rows)
  set(pz_checked 0)
  foreach(spec IN ITEMS ntsc:192:46516 ntsc:224:24628 pal:192:81400 pal:224:59512 pal:240:48568)
    string(REPLACE ":" ";" spec "${spec}")
    list(GET spec 0 tv)
    list(GET spec 1 height)
    list(GET spec 2 literal)
    set(lines 262)
    if(tv STREQUAL "pal")
      set(lines 313)
    endif()
    math(EXPR ticks "(${lines} - 2 - ${height}) * 684 + 4")
    if(NOT ticks EQUAL literal)
      message(FATAL_ERROR "pause closed form: ${tv} ${height} ticks ${ticks}, expected ${literal}")
    endif()
    math(EXPR cycles "${ticks} / 3")
    math(EXPR lo "(${cycles} - ${pz_s_lo}) / ${pz_body}")
    math(EXPR hi "(${cycles} - ${pz_s_hi}) / ${pz_body}")
    set(found "")
    foreach(row IN LISTS pz_rows)
      string(REPLACE "," ";" f "${row}")
      list(GET f 0 row_rom)
      list(GET f 1 row_test)
      list(GET f 2 row_profile)
      list(GET f 3 row_hex)
      if(row_rom STREQUAL "pause_nmi_${height}" AND row_test MATCHES "^01:" AND row_profile STREQUAL tv)
        set(found ${row_hex})
      endif()
    endforeach()
    math(EXPR lo_hex "${lo}" OUTPUT_FORMAT HEXADECIMAL)
    math(EXPR hi_hex "${hi}" OUTPUT_FORMAT HEXADECIMAL)
    message(STATUS "pause closed form: ${tv} ${height}: ${ticks} ticks, ${cycles} cycles, n in ${lo}..${hi} (${lo_hex}..${hi_hex})")
    if(NOT found MATCHES "^([0-9A-F]+)-([0-9A-F]+)$")
      message(FATAL_ERROR "pause closed form: pause_nmi_${height} ${tv} 01 has no lo-hi csv row")
    endif()
    math(EXPR csv_lo "0x${CMAKE_MATCH_1}")
    math(EXPR csv_hi "0x${CMAKE_MATCH_2}")
    if(NOT csv_lo EQUAL lo OR NOT csv_hi EQUAL hi)
      message(FATAL_ERROR "pause closed form: pause_nmi_${height} ${tv} 01 csv ${csv_lo}-${csv_hi} computed ${lo}-${hi}")
    endif()
    math(EXPR pz_checked "${pz_checked} + 1")
  endforeach()
  message(STATUS "pause closed form: ${pz_checked} csv rows equal the recomputed bands")
  return()
endif()
if(CMAKE_SCRIPT_MODE_FILE)
  # The image's SHA-256 equals its line in tests/built/expected.txt.
  file(STRINGS "${EXPECTED}" want REGEX "^${NAME} ")
  if(NOT want MATCHES "^${NAME} ([0-9a-f]+)$")
    message(FATAL_ERROR "${EXPECTED}: no line '${NAME} <sha256>'")
  endif()
  set(want ${CMAKE_MATCH_1})
  file(SHA256 "${IMAGE}" got)
  if(NOT got STREQUAL want)
    message(FATAL_ERROR "${IMAGE}: SHA-256 ${got}, expected ${want}")
  endif()
  if(CASE STREQUAL "play")
    set(script "")
    set(oracle_arg "")
    if(INPUT)
      set(script --input ${INPUT})
      set(oracle_arg scripted)
      if(PAD2)
        set(oracle_arg pad2)
      endif()
      if(BOTH)
        set(oracle_arg both)
      endif()
    endif()
    execute_process(COMMAND ${EMULATOR} ${ORACLE} ${IMAGE} ${oracle_arg} RESULT_VARIABLE rc
      OUTPUT_VARIABLE want ERROR_VARIABLE err)
    if(NOT rc STREQUAL "0")
      message(FATAL_ERROR "pixel oracle exit ${rc}: ${err}")
    endif()
    string(STRIP "${want}" want)
    if(WRONG)
      set(want "deliberately-wrong-pixel-digest")
    endif()
    execute_process(COMMAND ${EMULATOR} ${RUN} ${IMAGE} --frames 14 --hash ${script}
      RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
    if(NOT rc STREQUAL "0" OR NOT out MATCHES "${want}")
      message(FATAL_ERROR "native frame mismatch: exit ${rc}, ${out} ${err}")
    endif()
    if(HOST)
      execute_process(COMMAND ${EMULATOR} ${HOST} ${MODULE} content ${IMAGE} ${want}
        RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
      if(NOT rc STREQUAL "0")
        message(FATAL_ERROR "content host exit ${rc}: ${out} ${err}")
      endif()
    endif()
  elseif(CASE STREQUAL "legacy")
    # A Graphics I or II image against the closed-form oracle (legacy.c), for VARIANT on PROFILE
    # (sms1, sms2 or gg_sms, the Game Gear in Master System video mode) after FRAMES frames. The oracle
    # compares every dot through the public API; with RUNNER the runner's --hash must also contain the
    # oracle's digest, so a renderer and the digest it prints are judged by one independent answer.
    # WRONG swaps in a digest that cannot match, so the comparison must refuse it. For the sprite image
    # COMPARE is all (every scenario, on both frames it is on screen, in one run) and SCENARIO names the
    # scenario whose picture the runner hash at FRAMES must equal; the other images leave both unset.
    if(NOT COMPARE)
      set(COMPARE ${FRAMES})
    endif()
    if(PROFILE STREQUAL "sms1")
      set(oracle_profile sms1)
      set(machine sms1)
    elseif(PROFILE STREQUAL "sms2")
      set(oracle_profile sms2)
      set(machine sms)
    elseif(PROFILE STREQUAL "gg_sms")
      set(oracle_profile gg)
      set(machine gg)
    else()
      message(FATAL_ERROR "legacy: unknown PROFILE '${PROFILE}'")
    endif()
    execute_process(COMMAND ${EMULATOR} ${ORACLE} ${IMAGE} ${VARIANT} ${oracle_profile} compare ${COMPARE}
      RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
    if(NOT rc STREQUAL "0")
      message(FATAL_ERROR "legacy image mismatch: ${VARIANT} on ${PROFILE}, compare ${COMPARE}, exit ${rc}: ${out}${err}")
    endif()
    if(RUNNER)
      execute_process(COMMAND ${EMULATOR} ${ORACLE} ${IMAGE} ${VARIANT} ${oracle_profile} digest ${SCENARIO}
        RESULT_VARIABLE rc OUTPUT_VARIABLE want ERROR_VARIABLE err)
      string(STRIP "${want}" want)
      if(NOT rc STREQUAL "0" OR NOT want MATCHES "^[0-9a-f]+$")
        message(FATAL_ERROR "legacy oracle digest: exit ${rc}: ${want}${err}")
      endif()
      if(WRONG)
        set(want "deliberately-wrong-pixel-digest")
      endif()
      execute_process(COMMAND ${EMULATOR} ${RUN} ${IMAGE} --machine ${machine} --video-mode sms
          --region ntsc-u --frames ${FRAMES} --hash
        RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
      if(NOT rc STREQUAL "0" OR NOT out MATCHES "${want}")
        message(FATAL_ERROR "legacy runner hash mismatch: ${VARIANT} on ${PROFILE}, exit ${rc}, ${out} ${err}")
      endif()
    endif()
  elseif(CASE STREQUAL "audio")
    # The runner hashes FRAMES frames of the image's sound for MACHINE (sms or gg); the in-process
    # host must deliver the same PCM through each way a frontend registers its audio callbacks.
    execute_process(COMMAND ${EMULATOR} ${RUN} ${IMAGE} --machine ${MACHINE} --video-mode ${MACHINE}
        --region ntsc-u --frames ${FRAMES} --hash-audio
      RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
    if(NOT rc STREQUAL "0"
        OR NOT out MATCHES "(^|\n)audio-pcm ${FRAMES} rate 48000 channels [12] samples [0-9]+ peak [0-9]+ clipped [0-9]+ sha256 ([0-9a-f]+)\n")
      message(FATAL_ERROR "--hash-audio ${IMAGE}: exit ${rc}, no audio-pcm line:\n${out}${err}")
    endif()
    execute_process(COMMAND ${EMULATOR} ${HOST} ${MODULE} audio ${IMAGE} ${FRAMES} ${MACHINE}
        ${CMAKE_MATCH_2}
      RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
    if(NOT rc STREQUAL "0")
      message(FATAL_ERROR "audio host exit ${rc}: ${out} ${err}")
    endif()
  elseif(CASE STREQUAL "fm_option")
    # The module's FM option against the runner. The runner's lines are the expectation: the
    # automatic setting on the Japanese console equals the unit attached, off and on override
    # the region, and on with Game Gear content is mapped to automatic so the content loads.
    function(runner_pcm out image machine region)
      execute_process(COMMAND ${EMULATOR} ${RUN} ${image} --machine ${machine} --video-mode ${machine}
          --region ${region} --frames 300 --hash-audio
        RESULT_VARIABLE rc OUTPUT_VARIABLE text ERROR_VARIABLE err)
      if(NOT rc STREQUAL "0" OR NOT text MATCHES "(^|\n)audio-pcm 300 [^\n]* sha256 ([0-9a-f]+)\n")
        message(FATAL_ERROR "--hash-audio ${image}: exit ${rc}, no audio-pcm line:\n${text}${err}")
      endif()
      set(${out} ${CMAKE_MATCH_2} PARENT_SCOPE)
    endfunction()
    function(host_pcm out image machine region fm)
      execute_process(COMMAND ${EMULATOR} ${HOST} ${MODULE} fm_option ${image} 300 ${machine} ${region} ${fm}
        RESULT_VARIABLE rc OUTPUT_VARIABLE text ERROR_VARIABLE err)
      if(NOT rc STREQUAL "0")
        message(FATAL_ERROR "host ${image} ${machine} ${region} fm ${fm}: exit ${rc}: ${text} ${err}")
      endif()
      string(STRIP "${text}" text)
      set(${out} ${text} PARENT_SCOPE)
    endfunction()
    runner_pcm(japan ${IMAGE} sms ntsc-j)
    runner_pcm(export ${IMAGE} sms ntsc-u)
    runner_pcm(gg ${GG_IMAGE} gg ntsc-u)
    if(japan STREQUAL export)
      message(FATAL_ERROR "the runner's Japanese and export fm_sweep hashes are equal")
    endif()
    host_pcm(got ${IMAGE} sms ntsc-j auto)
    if(NOT got STREQUAL japan)
      message(FATAL_ERROR "fm auto on ntsc-j: ${got}, the runner's Japanese run gives ${japan}")
    endif()
    host_pcm(got ${IMAGE} sms ntsc-j off)
    if(got STREQUAL japan)
      message(FATAL_ERROR "fm off on ntsc-j still gives the Japanese run's hash ${japan}")
    endif()
    host_pcm(got ${IMAGE} sms ntsc-u auto)
    if(NOT got STREQUAL export)
      message(FATAL_ERROR "fm auto on ntsc-u: ${got}, the runner's export run gives ${export}")
    endif()
    host_pcm(got ${IMAGE} sms ntsc-u on)
    if(got STREQUAL export)
      message(FATAL_ERROR "fm on with ntsc-u still gives the export run's hash ${export}")
    endif()
    host_pcm(got ${GG_IMAGE} gg auto on)
    if(NOT got STREQUAL gg)
      message(FATAL_ERROR "fm on with Game Gear content: ${got}, the runner's run gives ${gg}")
    endif()
  elseif(CASE STREQUAL "replay")
    # The library records a run of IMAGE with DEVICE on port 1, once for each region in REGIONS (comma
    # separated), and writes the script it sent to SCRIPT; the runner replays the script and must print the
    # recorder's last-frame hash line. EDIT changes one count of the last record before the replay, for the
    # negative control. The recorder also finishes the run again from a state saved half way and must agree.
    string(REPLACE "," ";" regions "${REGIONS}")
    foreach(region IN LISTS regions)
      execute_process(COMMAND ${EMULATOR} ${RECORDER} record ${IMAGE} ${DEVICE} ${region} ${SCRIPT} ${FRAMES}
        RESULT_VARIABLE rc OUTPUT_VARIABLE recorded ERROR_VARIABLE err)
      if(NOT rc STREQUAL "0" OR NOT recorded MATCHES "^frame ${FRAMES} sha256 [0-9a-f]+\n$")
        message(FATAL_ERROR "replay: recording ${IMAGE} on ${region}: exit ${rc}\n${recorded}${err}")
      endif()
      if(EDIT)
        # The units digit moves by one count (a 9 becomes an 8), so no digit carries.
        file(STRINGS ${SCRIPT} lines)
        list(POP_BACK lines last)
        string(REGEX MATCH "^(.* x1=[+-][0-9][0-9][0-9][0-9])([0-9])(.*)$" unused "${last}")
        set(digit 8)
        if(CMAKE_MATCH_2 LESS 9)
          math(EXPR digit "${CMAKE_MATCH_2} + 1")
        endif()
        list(APPEND lines "${CMAKE_MATCH_1}${digit}${CMAKE_MATCH_3}")
        list(JOIN lines "\n" body)
        file(WRITE ${SCRIPT} "${body}\n")
      endif()
      execute_process(COMMAND ${EMULATOR} ${RUN} ${IMAGE} --port1 ${DEVICE} --region ${region}
          --frames ${FRAMES} --input ${SCRIPT} --hash
        RESULT_VARIABLE rc OUTPUT_VARIABLE replayed ERROR_VARIABLE err)
      if(NOT rc STREQUAL "0" OR NOT replayed MATCHES "(^|\n)(frame ${FRAMES} sha256 [0-9a-f]+)\n")
        message(FATAL_ERROR "replay: the runner on ${region}: exit ${rc}\n${replayed}${err}")
      endif()
      set(line "${CMAKE_MATCH_2}")
      string(STRIP "${recorded}" recorded)
      if(NOT line STREQUAL recorded)
        message(FATAL_ERROR "replay: on ${region} the replay's '${line}' differs from the recording '${recorded}'")
      endif()
    endforeach()
  elseif(CASE STREQUAL "image")
    # The runner's probe finds the header checksum good.
    execute_process(COMMAND ${EMULATOR} ${RUN} --probe ${IMAGE} RESULT_VARIABLE rc
      OUTPUT_VARIABLE out ERROR_VARIABLE out)
    if(NOT rc STREQUAL "0" OR NOT out MATCHES "(^|\n)checksum_ok=1\n")
      message(FATAL_ERROR "--probe ${IMAGE}: expected exit 0 and checksum_ok=1, got exit ${rc}\n${out}")
    endif()
  elseif(CASE STREQUAL "frame")
    # The core, with no option set, shows the frame the runner shows for IMAGE after 120 runs. WRONG
    # replaces the runner's digest with zeros of its length, for the negative control.
    execute_process(COMMAND ${EMULATOR} ${RUN} ${IMAGE} --frames 120 --hash
      RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
    if(NOT rc STREQUAL "0" OR NOT out MATCHES "sha256 ([0-9a-f]+)")
      message(FATAL_ERROR "frame: the runner's hash of ${IMAGE}: exit ${rc}\n${out}${err}")
    endif()
    set(digest ${CMAKE_MATCH_1})
    if(WRONG)
      string(LENGTH "${digest}" digits)
      string(REPEAT "0" ${digits} digest)
    endif()
    execute_process(COMMAND ${EMULATOR} ${HOST} ${MODULE} frame ${IMAGE} 120 ${digest}
      RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
    if(NOT rc STREQUAL "0")
      message(FATAL_ERROR "frame digest: the core's frame of ${IMAGE} is not the runner's: exit ${rc}\n${out}${err}")
    endif()
  elseif(CASE STREQUAL "probe")
    # The runner's probe finds each board in BOARDS (comma separated) by the scan: board_source is
    # SOURCE (default heuristic), the board is named for the image, and no ROM header was read.
    if(NOT SOURCE)
      set(SOURCE heuristic)
    endif()
    string(REPLACE "," ";" boards "${BOARDS}")
    foreach(board IN LISTS boards)
      execute_process(COMMAND ${EMULATOR} ${RUN} --probe ${DIR}/mapper_${board}.sms
        RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
      if(NOT rc STREQUAL "0")
        message(FATAL_ERROR "korean_probe: --probe ${board}: exit ${rc}\n${err}")
      endif()
      if(NOT out MATCHES "(^|\n)board_source=${SOURCE}\n")
        message(FATAL_ERROR "korean_probe: board_source of ${board} is not ${SOURCE}\n${out}")
      endif()
      if(NOT out MATCHES "(^|\n)mapper_guess=${board}\n" OR NOT out MATCHES "(^|\n)header_magic=0\n")
        message(FATAL_ERROR "korean_probe: ${board} is not found by the scan with no header\n${out}")
      endif()
    endforeach()
  elseif(CASE STREQUAL "mailbox")
    # The runner prints the image's mailbox, at most FRAMES frames in, into
    # OUT; mailbox.cmake (JUDGE) then judges it against CSV for PROFILE.
    # MACHINE and REGION, when set, are passed as --machine and --region: an
    # image with no header, or a profile other than the default.
    set(machine "")
    if(MACHINE)
      set(machine --machine ${MACHINE})
    endif()
    if(REGION)
      list(APPEND machine --region ${REGION})
    endif()
    if(VIDEO)
      list(APPEND machine --video-mode ${VIDEO})
    endif()
    if(INPUT)
      list(APPEND machine --input ${INPUT})
    endif()
    if(PORT1)
      list(APPEND machine --port1 ${PORT1})
    endif()
    if(PORT2)
      list(APPEND machine --port2 ${PORT2})
    endif()
    execute_process(COMMAND ${EMULATOR} ${RUN} ${machine} ${IMAGE} --frames ${FRAMES} --mailbox
      RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
    if(NOT rc STREQUAL "0")
      message(FATAL_ERROR "--mailbox ${IMAGE}: exit ${rc}\n${err}")
    endif()
    file(WRITE "${OUT}" "${out}")
    # ROM, when set, names the CSV rows of an image built from another
    # fixture's source.
    if(NOT ROM)
      set(ROM ${NAME})
    endif()
    include(${JUDGE})
  elseif(CASE STREQUAL "eyes")
    # The runner counts the eye the core reports for each of FRAMES frames of IMAGE and prints one line;
    # its counts must equal the WANT_ values (the table's counts) and the image's mailbox, read from a
    # second run, must sit inside its rows. MACHINE and REGION, when set, choose the profile.
    set(machine "")
    if(MACHINE)
      set(machine --machine ${MACHINE})
    endif()
    if(REGION)
      list(APPEND machine --region ${REGION})
    endif()
    execute_process(COMMAND ${EMULATOR} ${RUN} ${machine} ${IMAGE} --frames ${FRAMES} --eyes
      RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
    if(NOT rc STREQUAL "0" OR NOT out MATCHES "(^|\n)eyes frames=${FRAMES} none=([0-9]+) left=([0-9]+) right=([0-9]+) flips=([0-9]+) last=([a-z]+)\n")
      message(FATAL_ERROR "--eyes ${IMAGE}: exit ${rc}, no eyes line\n${out}${err}")
    endif()
    set(got_none ${CMAKE_MATCH_2})
    set(got_left ${CMAKE_MATCH_3})
    set(got_right ${CMAKE_MATCH_4})
    set(got_flips ${CMAKE_MATCH_5})
    set(got_last ${CMAKE_MATCH_6})
    foreach(key IN ITEMS none left right flips last)
      if(NOT got_${key} STREQUAL WANT_${key})
        message(FATAL_ERROR "eyes: ${key}=${got_${key}}, expected ${WANT_${key}}\n${out}")
      endif()
    endforeach()
    execute_process(COMMAND ${EMULATOR} ${RUN} ${machine} ${IMAGE} --frames 120 --mailbox
      RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
    if(NOT rc STREQUAL "0")
      message(FATAL_ERROR "--mailbox ${IMAGE}: exit ${rc}\n${err}")
    endif()
    file(WRITE "${OUT}" "${out}")
    set(ROM ${NAME})
    include(${JUDGE})
  elseif(CASE STREQUAL "eyes_hash")
    # The last frame's hash is the same with and without the eye report: the report changes no pixel.
    execute_process(COMMAND ${EMULATOR} ${RUN} ${IMAGE} --frames ${FRAMES} --hash
      RESULT_VARIABLE rc OUTPUT_VARIABLE plain ERROR_VARIABLE err)
    execute_process(COMMAND ${EMULATOR} ${RUN} ${IMAGE} --frames ${FRAMES} --hash --eyes
      RESULT_VARIABLE rc_eyes OUTPUT_VARIABLE with_eyes ERROR_VARIABLE err_eyes)
    if(NOT rc STREQUAL "0" OR NOT rc_eyes STREQUAL "0" OR NOT plain MATCHES "(^|\n)frame ${FRAMES} sha256 ([0-9a-f]+)\n")
      message(FATAL_ERROR "eyes_hash: exit ${rc} and ${rc_eyes}\n${plain}${err}${err_eyes}")
    endif()
    set(plain_hash ${CMAKE_MATCH_2})
    if(NOT with_eyes MATCHES "(^|\n)frame ${FRAMES} sha256 ${plain_hash}\n" OR NOT with_eyes MATCHES "(^|\n)eyes frames=")
      message(FATAL_ERROR "eyes_hash: the hash ${plain_hash} changes with --eyes\n${with_eyes}")
    endif()
  elseif(CASE STREQUAL "bios_boot")
    # One frame (K = 1) is inside the BIOS phase of the owned image: it paints a visible backdrop and waits about
    # two frames before its hand-off, so the picture differs from the black fast-boot frame. Frame N of a BIOS run
    # is not frame N of a fast run; that is what is being shown here, not a fault.
    set(K 1)
    function(frame_hash hash_var err_var)
      execute_process(COMMAND ${EMULATOR} ${RUN} ${IMAGE} ${ARGN}
        RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
      if(NOT rc STREQUAL "0" OR NOT out MATCHES "^frame ${K} sha256 ([0-9a-f]+)\n$")
        message(FATAL_ERROR "cogmaster-run ${ARGN}: expected exit 0 and one hash line, got exit ${rc}\n${out}${err}")
      endif()
      set(${hash_var} ${CMAKE_MATCH_1} PARENT_SCOPE)
      set(${err_var} "${err}" PARENT_SCOPE)
    endfunction()
    # The digest the runner prints is the file's own SHA-1: CMake computes it independently of the core.
    function(expect_note note path)
      string(REPEAT "[0-9a-f]" 40 h40)
      if(NOT note MATCHES "BIOS not recognised [(]sha1 (${h40})[)]")
        message(FATAL_ERROR "the BIOS note is missing or malformed:\n${note}")
      endif()
      set(printed ${CMAKE_MATCH_1})
      file(SHA1 "${path}" want)
      if(NOT printed STREQUAL want)
        message(FATAL_ERROR "the note's digest ${printed} is not the file's SHA-1 ${want}")
      endif()
      string(FIND "${note}" "${path}" at)
      if(NOT at EQUAL -1)
        message(FATAL_ERROR "the BIOS note holds the path of the file:\n${note}")
      endif()
      set(digest ${printed} PARENT_SCOPE)
    endfunction()
    frame_hash(fast_plain ignored --frames ${K} --hash)
    frame_hash(fast_named ignored --frames ${K} --hash --boot fast)
    frame_hash(bios_first first_note --frames ${K} --hash --boot bios --bios ${BIOS})
    frame_hash(bios_again again_note --frames ${K} --hash --bios ${BIOS} --boot bios)
    if(NOT bios_first STREQUAL bios_again)
      message(FATAL_ERROR "two BIOS runs differ: ${bios_first} and ${bios_again}")
    endif()
    if(NOT fast_plain STREQUAL fast_named)
      message(FATAL_ERROR "--boot fast changed the hash: ${fast_plain} and ${fast_named}")
    endif()
    if(bios_first STREQUAL fast_plain)
      message(FATAL_ERROR "the BIOS run and the fast run hash alike at frame ${K}: ${bios_first}")
    endif()
    expect_note("${first_note}" "${BIOS}")
    set(first_digest ${digest})
    # A second owned file of 8192 bytes: the run prints its own digest, not a constant and not the first's.
    string(REPEAT "b" 8191 body)
    file(MAKE_DIRECTORY "${OUT}")
    file(CONFIGURE OUTPUT "${OUT}/other.bin" CONTENT "${body}" NEWLINE_STYLE UNIX @ONLY)
    file(SIZE "${OUT}/other.bin" size)
    if(NOT size EQUAL 8192)
      message(FATAL_ERROR "${OUT}/other.bin: wanted 8192 bytes, got ${size}")
    endif()
    frame_hash(ignored other_note --frames ${K} --hash --boot bios --bios ${OUT}/other.bin)
    expect_note("${other_note}" "${OUT}/other.bin")
    if(digest STREQUAL first_digest)
      message(FATAL_ERROR "two different files print the same digest ${digest}")
    endif()
  elseif(CASE STREQUAL "pin")
    # The digest check above is the whole case: the image is not a cartridge, so nothing runs it.
  else()
    message(FATAL_ERROR "unknown CASE '${CASE}'")
  endif()
  return()
endif()

set(COGMASTER_FIXTURES_DIR ${PROJECT_BINARY_DIR}/fixtures)
set(fixtures_src ${PROJECT_SOURCE_DIR}/tests/roms/src)
set(fixtures_images "")
foreach(name IN ITEMS hello mapper_sega mapper_codemasters ports ports_gg boot_to_play boot_to_play_gg battery eeprom
    psg_sweep psg_sweep_gg fm_sweep sports_pad paddle glasses light_phaser)
  set(extension sms)
  set(image_name ${name})
  if(name STREQUAL "boot_to_play_gg")
    set(extension gg)
    set(image_name boot_to_play)
  elseif(name STREQUAL "psg_sweep_gg")
    set(extension gg)
    set(image_name psg_sweep)
  elseif(name STREQUAL "eeprom")
    set(extension gg)
  endif()
  set(image ${COGMASTER_FIXTURES_DIR}/${image_name}.${extension})
  set(identity_check "")
  set(owned_fixture_check ${CMAKE_COMMAND} -DHASH_ONLY=1 -DIMAGE=${image}
    -DFACT=${PROJECT_SOURCE_DIR}/tests/cart/fixture_facts.txt)
  if(name STREQUAL "battery")
    set(identity_check COMMAND ${owned_fixture_check} -DROW=1
      -P ${PROJECT_SOURCE_DIR}/tests/cart/fixture.cmake)
  elseif(name STREQUAL "eeprom")
    set(identity_check COMMAND ${owned_fixture_check} -DROW=2
      -P ${PROJECT_SOURCE_DIR}/tests/cart/fixture.cmake)
  elseif(name STREQUAL "sports_pad")
    set(identity_check COMMAND ${owned_fixture_check} -DROW=3
      -P ${PROJECT_SOURCE_DIR}/tests/cart/fixture.cmake)
  elseif(name STREQUAL "paddle")
    set(identity_check COMMAND ${owned_fixture_check} -DROW=4
      -P ${PROJECT_SOURCE_DIR}/tests/cart/fixture.cmake)
  endif()
  # The source directory is the assembler's working directory, so .INCLUDE
  # finds header.inc; the linker's is the build directory, so the link file
  # names the object without a path.
  add_custom_command(OUTPUT ${image}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${COGMASTER_FIXTURES_DIR}
    COMMAND ${COGMASTER_WLA_Z80} -q -o ${COGMASTER_FIXTURES_DIR}/${name}.o ${name}.asm
    COMMAND ${CMAKE_COMMAND} -E chdir ${COGMASTER_FIXTURES_DIR}
      ${COGMASTER_WLALINK} -r ${fixtures_src}/${name}.link ${image}
    ${identity_check}
    WORKING_DIRECTORY ${fixtures_src}
    DEPENDS ${fixtures_src}/${name}.asm ${fixtures_src}/${name}.link ${fixtures_src}/header.inc
      ${fixtures_src}/mailbox.inc ${fixtures_src}/ports.asm ${fixtures_src}/boot_to_play.asm
      ${fixtures_src}/psg_sweep.asm ${fixtures_src}/fm_sweep.asm
      ${PROJECT_SOURCE_DIR}/tests/cart/fixture_facts.txt ${PROJECT_SOURCE_DIR}/tests/cart/fixture.cmake
    COMMENT "Assembling the ${name} fixture"
    VERBATIM)
  list(APPEND fixtures_images ${image})
endforeach()
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${fixtures_src}/vdp_lines.link)
file(READ ${fixtures_src}/vdp_lines.link lines_link)
foreach(height IN ITEMS 192 224 240)
  string(REPLACE "vdp_lines.o" "vdp_lines_${height}.o" link "${lines_link}")
  set(image ${COGMASTER_FIXTURES_DIR}/vdp_lines_${height}.sms)
  file(GENERATE OUTPUT ${COGMASTER_FIXTURES_DIR}/vdp_lines_${height}.link
    CONTENT "${link}")
  add_custom_command(OUTPUT ${image}
    COMMAND ${COGMASTER_WLA_Z80} -q -D LINES_HEIGHT=${height}
      -o ${COGMASTER_FIXTURES_DIR}/vdp_lines_${height}.o vdp_lines.asm
    COMMAND ${CMAKE_COMMAND} -E chdir ${COGMASTER_FIXTURES_DIR}
      ${COGMASTER_WLALINK} -r vdp_lines_${height}.link ${image}
    WORKING_DIRECTORY ${fixtures_src}
    DEPENDS ${fixtures_src}/vdp_lines.asm ${fixtures_src}/vdp_lines.link
      ${fixtures_src}/header.inc ${fixtures_src}/mailbox.inc VERBATIM)
  list(APPEND fixtures_images ${image})
endforeach()
# One source builds fm_sweep and its Japan header variant: FIXTURE_REGION_CODE=3 is Master System Japan, so
# the library judge meets a cartridge whose header names Japan. The link file names the variant's object.
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${fixtures_src}/fm_sweep.link)
file(READ ${fixtures_src}/fm_sweep.link fm_link)
string(REPLACE "fm_sweep.o" "fm_sweep_jp.o" link "${fm_link}")
set(image ${COGMASTER_FIXTURES_DIR}/fm_sweep_jp.sms)
file(GENERATE OUTPUT ${COGMASTER_FIXTURES_DIR}/fm_sweep_jp.link CONTENT "${link}")
add_custom_command(OUTPUT ${image}
  COMMAND ${COGMASTER_WLA_Z80} -q -D FIXTURE_REGION_CODE=3
    -o ${COGMASTER_FIXTURES_DIR}/fm_sweep_jp.o fm_sweep.asm
  COMMAND ${CMAKE_COMMAND} -E chdir ${COGMASTER_FIXTURES_DIR}
    ${COGMASTER_WLALINK} -r fm_sweep_jp.link ${image}
  WORKING_DIRECTORY ${fixtures_src}
  DEPENDS ${fixtures_src}/fm_sweep.asm ${fixtures_src}/fm_sweep.link
    ${fixtures_src}/header.inc ${fixtures_src}/mailbox.inc VERBATIM)
list(APPEND fixtures_images ${image})
# One source builds the V counter table images, as vdp_lines does: LINES_HEIGHT picks the
# screen height and each image gets its own link file.
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${fixtures_src}/vdp_vtable.link)
file(READ ${fixtures_src}/vdp_vtable.link vtable_link)
foreach(height IN ITEMS 192 224 240)
  string(REPLACE "vdp_vtable.o" "vdp_vtable_${height}.o" link "${vtable_link}")
  set(image ${COGMASTER_FIXTURES_DIR}/vdp_vtable_${height}.sms)
  file(GENERATE OUTPUT ${COGMASTER_FIXTURES_DIR}/vdp_vtable_${height}.link
    CONTENT "${link}")
  add_custom_command(OUTPUT ${image}
    COMMAND ${COGMASTER_WLA_Z80} -q -D LINES_HEIGHT=${height}
      -o ${COGMASTER_FIXTURES_DIR}/vdp_vtable_${height}.o vdp_vtable.asm
    COMMAND ${CMAKE_COMMAND} -E chdir ${COGMASTER_FIXTURES_DIR}
      ${COGMASTER_WLALINK} -r vdp_vtable_${height}.link ${image}
    WORKING_DIRECTORY ${fixtures_src}
    DEPENDS ${fixtures_src}/vdp_vtable.asm ${fixtures_src}/vdp_vtable.link
      ${fixtures_src}/header.inc ${fixtures_src}/mailbox.inc VERBATIM)
  list(APPEND fixtures_images ${image})
endforeach()
# One source builds the Pause NMI images, as vdp_vtable does.
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${fixtures_src}/pause_nmi.link)
file(READ ${fixtures_src}/pause_nmi.link pause_link)
foreach(height IN ITEMS 192 224 240)
  string(REPLACE "pause_nmi.o" "pause_nmi_${height}.o" link "${pause_link}")
  set(image ${COGMASTER_FIXTURES_DIR}/pause_nmi_${height}.sms)
  file(GENERATE OUTPUT ${COGMASTER_FIXTURES_DIR}/pause_nmi_${height}.link
    CONTENT "${link}")
  add_custom_command(OUTPUT ${image}
    COMMAND ${COGMASTER_WLA_Z80} -q -D LINES_HEIGHT=${height}
      -o ${COGMASTER_FIXTURES_DIR}/pause_nmi_${height}.o pause_nmi.asm
    COMMAND ${CMAKE_COMMAND} -E chdir ${COGMASTER_FIXTURES_DIR}
      ${COGMASTER_WLALINK} -r pause_nmi_${height}.link ${image}
    WORKING_DIRECTORY ${fixtures_src}
    DEPENDS ${fixtures_src}/pause_nmi.asm ${fixtures_src}/pause_nmi.link
      ${fixtures_src}/header.inc ${fixtures_src}/mailbox.inc VERBATIM)
  list(APPEND fixtures_images ${image})
endforeach()
# One source builds the Korean, Zemina and 4 PAK images, as vdp_lines does: FIXTURE_BOARD picks
# the board and each image gets its own link file.
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${fixtures_src}/mapper_korean.link)
file(READ ${fixtures_src}/mapper_korean.link korean_link)
foreach(spec IN ITEMS korean:1 zemina:2 hes_4pak:3)
  string(REPLACE ":" ";" spec "${spec}")
  list(GET spec 0 board)
  list(GET spec 1 board_number)
  string(REPLACE "mapper_korean.o" "mapper_${board}.o" link "${korean_link}")
  set(image ${COGMASTER_FIXTURES_DIR}/mapper_${board}.sms)
  file(GENERATE OUTPUT ${COGMASTER_FIXTURES_DIR}/mapper_${board}.link CONTENT "${link}")
  add_custom_command(OUTPUT ${image}
    COMMAND ${COGMASTER_WLA_Z80} -q -D FIXTURE_BOARD=${board_number}
      -o ${COGMASTER_FIXTURES_DIR}/mapper_${board}.o mapper_korean.asm
    COMMAND ${CMAKE_COMMAND} -E chdir ${COGMASTER_FIXTURES_DIR}
      ${COGMASTER_WLALINK} -r mapper_${board}.link ${image}
    WORKING_DIRECTORY ${fixtures_src}
    DEPENDS ${fixtures_src}/mapper_korean.asm ${fixtures_src}/mapper_korean.link
      ${fixtures_src}/mailbox.inc VERBATIM)
  list(APPEND fixtures_images ${image})
endforeach()
# One source builds the BIOS-shaped test image, its probe cartridge and the two bad images:
# name:extension:selector, as vdp_lines does. The image is not a cartridge and has no header.
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${fixtures_src}/bios_shape.link)
file(READ ${fixtures_src}/bios_shape.link bios_link)
set(bios_images bios_probe bios_shape bios_shape_bad_ram bios_shape_bad_sp)
foreach(spec IN ITEMS bios_probe:sms:PROBE=1 bios_shape:bin:FULL=1 bios_shape_bad_ram:bin:BAD_RAM=1
    bios_shape_bad_sp:bin:BAD_SP=1)
  string(REPLACE ":" ";" spec "${spec}")
  list(GET spec 0 name)
  list(GET spec 1 extension)
  list(GET spec 2 define)
  string(REPLACE "bios_shape.o" "${name}.o" link "${bios_link}")
  set(image ${COGMASTER_FIXTURES_DIR}/${name}.${extension})
  file(GENERATE OUTPUT ${COGMASTER_FIXTURES_DIR}/${name}.link CONTENT "${link}")
  add_custom_command(OUTPUT ${image}
    COMMAND ${COGMASTER_WLA_Z80} -q -D ${define} -o ${COGMASTER_FIXTURES_DIR}/${name}.o bios_shape.asm
    COMMAND ${CMAKE_COMMAND} -E chdir ${COGMASTER_FIXTURES_DIR}
      ${COGMASTER_WLALINK} -r ${name}.link ${image}
    WORKING_DIRECTORY ${fixtures_src}
    DEPENDS ${fixtures_src}/bios_shape.asm ${fixtures_src}/bios_shape.link VERBATIM)
  list(APPEND fixtures_images ${image})
endforeach()
# One function builds every Graphics I and II image from two sources, as vdp_lines builds its three: the
# image name, the source (legacy_modes or legacy_sprites) and the -D define that picks the variant. Each
# image gets its own link file, because the linker names the object it reads.
function(fixtures_legacy_image name source define)
  set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${fixtures_src}/${source}.link)
  file(READ ${fixtures_src}/${source}.link legacy_link)
  string(REPLACE "${source}.o" "${name}.o" legacy_link "${legacy_link}")
  set(image ${COGMASTER_FIXTURES_DIR}/${name}.sms)
  file(GENERATE OUTPUT ${COGMASTER_FIXTURES_DIR}/${name}.link CONTENT "${legacy_link}")
  add_custom_command(OUTPUT ${image}
    COMMAND ${COGMASTER_WLA_Z80} -q -D ${define} -o ${COGMASTER_FIXTURES_DIR}/${name}.o ${source}.asm
    COMMAND ${CMAKE_COMMAND} -E chdir ${COGMASTER_FIXTURES_DIR}
      ${COGMASTER_WLALINK} -r ${name}.link ${image}
    WORKING_DIRECTORY ${fixtures_src}
    DEPENDS ${fixtures_src}/${source}.asm ${fixtures_src}/${source}.link
      ${fixtures_src}/header.inc ${fixtures_src}/mailbox.inc VERBATIM)
  list(APPEND fixtures_images ${image})
  set(fixtures_images ${fixtures_images} PARENT_SCOPE)
endfunction()
fixtures_legacy_image(legacy_modes_g1 legacy_modes V_G1=1)
fixtures_legacy_image(legacy_modes_g2 legacy_modes V_G2=1)
fixtures_legacy_image(legacy_modes_g2_masked legacy_modes V_G2_MASKED=1)
fixtures_legacy_image(legacy_modes_g2_late legacy_modes V_G2_LATE=1)
fixtures_legacy_image(legacy_modes_g2_switch legacy_modes V_G2_SWITCH=1)
fixtures_legacy_image(legacy_sprites legacy_sprites V_SPRITES=1)
add_custom_target(cogmaster-fixtures ALL DEPENDS ${fixtures_images})

add_executable(cogmaster-test-lines ${CMAKE_CURRENT_LIST_DIR}/lines.c
  ${PROJECT_SOURCE_DIR}/runner/sha256.c)
target_include_directories(cogmaster-test-lines PRIVATE ${PROJECT_SOURCE_DIR}/runner)
target_link_libraries(cogmaster-test-lines PRIVATE cogmaster::cogmaster)
cogmaster_warnings(cogmaster-test-lines)

add_executable(cogmaster-test-play ${CMAKE_CURRENT_LIST_DIR}/play.c
  ${PROJECT_SOURCE_DIR}/runner/sha256.c)
target_include_directories(cogmaster-test-play PRIVATE ${PROJECT_SOURCE_DIR}/runner)
target_link_libraries(cogmaster-test-play PRIVATE cogmaster::cogmaster)
cogmaster_warnings(cogmaster-test-play)

# The closed-form oracle for the Graphics I and II images: it prints the expected digest and compares
# a run with it. A later test registers the comparisons; this one checks the oracle itself.
add_executable(cogmaster-test-legacy-fixture ${CMAKE_CURRENT_LIST_DIR}/legacy.c
  ${PROJECT_SOURCE_DIR}/runner/sha256.c)
target_include_directories(cogmaster-test-legacy-fixture PRIVATE ${PROJECT_SOURCE_DIR}/runner)
target_link_libraries(cogmaster-test-legacy-fixture PRIVATE cogmaster::cogmaster)
cogmaster_warnings(cogmaster-test-legacy-fixture)

# Fault prevented: guest-only rendering regressions across both TV standards and every height.
foreach(tv IN ITEMS ntsc pal)
  foreach(height IN ITEMS 192 224 240)
    cogmaster_add_test(NAME fixtures.vdp_lines.pixels.${tv}.${height}
      COMMAND cogmaster-test-lines ${COGMASTER_FIXTURES_DIR}/vdp_lines_${height}.sms ${height} ${tv}
      LABELS fixtures portable TIMEOUT 10)
  endforeach()
endforeach()

# Fault prevented: an oracle that disagrees with hand-computed pixels or status bytes, which would make
# every later comparison against it a comparison with a wrong answer, and a legacy_sprites row of
# tests/diag/expected.csv that is typed by hand instead of computed by legacy_status.
# Timeout: 10 s floor; the literal check runs in well under a second, provisional.
cogmaster_add_test(NAME fixtures.legacy.oracle
  COMMAND cogmaster-test-legacy-fixture - all all literals
    ${PROJECT_SOURCE_DIR}/tests/diag/expected.csv
  LABELS fixtures portable TIMEOUT 10)

string(REPLACE ";" "$<SEMICOLON>" fixtures_emulator "${CMAKE_CROSSCOMPILING_EMULATOR}")

# Fault prevented: blank content video or a runner digest shared with a bad renderer.
cogmaster_add_test(NAME fixtures.boot_to_play
  COMMAND ${CMAKE_COMMAND} -DCASE=play -DNAME=boot_to_play
    -DIMAGE=${COGMASTER_FIXTURES_DIR}/boot_to_play.sms
    -DEXPECTED=${PROJECT_SOURCE_DIR}/tests/built/expected.txt
    -DORACLE=$<TARGET_FILE:cogmaster-test-play> -DRUN=$<TARGET_FILE:cogmaster-run>
    "-DEMULATOR=${fixtures_emulator}" -P ${CMAKE_CURRENT_LIST_FILE}
  LABELS fixtures portable TIMEOUT 10)

set(play_input ${PROJECT_BINARY_DIR}/tests/play.input)
set(play_body "")
foreach(frame RANGE 1 14)
  set(pad 00)
  if(frame EQUAL 5 OR frame EQUAL 6 OR frame EQUAL 8)
    set(pad 10)
  endif()
  string(APPEND play_body "p1=${pad} p2=00 pause=0 reset=0 start=0\n")
endforeach()
file(WRITE ${play_input} "${play_body}")
set(play_pad2 "")
foreach(frame RANGE 1 14)
  set(pad 00)
  if(frame EQUAL 5 OR frame EQUAL 6 OR frame EQUAL 8)
    set(pad 10)
  endif()
  string(APPEND play_pad2 "p1=00 p2=${pad} pause=0 reset=0 start=0\n")
endforeach()
file(WRITE ${PROJECT_BINARY_DIR}/tests/play-pad2.input "${play_pad2}")
set(play_both "")
foreach(frame RANGE 1 14)
  set(p1 00)
  set(p2 00)
  if(frame EQUAL 5 OR frame EQUAL 6 OR frame EQUAL 8)
    set(p1 10)
  endif()
  if(frame EQUAL 6 OR frame EQUAL 7 OR frame EQUAL 10)
    set(p2 10)
  endif()
  string(APPEND play_both "p1=${p1} p2=${p2} pause=0 reset=0 start=0\n")
endforeach()
file(WRITE ${PROJECT_BINARY_DIR}/tests/play-both.input "${play_both}")
# The Pause script: 40 records, the button held on frames 3 to 7 (one edge, so one NMI) and again on
# frame 20 (a second edge), as pause_nmi expects. Written with Unix line ends on every host.
set(pause_input ${PROJECT_BINARY_DIR}/tests/pause.input)
set(pause_body "")
foreach(frame RANGE 1 40)
  set(pause 0)
  if((frame GREATER 2 AND frame LESS 8) OR frame EQUAL 20)
    set(pause 1)
  endif()
  string(APPEND pause_body "p1=00 p2=00 pause=${pause} reset=0 start=0\n")
endforeach()
file(CONFIGURE OUTPUT ${pause_input} CONTENT "${pause_body}" NEWLINE_STYLE UNIX @ONLY)
cogmaster_add_test(NAME fixtures.boot_to_play.both_pads
  COMMAND ${CMAKE_COMMAND} -DCASE=play -DNAME=boot_to_play -DBOTH=1
    -DINPUT=${PROJECT_BINARY_DIR}/tests/play-both.input
    -DIMAGE=${COGMASTER_FIXTURES_DIR}/boot_to_play.sms
    -DEXPECTED=${PROJECT_SOURCE_DIR}/tests/built/expected.txt
    -DORACLE=$<TARGET_FILE:cogmaster-test-play> -DRUN=$<TARGET_FILE:cogmaster-run>
    "-DEMULATOR=${fixtures_emulator}" -P ${CMAKE_CURRENT_LIST_FILE}
  LABELS fixtures portable TIMEOUT 10)
set(gg_body "")
foreach(frame RANGE 1 14)
  set(start 0)
  if(frame EQUAL 5 OR frame EQUAL 6 OR frame EQUAL 8)
    set(start 1)
  endif()
  string(APPEND gg_body "p1=00 p2=00 pause=0 reset=0 start=${start}\n")
endforeach()
file(WRITE ${PROJECT_BINARY_DIR}/tests/play-gg.input "${gg_body}")
foreach(variant IN ITEMS baseline start)
  set(input "")
  if(variant STREQUAL "start")
    set(input -DINPUT=${PROJECT_BINARY_DIR}/tests/play-gg.input)
  endif()
  cogmaster_add_test(NAME fixtures.boot_to_play.gg.${variant}
    COMMAND ${CMAKE_COMMAND} -DCASE=play -DNAME=boot_to_play_gg ${input}
      -DIMAGE=${COGMASTER_FIXTURES_DIR}/boot_to_play.gg
      -DEXPECTED=${PROJECT_SOURCE_DIR}/tests/built/expected.txt
      -DORACLE=$<TARGET_FILE:cogmaster-test-play> -DRUN=$<TARGET_FILE:cogmaster-run>
      "-DEMULATOR=${fixtures_emulator}" -P ${CMAKE_CURRENT_LIST_FILE}
    LABELS fixtures portable TIMEOUT 10)
endforeach()
cogmaster_add_test(NAME fixtures.boot_to_play.pad2
  COMMAND ${CMAKE_COMMAND} -DCASE=play -DNAME=boot_to_play -DPAD2=1
    -DINPUT=${PROJECT_BINARY_DIR}/tests/play-pad2.input
    -DIMAGE=${COGMASTER_FIXTURES_DIR}/boot_to_play.sms
    -DEXPECTED=${PROJECT_SOURCE_DIR}/tests/built/expected.txt
    -DORACLE=$<TARGET_FILE:cogmaster-test-play> -DRUN=$<TARGET_FILE:cogmaster-run>
    "-DEMULATOR=${fixtures_emulator}" -P ${CMAKE_CURRENT_LIST_FILE}
  LABELS fixtures portable TIMEOUT 10)
# Fault prevented: held input counting twice, late boundary input, or disconnected pins sticking.
cogmaster_add_test(NAME fixtures.boot_to_play.input
  COMMAND ${CMAKE_COMMAND} -DCASE=play -DNAME=boot_to_play -DINPUT=${play_input}
    -DIMAGE=${COGMASTER_FIXTURES_DIR}/boot_to_play.sms
    -DEXPECTED=${PROJECT_SOURCE_DIR}/tests/built/expected.txt
    -DORACLE=$<TARGET_FILE:cogmaster-test-play> -DRUN=$<TARGET_FILE:cogmaster-run>
    "-DEMULATOR=${fixtures_emulator}" -P ${CMAKE_CURRENT_LIST_FILE}
  LABELS fixtures portable TIMEOUT 10)

string(REPLACE ";" "\\;" fixtures_inner_emulator "${CMAKE_CROSSCOMPILING_EMULATOR}")
set(cmd ${CMAKE_COMMAND} -DCASE=play -DNAME=boot_to_play -DWRONG=1
  -DIMAGE=${COGMASTER_FIXTURES_DIR}/boot_to_play.sms
  -DEXPECTED=${PROJECT_SOURCE_DIR}/tests/built/expected.txt
  -DORACLE=$<TARGET_FILE:cogmaster-test-play> -DRUN=$<TARGET_FILE:cogmaster-run>
  "-DEMULATOR=${fixtures_inner_emulator}" -P ${CMAKE_CURRENT_LIST_FILE})
string(REPLACE ";" "$<SEMICOLON>" cmd "${cmd}")
# Fault prevented: a digest comparator accepting one changed expected pixel.
cogmaster_add_test(NAME fixtures.boot_to_play.wrong_hash
  COMMAND ${CMAKE_COMMAND} "-DCMD=${cmd}" -DCODE=1 "-DREGEX=native frame mismatch"
    -P ${PROJECT_SOURCE_DIR}/tests/cmake/expect_fail.cmake
  LABELS fixtures portable TIMEOUT 10)

# Fault prevented: a legacy renderer that disagrees with the closed-form oracle on a dot of a Graphics I
# or II image, on any of the three profiles, or a runner hash that differs from the oracle digest. Each
# row is <variant>:<frames>:<profiles that also compare the runner hash>; the static images are complete
# from frame 17 (Graphics I from frame 3), the late image from frame 616 (16 load frames and 600 counted
# ones), and the switch image is judged at the frame its switch line is pinned for, by dot, not by hash.
# Timeout: 10 s floor; provisional, the first hosted run sets it (the late image runs 620 frames).
foreach(spec IN ITEMS g1:20:all g2:20:all g2_masked:20:all g2_late:620:sms2 g2_switch:30:none)
  string(REPLACE ":" ";" spec "${spec}")
  list(GET spec 0 legacy_variant)
  list(GET spec 1 legacy_frames)
  list(GET spec 2 legacy_runner)
  foreach(legacy_profile IN ITEMS sms1 sms2 gg_sms)
    set(legacy_flag "")
    if(legacy_runner STREQUAL "all" OR legacy_runner STREQUAL legacy_profile)
      set(legacy_flag -DRUNNER=1)
    endif()
    cogmaster_add_test(NAME fixtures.legacy_modes.${legacy_variant}.${legacy_profile}
      COMMAND ${CMAKE_COMMAND} -DCASE=legacy -DNAME=legacy_modes_${legacy_variant}
        -DVARIANT=${legacy_variant} -DPROFILE=${legacy_profile} -DFRAMES=${legacy_frames} ${legacy_flag}
        -DIMAGE=${COGMASTER_FIXTURES_DIR}/legacy_modes_${legacy_variant}.sms
        -DEXPECTED=${PROJECT_SOURCE_DIR}/tests/built/expected.txt
        -DORACLE=$<TARGET_FILE:cogmaster-test-legacy-fixture> -DRUN=$<TARGET_FILE:cogmaster-run>
        "-DEMULATOR=${fixtures_emulator}" -P ${CMAKE_CURRENT_LIST_FILE}
      LABELS fixtures portable TIMEOUT 10)
  endforeach()
endforeach()
set(cmd ${CMAKE_COMMAND} -DCASE=legacy -DNAME=legacy_modes_g1 -DVARIANT=g1 -DPROFILE=sms2
  -DFRAMES=20 -DRUNNER=1 -DWRONG=1
  -DIMAGE=${COGMASTER_FIXTURES_DIR}/legacy_modes_g1.sms
  -DEXPECTED=${PROJECT_SOURCE_DIR}/tests/built/expected.txt
  -DORACLE=$<TARGET_FILE:cogmaster-test-legacy-fixture> -DRUN=$<TARGET_FILE:cogmaster-run>
  "-DEMULATOR=${fixtures_inner_emulator}" -P ${CMAKE_CURRENT_LIST_FILE})
string(REPLACE ";" "$<SEMICOLON>" cmd "${cmd}")
# Fault prevented: a legacy digest comparator accepting a digest that no frame has.
cogmaster_add_test(NAME fixtures.legacy_modes.wrong_hash
  COMMAND ${CMAKE_COMMAND} "-DCMD=${cmd}" -DCODE=1 "-DREGEX=legacy runner hash mismatch"
    -P ${PROJECT_SOURCE_DIR}/tests/cmake/expect_fail.cmake
  LABELS fixtures portable TIMEOUT 10)

# Fault prevented: a fixture assembled to different bytes than the ones
# pinned (another assembler, a changed source, CRLF line endings), or a
# header checksum the runner's probe disagrees with.
# Timeout: 10 s floor; 2 x 0.01 s, slowest of 2 runs, run 39376644025, provisional.
cogmaster_add_test(NAME fixtures.hello
  COMMAND ${CMAKE_COMMAND} -DCASE=image -DNAME=hello -DIMAGE=${COGMASTER_FIXTURES_DIR}/hello.sms
    -DEXPECTED=${PROJECT_SOURCE_DIR}/tests/built/expected.txt -DRUN=$<TARGET_FILE:cogmaster-run>
    "-DEMULATOR=${fixtures_emulator}" -P ${CMAKE_CURRENT_LIST_FILE}
  LABELS fixtures portable TIMEOUT 10)

# Fault prevented: a BIOS-shaped test image, its probe cartridge or one of the two bad images
# whose bytes drift from their pin (another assembler, a changed source, CRLF line endings), so
# the equivalence check would judge a different program than the one pinned.
# Timeout: 20 s floor; 2 x 0.06 s (windows-arm64), slowest of 9 runs, run 43074158512.
foreach(name IN LISTS bios_images)
  set(extension bin)
  if(name STREQUAL "bios_probe")
    set(extension sms)
  endif()
  cogmaster_add_test(NAME fixtures.bios_shape.pin.${name}
    COMMAND ${CMAKE_COMMAND} -DCASE=pin -DNAME=${name} -DIMAGE=${COGMASTER_FIXTURES_DIR}/${name}.${extension}
      -DEXPECTED=${PROJECT_SOURCE_DIR}/tests/built/expected.txt -P ${CMAKE_CURRENT_LIST_FILE}
    LABELS fixtures portable TIMEOUT 20)
endforeach()

# Fault prevented: a fast-boot hand-off that drifts from the state the hand-off facts describe, a
# comparator that ignores a field silently, and a vacuous equivalence that passes because both
# sides came from the same constants. The probe cartridge and the owned BIOS-shaped image are
# passed as the built paths; the honest limit (the mechanism and the single source of constants,
# not what Sega's BIOS leaves behind) is stated in the test.
# Timeout: 20 s floor; 2 x 1.42 s (be32), slowest of 9 runs, run 43074158512.
cogmaster_add_test(NAME bios.equivalence
  COMMAND cogmaster-test-bios equivalence ${COGMASTER_FIXTURES_DIR}/bios_probe.sms
    ${COGMASTER_FIXTURES_DIR}/bios_shape.bin
  LABELS bios fixtures portable TIMEOUT 20)

# Fault prevented: a bad image (the top-of-RAM store left out, SP two bytes high) that passes, or
# fails for another reason than the field the comparator names; the pattern is the comparator's own
# fixed failure line, which a missing file or a crash does not print.
# Timeout: 20 s floor; 2 x 0.21 s (be32), slowest of 9 runs, run 43074158512.
foreach(spec IN ITEMS bad_ram:ram bad_sp:z80_sp)
  string(REPLACE ":" ";" spec "${spec}")
  list(GET spec 0 bad)
  list(GET spec 1 field)
  set(bad_cmd ${CMAKE_CROSSCOMPILING_EMULATOR} $<TARGET_FILE:cogmaster-test-bios> equivalence
    ${COGMASTER_FIXTURES_DIR}/bios_probe.sms ${COGMASTER_FIXTURES_DIR}/bios_shape_${bad}.bin)
  string(REPLACE ";" "$<SEMICOLON>" bad_cmd "${bad_cmd}")
  cogmaster_add_test(NAME bios.equivalence.${bad}
    COMMAND ${CMAKE_COMMAND} "-DCMD=${bad_cmd}" -DCODE=1
      "-DREGEX=equivalence: differing fields \\[${field}\\]"
      -P ${PROJECT_SOURCE_DIR}/tests/cmake/expect_fail.cmake
    LABELS bios fixtures portable TIMEOUT 20)
endforeach()

# Fault prevented: a BIOS boot whose first frame is the fast-boot frame (the BIOS never ran), a boot that differs
# between two runs, a fast boot that changes when the flag is merely spelled, and a label line that prints a constant
# or the digest of other bytes than the file's. Recognition of a known dump is proved by host.bios through the real
# identify path with an injected table, because no real dump may exist in the tree.
# Timeout: 20 s floor; 2 x 0.27 s (be32), slowest of 9 runs, run 43074158512.
cogmaster_add_test(NAME runner.bios.boot
  COMMAND ${CMAKE_COMMAND} -DCASE=bios_boot -DNAME=bios_probe -DIMAGE=${COGMASTER_FIXTURES_DIR}/bios_probe.sms
    -DBIOS=${COGMASTER_FIXTURES_DIR}/bios_shape.bin -DOUT=${PROJECT_BINARY_DIR}/tests/bios-boot
    -DEXPECTED=${PROJECT_SOURCE_DIR}/tests/built/expected.txt -DRUN=$<TARGET_FILE:cogmaster-run>
    "-DEMULATOR=${fixtures_emulator}" -P ${CMAKE_CURRENT_LIST_FILE}
  LABELS portable runner TIMEOUT 20)

# Fault prevented: a fixture whose measurements are printed but never
# compared, so a core that pages the wrong bank passes. The image's digest is
# checked, then its mailbox records are judged against tests/diag/expected.csv.
set(fixtures_mailbox -DCASE=mailbox -DEXPECTED=${PROJECT_SOURCE_DIR}/tests/built/expected.txt
  -DRUN=$<TARGET_FILE:cogmaster-run> "-DEMULATOR=${fixtures_emulator}"
  -DJUDGE=${PROJECT_SOURCE_DIR}/tests/cmake/mailbox.cmake -DPROFILE=sms-export)
set(fixtures_out ${PROJECT_BINARY_DIR}/tests/mailbox)
file(MAKE_DIRECTORY ${fixtures_out})
foreach(variant IN ITEMS baseline start)
  set(input "")
  if(variant STREQUAL "start")
    set(input -DINPUT=${PROJECT_BINARY_DIR}/tests/play-gg.input)
  endif()
  cogmaster_add_test(NAME fixtures.boot_to_play.gg.mailbox.${variant}
    COMMAND ${CMAKE_COMMAND} ${fixtures_mailbox} -DNAME=boot_to_play_gg
      -DPROFILE=${variant} ${input}
      -DIMAGE=${COGMASTER_FIXTURES_DIR}/boot_to_play.gg -DFRAMES=14
      -DCSV=${PROJECT_SOURCE_DIR}/tests/diag/expected.csv
      -DOUT=${fixtures_out}/boot_to_play_gg.${variant}.txt -P ${CMAKE_CURRENT_LIST_FILE}
    LABELS fixtures portable TIMEOUT 10)
endforeach()
# Fault prevented: input measurements with invalid mailbox CRC or missing records.
cogmaster_add_test(NAME fixtures.boot_to_play.mailbox
  COMMAND ${CMAKE_COMMAND} ${fixtures_mailbox} -DNAME=boot_to_play
    -DIMAGE=${COGMASTER_FIXTURES_DIR}/boot_to_play.sms -DFRAMES=20
    -DCSV=${PROJECT_SOURCE_DIR}/tests/diag/expected.csv -DOUT=${fixtures_out}/boot_to_play.txt
    -P ${CMAKE_CURRENT_LIST_FILE}
  LABELS fixtures portable TIMEOUT 10)
# Fault prevented: a PSG script whose frame or write counters drift, or an image that is not
# the one pinned, so the sound hashes over it would judge a different program.
# Timeout: 10 s floor; 0.05 s slowest local run, no hosted run yet, so provisional.
cogmaster_add_test(NAME fixtures.psg_sweep
  COMMAND ${CMAKE_COMMAND} ${fixtures_mailbox} -DNAME=psg_sweep
    -DIMAGE=${COGMASTER_FIXTURES_DIR}/psg_sweep.sms -DFRAMES=300
    -DCSV=${PROJECT_SOURCE_DIR}/tests/diag/expected.csv -DOUT=${fixtures_out}/psg_sweep.txt
    -P ${CMAKE_CURRENT_LIST_FILE}
  LABELS fixtures portable psg TIMEOUT 10)
# The Game Gear image is the same table with the port $06 entries, judged by the same rows under
# the gg profile.
# Timeout: 10 s floor; 0.05 s slowest local run, no hosted run yet, so provisional.
cogmaster_add_test(NAME fixtures.psg_sweep.gg
  COMMAND ${CMAKE_COMMAND} ${fixtures_mailbox} -DNAME=psg_sweep_gg -DROM=psg_sweep -DPROFILE=gg
    -DIMAGE=${COGMASTER_FIXTURES_DIR}/psg_sweep.gg -DFRAMES=300
    -DCSV=${PROJECT_SOURCE_DIR}/tests/diag/expected.csv -DOUT=${fixtures_out}/psg_sweep_gg.txt
    -P ${CMAKE_CURRENT_LIST_FILE}
  LABELS fixtures portable psg TIMEOUT 10)
# Fault prevented: an FM unit that the fixture's own detection routine finds where no unit
# exists, or misses where one is attached, or a script whose frame or write counters drift, so
# the FM hashes over it would judge a different program. The Japanese console carries the unit;
# the export console and the Game Gear (running the same image) do not, and the same script runs
# on all three.
# Timeout: 10 s floor; 0.05 s slowest local run, no hosted run yet, so provisional.
foreach(profile IN ITEMS sms-japan sms-export gg)
  if(profile STREQUAL "sms-japan")
    set(args -DMACHINE=sms -DREGION=ntsc-j)
  elseif(profile STREQUAL "sms-export")
    set(args -DMACHINE=sms -DREGION=ntsc-u)
  else()
    set(args -DMACHINE=gg)
  endif()
  cogmaster_add_test(NAME fixtures.fm_sweep.${profile}
    COMMAND ${CMAKE_COMMAND} ${fixtures_mailbox} -DNAME=fm_sweep ${args} -DPROFILE=${profile}
      -DIMAGE=${COGMASTER_FIXTURES_DIR}/fm_sweep.sms -DFRAMES=300
      -DCSV=${PROJECT_SOURCE_DIR}/tests/diag/expected.csv
      -DOUT=${fixtures_out}/fm_sweep.${profile}.txt -P ${CMAKE_CURRENT_LIST_FILE}
    LABELS fixtures fm portable TIMEOUT 10)
endforeach()
# Fault prevented: the Japan header variant of fm_sweep assembled to other bytes than the pinned ones
# (another assembler, a changed header.inc), which would leave the library judge's admitted image stale.
# Timeout: 20 s floor; 2 x 0.06 s, slowest of 9 runs, run 43126677381.
cogmaster_add_test(NAME fixtures.fm_sweep_jp.pin
  COMMAND ${CMAKE_COMMAND} -DCASE=pin -DNAME=fm_sweep_jp -DIMAGE=${COGMASTER_FIXTURES_DIR}/fm_sweep_jp.sms
    -DEXPECTED=${PROJECT_SOURCE_DIR}/tests/built/expected.txt -P ${CMAKE_CURRENT_LIST_FILE}
  LABELS fixtures portable TIMEOUT 20)
# Fault prevented: a detection routine that is only proven through the runner's default unit
# setting. The same built image runs on an export console with the unit switched on (it must find
# it) and on a Japanese console with the unit switched off (it must not), reading the mailbox
# through cogmaster_peek as the runner does.
# Timeout: 10 s floor; 0.1 s slowest local run, no hosted run yet, so provisional.
cogmaster_add_test(NAME fm.detect
  COMMAND cogmaster-test-fm detect ${COGMASTER_FIXTURES_DIR}/fm_sweep.sms
  LABELS fm portable TIMEOUT 10)
# Timeout: 10 s floor; 2 x 0.02 s, slowest of 2 runs, run 39376644025, provisional.
cogmaster_add_test(NAME fixtures.mapper_sega
  COMMAND ${CMAKE_COMMAND} ${fixtures_mailbox} -DNAME=mapper_sega
    -DIMAGE=${COGMASTER_FIXTURES_DIR}/mapper_sega.sms -DFRAMES=120
    -DCSV=${PROJECT_SOURCE_DIR}/tests/diag/expected.csv -DOUT=${fixtures_out}/mapper_sega.txt
    -P ${CMAKE_CURRENT_LIST_FILE}
  LABELS fixtures portable TIMEOUT 10)
# The Codemasters board has no TMR SEGA header to name the machine.
# Timeout: 10 s floor; 2 x 0.01 s, slowest of 2 runs, run 39376644025, provisional.
cogmaster_add_test(NAME fixtures.mapper_codemasters
  COMMAND ${CMAKE_COMMAND} ${fixtures_mailbox} -DNAME=mapper_codemasters -DMACHINE=sms
    -DIMAGE=${COGMASTER_FIXTURES_DIR}/mapper_codemasters.sms -DFRAMES=120
    -DCSV=${PROJECT_SOURCE_DIR}/tests/diag/expected.csv -DOUT=${fixtures_out}/mapper_codemasters.txt
    -P ${CMAKE_CURRENT_LIST_FILE}
  LABELS fixtures portable TIMEOUT 10)
# Fault prevented: a Korean-family board that pages the wrong bank, does not wrap, or answers the
# Sega registers other than as documented, or an image the scan sends to another board.
foreach(board IN ITEMS korean zemina hes_4pak)
  # Timeout: 10 s floor; 0.02 s slowest local run, no hosted run yet, so provisional.
  cogmaster_add_test(NAME fixtures.mapper_${board}
    COMMAND ${CMAKE_COMMAND} ${fixtures_mailbox} -DNAME=mapper_${board} -DMACHINE=sms
      -DIMAGE=${COGMASTER_FIXTURES_DIR}/mapper_${board}.sms -DFRAMES=120
      -DCSV=${PROJECT_SOURCE_DIR}/tests/diag/expected.csv -DOUT=${fixtures_out}/mapper_${board}.txt
      -P ${CMAKE_CURRENT_LIST_FILE}
    LABELS fixtures portable TIMEOUT 10)
endforeach()
# Fault prevented: a Korean-family image the scan does not find, or finds as another board, with
# no header and no setting: each of the three must print board_source=heuristic and its own name.
set(fixtures_probe -DCASE=probe -DEXPECTED=${PROJECT_SOURCE_DIR}/tests/built/expected.txt
  -DNAME=mapper_korean -DIMAGE=${COGMASTER_FIXTURES_DIR}/mapper_korean.sms
  -DDIR=${COGMASTER_FIXTURES_DIR} -DBOARDS=korean,zemina,hes_4pak -DRUN=$<TARGET_FILE:cogmaster-run>)
# Timeout: 10 s floor; 0.02 s slowest local run, no hosted run yet, so provisional.
cogmaster_add_test(NAME fixtures.korean_probe
  COMMAND ${CMAKE_COMMAND} ${fixtures_probe} "-DEMULATOR=${fixtures_emulator}" -P ${CMAKE_CURRENT_LIST_FILE}
  LABELS fixtures portable TIMEOUT 10)
# Fault prevented: a probe check that passes whatever board_source the runner prints. The same
# case expects database, which the scan never reports for these images, and must fail naming it.
string(REPLACE ";" "\;" fixtures_probe_emulator "${CMAKE_CROSSCOMPILING_EMULATOR}")
set(cmd ${CMAKE_COMMAND} ${fixtures_probe} "-DEMULATOR=${fixtures_probe_emulator}" -DSOURCE=database
  -P ${CMAKE_CURRENT_LIST_FILE})
string(REPLACE ";" "$<SEMICOLON>" cmd "${cmd}")
# Timeout: 10 s floor; 0.02 s slowest local run, no hosted run yet, so provisional.
cogmaster_add_test(NAME fixtures.korean_probe_control
  COMMAND ${CMAKE_COMMAND} "-DCMD=${cmd}" -DCODE=1 "-DREGEX=korean_probe: board_source"
    -P ${PROJECT_SOURCE_DIR}/tests/cmake/expect_fail.cmake
  LABELS fixtures portable TIMEOUT 10)
set_tests_properties(fixtures.korean_probe_control PROPERTIES DEPENDS fixtures.korean_probe)
# Fault prevented: unjudged guest counter/IRQ/status measurements.
foreach(tv IN ITEMS ntsc pal)
  set(region ntsc-u)
  if(tv STREQUAL "pal")
    set(region pal)
  endif()
  foreach(height IN ITEMS 192 224 240)
    cogmaster_add_test(NAME fixtures.vdp_lines.mailbox.${tv}.${height}
      COMMAND ${CMAKE_COMMAND} ${fixtures_mailbox} -DNAME=vdp_lines_${height}
        -DREGION=${region} -DPROFILE=${tv}
        -DIMAGE=${COGMASTER_FIXTURES_DIR}/vdp_lines_${height}.sms -DFRAMES=30
        -DCSV=${PROJECT_SOURCE_DIR}/tests/diag/expected.csv
        -DOUT=${fixtures_out}/vdp_lines.${tv}.${height}.txt -P ${CMAKE_CURRENT_LIST_FILE}
      LABELS fixtures portable TIMEOUT 10)
  endforeach()
endforeach()

# Fault prevented: a legacy sprite engine that draws a wrong dot in any of the 20 scenarios, on either of
# the two frames each is on screen, on any of the three profiles, or a runner hash of the last scenario's
# picture (frame 47, then held) that differs from the oracle's digest. One run steps through the whole list
# and compares every dot through the public API; the runner hash is a second, independent judgement of the
# same picture. The status records are judged by the mailbox tests below.
# Timeout: 10 s floor; provisional, the first hosted run sets it (48 frames, about 2 million dots compared).
foreach(legacy_profile IN ITEMS sms1 sms2 gg_sms)
  cogmaster_add_test(NAME fixtures.legacy_sprites.${legacy_profile}
    COMMAND ${CMAKE_COMMAND} -DCASE=legacy -DNAME=legacy_sprites -DVARIANT=sprites
      -DPROFILE=${legacy_profile} -DCOMPARE=all -DSCENARIO=20 -DFRAMES=47 -DRUNNER=1
      -DIMAGE=${COGMASTER_FIXTURES_DIR}/legacy_sprites.sms
      -DEXPECTED=${PROJECT_SOURCE_DIR}/tests/built/expected.txt
      -DORACLE=$<TARGET_FILE:cogmaster-test-legacy-fixture> -DRUN=$<TARGET_FILE:cogmaster-run>
      "-DEMULATOR=${fixtures_emulator}" -P ${CMAKE_CURRENT_LIST_FILE}
    LABELS fixtures portable TIMEOUT 10)
endforeach()
# Fault prevented: sprite status flags that are measured by the guest but never judged: the fifth sprite
# flag with its index, the coincidence flag, the end-of-list Y value, the early clock, both sizes and
# magnification, and the tail read of a line, where the fifth index must survive a status read between the
# scan of the line and its first dot. Every record is judged against its row in tests/diag/expected.csv,
# which legacy_status computes, on the Master System 1, the Master System 2 and the Game Gear in Master
# System video mode.
# Timeout: 10 s floor; provisional, the first hosted run sets it (the image reports after 49 frames).
foreach(spec IN ITEMS sms1:sms1 sms2:sms gg_sms:gg)
  string(REPLACE ":" ";" spec "${spec}")
  list(GET spec 0 legacy_profile)
  list(GET spec 1 legacy_machine)
  cogmaster_add_test(NAME fixtures.legacy_sprites.mailbox.${legacy_profile}
    COMMAND ${CMAKE_COMMAND} ${fixtures_mailbox} -DNAME=legacy_sprites -DMACHINE=${legacy_machine}
      -DVIDEO=sms -DREGION=ntsc-u -DPROFILE=${legacy_profile}
      -DIMAGE=${COGMASTER_FIXTURES_DIR}/legacy_sprites.sms -DFRAMES=200
      -DCSV=${PROJECT_SOURCE_DIR}/tests/diag/expected.csv
      -DOUT=${fixtures_out}/legacy_sprites.${legacy_profile}.txt -P ${CMAKE_CURRENT_LIST_FILE}
    LABELS fixtures portable TIMEOUT 10)
endforeach()

# Fault prevented: V counter records that are measured but never judged, an expectation table with a
# transcription error that passes because the same error is in the fixture, and a PAL image that
# passes on an NTSC run. The first model has no 224 or 240 height, so its two runs use the 224 image
# and the rows of 192 lines.
# Timeout: 20 s floor; 2 x 1.50 s (macos-x64 lane for closed_form), slowest of 4 runs, run 43111533403.
foreach(tv IN ITEMS ntsc pal)
  set(region ntsc-u)
  set(heights 192 224)
  if(tv STREQUAL "pal")
    set(region pal)
    set(heights 192 224 240)
  endif()
  foreach(height IN LISTS heights)
    cogmaster_add_test(NAME fixtures.vdp_vtable.mailbox.${tv}.${height}
      COMMAND ${CMAKE_COMMAND} ${fixtures_mailbox} -DNAME=vdp_vtable_${height}
        -DREGION=${region} -DPROFILE=${tv}
        -DIMAGE=${COGMASTER_FIXTURES_DIR}/vdp_vtable_${height}.sms -DFRAMES=30
        -DCSV=${PROJECT_SOURCE_DIR}/tests/diag/expected.csv
        -DOUT=${fixtures_out}/vdp_vtable.${tv}.${height}.txt -P ${CMAKE_CURRENT_LIST_FILE}
      LABELS fixtures portable TIMEOUT 20)
  endforeach()
  cogmaster_add_test(NAME fixtures.vdp_vtable.sms1.${tv}
    COMMAND ${CMAKE_COMMAND} ${fixtures_mailbox} -DNAME=vdp_vtable_224 -DMACHINE=sms1
      -DREGION=${region} -DPROFILE=${tv}-sms1
      -DIMAGE=${COGMASTER_FIXTURES_DIR}/vdp_vtable_224.sms -DFRAMES=30
      -DCSV=${PROJECT_SOURCE_DIR}/tests/diag/expected.csv
      -DOUT=${fixtures_out}/vdp_vtable.sms1.${tv}.txt -P ${CMAKE_CURRENT_LIST_FILE}
    LABELS fixtures portable TIMEOUT 20)
endforeach()
cogmaster_add_test(NAME fixtures.vdp_vtable.closed_form
  COMMAND ${CMAKE_COMMAND} -DCASE=vtable_closed_form -DCSV=${PROJECT_SOURCE_DIR}/tests/diag/expected.csv
    -P ${CMAKE_CURRENT_LIST_FILE}
  LABELS fixtures portable TIMEOUT 20)

# Fault prevented: a Pause NMI on the wrong line or tick that no in-process test sees from a program,
# and a band fitted to a run. Each image counts n between the frame interrupt and the NMI, reads V
# in the handler and counts NMIs for a script that holds Pause for several frames and presses it
# again later; the n band comes from the listing's cycle counts (closed_form), not from a run. The
# Game Gear in its own mode has no Pause NMI, so its four records are 0. PAL placement is derived,
# not measured: no logic-analyser capture of a PAL console exists.
# Timeout: 20 s floor; 2 x 0.69 s (be32 lane for pal.240), slowest of 4 runs, run 43115877197.
foreach(tv IN ITEMS ntsc pal)
  set(region ntsc-u)
  set(heights 192 224)
  if(tv STREQUAL "pal")
    set(region pal)
    set(heights 192 224 240)
  endif()
  foreach(height IN LISTS heights)
    cogmaster_add_test(NAME fixtures.pause_nmi.mailbox.${tv}.${height}
      COMMAND ${CMAKE_COMMAND} ${fixtures_mailbox} -DNAME=pause_nmi_${height}
        -DREGION=${region} -DPROFILE=${tv} -DINPUT=${pause_input}
        -DIMAGE=${COGMASTER_FIXTURES_DIR}/pause_nmi_${height}.sms -DFRAMES=40
        -DCSV=${PROJECT_SOURCE_DIR}/tests/diag/expected.csv
        -DOUT=${fixtures_out}/pause_nmi.${tv}.${height}.txt -P ${CMAKE_CURRENT_LIST_FILE}
      LABELS fixtures portable TIMEOUT 20)
  endforeach()
endforeach()
cogmaster_add_test(NAME fixtures.pause_nmi.gg
  COMMAND ${CMAKE_COMMAND} ${fixtures_mailbox} -DNAME=pause_nmi_192 -DMACHINE=gg -DVIDEO=gg
    -DPROFILE=gg -DINPUT=${pause_input}
    -DIMAGE=${COGMASTER_FIXTURES_DIR}/pause_nmi_192.sms -DFRAMES=40
    -DCSV=${PROJECT_SOURCE_DIR}/tests/diag/expected.csv
    -DOUT=${fixtures_out}/pause_nmi.gg.txt -P ${CMAKE_CURRENT_LIST_FILE}
  LABELS fixtures portable TIMEOUT 20)
cogmaster_add_test(NAME fixtures.pause_nmi.closed_form
  COMMAND ${CMAKE_COMMAND} -DCASE=pause_closed_form -DCSV=${PROJECT_SOURCE_DIR}/tests/diag/expected.csv
    -P ${CMAKE_CURRENT_LIST_FILE}
  LABELS fixtures portable TIMEOUT 20)

# The ports fixture once per profile: the I/O chip and the VDP answer by
# machine, region and TV standard. The Game Gear runs ports_gg, the same
# source under a Game Gear header, judged against the ports rows.
foreach(profile IN ITEMS sms-export sms-japan sms-pal gg)
  set(name ports)
  if(profile STREQUAL "sms-export")
    set(args -DMACHINE=sms -DREGION=ntsc-u)
  elseif(profile STREQUAL "sms-japan")
    set(args -DMACHINE=sms -DREGION=ntsc-j)
  elseif(profile STREQUAL "sms-pal")
    set(args -DMACHINE=sms -DREGION=pal)
  else()
    set(name ports_gg)
    set(args -DMACHINE=gg -DROM=ports)
  endif()
  # Timeout: 10 s floor; 2 x 0.01 s, slowest of 2 runs, run 39376644025, provisional.
  cogmaster_add_test(NAME fixtures.ports.${profile}
    COMMAND ${CMAKE_COMMAND} ${fixtures_mailbox} -DNAME=${name} ${args} -DPROFILE=${profile}
      -DIMAGE=${COGMASTER_FIXTURES_DIR}/${name}.sms -DFRAMES=120
      -DCSV=${PROJECT_SOURCE_DIR}/tests/diag/expected.csv -DOUT=${fixtures_out}/ports.${profile}.txt
      -P ${CMAKE_CURRENT_LIST_FILE}
    LABELS fixtures portable TIMEOUT 10)
endforeach()

# The sports_pad fixture polls once a frame. This list is its oracle: one entry per frame with the
# motion of port 1 (x, y) and the held pad byte. The scripts are written from it, and the values the
# rows of tests/diag/expected.csv must hold are computed from it below, so the two cannot drift. The
# pad sends the negated motion, at most 127 counts a read, and keeps the rest for the next read
# (SMS-CARTRIDGES section 3.5). Frames 11 and 12 are the idle-timeout probes and 13 to 15 hold buttons.
set(sports_frames 5,-3,00 0,0,00 127,-128,00 300,-300,00 0,0,00 0,0,00 -200,50,00 0,0,00 1,1,00 -1,-1,00
  5,-100,00 5,-100,00 0,0,10 0,0,20 0,0,30 9,-4,00 9,-4,00 7,3,00)
list(LENGTH sports_frames sports_count)

function(sports_axis out value)
  set(sign +)
  if(value LESS 0)
    set(sign -)
    math(EXPR value "-(${value})")
  endif()
  string(LENGTH "${value}" digits)
  math(EXPR pad "5 - ${digits}")
  string(REPEAT 0 ${pad} zeros)
  set(${out} "${sign}${zeros}${value}" PARENT_SCOPE)
endfunction()

# Writes the runner script for the list FRAMES names: MOTION 0 gives the same buttons with no motion, DROP the
# 1-based frame whose x motion is left out.
function(device_script path motion drop frames)
  set(body "")
  set(frame 0)
  foreach(entry IN LISTS ${frames})
    math(EXPR frame "${frame} + 1")
    string(REPLACE "," ";" entry "${entry}")
    list(GET entry 0 x)
    list(GET entry 1 y)
    list(GET entry 2 held)
    if(NOT motion)
      set(x 0)
      set(y 0)
    elseif(frame EQUAL drop)
      set(x 0)
    endif()
    sports_axis(xs ${x})
    sports_axis(ys ${y})
    string(APPEND body "p1=${held} p2=00 pause=0 reset=0 start=0 x1=${xs} y1=${ys} x2=+00000 y2=+00000\n")
  endforeach()
  file(WRITE ${path} "${body}")
endfunction()
device_script(${PROJECT_BINARY_DIR}/tests/sports.input 1 0 sports_frames)
device_script(${PROJECT_BINARY_DIR}/tests/sports-dropped.input 1 1 sports_frames)
device_script(${PROJECT_BINARY_DIR}/tests/sports-still.input 0 0 sports_frames)

# One read of the pad: the byte of an axis is the negated motion clamped to 127, the rest stays.
macro(sports_latch axis)
  set(step ${sports_acc_${axis}})
  if(step GREATER 127)
    set(step 127)
  elseif(step LESS -127)
    set(step -127)
  endif()
  math(EXPR sports_acc_${axis} "${sports_acc_${axis}} - (${step})")
  math(EXPR sports_byte_${axis} "(-(${step})) & 255")
endmacro()

set(sports_acc_x 0)
set(sports_acc_y 0)
set(sports_want "")
set(frame 0)
foreach(entry IN LISTS sports_frames)
  math(EXPR frame "${frame} + 1")
  string(REPLACE "," ";" entry "${entry}")
  list(GET entry 0 x)
  list(GET entry 1 y)
  list(GET entry 2 held)
  math(EXPR sports_acc_x "${sports_acc_x} + ${x}")
  math(EXPR sports_acc_y "${sports_acc_y} + ${y}")
  sports_latch(x)
  sports_latch(y)
  if(frame LESS 11)
    math(EXPR value "(${sports_byte_y} << 8) | ${sports_byte_x}")
  elseif(frame EQUAL 11)
    # The second fall after 511 cycles goes on with Y: the high nibble of Y's byte.
    math(EXPR value "(${sports_byte_x} >> 4) | ((${sports_byte_y} >> 4) << 8)")
  elseif(frame EQUAL 12)
    # After 512 cycles it starts again, with the motion that is left.
    math(EXPR first "${sports_byte_x} >> 4")
    sports_latch(x)
    math(EXPR value "${first} | ((${sports_byte_x} >> 4) << 8)")
  elseif(frame LESS 16)
    math(EXPR value "((0x${held} >> 4) & 3) * 257")
  else()
    continue()
  endif()
  list(APPEND sports_want ${value})
endforeach()
set_property(DIRECTORY ${PROJECT_SOURCE_DIR} APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
  ${PROJECT_SOURCE_DIR}/tests/diag/expected.csv)
file(STRINGS ${PROJECT_SOURCE_DIR}/tests/diag/expected.csv sports_rows REGEX "^sports_pad,")
list(LENGTH sports_rows sports_row_count)
if(NOT sports_row_count EQUAL 30)
  message(FATAL_ERROR "tests/diag/expected.csv: ${sports_row_count} sports_pad rows, expected 30")
endif()
foreach(row IN LISTS sports_rows)
  string(REPLACE "," ";" row "${row}")
  list(GET row 1 test)
  list(GET row 3 literal)
  string(SUBSTRING "${test}" 0 2 id)
  math(EXPR index "0x${id} - 1")
  math(EXPR literal "0x${literal}")
  list(GET sports_want ${index} want)
  if(NOT literal EQUAL want)
    message(FATAL_ERROR "tests/diag/expected.csv: sports_pad row ${test} is ${literal}, the delta list gives ${want}")
  endif()
endforeach()

# Fault prevented: a Sports Pad read sequence that drops, doubles or flips a delta, or a profile on which the
# sequence differs, so a game tracks the wrong way or desyncs. The same rows are judged on the export and the
# Japanese console; a script missing one delta and a plain pad on the port must each fall outside a row.
# Timeout: 10 s floor; 2 x 0.27 s, slowest of 1 run, run 42973268415.
foreach(profile IN ITEMS export japan)
  set(region ntsc-u)
  if(profile STREQUAL "japan")
    set(region ntsc-j)
  endif()
  cogmaster_add_test(NAME fixtures.sports_pad.mailbox.${profile}
    COMMAND ${CMAKE_COMMAND} ${fixtures_mailbox} -DNAME=sports_pad -DMACHINE=sms -DREGION=${region}
      -DPROFILE=sms-${profile} -DPORT1=sports -DINPUT=${PROJECT_BINARY_DIR}/tests/sports.input
      -DIMAGE=${COGMASTER_FIXTURES_DIR}/sports_pad.sms -DFRAMES=${sports_count}
      -DCSV=${PROJECT_SOURCE_DIR}/tests/diag/expected.csv
      -DOUT=${fixtures_out}/sports_pad.${profile}.txt -P ${CMAKE_CURRENT_LIST_FILE}
    LABELS fixtures portable TIMEOUT 10)
endforeach()
foreach(control IN ITEMS dropped_delta pad)
  set(input sports-dropped)
  set(device sports)
  if(control STREQUAL "pad")
    set(input sports-still)
    set(device pad)
  endif()
  set(cmd ${CMAKE_COMMAND} -DCASE=mailbox -DEXPECTED=${PROJECT_SOURCE_DIR}/tests/built/expected.txt
    -DRUN=$<TARGET_FILE:cogmaster-run> "-DEMULATOR=${fixtures_inner_emulator}"
    -DJUDGE=${PROJECT_SOURCE_DIR}/tests/cmake/mailbox.cmake -DPROFILE=sms-export -DNAME=sports_pad
    -DMACHINE=sms -DREGION=ntsc-u -DPORT1=${device} -DINPUT=${PROJECT_BINARY_DIR}/tests/${input}.input
    -DIMAGE=${COGMASTER_FIXTURES_DIR}/sports_pad.sms -DFRAMES=${sports_count}
    -DCSV=${PROJECT_SOURCE_DIR}/tests/diag/expected.csv -DOUT=${fixtures_out}/sports_pad.${control}.txt
    -P ${CMAKE_CURRENT_LIST_FILE})
  string(REPLACE ";" "$<SEMICOLON>" cmd "${cmd}")
  cogmaster_add_test(NAME fixtures.sports_pad.control.${control}
    COMMAND ${CMAKE_COMMAND} "-DCMD=${cmd}" -DCODE=1 "-DREGEX=outside its row"
      -P ${PROJECT_SOURCE_DIR}/tests/cmake/expect_fail.cmake
    LABELS fixtures portable TIMEOUT 10)
endforeach()

# Fault prevented: a table default that never reaches a host, or a Paddle dump that loads as export and plays as a pad.
# The Sports Pad image, run with no --port1, --machine or --region, takes its controller from the table and is judged
# against the same rows as the explicit export case.
# Timeout: 10 s floor; 2 x 0.24 s, slowest of 1 run, run 42978311844.
cogmaster_add_test(NAME fixtures.sports_pad.default
  COMMAND ${CMAKE_COMMAND} ${fixtures_mailbox} -DNAME=sports_pad -DPROFILE=sms-export
    -DINPUT=${PROJECT_BINARY_DIR}/tests/sports.input
    -DIMAGE=${COGMASTER_FIXTURES_DIR}/sports_pad.sms -DFRAMES=${sports_count}
    -DCSV=${PROJECT_SOURCE_DIR}/tests/diag/expected.csv
    -DOUT=${fixtures_out}/sports_pad.default.txt -P ${CMAKE_CURRENT_LIST_FILE}
  LABELS fixtures portable TIMEOUT 10)

# Fault prevented: a recorded delta script that replays to a different frame, so a replay no longer matches the
# run it recorded. The library records the pad's motion on every TV standard, the runner replays the written
# script to the same hash, and the same script with one count changed in its last record must not.
# Timeout: 10 s floor; 2 x 3.67 s, slowest of 1 run, run 42973268415.
set(sports_replay -DCASE=replay -DNAME=sports_pad -DIMAGE=${COGMASTER_FIXTURES_DIR}/sports_pad.sms
  -DEXPECTED=${PROJECT_SOURCE_DIR}/tests/built/expected.txt -DRECORDER=$<TARGET_FILE:cogmaster-test-ports-devices>
  -DRUN=$<TARGET_FILE:cogmaster-run> -DDEVICE=sports -DREGIONS=ntsc-u,ntsc-j,pal -DFRAMES=40)
cogmaster_add_test(NAME fixtures.sports_pad.replay
  COMMAND ${CMAKE_COMMAND} ${sports_replay} "-DEMULATOR=${fixtures_emulator}"
    -DSCRIPT=${fixtures_out}/sports_pad.replay.input -P ${CMAKE_CURRENT_LIST_FILE}
  LABELS fixtures portable TIMEOUT 10)
set(cmd ${CMAKE_COMMAND} ${sports_replay} "-DEMULATOR=${fixtures_inner_emulator}" -DEDIT=1
  -DSCRIPT=${fixtures_out}/sports_pad.replay_control.input -P ${CMAKE_CURRENT_LIST_FILE})
string(REPLACE ";" "$<SEMICOLON>" cmd "${cmd}")
cogmaster_add_test(NAME fixtures.sports_pad.replay_control
  COMMAND ${CMAKE_COMMAND} "-DCMD=${cmd}" -DCODE=1 "-DREGEX=differs[^a-zA-Z]+from[^a-zA-Z]+the[^a-zA-Z]+recording"
    -P ${PROJECT_SOURCE_DIR}/tests/cmake/expect_fail.cmake
  LABELS fixtures portable TIMEOUT 10)

# The paddle fixture reads the position once a frame for the first 14 frames and then runs its sweep, which
# takes a few more, so the script's tail is motionless. This list is its oracle: x, y and the held pad byte of
# port 1 a frame (a Paddle ignores y, and frames 2 and 8 send one; bit 4 of the byte is its button). The scripts
# are written from it and the position of each frame is computed from it below: the position starts at 128 and
# is clamped to 0 to 255 after every frame's motion, the way a pot stops at its ends (SMS-CARTRIDGES section 3.6).
set(paddle_deltas 5,0,00 -3,9,00 100,0,00 100,0,10 -2,0,00 0,0,00 -300,0,00 4,-6,00 -9,0,00 1,0,00 120,0,10
  0,0,00 -121,0,00 127,0,00)
set(paddle_frames ${paddle_deltas} 0,0,00 0,0,00 0,0,00 0,0,00 0,0,00 0,0,00 0,0,00 0,0,00 0,0,00 0,0,00 0,0,00
  0,0,00)
list(LENGTH paddle_frames paddle_count)
device_script(${PROJECT_BINARY_DIR}/tests/paddle.input 1 0 paddle_frames)
device_script(${PROJECT_BINARY_DIR}/tests/paddle-dropped.input 1 1 paddle_frames)
device_script(${PROJECT_BINARY_DIR}/tests/paddle-still.input 0 0 paddle_frames)

set(paddle_at 128)
set(paddle_want "")
foreach(entry IN LISTS paddle_deltas)
  string(REPLACE "," ";" entry "${entry}")
  list(GET entry 0 x)
  list(GET entry 2 held)
  math(EXPR paddle_at "${paddle_at} + ${x}")
  if(paddle_at LESS 0)
    set(paddle_at 0)
  elseif(paddle_at GREATER 255)
    set(paddle_at 255)
  endif()
  math(EXPR value "${paddle_at} + (((0x${held} >> 4) & 1) << 8)")
  list(APPEND paddle_want ${value})
endforeach()
file(STRINGS ${PROJECT_SOURCE_DIR}/tests/diag/expected.csv paddle_rows REGEX "^paddle,")
list(LENGTH paddle_rows paddle_row_count)
if(NOT paddle_row_count EQUAL 26)
  message(FATAL_ERROR "tests/diag/expected.csv: ${paddle_row_count} paddle rows, expected 26")
endif()
foreach(row IN LISTS paddle_rows)
  string(REPLACE "," ";" row "${row}")
  list(GET row 1 test)
  list(GET row 3 literal)
  string(SUBSTRING "${test}" 0 2 id)
  math(EXPR index "0x${id} - 0x0C")
  if(index LESS 0)
    continue()
  endif()
  math(EXPR literal "0x${literal}")
  list(GET paddle_want ${index} want)
  if(NOT literal EQUAL want)
    message(FATAL_ERROR "tests/diag/expected.csv: paddle row ${test} is ${literal}, the delta list gives ${want}")
  endif()
endforeach()

# Fault prevented: a Paddle TR clock that locks to a game's polling loop (a frozen reading of 0 or 100 percent) or a
# position that drifts from the scripted deltas, so a Japanese paddle title cannot be played. The Japanese console
# judges every record; the export console, given a plain pad, judges the region test alone. A frozen TR, a plain pad
# and a script missing one delta must each fall outside a row.
# Timeout: 10 s floor; 2 x 0.33 s, slowest of 1 run, run 42973268415.
foreach(profile IN ITEMS japan export)
  set(region ntsc-j)
  set(device paddle)
  set(input paddle)
  if(profile STREQUAL "export")
    set(region ntsc-u)
    set(device pad)
    set(input paddle-still)
  endif()
  cogmaster_add_test(NAME fixtures.paddle.mailbox.${profile}
    COMMAND ${CMAKE_COMMAND} ${fixtures_mailbox} -DNAME=paddle -DMACHINE=sms -DREGION=${region}
      -DPROFILE=sms-${profile} -DPORT1=${device} -DINPUT=${PROJECT_BINARY_DIR}/tests/${input}.input
      -DIMAGE=${COGMASTER_FIXTURES_DIR}/paddle.sms -DFRAMES=${paddle_count}
      -DCSV=${PROJECT_SOURCE_DIR}/tests/diag/expected.csv
      -DOUT=${fixtures_out}/paddle.${profile}.txt -P ${CMAKE_CURRENT_LIST_FILE}
    LABELS fixtures portable TIMEOUT 10)
endforeach()
foreach(control IN ITEMS frozen pad dropped_delta)
  set(device paddle)
  set(input paddle-dropped)
  if(NOT control STREQUAL "dropped_delta")
    set(device none)
    set(input paddle-still)
    if(control STREQUAL "pad")
      set(device pad)
    endif()
  endif()
  set(cmd ${CMAKE_COMMAND} -DCASE=mailbox -DEXPECTED=${PROJECT_SOURCE_DIR}/tests/built/expected.txt
    -DRUN=$<TARGET_FILE:cogmaster-run> "-DEMULATOR=${fixtures_inner_emulator}"
    -DJUDGE=${PROJECT_SOURCE_DIR}/tests/cmake/mailbox.cmake -DPROFILE=sms-japan -DNAME=paddle
    -DMACHINE=sms -DREGION=ntsc-j -DPORT1=${device} -DINPUT=${PROJECT_BINARY_DIR}/tests/${input}.input
    -DIMAGE=${COGMASTER_FIXTURES_DIR}/paddle.sms -DFRAMES=${paddle_count}
    -DCSV=${PROJECT_SOURCE_DIR}/tests/diag/expected.csv -DOUT=${fixtures_out}/paddle.${control}.txt
    -P ${CMAKE_CURRENT_LIST_FILE})
  string(REPLACE ";" "$<SEMICOLON>" cmd "${cmd}")
  cogmaster_add_test(NAME fixtures.paddle.control.${control}
    COMMAND ${CMAKE_COMMAND} "-DCMD=${cmd}" -DCODE=1 "-DREGEX=outside its row"
      -P ${PROJECT_SOURCE_DIR}/tests/cmake/expect_fail.cmake
    LABELS fixtures portable TIMEOUT 10)
endforeach()

# The same fault for the Paddle image, whose table row also gives its region: run with no --port1 and no --region it
# is a Japanese-console dump with a Paddle and is judged on the Japanese rows. The control names an export region, so
# the default degrades to a pad and the detect rows must fail.
# Timeout: 10 s floor; 2 x 0.32 s, slowest of 1 run, run 42978311844.
cogmaster_add_test(NAME fixtures.paddle.default
  COMMAND ${CMAKE_COMMAND} ${fixtures_mailbox} -DNAME=paddle -DPROFILE=sms-japan
    -DINPUT=${PROJECT_BINARY_DIR}/tests/paddle.input
    -DIMAGE=${COGMASTER_FIXTURES_DIR}/paddle.sms -DFRAMES=${paddle_count}
    -DCSV=${PROJECT_SOURCE_DIR}/tests/diag/expected.csv
    -DOUT=${fixtures_out}/paddle.default.txt -P ${CMAKE_CURRENT_LIST_FILE}
  LABELS fixtures portable TIMEOUT 10)
set(cmd ${CMAKE_COMMAND} -DCASE=mailbox -DEXPECTED=${PROJECT_SOURCE_DIR}/tests/built/expected.txt
  -DRUN=$<TARGET_FILE:cogmaster-run> "-DEMULATOR=${fixtures_inner_emulator}"
  -DJUDGE=${PROJECT_SOURCE_DIR}/tests/cmake/mailbox.cmake -DPROFILE=sms-japan -DNAME=paddle
  -DREGION=ntsc-u -DINPUT=${PROJECT_BINARY_DIR}/tests/paddle-still.input
  -DIMAGE=${COGMASTER_FIXTURES_DIR}/paddle.sms -DFRAMES=${paddle_count}
  -DCSV=${PROJECT_SOURCE_DIR}/tests/diag/expected.csv -DOUT=${fixtures_out}/paddle.degrade.txt
  -P ${CMAKE_CURRENT_LIST_FILE})
string(REPLACE ";" "$<SEMICOLON>" cmd "${cmd}")
cogmaster_add_test(NAME fixtures.paddle.control.degrade
  COMMAND ${CMAKE_COMMAND} "-DCMD=${cmd}" -DCODE=1 "-DREGEX=outside its row"
    -P ${PROJECT_SOURCE_DIR}/tests/cmake/expect_fail.cmake
  LABELS fixtures portable TIMEOUT 10)

# Fault prevented: a Paddle position accumulated in state that a replay of the recorded deltas does not
# reproduce. The same replay case, on the Japanese console the Paddle needs: the recorder drives the paddle image,
# the runner replays the script it wrote to the same hash, and one count changed in the last record must not.
# Timeout: 10 s floor; 2 x 1.31 s, slowest of 1 run, run 42973268415.
set(paddle_replay -DCASE=replay -DNAME=paddle -DIMAGE=${COGMASTER_FIXTURES_DIR}/paddle.sms
  -DEXPECTED=${PROJECT_SOURCE_DIR}/tests/built/expected.txt -DRECORDER=$<TARGET_FILE:cogmaster-test-ports-devices>
  -DRUN=$<TARGET_FILE:cogmaster-run> -DDEVICE=paddle -DREGIONS=ntsc-j -DFRAMES=40)
cogmaster_add_test(NAME fixtures.paddle.replay
  COMMAND ${CMAKE_COMMAND} ${paddle_replay} "-DEMULATOR=${fixtures_emulator}"
    -DSCRIPT=${fixtures_out}/paddle.replay.input -P ${CMAKE_CURRENT_LIST_FILE}
  LABELS fixtures portable TIMEOUT 10)
set(cmd ${CMAKE_COMMAND} ${paddle_replay} "-DEMULATOR=${fixtures_inner_emulator}" -DEDIT=1
  -DSCRIPT=${fixtures_out}/paddle.replay_control.input -P ${CMAKE_CURRENT_LIST_FILE})
string(REPLACE ";" "$<SEMICOLON>" cmd "${cmd}")
cogmaster_add_test(NAME fixtures.paddle.replay_control
  COMMAND ${CMAKE_COMMAND} "-DCMD=${cmd}" -DCODE=1 "-DREGEX=differs[^a-zA-Z]+from[^a-zA-Z]+the[^a-zA-Z]+recording"
    -P ${PROJECT_SOURCE_DIR}/tests/cmake/expect_fail.cmake
  LABELS fixtures portable TIMEOUT 10)

# The glasses fixture writes the shutter register from the table in tests/roms/src/glasses.asm: one entry a
# frame, 0 or 1 the byte written and 255 (the $FF in the source) no write. The table is the oracle; the counts
# the runner must print are computed from it here, so neither can drift. A write lands in its own frame, the
# eye in force carries into a frame with no write, and a frame before the first write reports none (SMS-
# CARTRIDGES-AND-PERIPHERALS section 3.4); flips counts the frames whose eye differs from the last one shown.
# The run is the table's 24 frames plus the six the mailbox work takes.
set(glasses_table 255 255 0 1 0 1 0 1 0 1 0 255 255 255 255 255 0 1 0 1 0 1 0 1)
set(glasses_count 30)
file(STRINGS ${fixtures_src}/glasses.asm glasses_rows REGEX "^[ \t]+[.]DB[ \t]")
list(LENGTH glasses_rows glasses_row_count)
if(NOT glasses_row_count EQUAL 1)
  message(FATAL_ERROR "tests/roms/src/glasses.asm: ${glasses_row_count} .DB rows, expected the one table")
endif()
string(REGEX REPLACE "^[ \t]+[.]DB[ \t]+" "" glasses_row "${glasses_rows}")
string(REGEX REPLACE "[ \t]" "" glasses_row "${glasses_row}")
string(REPLACE "$" "0x" glasses_row "${glasses_row}")
string(REPLACE "," ";" glasses_row "${glasses_row}")
set(glasses_source "")
foreach(entry IN LISTS glasses_row)
  math(EXPR entry "${entry}")
  list(APPEND glasses_source ${entry})
endforeach()
if(NOT glasses_source STREQUAL glasses_table)
  message(FATAL_ERROR "tests/roms/src/glasses.asm: the table is ${glasses_source}, tests.cmake holds ${glasses_table}")
endif()
set(glasses_seen none)
set(glasses_last none)
set(glasses_want_none 0)
set(glasses_want_left 0)
set(glasses_want_right 0)
set(glasses_want_flips 0)
foreach(frame RANGE 1 ${glasses_count})
  set(entry 255)
  list(LENGTH glasses_table glasses_length)
  if(frame LESS_EQUAL glasses_length)
    math(EXPR index "${frame} - 1")
    list(GET glasses_table ${index} entry)
  endif()
  if(NOT entry EQUAL 255)
    set(glasses_seen right)
    math(EXPR left "${entry} & 1")
    if(left)
      set(glasses_seen left)
    endif()
  endif()
  math(EXPR glasses_want_${glasses_seen} "${glasses_want_${glasses_seen}} + 1")
  if(NOT glasses_seen STREQUAL "none")
    if(NOT glasses_last STREQUAL "none" AND NOT glasses_last STREQUAL glasses_seen)
      math(EXPR glasses_want_flips "${glasses_want_flips} + 1")
    endif()
    set(glasses_last ${glasses_seen})
  endif()
endforeach()
# The cross-lane frame hash of the image runs as many frames as this neutral script has lines.
set(glasses_frames "")
foreach(frame RANGE 1 ${glasses_count})
  list(APPEND glasses_frames 0,0,00)
endforeach()
device_script(${PROJECT_BINARY_DIR}/tests/glasses.input 0 0 glasses_frames)
# The cross-lane frame hash of the BIOS run is its last frame's: two frames end inside the owned image's BIOS phase,
# where the picture is not the blank frame a fast boot shows (from the third frame the probe cartridge has the
# machine and both boots hash alike, so a longer run would compare nothing the BIOS phase did).
set(bios_frames "")
foreach(frame RANGE 1 2)
  list(APPEND bios_frames 0,0,00)
endforeach()
device_script(${PROJECT_BINARY_DIR}/tests/bios.input 0 0 bios_frames)
set(fixtures_eyes -DCASE=eyes -DEXPECTED=${PROJECT_SOURCE_DIR}/tests/built/expected.txt
  -DRUN=$<TARGET_FILE:cogmaster-run> "-DEMULATOR=${fixtures_emulator}"
  -DJUDGE=${PROJECT_SOURCE_DIR}/tests/cmake/mailbox.cmake -DNAME=glasses -DFRAMES=${glasses_count}
  -DIMAGE=${COGMASTER_FIXTURES_DIR}/glasses.sms -DCSV=${PROJECT_SOURCE_DIR}/tests/diag/expected.csv)

# Fault prevented: a 3-D eye reported in the wrong frame, lost when the cartridge is disabled, invented on a
# Game Gear, or a frame hash that changes with the eye report. The export and Japanese consoles must print the
# table's counts (the cartridge-off frames and the one-write frames included), a Game Gear none for every frame,
# and a count off by one must fail.
# Timeout: 10 s floor; 2 x 0.73 s, slowest of 1 run, run 42973268415.
foreach(profile IN ITEMS export japan gg)
  set(args -DMACHINE=sms -DREGION=ntsc-u -DWANT_none=${glasses_want_none} -DWANT_left=${glasses_want_left}
    -DWANT_right=${glasses_want_right} -DWANT_flips=${glasses_want_flips} -DWANT_last=${glasses_last})
  if(profile STREQUAL "japan")
    set(args -DMACHINE=sms -DREGION=ntsc-j -DWANT_none=${glasses_want_none} -DWANT_left=${glasses_want_left}
      -DWANT_right=${glasses_want_right} -DWANT_flips=${glasses_want_flips} -DWANT_last=${glasses_last})
  elseif(profile STREQUAL "gg")
    set(args -DMACHINE=gg -DWANT_none=${glasses_count} -DWANT_left=0 -DWANT_right=0 -DWANT_flips=0 -DWANT_last=none)
  endif()
  cogmaster_add_test(NAME fixtures.glasses.eyes.${profile}
    COMMAND ${CMAKE_COMMAND} ${fixtures_eyes} ${args} -DPROFILE=${profile}
      -DOUT=${fixtures_out}/glasses.${profile}.txt -P ${CMAKE_CURRENT_LIST_FILE}
    LABELS fixtures portable TIMEOUT 10)
endforeach()
math(EXPR glasses_off "${glasses_want_left} + 1")
set(cmd ${CMAKE_COMMAND} ${fixtures_eyes} "-DEMULATOR=${fixtures_inner_emulator}" -DMACHINE=sms -DREGION=ntsc-u
  -DWANT_none=${glasses_want_none} -DWANT_left=${glasses_off} -DWANT_right=${glasses_want_right}
  -DWANT_flips=${glasses_want_flips} -DWANT_last=${glasses_last} -DPROFILE=export
  -DOUT=${fixtures_out}/glasses.control.txt -P ${CMAKE_CURRENT_LIST_FILE})
string(REPLACE ";" "$<SEMICOLON>" cmd "${cmd}")
cogmaster_add_test(NAME fixtures.glasses.eyes.control
  COMMAND ${CMAKE_COMMAND} "-DCMD=${cmd}" -DCODE=1 "-DREGEX=eyes: left=${glasses_want_left}, expected ${glasses_off}"
    -P ${PROJECT_SOURCE_DIR}/tests/cmake/expect_fail.cmake
  LABELS fixtures portable TIMEOUT 10)
cogmaster_add_test(NAME fixtures.glasses.hash_same
  COMMAND ${CMAKE_COMMAND} -DCASE=eyes_hash -DEXPECTED=${PROJECT_SOURCE_DIR}/tests/built/expected.txt
    -DRUN=$<TARGET_FILE:cogmaster-run> "-DEMULATOR=${fixtures_emulator}" -DNAME=glasses
    -DIMAGE=${COGMASTER_FIXTURES_DIR}/glasses.sms -DFRAMES=${glasses_count} -P ${CMAKE_CURRENT_LIST_FILE}
  LABELS fixtures portable TIMEOUT 10)

# The light_phaser fixture's oracle. Every expected value is derived here from the positions the source defines
# and from the sensor's rule (a disc of radius 6 around the aim, the light ending at the first dot that is not
# bright, H = (dot + 35) >> 1 for a release at that dot, 38 ticks after it is drawn), never read from a run of
# the core; each literal in tests/diag/expected.csv must equal what this derives. A dot is 0-based here, and an
# aim in a script is the dot plus one.
file(STRINGS ${fixtures_src}/light_phaser.asm lp_defines
  REGEX "^[.]DEFINE (SQ_X|SQ_Y|DARK_X|DARK_Y|BAND_X|BAND_Y|PHASE_[A-Z]+) [0-9]+$")
foreach(lp_define IN LISTS lp_defines)
  string(REGEX REPLACE "^[.]DEFINE ([A-Z_]+) ([0-9]+)$" "\\1" lp_key "${lp_define}")
  string(REGEX REPLACE "^[.]DEFINE ([A-Z_]+) ([0-9]+)$" "\\2" lp_value "${lp_define}")
  set(lp_${lp_key} ${lp_value})
endforeach()
foreach(lp_key IN ITEMS SQ_X SQ_Y DARK_X DARK_Y BAND_X BAND_Y PHASE_PARKED PHASE_PERIODS PHASE_DARK PHASE_SQUARE
    PHASE_BAND)
  if(NOT DEFINED lp_${lp_key})
    message(FATAL_ERROR "tests/roms/src/light_phaser.asm: no .DEFINE ${lp_key}")
  endif()
endforeach()
set(lp_periods 29 31 47 64 97 100 101 200 114 228)
set(lp_total 40)

# The half width of the disc's chord dy rows from the aim: the largest h with h * h + dy * dy within 6 * 6.
function(lp_half out dy)
  math(EXPR dy "${dy} * ${dy}")
  set(h 6)
  while(h GREATER 0)
    math(EXPR size "${h} * ${h} + ${dy}")
    if(size LESS_EQUAL 36)
      break()
    endif()
    math(EXPR h "${h} - 1")
  endwhile()
  set(${out} ${h} PARENT_SCOPE)
endfunction()

# The dot the light ends at on ROW when the aim is dot AIM_X of row AIM_Y: the first dot after the chord's lit run
# that is not lit, the dot after the chord counting as dark; 0 when the row has no light. SHAPE is square (a lit
# 16 by 16 square at LEFT, TOP) or band (a lit line across the whole width on row TOP).
function(lp_release out shape left top aim_x aim_y row)
  math(EXPR dy "${row} - ${aim_y}")
  lp_half(h ${dy})
  math(EXPR first "${aim_x} - ${h}")
  if(first LESS 0)
    set(first 0)
  endif()
  math(EXPR last "${aim_x} + ${h}")
  if(last GREATER 255)
    set(last 255)
  endif()
  math(EXPR stop "${last} + 1")
  math(EXPR right "${left} + 15")
  math(EXPR bottom "${top} + 15")
  set(release 0)
  set(was 0)
  foreach(dot RANGE ${first} ${stop})
    set(lit 0)
    if(dot LESS_EQUAL last)
      if(shape STREQUAL "band" AND row EQUAL top)
        set(lit 1)
      elseif(shape STREQUAL "square" AND row GREATER_EQUAL top AND row LESS_EQUAL bottom
          AND dot GREATER_EQUAL left AND dot LESS_EQUAL right)
        set(lit 1)
      endif()
    endif()
    if(was AND NOT lit AND release EQUAL 0)
      set(release ${dot})
    endif()
    set(was ${lit})
  endforeach()
  set(${out} ${release} PARENT_SCOPE)
endfunction()

# The aim at the square's right edge dot and centre row dot. The light must end at the dot after the edge on
# every one of the 13 rows of the disc, so the latched H does not depend on the row; this is checked here.
math(EXPR lp_square_x "${lp_SQ_X} + 15")
math(EXPR lp_square_y "${lp_SQ_Y} + 8")
foreach(lp_dy RANGE -6 6)
  math(EXPR lp_row "${lp_square_y} + ${lp_dy}")
  lp_release(lp_release_dot square ${lp_SQ_X} ${lp_SQ_Y} ${lp_square_x} ${lp_square_y} ${lp_row})
  math(EXPR lp_want_dot "${lp_SQ_X} + 16")
  if(NOT lp_release_dot EQUAL lp_want_dot)
    message(FATAL_ERROR "tests/roms/src/light_phaser.asm: the light ends at dot ${lp_release_dot} on row ${lp_row}, "
      "expected ${lp_want_dot} on every row of the disc")
  endif()
endforeach()
math(EXPR lp_square_h "(${lp_SQ_X} + 16 + 35) >> 1")
# The line: the aim is on its dot row and the light ends at the end of the chord.
math(EXPR lp_band_x "${lp_BAND_X} - 1")
lp_release(lp_band_dot band 0 ${lp_BAND_Y} ${lp_band_x} ${lp_BAND_Y} ${lp_BAND_Y})
math(EXPR lp_want_dot "${lp_BAND_X} + 6")
if(NOT lp_band_dot EQUAL lp_want_dot)
  message(FATAL_ERROR "tests/roms/src/light_phaser.asm: the light ends at dot ${lp_band_dot} on the line, "
    "expected ${lp_want_dot}")
endif()
math(EXPR lp_band_h "(${lp_BAND_X} + 6 + 35) >> 1")

# Four hex digits, upper case.
function(lp_hex out value)
  math(EXPR value "${value}" OUTPUT_FORMAT HEXADECIMAL)
  string(REGEX REPLACE "^0x" "" value "${value}")
  string(TOUPPER "${value}" value)
  string(LENGTH "${value}" digits)
  math(EXPR pad "4 - ${digits}")
  string(REPEAT 0 ${pad} zeros)
  set(${out} "${zeros}${value}" PARENT_SCOPE)
endfunction()

# The rows the fixture must hold, as "<test>|<profile>|<expected>"; plus or minus 1 H count around a derived one.
set(lp_want_rows "01:parked_th_low_reads|*|0000" "16:dark_th_low_reads|*|0000" "17:lit_th_low_reads|*|0001-FFFF"
  "1A:th_bit_that_saw_light|port1|0040" "1A:th_bit_that_saw_light|port2|0080")
function(lp_range out value)
  math(EXPR low "${value} - 1")
  math(EXPR high "${value} + 1")
  lp_hex(low_hex ${low})
  lp_hex(high_hex ${high})
  set(${out} "${low_hex}-${high_hex}" PARENT_SCOPE)
endfunction()
lp_range(lp_square_range ${lp_square_h})
lp_range(lp_band_range ${lp_band_h})
set(lp_index 0)
foreach(lp_period IN LISTS lp_periods)
  math(EXPR lp_id_b "2 + ${lp_index}")
  math(EXPR lp_id_a "12 + ${lp_index}")
  lp_hex(lp_id_b ${lp_id_b})
  lp_hex(lp_id_a ${lp_id_a})
  string(SUBSTRING "${lp_id_b}" 2 2 lp_id_b)
  string(SUBSTRING "${lp_id_a}" 2 2 lp_id_a)
  string(LENGTH "${lp_period}" lp_digits)
  math(EXPR lp_pad "3 - ${lp_digits}")
  string(REPEAT 0 ${lp_pad} lp_zeros)
  list(APPEND lp_want_rows "${lp_id_b}:period_${lp_zeros}${lp_period}_after_th_high|*|${lp_square_range}"
    "${lp_id_a}:period_${lp_zeros}${lp_period}_after_poll|*|${lp_square_range}")
  math(EXPR lp_index "${lp_index} + 1")
endforeach()
# The V counter read at the poll: the disc's first row to its last, two rows either side for the loop and the
# read, and one more line per 228 cycles of loop (both periods here are shorter than a line).
math(EXPR lp_v_low "${lp_SQ_Y} + 2 - 2")
math(EXPR lp_v_high "${lp_SQ_Y} + 14 + 2 + 1")
lp_hex(lp_v_low_hex ${lp_v_low})
lp_hex(lp_v_high_hex ${lp_v_high})
list(APPEND lp_want_rows "18:period_029_v_counter|*|${lp_v_low_hex}-${lp_v_high_hex}"
  "19:period_064_v_counter|*|${lp_v_low_hex}-${lp_v_high_hex}"
  "1B:line_after_th_high|*|${lp_band_range}" "1C:line_after_poll|*|${lp_band_range}")
list(LENGTH lp_want_rows lp_want_count)
file(STRINGS ${PROJECT_SOURCE_DIR}/tests/diag/expected.csv lp_rows REGEX "^light_phaser,")
list(LENGTH lp_rows lp_row_count)
if(NOT lp_row_count EQUAL lp_want_count)
  message(FATAL_ERROR "tests/diag/expected.csv: ${lp_row_count} light_phaser rows, the oracle derives ${lp_want_count}")
endif()
foreach(lp_row IN LISTS lp_rows)
  string(REPLACE "," ";" lp_row "${lp_row}")
  list(GET lp_row 1 lp_test)
  list(GET lp_row 2 lp_profile)
  list(GET lp_row 3 lp_literal)
  if(NOT "${lp_test}|${lp_profile}|${lp_literal}" IN_LIST lp_want_rows)
    message(FATAL_ERROR "tests/diag/expected.csv: light_phaser row ${lp_test} (${lp_profile}) is ${lp_literal}, "
      "which the oracle does not derive")
  endif()
endforeach()

# The scripts, one line a frame. The part of the program that runs in a frame is named by the source's PHASE_
# values: the ten periods and the lit run aim at the square, the dark part at the dark square's centre, the line
# part at the line, and every other frame has no aim. The trigger is never pulled. phaser aims port 1, port2
# aims port 2, left is the same 16 dots to the left, parked has no aim and dark aims at the dark square all the
# time (the runs that must equal a pad's).
math(EXPR lp_dark_x "${lp_DARK_X} + 9")
math(EXPR lp_dark_y "${lp_DARK_Y} + 9")
math(EXPR lp_square_aim_x "${lp_SQ_X} + 16")
math(EXPR lp_square_aim_y "${lp_SQ_Y} + 9")
math(EXPR lp_band_aim_y "${lp_BAND_Y} + 1")
math(EXPR lp_periods_end "${lp_PHASE_PERIODS} + 9")
set(lp_zero "+00000")
foreach(lp_variant IN ITEMS phaser port2 left parked dark)
  set(lp_body "")
  foreach(lp_frame RANGE 1 ${lp_total})
    set(lp_ax 0)
    set(lp_ay 0)
    if(lp_frame EQUAL lp_PHASE_SQUARE OR (lp_frame GREATER_EQUAL lp_PHASE_PERIODS AND lp_frame LESS_EQUAL lp_periods_end))
      set(lp_ax ${lp_square_aim_x})
      set(lp_ay ${lp_square_aim_y})
    elseif(lp_frame EQUAL lp_PHASE_DARK)
      set(lp_ax ${lp_dark_x})
      set(lp_ay ${lp_dark_y})
    elseif(lp_frame EQUAL lp_PHASE_BAND)
      set(lp_ax ${lp_BAND_X})
      set(lp_ay ${lp_band_aim_y})
    endif()
    if(lp_variant STREQUAL "left" AND lp_ax GREATER 0)
      math(EXPR lp_ax "${lp_ax} - 16")
    elseif(lp_variant STREQUAL "parked")
      set(lp_ax 0)
      set(lp_ay 0)
    elseif(lp_variant STREQUAL "dark")
      set(lp_ax ${lp_dark_x})
      set(lp_ay ${lp_dark_y})
    endif()
    sports_axis(lp_xs ${lp_ax})
    sports_axis(lp_ys ${lp_ay})
    if(lp_variant STREQUAL "port2")
      string(APPEND lp_body "p1=00 p2=00 pause=0 reset=0 start=0 x1=${lp_zero} y1=${lp_zero} x2=${lp_xs} y2=${lp_ys}\n")
    else()
      string(APPEND lp_body "p1=00 p2=00 pause=0 reset=0 start=0 x1=${lp_xs} y1=${lp_ys} x2=${lp_zero} y2=${lp_zero}\n")
    endif()
  endforeach()
  set(lp_file phaser-${lp_variant})
  if(lp_variant STREQUAL "phaser")
    set(lp_file phaser)
  endif()
  file(CONFIGURE OUTPUT ${PROJECT_BINARY_DIR}/tests/${lp_file}.input CONTENT "${lp_body}" NEWLINE_STYLE UNIX @ONLY)
endforeach()

# Fault prevented: a sensor that latches at the read, at the leading edge or at the wrong delay, a geometric TH
# that a loop length locks away from, or a fixture whose aim is a pixel off. The same rows are judged on the
# export, Japanese, PAL and first-model consoles with the gun on port 1, and on port 2 with a pad on port 1 (the
# row that names the TH bit differs); an aim 16 dots to the left, no aim and a pad on the port must each fall
# outside a row.
# Timeout: 10 s floor; 2 x 0.41 s (mailbox.japan on be32), slowest of 1 run, run 43010180943.
foreach(profile IN ITEMS export japan pal sms1)
  set(machine sms)
  set(region ntsc-u)
  if(profile STREQUAL "japan")
    set(region ntsc-j)
  elseif(profile STREQUAL "pal")
    set(region pal)
  elseif(profile STREQUAL "sms1")
    set(machine sms1)
  endif()
  cogmaster_add_test(NAME fixtures.light_phaser.mailbox.${profile}
    COMMAND ${CMAKE_COMMAND} ${fixtures_mailbox} -DNAME=light_phaser -DMACHINE=${machine} -DREGION=${region}
      -DPROFILE=port1 -DPORT1=lphaser -DINPUT=${PROJECT_BINARY_DIR}/tests/phaser.input
      -DIMAGE=${COGMASTER_FIXTURES_DIR}/light_phaser.sms -DFRAMES=${lp_total}
      -DCSV=${PROJECT_SOURCE_DIR}/tests/diag/expected.csv
      -DOUT=${fixtures_out}/light_phaser.${profile}.txt -P ${CMAKE_CURRENT_LIST_FILE}
    LABELS fixtures portable TIMEOUT 10)
endforeach()
cogmaster_add_test(NAME fixtures.light_phaser.port2
  COMMAND ${CMAKE_COMMAND} ${fixtures_mailbox} -DNAME=light_phaser -DMACHINE=sms -DREGION=ntsc-u
    -DPROFILE=port2 -DPORT1=pad -DPORT2=lphaser -DINPUT=${PROJECT_BINARY_DIR}/tests/phaser-port2.input
    -DIMAGE=${COGMASTER_FIXTURES_DIR}/light_phaser.sms -DFRAMES=${lp_total}
    -DCSV=${PROJECT_SOURCE_DIR}/tests/diag/expected.csv
    -DOUT=${fixtures_out}/light_phaser.port2.txt -P ${CMAKE_CURRENT_LIST_FILE}
  LABELS fixtures portable TIMEOUT 10)
# Timeout: 10 s floor; 2 x 0.38 s (control.parked on be32), slowest of 1 run, run 43010180943.
foreach(control IN ITEMS left parked pad)
  set(input phaser-${control})
  set(device lphaser)
  if(control STREQUAL "pad")
    set(input phaser-parked)
    set(device pad)
  endif()
  set(cmd ${CMAKE_COMMAND} -DCASE=mailbox -DEXPECTED=${PROJECT_SOURCE_DIR}/tests/built/expected.txt
    -DRUN=$<TARGET_FILE:cogmaster-run> "-DEMULATOR=${fixtures_inner_emulator}"
    -DJUDGE=${PROJECT_SOURCE_DIR}/tests/cmake/mailbox.cmake -DPROFILE=port1 -DNAME=light_phaser
    -DMACHINE=sms -DREGION=ntsc-u -DPORT1=${device} -DINPUT=${PROJECT_BINARY_DIR}/tests/${input}.input
    -DIMAGE=${COGMASTER_FIXTURES_DIR}/light_phaser.sms -DFRAMES=${lp_total}
    -DCSV=${PROJECT_SOURCE_DIR}/tests/diag/expected.csv -DOUT=${fixtures_out}/light_phaser.${control}.txt
    -P ${CMAKE_CURRENT_LIST_FILE})
  string(REPLACE ";" "$<SEMICOLON>" cmd "${cmd}")
  cogmaster_add_test(NAME fixtures.light_phaser.control.${control}
    COMMAND ${CMAKE_COMMAND} "-DCMD=${cmd}" -DCODE=1 "-DREGEX=outside its row"
      -P ${PROJECT_SOURCE_DIR}/tests/cmake/expect_fail.cmake
    LABELS fixtures portable TIMEOUT 10)
endforeach()

# Fault prevented: a judge that passes what it should fail. Each control
# runs the mapper_sega fixture through the judge and must fail with exit 1
# and its own message: a CSV copy with one value changed, a copy missing one
# row, and a run cut after one frame, before the ROM reports done.
set_property(DIRECTORY ${PROJECT_SOURCE_DIR} APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
  ${PROJECT_SOURCE_DIR}/tests/diag/expected.csv)
file(READ ${PROJECT_SOURCE_DIR}/tests/diag/expected.csv fixtures_csv)
string(REGEX REPLACE "(\nmapper_sega,03:[^,]*,[^,]*,)5AF3," "\\15AF4," fixtures_wrong
  "${fixtures_csv}")
string(REGEX REPLACE "\nmapper_sega,02:[^\n]*" "" fixtures_missing "${fixtures_csv}")
string(REGEX REPLACE "(\nvdp_lines_192,01:[^,]*,[^,]*,)0006," "\\10005," fixtures_wrong "${fixtures_wrong}")
string(REGEX REPLACE "\nvdp_lines_192,02:[^\n]*" "" fixtures_missing "${fixtures_missing}")
string(REGEX REPLACE "(\nvdp_vtable_192,02:[^,]*,ntsc,)848D," "\\1848E," fixtures_wrong "${fixtures_wrong}")
string(REGEX REPLACE "\nvdp_vtable_192,02:[^\n]*" "" fixtures_missing "${fixtures_missing}")
string(REGEX REPLACE "(\npause_nmi_192,01:[^,]*,ntsc,)[0-9A-F]+-[0-9A-F]+," "\\10000-0001," fixtures_wrong "${fixtures_wrong}")
string(REGEX REPLACE "\npause_nmi_192,02:[^\n]*" "" fixtures_missing "${fixtures_missing}")
file(WRITE ${fixtures_out}/wrong_value.csv "${fixtures_wrong}")
file(WRITE ${fixtures_out}/missing_row.csv "${fixtures_missing}")
string(REPLACE ";" "\;" fixtures_inner_emulator "${CMAKE_CROSSCOMPILING_EMULATOR}")
foreach(rom IN ITEMS mapper_sega vdp_lines_192 vdp_vtable_192 pause_nmi_192)
  set(prefix fixtures.judge)
  set(profile sms-export)
  set(script "")
  if(rom STREQUAL "vdp_lines_192")
    set(prefix fixtures.vdp_lines.judge)
    set(profile ntsc)
  elseif(rom STREQUAL "vdp_vtable_192")
    set(prefix fixtures.vdp_vtable.judge)
    set(profile ntsc)
  elseif(rom STREQUAL "pause_nmi_192")
    set(prefix fixtures.pause_nmi.judge)
    set(profile ntsc)
  endif()
  foreach(control IN ITEMS wrong_value missing_row hang)
    set(csv ${fixtures_out}/${control}.csv)
    set(frames 120)
    if(rom STREQUAL "pause_nmi_192")
      # The script holds exactly the frames of the run; the one-frame hang needs none.
      set(frames 40)
      set(script -DINPUT=${pause_input})
    endif()
    if(control STREQUAL "wrong_value")
      set(regex "outside its row")
    elseif(control STREQUAL "missing_row")
      set(regex "has no row")
    else()
      set(csv ${PROJECT_SOURCE_DIR}/tests/diag/expected.csv)
      set(frames 1)
      set(script "")
      set(regex "[(]hang[)]")
    endif()
    set(cmd ${CMAKE_COMMAND} -DCASE=mailbox -DEXPECTED=${PROJECT_SOURCE_DIR}/tests/built/expected.txt
      -DRUN=$<TARGET_FILE:cogmaster-run> "-DEMULATOR=${fixtures_inner_emulator}"
      -DJUDGE=${PROJECT_SOURCE_DIR}/tests/cmake/mailbox.cmake -DPROFILE=${profile}
      -DNAME=${rom} -DIMAGE=${COGMASTER_FIXTURES_DIR}/${rom}.sms -DFRAMES=${frames} ${script}
      -DCSV=${csv} -DOUT=${fixtures_out}/${rom}.${control}.txt -P ${CMAKE_CURRENT_LIST_FILE})
    string(REPLACE ";" "$<SEMICOLON>" cmd "${cmd}")
    # Timeout: 10 s floor; 2 x 0.02 s, slowest of 2 runs, run 39376644025, provisional.
    cogmaster_add_test(NAME ${prefix}.${control}
      COMMAND ${CMAKE_COMMAND} "-DCMD=${cmd}" -DCODE=1 "-DREGEX=${regex}"
        -P ${PROJECT_SOURCE_DIR}/tests/cmake/expect_fail.cmake
      LABELS fixtures portable TIMEOUT 10)
  endforeach()
endforeach()

# Fault prevented: a PAL image that passes on an NTSC run. The 192-line image runs at --region ntsc-u
# and is judged against the PAL rows: the line count reads 262 against 313 and the sum 33,933 against
# 44,838, so the judge must fail.
set(cmd ${CMAKE_COMMAND} -DCASE=mailbox -DEXPECTED=${PROJECT_SOURCE_DIR}/tests/built/expected.txt
  -DRUN=$<TARGET_FILE:cogmaster-run> "-DEMULATOR=${fixtures_inner_emulator}"
  -DJUDGE=${PROJECT_SOURCE_DIR}/tests/cmake/mailbox.cmake -DPROFILE=pal -DREGION=ntsc-u
  -DNAME=vdp_vtable_192 -DIMAGE=${COGMASTER_FIXTURES_DIR}/vdp_vtable_192.sms -DFRAMES=30
  -DCSV=${PROJECT_SOURCE_DIR}/tests/diag/expected.csv
  -DOUT=${fixtures_out}/vdp_vtable_192.wrong_profile.txt -P ${CMAKE_CURRENT_LIST_FILE})
string(REPLACE ";" "$<SEMICOLON>" cmd "${cmd}")
# Timeout: 10 s floor; 2 x 0.16 s, slowest of 1 run (9 lanes), run 43117518796.
cogmaster_add_test(NAME fixtures.vdp_vtable.control.wrong_profile
  COMMAND ${CMAKE_COMMAND} "-DCMD=${cmd}" -DCODE=1 "-DREGEX=outside its row"
    -P ${PROJECT_SOURCE_DIR}/tests/cmake/expect_fail.cmake
  LABELS fixtures portable TIMEOUT 10)

# Fault prevented: a PAL Pause image that passes on an NTSC run. The PAL image at --region ntsc-u is
# judged against the PAL band: the NMI comes 34,884 ticks sooner, so n reads about 150 bodies short.
set(cmd ${CMAKE_COMMAND} -DCASE=mailbox -DEXPECTED=${PROJECT_SOURCE_DIR}/tests/built/expected.txt
  -DRUN=$<TARGET_FILE:cogmaster-run> "-DEMULATOR=${fixtures_inner_emulator}"
  -DJUDGE=${PROJECT_SOURCE_DIR}/tests/cmake/mailbox.cmake -DPROFILE=pal -DREGION=ntsc-u
  -DNAME=pause_nmi_192 -DIMAGE=${COGMASTER_FIXTURES_DIR}/pause_nmi_192.sms -DFRAMES=40
  -DINPUT=${pause_input} -DCSV=${PROJECT_SOURCE_DIR}/tests/diag/expected.csv
  -DOUT=${fixtures_out}/pause_nmi_192.wrong_profile.txt -P ${CMAKE_CURRENT_LIST_FILE})
string(REPLACE ";" "$<SEMICOLON>" cmd "${cmd}")
# Timeout: 20 s floor; 2 x 0.45 s, slowest of 1 run (9 lanes), run 43117518796.
cogmaster_add_test(NAME fixtures.pause_nmi.control.wrong_profile
  COMMAND ${CMAKE_COMMAND} "-DCMD=${cmd}" -DCODE=1 "-DREGEX=outside its row"
    -P ${PROJECT_SOURCE_DIR}/tests/cmake/expect_fail.cmake
  LABELS fixtures portable TIMEOUT 20)

# Fault prevented: an assembler pin that accepts a tarball other than the
# pinned one. A local tarball checked against the wla-dx row's digest must
# fail the download step with a hash mismatch; file:// needs no network.
set(fixtures_tamper ${PROJECT_BINARY_DIR}/tests/wla-pin)
file(MAKE_DIRECTORY ${fixtures_tamper})
file(WRITE ${fixtures_tamper}/payload.txt "not the pinned WLA-DX tarball\n")
file(ARCHIVE_CREATE OUTPUT ${fixtures_tamper}/tampered.tar.gz PATHS ${fixtures_tamper}/payload.txt
  FORMAT gnutar COMPRESSION GZip)
set(fixtures_url "file://${fixtures_tamper}/tampered.tar.gz")
if(NOT fixtures_tamper MATCHES "^/")
  set(fixtures_url "file:///${fixtures_tamper}/tampered.tar.gz")
endif()
set(cmd ${CMAKE_COMMAND} -DURL=${fixtures_url} -DSHA256=${COGMASTER_WLA_SHA256}
  -DOUT=${fixtures_tamper}/download.tar.gz -P ${PROJECT_SOURCE_DIR}/cmake/WlaDx.cmake)
string(REPLACE ";" "$<SEMICOLON>" cmd "${cmd}")
# Timeout: 10 s floor; 2 x 0.01 s, slowest of 2 runs, run 39376644025, provisional.
cogmaster_add_test(NAME fixtures.wla_pin.self_test
  COMMAND ${CMAKE_COMMAND} "-DCMD=${cmd}" -DCODE=1 "-DREGEX=[Hh]ash|EXPECTED_HASH|mismatch"
    -P ${PROJECT_SOURCE_DIR}/tests/cmake/expect_fail.cmake
  LABELS fixtures TIMEOUT 10)
