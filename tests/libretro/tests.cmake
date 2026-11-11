# The libretro core: its vendored header, its .info claims, and the core
# called in-process as a frontend calls it.
#
# Run as a script (cmake -DCASE=bios ... -P tests.cmake), this file is the BIOS boot check; include()d by
# tests/tests.cmake, it registers the libretro tests.
if(CMAKE_SCRIPT_MODE_FILE)
  # The runner's frame digest of the cartridge at frame K with the given runner options.
  function(runner_digest var cart)
    execute_process(COMMAND ${EMULATOR} ${RUN} ${cart} --frames ${K} --hash ${ARGN}
      RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
    if(NOT rc STREQUAL "0" OR NOT out MATCHES "^frame ${K} sha256 ([0-9a-f]+)\n$")
      message(FATAL_ERROR "cogmaster-run ${cart} ${ARGN}: exit ${rc}\n${out}${err}")
    endif()
    set(${var} ${CMAKE_MATCH_1} PARENT_SCOPE)
  endfunction()
  # One leg: the module under the host with a boot option, a region option and a system folder must
  # deliver digest, ask for the folder calls times and log lines lines holding word.
  function(leg cart digest boot region dir calls lines word)
    execute_process(COMMAND ${EMULATOR} ${HOST} ${MODULE} bios ${cart} ${K} ${digest} ${boot} ${region}
        "${dir}" ${calls} ${lines} "${word}"
      RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
    if(NOT rc STREQUAL "0")
      message(FATAL_ERROR "boot ${boot}, region ${region}, folder ${dir}: exit ${rc}\n${out}${err}")
    endif()
  endfunction()
  # The three file names are the .info's, read from the built file and never typed here.
  file(STRINGS "${INFO}" firmware REGEX "^firmware[0-9]+_path = ")
  set(names "")
  foreach(line IN LISTS firmware)
    string(REGEX REPLACE "^[^\"]*\"([^\"]*)\"$" "\\1" name "${line}")
    list(APPEND names ${name})
  endforeach()
  list(LENGTH names count)
  if(NOT count EQUAL 3)
    message(FATAL_ERROR "${INFO}: expected three firmware paths, got '${names}'")
  endif()
  list(GET names 0 file_U)
  list(GET names 1 file_E)
  list(GET names 2 file_J)
  set(K 1)
  # A file that is not 8192 bytes: three bytes, and one over the read cap.
  string(REPEAT "b" 8192 over)
  file(REMOVE_RECURSE "${OUT}")
  foreach(folder IN ITEMS only_U only_E only_J hostile over empty)
    file(MAKE_DIRECTORY "${OUT}/${folder}")
  endforeach()
  foreach(owner IN ITEMS U E J)
    foreach(region IN ITEMS U E J)
      if(owner STREQUAL region)
        file(COPY_FILE "${BIOS}" "${OUT}/only_${owner}/${file_${region}}")
      else()
        file(CONFIGURE OUTPUT "${OUT}/only_${owner}/${file_${region}}" CONTENT "ab" NEWLINE_STYLE UNIX @ONLY)
      endif()
    endforeach()
  endforeach()
  foreach(region IN ITEMS U E J)
    file(CONFIGURE OUTPUT "${OUT}/hostile/${file_${region}}" CONTENT "ab" NEWLINE_STYLE UNIX @ONLY)
    file(CONFIGURE OUTPUT "${OUT}/over/${file_${region}}" CONTENT "${over}" NEWLINE_STYLE UNIX @ONLY)
  endforeach()
  file(SIZE "${OUT}/over/${file_U}" size)
  file(SIZE "${OUT}/only_E/${file_J}" tiny)
  file(SIZE "${OUT}/only_J/${file_J}" good)
  if(NOT size EQUAL 8193 OR NOT tiny EQUAL 3 OR NOT good EQUAL 8192)
    message(FATAL_ERROR "the test folders hold files of ${size}, ${tiny} and ${good} bytes, not 8193, 3 and 8192")
  endif()
  # What the runner shows: the BIOS frame and the fast frame at K for each region. The two must differ, or
  # a leg that fell back to the fast boot would pass as a BIOS boot.
  set(flag_U ntsc-u)
  set(flag_E pal)
  set(flag_J ntsc-j)
  runner_digest(fast_auto "${CART}")
  runner_digest(bios_auto "${CART}" --boot bios --bios "${BIOS}")
  runner_digest(fast_gg "${GG_CART}")
  foreach(region IN ITEMS U E J)
    runner_digest(fast_${region} "${CART}" --region ${flag_${region}})
    runner_digest(bios_${region} "${CART}" --region ${flag_${region}} --boot bios --bios "${BIOS}")
    if(bios_${region} STREQUAL fast_${region})
      message(FATAL_ERROR "the BIOS and fast frames hash alike at frame ${K} for ${region}")
    endif()
  endforeach()
  if(bios_auto STREQUAL fast_auto)
    message(FATAL_ERROR "the BIOS and fast frames hash alike at frame ${K}")
  endif()
  # The right file gives the runner's BIOS frame with no log line; with the option at its default, or with
  # any other file wrong-sized, the folder is not read, or the file the region names is the one refused.
  leg("${CART}" ${bios_auto} bios auto "${OUT}/only_U" 1 0 -)
  foreach(owner IN ITEMS U E J)
    foreach(region IN ITEMS U E J)
      if(owner STREQUAL region)
        leg("${CART}" ${bios_${region}} bios ${flag_${region}} "${OUT}/only_${owner}" 1 0 -)
      else()
        leg("${CART}" ${fast_${region}} bios ${flag_${region}} "${OUT}/only_${owner}" 1 1 "${file_${region}}")
      endif()
    endforeach()
  endforeach()
  leg("${CART}" ${fast_auto} fast auto "${OUT}/only_U" 0 0 -)
  leg("${CART}" ${fast_auto} fast auto "${OUT}/hostile" 0 0 -)
  leg("${CART}" ${fast_auto} bios auto "${OUT}/empty" 1 1 "${file_U}")
  leg("${CART}" ${fast_auto} bios auto "${OUT}/over" 1 1 "${file_U}")
  leg("${CART}" ${fast_auto} bios auto @null 1 1 "${file_U}")
  leg("${CART}" ${fast_auto} bios auto @empty 1 1 "${file_U}")
  leg("${CART}" ${fast_auto} bios auto @long 1 1 "${file_U}")
  leg("${CART}" ${fast_auto} bios auto "@false=${OUT}/only_U" 1 1 "${file_U}")
  leg("${GG_CART}" ${fast_gg} bios auto "${OUT}/only_U" 0 1 "Game Gear")
  return()
endif()

set(libretro_scripts ${CMAKE_CURRENT_LIST_DIR}/../cmake)

# Fault prevented: the vendored libretro.h edited in place, so the core no
# longer builds against the API its notice and digest describe.
cogmaster_add_test(NAME libretro.vendored
  COMMAND ${CMAKE_COMMAND} -DCHECK=vendored -P ${libretro_scripts}/repository.cmake
  LABELS libretro TIMEOUT 10)

if(TARGET cogmaster_libretro)
  add_executable(cogmaster-test-screenshot ${PROJECT_SOURCE_DIR}/tests/retroarch/compare.c)
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
  # Fault prevented: a comparator accepting a changed pixel at (7,3).
  set(flip_cmd ${CMAKE_CROSSCOMPILING_EMULATOR} $<TARGET_FILE:cogmaster-test-screenshot>
    ${screenshot} ${screenshot} --flip)
  string(REPLACE ";" "$<SEMICOLON>" flip_cmd "${flip_cmd}")
  cogmaster_add_test(NAME libretro.screenshot.pixel_control
    COMMAND ${CMAKE_COMMAND} "-DCMD=${flip_cmd}" -DCODE=1 "-DREGEX=first difference at 7,3:"
      -P ${PROJECT_SOURCE_DIR}/tests/cmake/expect_fail.cmake
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
    cogmaster_add_test(NAME libretro.controls.${format}
      COMMAND cogmaster-test-libretro-host $<TARGET_FILE:${module}> controls ${format}
      LABELS libretro portable TIMEOUT 60)
    cogmaster_add_test(NAME libretro.presentation.${format}
      COMMAND cogmaster-test-libretro-host $<TARGET_FILE:${module}> presentation ${format}
      LABELS libretro portable TIMEOUT 60)
  endforeach()
  if(COGMASTER_FIXTURES)
    foreach(machine sms2 gg)
      # Fault prevented: save memory changes size, disappears on enable or is lost on Restart.
      cogmaster_add_test(NAME libretro.battery.${machine}
        COMMAND cogmaster-test-libretro-host $<TARGET_FILE:cogmaster_libretro>
          battery ${PROJECT_BINARY_DIR}/fixtures/battery.sms ${machine}
        LABELS libretro portable TIMEOUT 60)
    endforeach()
    # Fault prevented: a frontend told saves are supported while the exported span differs from the
    # battery span, so RetroArch writes a file of the wrong size or none.
    foreach(span IN ITEMS "battery|battery.sms|sms2|32768" "eeprom|eeprom.gg|gg|128"
        "plain|boot_to_play.sms|sms2|0")
      string(REPLACE "|" ";" span "${span}")
      list(GET span 0 name)
      list(GET span 1 image)
      list(GET span 2 machine)
      list(GET span 3 length)
      cogmaster_add_test(NAME libretro.span.${name}
        COMMAND cogmaster-test-libretro-host $<TARGET_FILE:cogmaster_libretro>
          span ${PROJECT_BINARY_DIR}/fixtures/${image} ${machine} ${length}
        LABELS libretro portable TIMEOUT 60)
    endforeach()
    # Fault prevented: a span guard that passes whatever length the core exports.
    set(span_cmd ${CMAKE_CROSSCOMPILING_EMULATOR} $<TARGET_FILE:cogmaster-test-libretro-host>
      $<TARGET_FILE:cogmaster_libretro> span ${PROJECT_BINARY_DIR}/fixtures/eeprom.gg gg 127)
    string(REPLACE ";" "$<SEMICOLON>" span_cmd "${span_cmd}")
    cogmaster_add_test(NAME libretro.span.self_test
      COMMAND ${CMAKE_COMMAND} "-DCMD=${span_cmd}" -DCODE=1 "-DREGEX=span mismatch"
        -P ${PROJECT_SOURCE_DIR}/tests/cmake/expect_fail.cmake
      LABELS libretro portable TIMEOUT 60)
    # Fault prevented: the installed adapter refusing content or polling after a frame starts.
    foreach(adapter content replay)
      foreach(machine sms gg)
        set(module cogmaster_libretro)
        if(adapter STREQUAL "replay")
          set(module cogmaster_libretro_replay)
        endif()
        set(suffix "")
        set(name boot_to_play)
        set(input play.input)
        if(machine STREQUAL "gg")
          set(suffix .gg)
          set(name boot_to_play_gg)
          set(input play-gg.input)
        endif()
      cogmaster_add_test(NAME libretro.host.${adapter}${suffix}
        COMMAND ${CMAKE_COMMAND} -DCASE=play -DNAME=${name}
          -DINPUT=${PROJECT_BINARY_DIR}/tests/${input}
          -DIMAGE=${PROJECT_BINARY_DIR}/fixtures/boot_to_play.${machine}
          -DEXPECTED=${PROJECT_SOURCE_DIR}/tests/built/expected.txt
          -DORACLE=$<TARGET_FILE:cogmaster-test-play> -DRUN=$<TARGET_FILE:cogmaster-run>
          -DHOST=$<TARGET_FILE:cogmaster-test-libretro-host> -DMODULE=$<TARGET_FILE:${module}>
          "-DEMULATOR=${CMAKE_CROSSCOMPILING_EMULATOR}"
          -P ${PROJECT_SOURCE_DIR}/tests/fixtures/tests.cmake
        LABELS libretro portable TIMEOUT 60)
      endforeach()
    endforeach()
    # Fault prevented: the core delivers sound the library did not produce, or a different amount of it,
    # when the frontend registers only the batch callback, only the single-sample one, or both.
    foreach(sound IN ITEMS "psg_sweep|sms|300" "psg_sweep|gg|300" "boot_to_play|sms|60"
        "boot_to_play|gg|60")
      string(REPLACE "|" ";" sound "${sound}")
      list(GET sound 0 name)
      list(GET sound 1 machine)
      list(GET sound 2 frames)
      set(image_name ${name})
      if(machine STREQUAL "gg")
        set(name ${name}_gg)
      endif()
      cogmaster_add_test(NAME libretro.host.audio.${image_name}.${machine}
        COMMAND ${CMAKE_COMMAND} -DCASE=audio -DNAME=${name} -DMACHINE=${machine} -DFRAMES=${frames}
          -DIMAGE=${PROJECT_BINARY_DIR}/fixtures/${image_name}.${machine}
          -DEXPECTED=${PROJECT_SOURCE_DIR}/tests/built/expected.txt
          -DRUN=$<TARGET_FILE:cogmaster-run> -DHOST=$<TARGET_FILE:cogmaster-test-libretro-host>
          -DMODULE=$<TARGET_FILE:cogmaster_libretro> "-DEMULATOR=${CMAKE_CROSSCOMPILING_EMULATOR}"
          -P ${PROJECT_SOURCE_DIR}/tests/fixtures/tests.cmake
        LABELS libretro portable TIMEOUT 10)
    endforeach()
    # Fault prevented: the core refusing or mis-paging a Korean board the runner plays: it loads the
    # headerless image with no option set and must show the runner's frame.
    set(korean_frame -DCASE=frame -DNAME=mapper_korean
      -DIMAGE=${PROJECT_BINARY_DIR}/fixtures/mapper_korean.sms
      -DEXPECTED=${PROJECT_SOURCE_DIR}/tests/built/expected.txt
      -DRUN=$<TARGET_FILE:cogmaster-run> -DHOST=$<TARGET_FILE:cogmaster-test-libretro-host>
      -DMODULE=$<TARGET_FILE:cogmaster_libretro>)
    cogmaster_add_test(NAME libretro.host.korean
      COMMAND ${CMAKE_COMMAND} ${korean_frame} "-DEMULATOR=${CMAKE_CROSSCOMPILING_EMULATOR}"
        -P ${PROJECT_SOURCE_DIR}/tests/fixtures/tests.cmake
      LABELS libretro portable TIMEOUT 10)
    # Fault prevented: a frame mode that passes whatever frame the core draws. The same case with a
    # digest of zeros must fail naming the frame digest.
    string(REPLACE ";" "\;" korean_emulator "${CMAKE_CROSSCOMPILING_EMULATOR}")
    set(korean_cmd ${CMAKE_COMMAND} ${korean_frame} "-DEMULATOR=${korean_emulator}" -DWRONG=1
      -P ${PROJECT_SOURCE_DIR}/tests/fixtures/tests.cmake)
    string(REPLACE ";" "$<SEMICOLON>" korean_cmd "${korean_cmd}")
    cogmaster_add_test(NAME libretro.host.korean_control
      COMMAND ${CMAKE_COMMAND} "-DCMD=${korean_cmd}" -DCODE=1 "-DREGEX=frame digest"
        -P ${PROJECT_SOURCE_DIR}/tests/cmake/expect_fail.cmake
      LABELS libretro portable TIMEOUT 10)
    set_tests_properties(libretro.host.korean_control PROPERTIES DEPENDS libretro.host.korean)
    # Fault prevented: a frontend user locked out by a BIOS that is merely present (a header-failing game
    # would lock up), a failed load where a warning and a working fast boot would do, a path or a file name
    # read from anywhere but the one system folder, a Game Gear cartridge sent to a BIOS, and a region that
    # opens another region's file. The file names come from the built .info.
    # Timeout: 20 s floor, provisional until the first hosted run times it.
    string(REPLACE ";" "$<SEMICOLON>" bios_emulator "${CMAKE_CROSSCOMPILING_EMULATOR}")
    cogmaster_add_test(NAME libretro.host.bios
      COMMAND ${CMAKE_COMMAND} -DCASE=bios -DINFO=${PROJECT_BINARY_DIR}/libretro/cogmaster_libretro.info
        -DCART=${COGMASTER_FIXTURES_DIR}/bios_probe.sms -DGG_CART=${COGMASTER_FIXTURES_DIR}/boot_to_play.gg
        -DBIOS=${COGMASTER_FIXTURES_DIR}/bios_shape.bin -DOUT=${PROJECT_BINARY_DIR}/tests/libretro-bios
        -DRUN=$<TARGET_FILE:cogmaster-run> -DHOST=$<TARGET_FILE:cogmaster-test-libretro-host>
        -DMODULE=$<TARGET_FILE:cogmaster_libretro> "-DEMULATOR=${bios_emulator}"
        -P ${CMAKE_CURRENT_LIST_FILE}
      LABELS libretro portable TIMEOUT 20)
    # Fault prevented: a Sports Pad or Paddle title that gets the wrong device, an unbounded motion sum or motion that never reaches
    # the guest, or a non-3-D title held on a picture that never arrives; a Light Phaser pointer
    # mapped without the crop, a crosshair drawn into a hashed or persistent buffer, or a Pause that
    # a gun on port 0 loses.
    foreach(adapter_case IN ITEMS devices.options devices.sources devices.clamp devices.motion
        stereo.alternate stereo.one_eye)
      string(REGEX REPLACE "^[a-z]+\\." "" adapter_name "${adapter_case}")
      # Timeout: 60 s; 2 x 15.76 s, slowest of 1 run, run 42978311844 (be32 lane).
      cogmaster_add_test(NAME libretro.host.${adapter_case}
        COMMAND cogmaster-test-libretro-host $<TARGET_FILE:cogmaster_libretro> adapter ${adapter_name}
          ${PROJECT_BINARY_DIR}/fixtures
        LABELS libretro portable TIMEOUT 60)
    endforeach()
    foreach(adapter_case IN ITEMS gun.aim gun.cursor gun.crosshair)
      string(REGEX REPLACE "^[a-z]+\\." "" adapter_name "${adapter_case}")
      # Timeout: 10 s floor; 2 x 1.72 s (gun.crosshair on be32), slowest of 8 runs, run 43015673654.
      cogmaster_add_test(NAME libretro.host.${adapter_case}
        COMMAND cogmaster-test-libretro-host $<TARGET_FILE:cogmaster_libretro> adapter ${adapter_name}
          ${PROJECT_BINARY_DIR}/fixtures
        LABELS libretro portable TIMEOUT 10)
    endforeach()
    # Fault prevented: the FM option attaching, detaching or refusing the unit differently from the
    # library's setting, or failing the load of Game Gear content.
    cogmaster_add_test(NAME libretro.host.fm_option
      COMMAND ${CMAKE_COMMAND} -DCASE=fm_option -DNAME=fm_sweep
        -DIMAGE=${PROJECT_BINARY_DIR}/fixtures/fm_sweep.sms
        -DGG_IMAGE=${PROJECT_BINARY_DIR}/fixtures/psg_sweep.gg
        -DEXPECTED=${PROJECT_SOURCE_DIR}/tests/built/expected.txt
        -DRUN=$<TARGET_FILE:cogmaster-run> -DHOST=$<TARGET_FILE:cogmaster-test-libretro-host>
        -DMODULE=$<TARGET_FILE:cogmaster_libretro> "-DEMULATOR=${CMAKE_CROSSCOMPILING_EMULATOR}"
        -P ${PROJECT_SOURCE_DIR}/tests/fixtures/tests.cmake
      LABELS libretro portable TIMEOUT 10)
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
  # Fault prevented: a firmware row in the .info that no test backs. A built .info claiming
  # firmware0_opt against an inventory without libretro.host.bios must fail naming the claim.
  set(libretro_firmware_info ${PROJECT_BINARY_DIR}/libretro/firmware_claim.info)
  set(libretro_firmware_inventory ${PROJECT_BINARY_DIR}/libretro/firmware_inventory.txt)
  file(CONFIGURE OUTPUT ${libretro_firmware_info} CONTENT "firmware0_opt = \"true\"\n"
    NEWLINE_STYLE UNIX @ONLY)
  file(CONFIGURE OUTPUT ${libretro_firmware_inventory} CONTENT "libretro.host.content\tlibretro,portable\n"
    NEWLINE_STYLE UNIX @ONLY)
  set(libretro_cmd ${CMAKE_COMMAND} -DINFO=${libretro_firmware_info}
    -DINVENTORY=${libretro_firmware_inventory} -DCHECK=info_claims -P ${libretro_scripts}/repository.cmake)
  string(REPLACE ";" "$<SEMICOLON>" libretro_cmd "${libretro_cmd}")
  cogmaster_add_test(NAME libretro.info_claims.self_test.firmware
    COMMAND ${CMAKE_COMMAND} "-DCMD=${libretro_cmd}" -DCODE=1
      "-DREGEX=firmware_opt needs libretro.host.bios" -P ${libretro_scripts}/expect_fail.cmake
    LABELS libretro TIMEOUT 10)
endif()
