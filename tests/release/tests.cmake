# Release tests, label release: they read the archives the package step
# writes, so the ci workflows run them in a second test step after packaging
# and the first test step excludes them.
set(release_scripts ${CMAKE_CURRENT_LIST_DIR}/../cmake)
set(release_suffix ${COGMASTER_OS}-${COGMASTER_ARCH})

cogmaster_add_test(NAME release.archive_contents
  COMMAND ${CMAKE_COMMAND} -DDIR=${PROJECT_BINARY_DIR} -DSUFFIX=${release_suffix}
    -DSOURCE_DIR=${PROJECT_SOURCE_DIR} -DBINARY_DIR=${PROJECT_BINARY_DIR}
    -P ${release_scripts}/archive_contents.cmake
  LABELS release TIMEOUT 60)

# The self test checks a libretro zip built without THIRD-PARTY-NOTICES.md
# and expects that file named.
set(release_fixture ${PROJECT_BINARY_DIR}/release/self_test)
set(release_core cogmaster_libretro.so)
if(COGMASTER_OS STREQUAL "macos")
  set(release_core cogmaster_libretro.dylib)
elseif(COGMASTER_OS STREQUAL "windows")
  set(release_core cogmaster_libretro.dll)
endif()
file(REMOVE_RECURSE ${release_fixture})
file(COPY ${PROJECT_SOURCE_DIR}/LICENSE DESTINATION ${release_fixture}/stage)
file(WRITE ${release_fixture}/stage/cores/${release_core} "core\n")
file(WRITE ${release_fixture}/stage/info/cogmaster_libretro.info "info\n")
file(MAKE_DIRECTORY ${release_fixture}/zips)
execute_process(
  COMMAND ${CMAKE_COMMAND} -E tar cf
    ${release_fixture}/zips/cogmaster-libretro-${release_suffix}.zip --format=zip
    LICENSE cores info
  WORKING_DIRECTORY ${release_fixture}/stage
  COMMAND_ERROR_IS_FATAL ANY)
set(release_cmd ${CMAKE_COMMAND} -DDIR=${release_fixture}/zips -DSUFFIX=${release_suffix}
  -DSOURCE_DIR=${PROJECT_SOURCE_DIR} -DBINARY_DIR=${release_fixture} -DCOMPONENTS=libretro
  -P ${release_scripts}/archive_contents.cmake)
string(REPLACE ";" "$<SEMICOLON>" release_cmd "${release_cmd}")
cogmaster_add_test(NAME release.archive_contents.self_test
  COMMAND ${CMAKE_COMMAND} "-DCMD=${release_cmd}" -DCODE=1
    "-DREGEX=lacks: THIRD-PARTY-NOTICES.md" -P ${release_scripts}/expect_fail.cmake
  LABELS release TIMEOUT 10)

# The floors each binary reads as, from its own extraction of the zips. The
# self test passes an impossible floor for this leg's system and expects it
# named.
set(release_floor_cmd ${CMAKE_COMMAND} -DDIR=${PROJECT_BINARY_DIR} -DSUFFIX=${release_suffix})
cogmaster_add_test(NAME release.floor
  COMMAND ${release_floor_cmd} -DWORK=${PROJECT_BINARY_DIR}/release/floor
    -P ${release_scripts}/floor.cmake
  LABELS release TIMEOUT 60)
set(release_floor 9.0)
set(release_regex "minimum OS [0-9.]+ is above the floor 9.0")
if(COGMASTER_OS STREQUAL "linux")
  set(release_floor 2.0)
  set(release_regex "GLIBC_[0-9.]+ is above the floor 2.0")
elseif(COGMASTER_OS STREQUAL "windows")
  set(release_floor "")
  set(release_regex "imports KERNEL32.dll, outside the allowlist")
endif()
set(release_cmd ${release_floor_cmd} -DWORK=${PROJECT_BINARY_DIR}/release/floor_self_test
  -DFLOOR_OVERRIDE=${release_floor} -P ${release_scripts}/floor.cmake)
string(REPLACE ";" "$<SEMICOLON>" release_cmd "${release_cmd}")
cogmaster_add_test(NAME release.floor.self_test
  COMMAND ${CMAKE_COMMAND} "-DCMD=${release_cmd}" -DCODE=1
    "-DREGEX=${release_regex}" -P ${release_scripts}/expect_fail.cmake
  LABELS release TIMEOUT 60)
