# The assembled fixtures: tests/roms/src/<name>.asm, linked by <name>.link,
# built with the pinned WLA-DX into build/<preset>/fixtures/<name>.sms. The
# images live only in the build tree: they are never committed and never
# packed into a release archive.
#
# Run as a script (cmake -DCASE=<case> ... -P tests.cmake), this file is one
# fixture check; include()d by tests/tests.cmake, it builds and registers them.
if(CMAKE_SCRIPT_MODE_FILE)
  if(CASE STREQUAL "image")
    # The image's SHA-256 equals its line in tests/built/expected.txt, then
    # the runner's probe finds the header checksum good.
    file(STRINGS "${EXPECTED}" want REGEX "^${NAME} ")
    if(NOT want MATCHES "^${NAME} ([0-9a-f]+)$")
      message(FATAL_ERROR "${EXPECTED}: no line '${NAME} <sha256>'")
    endif()
    set(want ${CMAKE_MATCH_1})
    file(SHA256 "${IMAGE}" got)
    if(NOT got STREQUAL want)
      message(FATAL_ERROR "${IMAGE}: SHA-256 ${got}, expected ${want}")
    endif()
    execute_process(COMMAND ${EMULATOR} ${RUN} --probe ${IMAGE} RESULT_VARIABLE rc
      OUTPUT_VARIABLE out ERROR_VARIABLE out)
    if(NOT rc STREQUAL "0" OR NOT out MATCHES "(^|\n)checksum_ok=1\n")
      message(FATAL_ERROR "--probe ${IMAGE}: expected exit 0 and checksum_ok=1, got exit ${rc}\n${out}")
    endif()
  else()
    message(FATAL_ERROR "unknown CASE '${CASE}'")
  endif()
  return()
endif()

set(COGMASTER_FIXTURES_DIR ${PROJECT_BINARY_DIR}/fixtures)
set(fixtures_src ${PROJECT_SOURCE_DIR}/tests/roms/src)
set(fixtures_images "")
foreach(name IN ITEMS hello)
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
