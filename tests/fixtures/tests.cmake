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
  if(CASE STREQUAL "image")
    # The runner's probe finds the header checksum good.
    execute_process(COMMAND ${EMULATOR} ${RUN} --probe ${IMAGE} RESULT_VARIABLE rc
      OUTPUT_VARIABLE out ERROR_VARIABLE out)
    if(NOT rc STREQUAL "0" OR NOT out MATCHES "(^|\n)checksum_ok=1\n")
      message(FATAL_ERROR "--probe ${IMAGE}: expected exit 0 and checksum_ok=1, got exit ${rc}\n${out}")
    endif()
  elseif(CASE STREQUAL "mailbox")
    # The runner prints the image's mailbox, at most FRAMES frames in, into
    # OUT; mailbox.cmake (JUDGE) then judges it against CSV for PROFILE.
    execute_process(COMMAND ${EMULATOR} ${RUN} ${IMAGE} --frames ${FRAMES} --mailbox
      RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
    if(NOT rc STREQUAL "0")
      message(FATAL_ERROR "--mailbox ${IMAGE}: exit ${rc}\n${err}")
    endif()
    file(WRITE "${OUT}" "${out}")
    set(ROM ${NAME})
    include(${JUDGE})
  else()
    message(FATAL_ERROR "unknown CASE '${CASE}'")
  endif()
  return()
endif()

set(COGMASTER_FIXTURES_DIR ${PROJECT_BINARY_DIR}/fixtures)
set(fixtures_src ${PROJECT_SOURCE_DIR}/tests/roms/src)
set(fixtures_images "")
foreach(name IN ITEMS hello mapper_sega)
  set(image ${COGMASTER_FIXTURES_DIR}/${name}.sms)
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
      ${fixtures_src}/mailbox.inc
    COMMENT "Assembling the ${name} fixture"
    VERBATIM)
  list(APPEND fixtures_images ${image})
endforeach()
add_custom_target(cogmaster-fixtures ALL DEPENDS ${fixtures_images})

string(REPLACE ";" "$<SEMICOLON>" fixtures_emulator "${CMAKE_CROSSCOMPILING_EMULATOR}")

# Fault prevented: a fixture assembled to different bytes than the ones
# pinned (another assembler, a changed source, CRLF line endings), or a
# header checksum the runner's probe disagrees with.
cogmaster_add_test(NAME fixtures.hello
  COMMAND ${CMAKE_COMMAND} -DCASE=image -DNAME=hello -DIMAGE=${COGMASTER_FIXTURES_DIR}/hello.sms
    -DEXPECTED=${PROJECT_SOURCE_DIR}/tests/built/expected.txt -DRUN=$<TARGET_FILE:cogmaster-run>
    "-DEMULATOR=${fixtures_emulator}" -P ${CMAKE_CURRENT_LIST_FILE}
  LABELS fixtures portable TIMEOUT 30)

# Fault prevented: a fixture whose measurements are printed but never
# compared, so a core that pages the wrong bank passes. The image's digest is
# checked, then its mailbox records are judged against tests/diag/expected.csv.
set(fixtures_mailbox -DCASE=mailbox -DEXPECTED=${PROJECT_SOURCE_DIR}/tests/built/expected.txt
  -DRUN=$<TARGET_FILE:cogmaster-run> "-DEMULATOR=${fixtures_emulator}"
  -DJUDGE=${PROJECT_SOURCE_DIR}/tests/cmake/mailbox.cmake -DPROFILE=sms-export)
set(fixtures_out ${PROJECT_BINARY_DIR}/tests/mailbox)
file(MAKE_DIRECTORY ${fixtures_out})
cogmaster_add_test(NAME fixtures.mapper_sega
  COMMAND ${CMAKE_COMMAND} ${fixtures_mailbox} -DNAME=mapper_sega
    -DIMAGE=${COGMASTER_FIXTURES_DIR}/mapper_sega.sms -DFRAMES=120
    -DCSV=${PROJECT_SOURCE_DIR}/tests/diag/expected.csv -DOUT=${fixtures_out}/mapper_sega.txt
    -P ${CMAKE_CURRENT_LIST_FILE}
  LABELS fixtures portable TIMEOUT 30)

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
cogmaster_add_test(NAME fixtures.wla_pin.self_test
  COMMAND ${CMAKE_COMMAND} "-DCMD=${cmd}" -DCODE=1 "-DREGEX=[Hh]ash|EXPECTED_HASH|mismatch"
    -P ${PROJECT_SOURCE_DIR}/tests/cmake/expect_fail.cmake
  LABELS fixtures TIMEOUT 30)
