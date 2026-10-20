cmake_minimum_required(VERSION 3.25)
foreach(var RETROARCH CORE INFO TEMPLATE RA_DIR FRAMES SHOT)
  if("${${var}}" STREQUAL "")
    message(FATAL_ERROR "run_retroarch: -D${var} is required")
  endif()
endforeach()
file(REMOVE_RECURSE "${RA_DIR}")
foreach(sub home assets cache cores info logs playlists saves shots states system)
  file(MAKE_DIRECTORY "${RA_DIR}/${sub}")
endforeach()
configure_file("${TEMPLATE}" "${RA_DIR}/test.cfg" @ONLY)
file(COPY "${INFO}" DESTINATION "${RA_DIR}/info")
file(REAL_PATH "${CORE}" core)
file(REAL_PATH "${RETROARCH}" retroarch)
get_filename_component(shot_dir "${SHOT}" DIRECTORY)
file(MAKE_DIRECTORY "${shot_dir}")
file(REMOVE "${SHOT}")
set(content "")
if(CONTENT)
  file(REAL_PATH "${CONTENT}" content)
endif()
execute_process(
  COMMAND "${CMAKE_COMMAND}" -E env "HOME=${RA_DIR}/home" "CFFIXED_USER_HOME=${RA_DIR}/home"
    "${retroarch}" --verbose --config "${RA_DIR}/test.cfg" -L "${core}"
    "--max-frames=${FRAMES}" --max-frames-ss "--max-frames-ss-path=${SHOT}"
    ${ARGS} ${content}
  RESULT_VARIABLE result
  OUTPUT_VARIABLE log
  ERROR_VARIABLE log
  TIMEOUT 50)
file(WRITE "${RA_DIR}/retroarch.log" "${log}")
if(NOT result STREQUAL "0")
  message(FATAL_ERROR "RetroArch exited with '${result}'. Its log:\n${log}")
endif()
if(NOT EXISTS "${SHOT}")
  message(FATAL_ERROR "RetroArch wrote no screenshot at ${SHOT}. Its log:\n${log}")
endif()
message(STATUS "RetroArch ran ${FRAMES} frames and wrote ${SHOT}")
