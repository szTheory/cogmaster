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
  src/crc32.c
  src/fault.c
  src/instance.c
  src/frame.c
  src/state.c
  src/testcard.c)
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
    runner/bench.c
    runner/clock.c
    runner/crc32.c
    runner/png.c
    runner/rss.c
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

# Tests. COGMASTER_TIMEOUT_SCALE multiplies every test's timeout on slow
# lanes; COGMASTER_INVENTORY names the tests/inventory/<name>.txt the
# registered tests must equal.
set(COGMASTER_TIMEOUT_SCALE 1 CACHE STRING "Multiplier for every test timeout")
set(COGMASTER_INVENTORY ci CACHE STRING "Test inventory file under tests/inventory")
if(COGMASTER_BUILD_TESTS)
  if(NOT COGMASTER_BUILD_RUNNER)
    message(FATAL_ERROR "COGMASTER_BUILD_TESTS needs COGMASTER_BUILD_RUNNER: the tests run cogmaster-run")
  endif()
  enable_testing()
  include(cmake/CogmasterTests.cmake)
  include(tests/tests.cmake)
endif()
