# The assembled fixtures: tests/roms/src/<name>.asm, linked by <name>.link,
# built with the pinned WLA-DX into build/<preset>/fixtures/<name>.sms. The
# images live only in the build tree: they are never committed and never
# packed into a release archive.
#
# Run as a script (cmake -DCASE=<case> ... -P tests.cmake), this file is one
# fixture check; include()d by tests/tests.cmake, it builds and registers them.
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
  elseif(CASE STREQUAL "image")
    # The runner's probe finds the header checksum good.
    execute_process(COMMAND ${EMULATOR} ${RUN} --probe ${IMAGE} RESULT_VARIABLE rc
      OUTPUT_VARIABLE out ERROR_VARIABLE out)
    if(NOT rc STREQUAL "0" OR NOT out MATCHES "(^|\n)checksum_ok=1\n")
      message(FATAL_ERROR "--probe ${IMAGE}: expected exit 0 and checksum_ok=1, got exit ${rc}\n${out}")
    endif()
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
    if(INPUT)
      list(APPEND machine --input ${INPUT})
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
  else()
    message(FATAL_ERROR "unknown CASE '${CASE}'")
  endif()
  return()
endif()

set(COGMASTER_FIXTURES_DIR ${PROJECT_BINARY_DIR}/fixtures)
set(fixtures_src ${PROJECT_SOURCE_DIR}/tests/roms/src)
set(fixtures_images "")
foreach(name IN ITEMS hello mapper_sega mapper_codemasters ports ports_gg boot_to_play boot_to_play_gg battery)
  set(extension sms)
  set(image_name ${name})
  if(name STREQUAL "boot_to_play_gg")
    set(extension gg)
    set(image_name boot_to_play)
  endif()
  set(image ${COGMASTER_FIXTURES_DIR}/${image_name}.${extension})
  # The source directory is the assembler's working directory, so .INCLUDE
  # finds header.inc; the linker's is the build directory, so the link file
  # names the object without a path.
  add_custom_command(OUTPUT ${image}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${COGMASTER_FIXTURES_DIR}
    COMMAND ${COGMASTER_WLA_Z80} -q -o ${COGMASTER_FIXTURES_DIR}/${name}.o ${name}.asm
    COMMAND ${CMAKE_COMMAND} -E chdir ${COGMASTER_FIXTURES_DIR}
      ${COGMASTER_WLALINK} -r ${fixtures_src}/${name}.link ${image}
    WORKING_DIRECTORY ${fixtures_src}
    DEPENDS ${fixtures_src}/${name}.asm ${fixtures_src}/${name}.link ${fixtures_src}/header.inc
      ${fixtures_src}/mailbox.inc ${fixtures_src}/ports.asm ${fixtures_src}/boot_to_play.asm
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

# Fault prevented: guest-only rendering regressions across both TV standards and every height.
foreach(tv IN ITEMS ntsc pal)
  foreach(height IN ITEMS 192 224 240)
    cogmaster_add_test(NAME fixtures.vdp_lines.pixels.${tv}.${height}
      COMMAND cogmaster-test-lines ${COGMASTER_FIXTURES_DIR}/vdp_lines_${height}.sms ${height} ${tv}
      LABELS fixtures portable TIMEOUT 10)
  endforeach()
endforeach()

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

# Fault prevented: a fixture assembled to different bytes than the ones
# pinned (another assembler, a changed source, CRLF line endings), or a
# header checksum the runner's probe disagrees with.
# Timeout: 10 s floor; 2 x 0.01 s, slowest of 2 runs, run 39376644025, provisional.
cogmaster_add_test(NAME fixtures.hello
  COMMAND ${CMAKE_COMMAND} -DCASE=image -DNAME=hello -DIMAGE=${COGMASTER_FIXTURES_DIR}/hello.sms
    -DEXPECTED=${PROJECT_SOURCE_DIR}/tests/built/expected.txt -DRUN=$<TARGET_FILE:cogmaster-run>
    "-DEMULATOR=${fixtures_emulator}" -P ${CMAKE_CURRENT_LIST_FILE}
  LABELS fixtures portable TIMEOUT 10)

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
file(WRITE ${fixtures_out}/wrong_value.csv "${fixtures_wrong}")
file(WRITE ${fixtures_out}/missing_row.csv "${fixtures_missing}")
string(REPLACE ";" "\;" fixtures_inner_emulator "${CMAKE_CROSSCOMPILING_EMULATOR}")
foreach(rom IN ITEMS mapper_sega vdp_lines_192)
  set(prefix fixtures.judge)
  set(profile sms-export)
  if(rom STREQUAL "vdp_lines_192")
    set(prefix fixtures.vdp_lines.judge)
    set(profile ntsc)
  endif()
  foreach(control IN ITEMS wrong_value missing_row hang)
    set(csv ${fixtures_out}/${control}.csv)
    set(frames 120)
    if(control STREQUAL "wrong_value")
      set(regex "outside its row")
    elseif(control STREQUAL "missing_row")
      set(regex "has no row")
    else()
      set(csv ${PROJECT_SOURCE_DIR}/tests/diag/expected.csv)
      set(frames 1)
      set(regex "[(]hang[)]")
    endif()
    set(cmd ${CMAKE_COMMAND} -DCASE=mailbox -DEXPECTED=${PROJECT_SOURCE_DIR}/tests/built/expected.txt
      -DRUN=$<TARGET_FILE:cogmaster-run> "-DEMULATOR=${fixtures_inner_emulator}"
      -DJUDGE=${PROJECT_SOURCE_DIR}/tests/cmake/mailbox.cmake -DPROFILE=${profile}
      -DNAME=${rom} -DIMAGE=${COGMASTER_FIXTURES_DIR}/${rom}.sms -DFRAMES=${frames}
      -DCSV=${csv} -DOUT=${fixtures_out}/${rom}.${control}.txt -P ${CMAKE_CURRENT_LIST_FILE})
    string(REPLACE ";" "$<SEMICOLON>" cmd "${cmd}")
    # Timeout: 10 s floor; 2 x 0.02 s, slowest of 2 runs, run 39376644025, provisional.
    cogmaster_add_test(NAME ${prefix}.${control}
      COMMAND ${CMAKE_COMMAND} "-DCMD=${cmd}" -DCODE=1 "-DREGEX=${regex}"
        -P ${PROJECT_SOURCE_DIR}/tests/cmake/expect_fail.cmake
      LABELS fixtures portable TIMEOUT 10)
  endforeach()
endforeach()

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
