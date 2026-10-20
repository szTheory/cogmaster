cmake_minimum_required(VERSION 3.25)

# version.txt is the single version source; release-please edits only it.
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/version.txt" COGMASTER_VERSION LIMIT_COUNT 1)
if(NOT COGMASTER_VERSION MATCHES "^[0-9]+\\.[0-9]+\\.[0-9]+$")
  message(FATAL_ERROR "version.txt must hold MAJOR.MINOR.PATCH, not '${COGMASTER_VERSION}'")
endif()
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS version.txt)

# Only C is needed to build the library. A project that embeds this one with
# add_subdirectory gets the library and the cogmaster::cogmaster alias; the
# runner and the tests are built by default only when this is the top-level
# project.
project(cogmaster VERSION ${COGMASTER_VERSION} LANGUAGES C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

option(COGMASTER_BUILD_RUNNER "Build the cogmaster-run command" ${PROJECT_IS_TOP_LEVEL})
option(COGMASTER_BUILD_TESTS "Register the tests" ${PROJECT_IS_TOP_LEVEL})
# The libretro core is a module the frontend loads at run time; WASI has no
# dynamic loading, so it is off there.
set(cogmaster_libretro_default ${PROJECT_IS_TOP_LEVEL})
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  set(cogmaster_libretro_default OFF)
endif()
option(COGMASTER_BUILD_LIBRETRO "Build the libretro core" ${cogmaster_libretro_default})

# The warning set, applied only when this is the top-level project so an
# embedder's older or console compiler never meets an unknown flag. Presets
# make warnings fatal through CMAKE_COMPILE_WARNING_AS_ERROR.
function(cogmaster_warnings target)
  if(PROJECT_IS_TOP_LEVEL)
    # Keep ISO C stdio on MSVC; its optional *_s alternatives are not portable.
    target_compile_definitions(${target} PRIVATE $<$<C_COMPILER_ID:MSVC>:_CRT_SECURE_NO_WARNINGS>)
    target_compile_options(${target} PRIVATE
      $<$<C_COMPILER_ID:GNU,Clang,AppleClang>:-Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion -Wstrict-prototypes -Wmissing-prototypes -Wvla -Wundef -Wcast-qual -Wimplicit-fallthrough -Wmissing-variable-declarations>
      $<$<C_COMPILER_ID:GNU>:-Wduplicated-cond -Wlogical-op>
      $<$<C_COMPILER_ID:MSVC>:/W4 /utf-8>)
  endif()
endfunction()

# The core library. It links nothing beyond memcpy, memmove, memset and
# memcmp: no stack protector or fortify helpers, no MSVC security cookie and
# no default runtime library, and no build path recorded in the objects.
add_library(cogmaster STATIC
  src/boot.c
  src/bus.c
  src/cart.c
  src/battery.c
  src/cartdb.c
  src/sha1.c
  src/crc32.c
  src/fault.c
  src/instance.c
  src/io.c
  src/frame.c
  src/mapper.c
  src/state.c
  src/testcard.c
  src/vdp.c
  src/vdp_render.c
  src/z80.c
  src/z80_cb.c
  src/z80_ed.c
  src/z80_x3.c)
add_library(cogmaster::cogmaster ALIAS cogmaster)
target_include_directories(cogmaster PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/generated>
  $<INSTALL_INTERFACE:include>)
set_target_properties(cogmaster PROPERTIES
  C_VISIBILITY_PRESET hidden
  POSITION_INDEPENDENT_CODE ON)
target_compile_options(cogmaster PRIVATE
  $<$<C_COMPILER_ID:GNU,Clang,AppleClang>:-fno-stack-protector -U_FORTIFY_SOURCE -ffile-prefix-map=${CMAKE_CURRENT_SOURCE_DIR}=.>
  $<$<C_COMPILER_ID:MSVC>:/GS- /Zl>)
cogmaster_warnings(cogmaster)

# The library version reaches the public header through a generated header,
# so version.txt stays the only file that holds it.
configure_file(cmake/version.h.in ${PROJECT_BINARY_DIR}/generated/cogmaster_version.h @ONLY)

# The library component: both headers, the static library, a CMake package
# exporting cogmaster::cogmaster and a pkg-config file. Before 1.0 a minor
# version may break the API, so the package accepts a request only from the
# same MAJOR.MINOR. Both files locate the prefix from their own directory, so
# an extracted archive works wherever it lands. An embedding project installs
# nothing of ours.
if(PROJECT_IS_TOP_LEVEL)
  include(CMakePackageConfigHelpers)
  set(COGMASTER_CMAKE_DIR lib/cmake/cogmaster)
  install(TARGETS cogmaster EXPORT cogmasterTargets
    ARCHIVE DESTINATION lib COMPONENT library)
  install(FILES include/cogmaster.h ${PROJECT_BINARY_DIR}/generated/cogmaster_version.h
    DESTINATION include COMPONENT library)
  install(EXPORT cogmasterTargets NAMESPACE cogmaster::
    DESTINATION ${COGMASTER_CMAKE_DIR} COMPONENT library)
  configure_package_config_file(cmake/cogmasterConfig.cmake.in
    ${PROJECT_BINARY_DIR}/cogmasterConfig.cmake
    INSTALL_DESTINATION ${COGMASTER_CMAKE_DIR})
  write_basic_package_version_file(${PROJECT_BINARY_DIR}/cogmasterConfigVersion.cmake
    COMPATIBILITY SameMinorVersion)
  configure_file(cmake/cogmaster.pc.in ${PROJECT_BINARY_DIR}/cogmaster.pc @ONLY)
  install(FILES
      ${PROJECT_BINARY_DIR}/cogmasterConfig.cmake
      ${PROJECT_BINARY_DIR}/cogmasterConfigVersion.cmake
    DESTINATION ${COGMASTER_CMAKE_DIR} COMPONENT library)
  install(FILES ${PROJECT_BINARY_DIR}/cogmaster.pc
    DESTINATION lib/pkgconfig COMPONENT library)
endif()

# The headless runner, built as ISO C17. It lands at the top of the build
# directory: build/<preset>/cogmaster-run.
# Host helpers shared by the runner, the libretro core and the tests. They
# are outside the core's determinism boundary: nothing here is hashed. Built
# position-independent and hidden, as the libretro module links them.
if(COGMASTER_BUILD_RUNNER OR COGMASTER_BUILD_LIBRETRO)
  add_library(cogmaster-host STATIC host/palette.c)
  target_include_directories(cogmaster-host PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/host)
  set_target_properties(cogmaster-host PROPERTIES
    C_VISIBILITY_PRESET hidden
    POSITION_INDEPENDENT_CODE ON)
  cogmaster_warnings(cogmaster-host)
endif()

if(COGMASTER_BUILD_RUNNER)
  # The runner's sources, also built into the tests' signal-hook runner.
  # _POSIX_C_SOURCE makes clock_gettime and getrusage visible under -std=c17
  # without other extensions.
  set(COGMASTER_RUNNER_SOURCES
    runner/main.c
    runner/args.c
    runner/input.c
    runner/bench.c
    runner/clock.c
    runner/crc32.c
    runner/mailbox.c
    runner/md5.c
    runner/png.c
    runner/probe.c
    runner/rss.c
    runner/save.c
    runner/sha1.c
    runner/sha256.c
    runner/stop.c)
  list(TRANSFORM COGMASTER_RUNNER_SOURCES PREPEND ${CMAKE_CURRENT_SOURCE_DIR}/)
  add_executable(cogmaster-run ${COGMASTER_RUNNER_SOURCES})
  target_link_libraries(cogmaster-run PRIVATE cogmaster::cogmaster cogmaster-host)
  target_compile_definitions(cogmaster-run PRIVATE _POSIX_C_SOURCE=200809L)
  cogmaster_warnings(cogmaster-run)
  install(TARGETS cogmaster-run RUNTIME DESTINATION bin COMPONENT runner)
endif()

# The libretro core: build/<preset>/libretro/cogmaster_libretro.<ext>.
if(COGMASTER_BUILD_LIBRETRO)
  add_subdirectory(libretro)
endif()

# Release archives, one per install component.
if(PROJECT_IS_TOP_LEVEL)
  include(cmake/packaging.cmake)
endif()

# The hygiene lane registers only the repository checks (formatting, the
# integer-only text scan, the analyzer, scripts/hygiene.sh and the policy
# scripts) and finds its own tools, so no one sets PATH: clang-format and
# clang-tidy 21 by their Debian names, in Homebrew's keg-only llvm@21 or in
# Ubuntu's llvm-21 directory, then on PATH. A missing tool fails configure
# naming it; no test skips itself.
option(COGMASTER_HYGIENE "Register only the hygiene tests" OFF)
if(COGMASTER_HYGIENE)
  set(hints /opt/homebrew/opt/llvm@21/bin /usr/local/opt/llvm@21/bin /usr/lib/llvm-21/bin)
  find_program(COGMASTER_CLANG_FORMAT NAMES clang-format-21 clang-format HINTS ${hints} REQUIRED)
  find_program(COGMASTER_CLANG_TIDY NAMES clang-tidy-21 clang-tidy HINTS ${hints} REQUIRED)
  foreach(tool IN ITEMS COGMASTER_CLANG_FORMAT COGMASTER_CLANG_TIDY)
    execute_process(COMMAND ${${tool}} --version OUTPUT_VARIABLE version ERROR_QUIET)
    if(NOT version MATCHES "version 21\\.")
      string(REGEX MATCH "version [0-9.]+" found "${version}")
      message(FATAL_ERROR "hygiene: ${${tool}} is ${found}, not 21 "
        "(macOS: brew install llvm@21; Ubuntu: apt install clang-format-21 clang-tidy-21)")
    endif()
  endforeach()
  find_program(COGMASTER_JQ jq REQUIRED)
  find_program(COGMASTER_SH sh REQUIRED)
  find_package(Git REQUIRED)
endif()

# The nofp lane proves the core integer-only by machine code, not text: GCC's
# -mgeneral-regs-only rejects any floating-point type in the core and in the
# abi fixtures, which copy the core's options. It registers only the abi tests.
option(COGMASTER_NOFP "Build the core with general registers only; register only the abi tests" OFF)
if(COGMASTER_NOFP)
  if(NOT CMAKE_C_COMPILER_ID STREQUAL "GNU")
    message(FATAL_ERROR "nofp: -mgeneral-regs-only needs GCC, not ${CMAKE_C_COMPILER_ID}")
  endif()
  target_compile_options(cogmaster PRIVATE -mgeneral-regs-only)
endif()

# The fuzz lane builds the two libFuzzer targets and registers only their
# timed runs. The preset compiles everything with
# -fsanitize=fuzzer-no-link,address,undefined; only the two targets link the
# libFuzzer runtime, so no other executable gets its main.
option(COGMASTER_LIBFUZZER "Build the libFuzzer targets; register only the fuzz runs" OFF)

# The nightly vectors lane converts all 1,000 tests of every upstream file
# from the fetched archive and registers only the full shards and the sample
# equality check.
option(COGMASTER_VECTORS_FULL "Convert the fetched z80 vectors; register only the full shards" OFF)
# Fault prevented: a nightly run that reports green after converting nothing
# because the fetched tree is missing.
if(COGMASTER_VECTORS_FULL AND NOT EXISTS "${PROJECT_SOURCE_DIR}/build/pins/sst-z80/v1")
  message(FATAL_ERROR "vectors-full: build/pins/sst-z80/v1 is missing; fetch it with\n"
    "  scripts/fetch-pin.sh sst-z80 build/pins/sst-z80")
endif()

# Tests. COGMASTER_TIMEOUT_SCALE multiplies every test's timeout on slow
# lanes; COGMASTER_INVENTORY names the tests/inventory/<name>.txt the
# registered tests must equal.
set(COGMASTER_TIMEOUT_SCALE 1 CACHE STRING "Multiplier for every test timeout")
# The assembled test fixtures: configure builds the pinned WLA-DX and the
# build assembles tests/roms/src into build/<preset>/fixtures.
option(COGMASTER_FIXTURES "Build WLA-DX and the assembled test fixtures" ON)
set(inventory ci)
if(COGMASTER_HYGIENE)
  set(inventory hygiene)
elseif(COGMASTER_NOFP)
  set(inventory nofp)
elseif(COGMASTER_LIBFUZZER)
  set(inventory fuzz)
elseif(COGMASTER_VECTORS_FULL)
  set(inventory vectors-full)
endif()
set(COGMASTER_INVENTORY ${inventory} CACHE STRING "Test inventory file under tests/inventory")
# Fault prevented: every wasm32 test failing on a missing runtime, with a
# message that does not say where the runtime comes from.
if(COGMASTER_BUILD_TESTS AND CMAKE_SYSTEM_NAME STREQUAL "WASI")
  list(GET CMAKE_CROSSCOMPILING_EMULATOR 0 emulator)
  if(NOT EXISTS "${emulator}")
    message(FATAL_ERROR "wasm32: the test runtime '${emulator}' is missing; fetch it with\n"
      "  scripts/fetch-pin.sh wasmtime-<arch>-linux build/pins/wasmtime")
  endif()
endif()
if(COGMASTER_BUILD_TESTS)
  if(NOT COGMASTER_BUILD_RUNNER)
    message(FATAL_ERROR "COGMASTER_BUILD_TESTS needs COGMASTER_BUILD_RUNNER: the tests run cogmaster-run")
  endif()
  enable_testing()
  include(cmake/CogmasterTests.cmake)
  include(tests/tests.cmake)
endif()
