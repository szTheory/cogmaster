# The hashes every lane prints, and their comparison across lanes, run as
#   cmake -DMODE=write -DRUN=<cogmaster-run> [-DEMULATOR=<list>] -DOUT=<dir>
#     [-DFIXTURES=<dir>] -P hashes.cmake
#   cmake -DMODE=compare -DFILES=<list> -DCOUNT=<n> -P hashes.cmake
set(hashes_dir "${CMAKE_CURRENT_LIST_DIR}")
# Judges the printed records of a mailbox run against tests/diag/expected.csv, so a lane cannot
# agree with the others on a wrong answer. It runs in a function scope because mailbox.cmake reads
# OUT as the runner's output file, and OUT here is the hash directory.
function(hashes_judge file records rom profile)
  file(CONFIGURE OUTPUT "${file}" NEWLINE_STYLE UNIX @ONLY CONTENT "${records}")
  set(OUT "${file}")
  set(CSV "${hashes_dir}/../diag/expected.csv")
  set(ROM "${rom}")
  set(PROFILE "${profile}")
  include("${hashes_dir}/mailbox.cmake")
endfunction()

if(MODE STREQUAL "write")
  # Fault prevented: a lane's output silently not compared. Writes
  # <OUT>/hashes.txt with the frame 60 hash line of the Master System card
  # and of the Game Gear card, then one "fixture <name> sha256 <hex>" line per
  # image in FIXTURES, the file CI collects from every lane.
  set(lines "")
  foreach(machine IN ITEMS sms gg)
    set(args --frames 60 --hash)
    if(machine STREQUAL "gg")
      list(APPEND args --machine gg)
    endif()
    execute_process(COMMAND ${EMULATOR} "${RUN}" ${args}
      OUTPUT_VARIABLE out ERROR_VARIABLE err RESULT_VARIABLE rc)
    if(NOT rc STREQUAL "0" OR NOT out MATCHES "^frame 60 sha256 [0-9a-f]+\n$")
      message(FATAL_ERROR "cogmaster-run ${args}: expected exit 0 and one hash line, "
        "got exit ${rc}\n${out}${err}")
    endif()
    string(APPEND lines "${out}")
  endforeach()
  if(FIXTURES)
    file(GLOB images "${FIXTURES}/*.sms" "${FIXTURES}/*.gg")
    if(NOT images)
      message(FATAL_ERROR "hashes: no fixture image in ${FIXTURES}")
    endif()
    list(SORT images)
    foreach(image IN LISTS images)
      get_filename_component(name "${image}" NAME_WE)
      file(SHA256 "${image}" digest)
      get_filename_component(filename "${image}" NAME)
      string(APPEND lines "fixture ${filename} sha256 ${digest}\n")
      if(filename MATCHES "^(boot_to_play[.]sms|boot_to_play[.]gg|vdp_lines_[0-9]+[.]sms)$")
        set(profiles sms sms1 gg-sms)
        if(filename STREQUAL "boot_to_play.gg")
          set(profiles native)
        elseif(name MATCHES "^vdp_lines_")
          list(APPEND profiles native)
        endif()
        foreach(profile IN LISTS profiles)
          set(machine sms)
          set(video sms)
          if(profile STREQUAL "sms1")
            set(machine sms1)
          elseif(profile STREQUAL "gg-sms" OR profile STREQUAL "native")
            set(machine gg)
            if(profile STREQUAL "native")
              set(video gg)
            endif()
          endif()
          foreach(region IN ITEMS ntsc-u ntsc-j pal)
            if(machine STREQUAL "gg" AND region STREQUAL "pal")
              continue()
            endif()
            set(variants baseline)
            if(name STREQUAL "boot_to_play")
              list(APPEND variants input pad2 both)
              if(profile STREQUAL "native")
                set(variants baseline input)
              endif()
            endif()
            foreach(variant IN LISTS variants)
              set(input "")
              if(NOT variant STREQUAL "baseline")
                set(script play)
                if(profile STREQUAL "native")
                  set(script play-gg)
                elseif(variant STREQUAL "pad2" OR variant STREQUAL "both")
                  set(script play-${variant})
                endif()
                set(input --input "${OUT}/tests/${script}.input")
              endif()
              execute_process(COMMAND ${EMULATOR} "${RUN}" "${image}" --machine ${machine}
                --video-mode ${video} --region ${region} --frames 14 --hash ${input}
                OUTPUT_VARIABLE frame ERROR_VARIABLE err RESULT_VARIABLE rc)
              if(NOT rc STREQUAL "0" OR NOT frame MATCHES "^frame 14 sha256 [0-9a-f]+\n")
                message(FATAL_ERROR "playing ${filename}/${profile}/${region}/${variant}: exit ${rc}: ${frame}${err}")
              endif()
              string(APPEND lines "playing ${filename} ${profile} ${region} ${variant} ${frame}")
            endforeach()
          endforeach()
        endforeach()
      endif()
      # Fault prevented: a device frame that differs between lanes. A row is <file>|<script>|<arguments>;
      # the frames are the script's lines, since the runner takes one record per frame.
      # A row may end in |<region> (default ntsc-j).
      foreach(row IN ITEMS "sports_pad.sms|sports|--port1 sports --input ${OUT}/tests/sports.input"
          "paddle.sms|paddle|--port1 paddle --input ${OUT}/tests/paddle.input"
          "glasses.sms|glasses|"
          "light_phaser.sms|phaser|--port1 lphaser --input ${OUT}/tests/phaser.input"
          "pause_nmi_192.sms|pause|--input ${OUT}/tests/pause.input|pal"
          "pause_nmi_224.sms|pause|--input ${OUT}/tests/pause.input|pal"
          "pause_nmi_240.sms|pause|--input ${OUT}/tests/pause.input|pal"
          "bios_probe.sms|bios|--boot bios --bios ${FIXTURES}/bios_shape.bin")
        if(row MATCHES "^${filename}[|]([^|]*)[|]([^|]*)([|](.*))?$")
          set(device_region ntsc-j)
          if(CMAKE_MATCH_4)
            set(device_region ${CMAKE_MATCH_4})
          endif()
          separate_arguments(device_args UNIX_COMMAND "${CMAKE_MATCH_2}")
          file(STRINGS "${OUT}/tests/${CMAKE_MATCH_1}.input" device_script)
          list(LENGTH device_script count)
          execute_process(COMMAND ${EMULATOR} "${RUN}" "${image}" --machine sms --video-mode sms --region ${device_region} --frames ${count} --hash ${device_args} OUTPUT_VARIABLE frame ERROR_VARIABLE err RESULT_VARIABLE rc)
          if(NOT rc STREQUAL "0" OR NOT frame MATCHES "^frame ${count} sha256 [0-9a-f]+\n")
            message(FATAL_ERROR "device ${filename}: exit ${rc}: ${frame}${err}")
          endif()
          string(APPEND lines "playing ${filename} sms ${device_region} device ${frame}")
        endif()
      endforeach()
      # Fault prevented: fixtures whose frame never changes (the V and Pause images paint nothing, so a
      # frame hash would be trivial) giving no cross-lane evidence. The digest of the printed records is
      # written for the PAL profile of every height and the NTSC profile of 192 and 224 (the NTSC 240
      # counter rolls and is not judged), and the same bytes are judged against the literals in
      # tests/diag/expected.csv, so the lanes agree with each other and with the table.
      if(filename MATCHES "^(vdp_vtable|pause_nmi)_([0-9]+)[.]sms$")
        set(mailbox_rom ${CMAKE_MATCH_1}_${CMAKE_MATCH_2})
        set(mailbox_height ${CMAKE_MATCH_2})
        set(mailbox_input "")
        set(mailbox_frames 30)
        if(CMAKE_MATCH_1 STREQUAL "pause_nmi")
          set(mailbox_input --input "${OUT}/tests/pause.input")
          set(mailbox_frames 40)
        endif()
        foreach(profile IN ITEMS ntsc pal)
          set(region ntsc-u)
          if(profile STREQUAL "pal")
            set(region pal)
          elseif(mailbox_height STREQUAL "240")
            continue()
          endif()
          execute_process(COMMAND ${EMULATOR} "${RUN}" "${image}" --machine sms --video-mode sms --region ${region} --frames ${mailbox_frames} ${mailbox_input} --mailbox OUTPUT_VARIABLE records ERROR_VARIABLE err RESULT_VARIABLE rc)
          if(NOT rc STREQUAL "0")
            message(FATAL_ERROR "mailbox ${filename}/${profile}: exit ${rc}: ${records}${err}")
          endif()
          hashes_judge("${OUT}/mailbox.${mailbox_rom}.${profile}.txt" "${records}" ${mailbox_rom} ${profile})
          string(SHA256 digest "${records}")
          string(APPEND lines "mailbox ${filename} ${profile} ${region} sha256 ${digest}\n")
        endforeach()
      endif()
    endforeach()
    # Fault prevented: a sound that differs between lanes. The two hash lines of
    # each sound run, tagged "<file> <profile> <region> <variant>" after the
    # keyword: the sound fixture on both machines and both TV standards and the
    # native Game Gear image, then the playable fixture without input (60
    # frames) and with the play script (14 frames). tests/audio/ pins them.
    foreach(row IN ITEMS "psg_sweep.sms|sms|sms|sms|ntsc-u|300|baseline|"
        "psg_sweep.sms|sms|sms|sms|pal|300|baseline|" "psg_sweep.sms|sms|sms|sms|ntsc-j|300|baseline|"
        "psg_sweep.sms|gg-sms|gg|sms|ntsc-u|300|baseline|"
        "psg_sweep.gg|native|gg|gg|ntsc-u|300|baseline|"
        "fm_sweep.sms|sms|sms|sms|ntsc-j|300|baseline|" "fm_sweep.sms|sms|sms|sms|ntsc-u|300|baseline|"
        "fm_sweep.sms|sms|sms|sms|pal|300|fm-on||on"
        "boot_to_play.sms|sms|sms|sms|ntsc-u|60|baseline|" "boot_to_play.sms|sms|sms|sms|ntsc-u|14|input|play"
        "boot_to_play.gg|native|gg|gg|ntsc-u|60|baseline|" "boot_to_play.gg|native|gg|gg|ntsc-u|14|input|play-gg")
      string(REPLACE "|" ";" row "${row}")
      list(GET row 0 filename)
      list(GET row 1 profile)
      list(GET row 2 machine)
      list(GET row 3 video)
      list(GET row 4 region)
      list(GET row 5 frames)
      list(GET row 6 variant)
      list(GET row 7 script)
      # The optional ninth field is the FM unit setting (default: the runner's auto).
      set(fm "")
      list(LENGTH row row_fields)
      if(row_fields GREATER 8)
        list(GET row 8 fm)
      endif()
      set(input "")
      if(script)
        set(input --input "${OUT}/tests/${script}.input")
      endif()
      if(fm)
        list(APPEND input --fm ${fm})
      endif()
      execute_process(COMMAND ${EMULATOR} "${RUN}" "${FIXTURES}/${filename}" --machine ${machine}
        --video-mode ${video} --region ${region} --frames ${frames} --hash-audio ${input}
        OUTPUT_VARIABLE sound ERROR_VARIABLE err RESULT_VARIABLE rc)
      if(NOT rc STREQUAL "0")
        message(FATAL_ERROR "sound ${filename}/${profile}/${region}/${variant}: exit ${rc}: ${sound}${err}")
      endif()
      string(REPLACE "\n" ";" sound "${sound}")
      foreach(line IN LISTS sound)
        if(line MATCHES "^audio-(transitions|pcm) (.*)$")
          string(APPEND lines "audio-${CMAKE_MATCH_1} ${filename} ${profile} ${region} ${variant} ${CMAKE_MATCH_2}\n")
        endif()
      endforeach()
    endforeach()
  endif()
  # Unix newlines on every host: file(WRITE) writes CRLF on Windows.
  file(CONFIGURE OUTPUT "${OUT}/hashes.txt" NEWLINE_STYLE UNIX CONTENT "${lines}")
elseif(MODE STREQUAL "compare")
  # Fault prevented: lanes that disagree, or a lane that stops uploading and
  # silently drops out. Passes only for exactly COUNT existing files that are
  # byte-identical.
  list(LENGTH FILES n)
  if(NOT n EQUAL COUNT)
    message(FATAL_ERROR "hashes: expected ${COUNT} files, got ${n}: ${FILES}")
  endif()
  list(GET FILES 0 first)
  foreach(file IN LISTS FILES)
    if(NOT EXISTS "${file}")
      message(FATAL_ERROR "hashes: ${file} does not exist")
    endif()
    execute_process(COMMAND ${CMAKE_COMMAND} -E compare_files "${first}" "${file}"
      RESULT_VARIABLE rc OUTPUT_QUIET ERROR_QUIET)
    if(NOT rc EQUAL 0)
      file(READ "${first}" want)
      file(READ "${file}" got)
      message(FATAL_ERROR "hashes: ${file} differs from ${first}\n${first}:\n${want}${file}:\n${got}")
    endif()
  endforeach()
  message(STATUS "hashes: ${n} identical files")
else()
  message(FATAL_ERROR "hashes: -DMODE=write or -DMODE=compare is required")
endif()
