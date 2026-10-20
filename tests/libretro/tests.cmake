# The libretro core: its vendored header, its .info claims, and the core
# called in-process as a frontend calls it.
set(libretro_scripts ${CMAKE_CURRENT_LIST_DIR}/../cmake)

# Fault prevented: the vendored libretro.h edited in place, so the core no
# longer builds against the API its notice and digest describe.
cogmaster_add_test(NAME libretro.vendored
  COMMAND ${CMAKE_COMMAND} -DCHECK=vendored -P ${libretro_scripts}/repository.cmake
  LABELS libretro TIMEOUT 10)

if(TARGET cogmaster_libretro)
  add_executable(cogmaster-test-screenshot ${PROJECT_SOURCE_DIR}/tests/retroarch/compare.c
    ${PROJECT_SOURCE_DIR}/tests/retroarch/bmp.c)
  add_executable(cogmaster-test-screenshot-fixture ${PROJECT_SOURCE_DIR}/tests/retroarch/oracle.c)
  cogmaster_warnings(cogmaster-test-screenshot)
  cogmaster_warnings(cogmaster-test-screenshot-fixture)
  set(screenshot ${PROJECT_BINARY_DIR}/tests/oracle.bmp)
  cogmaster_add_test(NAME libretro.screenshot.fixture
    COMMAND cogmaster-test-screenshot-fixture ${screenshot}
    LABELS libretro portable TIMEOUT 10)
  # Fault prevented: agreeing images hiding a wrong counter or scroll.
  set(screenshot_cmd ${CMAKE_COMMAND} -E env EXPECT_SCROLL=2
    ${CMAKE_CROSSCOMPILING_EMULATOR} $<TARGET_FILE:cogmaster-test-screenshot> ${screenshot} ${screenshot})
  string(REPLACE ";" "$<SEMICOLON>" screenshot_cmd "${screenshot_cmd}")
  cogmaster_add_test(NAME libretro.screenshot.oracle_control
    COMMAND ${CMAKE_COMMAND} "-DCMD=${screenshot_cmd}" -DCODE=1 "-DREGEX=oracle.*difference"
      -P ${PROJECT_SOURCE_DIR}/tests/cmake/expect_fail.cmake
    LABELS libretro portable TIMEOUT 10)
  set_tests_properties(libretro.screenshot.oracle_control PROPERTIES DEPENDS libretro.screenshot.fixture)
  cogmaster_add_test(NAME libretro.screenshot.oracle
    COMMAND ${CMAKE_COMMAND} -E env EXPECT_SCROLL=0 ${CMAKE_CROSSCOMPILING_EMULATOR}
      $<TARGET_FILE:cogmaster-test-screenshot> ${screenshot} ${screenshot}
    LABELS libretro portable TIMEOUT 10)
  set_tests_properties(libretro.screenshot.oracle PROPERTIES DEPENDS libretro.screenshot.fixture)
  cogmaster_add_test(NAME libretro.screenshot.pixel_control
    COMMAND ${CMAKE_COMMAND} -DCOMPARE=$<TARGET_FILE:cogmaster-test-screenshot>
      -DEXPECTED=${screenshot} -DBMP=${screenshot}
      "-DEMULATOR=${CMAKE_CROSSCOMPILING_EMULATOR}"
      -P ${PROJECT_SOURCE_DIR}/tests/retroarch/flip.cmake
    LABELS libretro portable TIMEOUT 10)
  set_tests_properties(libretro.screenshot.pixel_control PROPERTIES DEPENDS libretro.screenshot.fixture)
  # The in-process frontend. It hashes the delivered frame with the runner's
  # SHA-256 and compares it with the same pinned card hash the runner's test
  # uses.
  add_executable(cogmaster-test-libretro-host
    ${CMAKE_CURRENT_LIST_DIR}/host.c ${PROJECT_SOURCE_DIR}/runner/sha256.c)
  target_include_directories(cogmaster-test-libretro-host PRIVATE
    ${PROJECT_SOURCE_DIR}/libretro ${PROJECT_SOURCE_DIR}/runner)
  target_link_libraries(cogmaster-test-libretro-host PRIVATE
    cogmaster::cogmaster cogmaster-host ${CMAKE_DL_LIBS})
  set_target_properties(cogmaster-test-libretro-host PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
  cogmaster_warnings(cogmaster-test-libretro-host)
  foreach(format xrgb8888 rgb565)
    set(module cogmaster_libretro)
    if(format STREQUAL "rgb565")
      set(module cogmaster_libretro_rgb565)
    endif()
    cogmaster_add_test(NAME libretro.presentation.${format}
      COMMAND cogmaster-test-libretro-host $<TARGET_FILE:${module}> presentation ${format}
      LABELS libretro portable TIMEOUT 60)
  endforeach()
  if(COGMASTER_FIXTURES)
    # Fault prevented: the installed adapter refusing content or polling after a frame starts.
    foreach(adapter content replay)
      set(module cogmaster_libretro)
      if(adapter STREQUAL "replay")
        set(module cogmaster_libretro_replay)
      endif()
    cogmaster_add_test(NAME libretro.host.${adapter}
      COMMAND ${CMAKE_COMMAND} -DCASE=play -DNAME=boot_to_play
        -DINPUT=${PROJECT_BINARY_DIR}/tests/play.input
        -DIMAGE=${PROJECT_BINARY_DIR}/fixtures/boot_to_play.sms
        -DEXPECTED=${PROJECT_SOURCE_DIR}/tests/built/expected.txt
        -DORACLE=$<TARGET_FILE:cogmaster-test-play> -DRUN=$<TARGET_FILE:cogmaster-run>
        -DHOST=$<TARGET_FILE:cogmaster-test-libretro-host> -DMODULE=$<TARGET_FILE:${module}>
        "-DEMULATOR=${CMAKE_CROSSCOMPILING_EMULATOR}"
        -P ${PROJECT_SOURCE_DIR}/tests/fixtures/tests.cmake
      LABELS libretro portable TIMEOUT 60)
    endforeach()
  endif()
  # Fault prevented: the core RetroArch loads shows a frame, rate, geometry
  # or audio length other than the library's, or needs content to start.
  # The arguments are the pinned hash, the crop and the display aspect.
  cogmaster_add_test(NAME libretro.host.xrgb8888
    COMMAND cogmaster-test-libretro-host $<TARGET_FILE:cogmaster_libretro> xrgb8888
      ${COGMASTER_TESTCARD_SHA256_SMS} 256 192 32 21
    LABELS libretro portable TIMEOUT 60)
  cogmaster_add_test(NAME libretro.host.rgb565
    COMMAND cogmaster-test-libretro-host $<TARGET_FILE:cogmaster_libretro_rgb565> rgb565
      ${COGMASTER_TESTCARD_SHA256_SMS} 256 192 32 21
    LABELS libretro portable TIMEOUT 60)
  # The Game Gear crop, 160 x 144 at a 6:5 pixel, shows at 4:3.
  cogmaster_add_test(NAME libretro.host.gg
    COMMAND cogmaster-test-libretro-host $<TARGET_FILE:cogmaster_libretro_gg> xrgb8888
      ${COGMASTER_TESTCARD_SHA256_GG} 160 144 4 3
    LABELS libretro portable TIMEOUT 60)

  # The .info claims only what a test proves.
  set(libretro_inventory ${PROJECT_SOURCE_DIR}/tests/inventory/${COGMASTER_INVENTORY}.txt)
  # Fault prevented: RetroArch turns on a feature the .info claims and no
  # test exercises.
  cogmaster_add_test(NAME libretro.info_claims
    COMMAND ${CMAKE_COMMAND} -DINFO=${PROJECT_BINARY_DIR}/libretro/cogmaster_libretro.info
      -DINVENTORY=${libretro_inventory} -DCHECK=info_claims -P ${libretro_scripts}/repository.cmake
    LABELS libretro TIMEOUT 10)
  # Fault prevented: a claims check that passes whatever the .info says. A
  # .info whose "true" claim maps to no test must fail, naming that claim.
  set(libretro_fixture ${PROJECT_BINARY_DIR}/libretro/unmapped_claim.info)
  file(WRITE ${libretro_fixture} "supports_no_game = \"true\"\nsavestate = \"true\"\n")
  set(libretro_cmd ${CMAKE_COMMAND} -DINFO=${libretro_fixture}
    -DINVENTORY=${libretro_inventory} -DCHECK=info_claims -P ${libretro_scripts}/repository.cmake)
  string(REPLACE ";" "$<SEMICOLON>" libretro_cmd "${libretro_cmd}")
  cogmaster_add_test(NAME libretro.info_claims.self_test
    COMMAND ${CMAKE_COMMAND} "-DCMD=${libretro_cmd}" -DCODE=1
      "-DREGEX=savestate = .true. maps to no test" -P ${libretro_scripts}/expect_fail.cmake
    LABELS libretro TIMEOUT 10)
endif()
