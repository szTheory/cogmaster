# WLA-DX, the assembler the test fixtures are built with: wla-z80 and wlalink,
# built at configure time from the tarball the wla-dx row of pins.txt names.
#
# Fault prevented: fixtures assembled by an unpinned or tampered assembler. The
# tarball must match the row's SHA-256 before it is unpacked. WLA-DX is
# GPL-2.0-or-later and runs only as a tool: its source is unpacked under the
# build tree and never copied into this repository (PROVENANCE.md section 1).
#
# include()d, it sets COGMASTER_WLA_Z80 and COGMASTER_WLALINK. Run as a script
# it only downloads and verifies, which the negative control uses:
#   cmake -DURL=<url> -DSHA256=<hex> -DOUT=<file> -P WlaDx.cmake

# Downloads url to out and fails configure unless its SHA-256 is sha256.
function(cogmaster__wla_download url sha256 out)
  file(DOWNLOAD "${url}" "${out}" EXPECTED_HASH SHA256=${sha256} STATUS status)
  list(GET status 0 code)
  if(NOT code EQUAL 0)
    list(GET status 1 text)
    file(REMOVE "${out}")
    message(FATAL_ERROR "wla-dx: download of ${url} failed: ${text}")
  endif()
endfunction()

if(CMAKE_SCRIPT_MODE_FILE)
  foreach(var URL SHA256 OUT)
    if("${${var}}" STREQUAL "")
      message(FATAL_ERROR "wla-dx: -D${var} is required")
    endif()
  endforeach()
  cogmaster__wla_download("${URL}" "${SHA256}" "${OUT}")
  return()
endif()

file(STRINGS ${PROJECT_SOURCE_DIR}/pins.txt wla_row REGEX "^wla-dx ")
separate_arguments(wla_row UNIX_COMMAND "${wla_row}")
list(LENGTH wla_row wla_n)
if(NOT wla_n EQUAL 4)
  message(FATAL_ERROR "wla-dx: pins.txt needs one row 'wla-dx version url sha256'")
endif()
list(GET wla_row 1 wla_version)
list(GET wla_row 2 COGMASTER_WLA_URL)
list(GET wla_row 3 COGMASTER_WLA_SHA256)

set(wla_dir ${PROJECT_BINARY_DIR}/wla-dx)
set(wla_exe "")
if(CMAKE_HOST_WIN32)
  set(wla_exe .exe)
endif()
# The build script writes its programs to <build>/binaries.
set(COGMASTER_WLA_Z80 ${wla_dir}/build/binaries/wla-z80${wla_exe})
set(COGMASTER_WLALINK ${wla_dir}/build/binaries/wlalink${wla_exe})
set(wla_stamp "${wla_version} ${COGMASTER_WLA_SHA256}")

set(wla_built "")
if(EXISTS ${wla_dir}/pin.txt)
  file(READ ${wla_dir}/pin.txt wla_built)
endif()
if(NOT wla_built STREQUAL wla_stamp OR NOT EXISTS ${COGMASTER_WLA_Z80}
    OR NOT EXISTS ${COGMASTER_WLALINK})
  string(TIMESTAMP wla_t0 "%s" UTC)
  file(REMOVE_RECURSE ${wla_dir}/src ${wla_dir}/build ${wla_dir}/pin.txt)
  cogmaster__wla_download(${COGMASTER_WLA_URL} ${COGMASTER_WLA_SHA256} ${wla_dir}/wla-dx.tar.gz)
  file(ARCHIVE_EXTRACT INPUT ${wla_dir}/wla-dx.tar.gz DESTINATION ${wla_dir}/src)
  # The archive holds one top directory; its name is not relied on.
  file(GLOB wla_top LIST_DIRECTORIES true ${wla_dir}/src/*)
  list(LENGTH wla_top wla_n)
  if(NOT wla_n EQUAL 1 OR NOT IS_DIRECTORY "${wla_top}")
    message(FATAL_ERROR "wla-dx: expected one top directory in the tarball, found: ${wla_top}")
  endif()

  # The tools run on this machine, and the build runs a table generator it
  # builds, so the child build is native: no toolchain file, and none of the
  # cross or sanitizer settings a lane passes through the environment. A
  # native lane hands over its own compilers. Release, because the script's
  # default build type is RelWithDebInfo.
  set(wla_args -S ${wla_top} -B ${wla_dir}/build -G Ninja -DCMAKE_BUILD_TYPE=Release -DGEN_DOC=OFF)
  if(CMAKE_MAKE_PROGRAM)
    list(APPEND wla_args -DCMAKE_MAKE_PROGRAM=${CMAKE_MAKE_PROGRAM})
  endif()
  if(NOT CMAKE_CROSSCOMPILING)
    list(APPEND wla_args -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER})
  endif()
  set(wla_env ${CMAKE_COMMAND} -E env --unset=CC --unset=CXX --unset=CFLAGS --unset=CXXFLAGS
    --unset=LDFLAGS --unset=CMAKE_TOOLCHAIN_FILE)
  foreach(step IN ITEMS configure build)
    if(step STREQUAL "configure")
      set(cmd ${wla_env} ${CMAKE_COMMAND} ${wla_args})
    else()
      set(cmd ${wla_env} ${CMAKE_COMMAND} --build ${wla_dir}/build --target wla-z80 wlalink)
    endif()
    execute_process(COMMAND ${cmd} RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE out)
    if(NOT rc EQUAL 0)
      message(FATAL_ERROR "wla-dx: the ${step} step failed (exit ${rc}):\n${out}")
    endif()
  endforeach()
  foreach(tool IN ITEMS ${COGMASTER_WLA_Z80} ${COGMASTER_WLALINK})
    if(NOT EXISTS ${tool})
      message(FATAL_ERROR "wla-dx: the build did not produce ${tool}")
    endif()
  endforeach()
  file(WRITE ${wla_dir}/pin.txt "${wla_stamp}")
  string(TIMESTAMP wla_t1 "%s" UTC)
  math(EXPR wla_s "${wla_t1} - ${wla_t0}")
  message(STATUS "wla-dx ${wla_version}: built wla-z80 and wlalink in ${wla_s} s")
else()
  message(STATUS "wla-dx ${wla_version}: wla-z80 and wlalink already built from the pin")
endif()
