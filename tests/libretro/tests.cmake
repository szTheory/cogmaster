# The libretro core: its vendored header, its .info claims, and the core
# called in-process as a frontend calls it.
set(libretro_scripts ${CMAKE_CURRENT_LIST_DIR}/../cmake)

# Fault prevented: the vendored libretro.h edited in place, so the core no
# longer builds against the API its notice and digest describe.
cogmaster_add_test(NAME libretro.vendored
  COMMAND ${CMAKE_COMMAND} -P ${libretro_scripts}/vendored.cmake
  LABELS libretro TIMEOUT 10)

# The .info claims only what a test proves.
if(TARGET cogmaster_libretro)
  set(libretro_inventory ${PROJECT_SOURCE_DIR}/tests/inventory/${COGMASTER_INVENTORY}.txt)
  # Fault prevented: RetroArch turns on a feature the .info claims and no
  # test exercises.
  cogmaster_add_test(NAME libretro.info_claims
    COMMAND ${CMAKE_COMMAND} -DINFO=${PROJECT_BINARY_DIR}/libretro/cogmaster_libretro.info
      -DINVENTORY=${libretro_inventory} -P ${libretro_scripts}/info_claims.cmake
    LABELS libretro TIMEOUT 10)
  # Fault prevented: a claims check that passes whatever the .info says. A
  # .info whose "true" claim maps to no test must fail, naming that claim.
  set(libretro_fixture ${PROJECT_BINARY_DIR}/libretro/unmapped_claim.info)
  file(WRITE ${libretro_fixture} "supports_no_game = \"true\"\nsavestate = \"true\"\n")
  set(libretro_cmd ${CMAKE_COMMAND} -DINFO=${libretro_fixture}
    -DINVENTORY=${libretro_inventory} -P ${libretro_scripts}/info_claims.cmake)
  string(REPLACE ";" "$<SEMICOLON>" libretro_cmd "${libretro_cmd}")
  cogmaster_add_test(NAME libretro.info_claims.self_test
    COMMAND ${CMAKE_COMMAND} "-DCMD=${libretro_cmd}" -DCODE=1
      "-DREGEX=savestate = .true. maps to no test" -P ${libretro_scripts}/expect_fail.cmake
    LABELS libretro TIMEOUT 10)
endif()
