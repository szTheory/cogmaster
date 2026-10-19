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
install(TARGETS cogmaster ARCHIVE DESTINATION lib COMPONENT library)
install(FILES include/cogmaster.h ${PROJECT_BINARY_DIR}/generated/cogmaster_version.h
  DESTINATION include COMPONENT library)

# The headless runner, built as ISO C17. It lands at the top of the build
# directory: build/<preset>/cogmaster-run.
if(COGMASTER_BUILD_RUNNER)
  # Host helpers shared by the runner, the libretro core and the tests. They
  # are outside the core's determinism boundary: nothing here is hashed.
  add_library(cogmaster-host STATIC host/palette.c)
  target_include_directories(cogmaster-host PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/host)
  cogmaster_warnings(cogmaster-host)

  add_executable(cogmaster-run
    runner/main.c
    runner/crc32.c
    runner/png.c
    runner/sha256.c)
  target_link_libraries(cogmaster-run PRIVATE cogmaster::cogmaster cogmaster-host)
  cogmaster_warnings(cogmaster-run)
  install(TARGETS cogmaster-run RUNTIME DESTINATION bin COMPONENT runner)
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
