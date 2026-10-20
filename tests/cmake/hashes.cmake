# The hashes every lane prints, and their comparison across lanes, run as
#   cmake -DMODE=write -DRUN=<cogmaster-run> [-DEMULATOR=<list>] -DOUT=<dir>
#     [-DFIXTURES=<dir>] -P hashes.cmake
#   cmake -DMODE=compare -DFILES=<list> -DCOUNT=<n> -P hashes.cmake
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
  if(DEFINED FIXTURES)
    file(GLOB images "${FIXTURES}/*.sms")
    if(NOT images)
      message(FATAL_ERROR "hashes: no fixture image in ${FIXTURES}")
    endif()
    list(SORT images)
    foreach(image IN LISTS images)
      get_filename_component(name "${image}" NAME_WE)
      file(SHA256 "${image}" digest)
      string(APPEND lines "fixture ${name} sha256 ${digest}\n")
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
