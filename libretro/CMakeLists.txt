# The libretro core: a module RetroArch loads at run time. Hidden visibility
# here and in the static libraries it links leaves only the retro_*
# functions, which libretro.h marks with default visibility or dllexport, in
# the export table.
function(cogmaster_libretro_module name)
  set(source libretro.c)
  if(ARGC GREATER 1)
    set(source ${ARGV1})
  endif()
  add_library(${name} MODULE ${source})
  target_include_directories(${name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
  target_link_libraries(${name} PRIVATE cogmaster::cogmaster cogmaster-host)
  target_compile_definitions(${name} PRIVATE COGMASTER_LIBRETRO_VERSION="${PROJECT_VERSION}")
  # No "lib" prefix: the frontend and the release name it cogmaster_libretro.
  # CMake gives a MODULE the suffix .so on macOS; RetroArch expects .dylib.
  set_target_properties(${name} PROPERTIES
    PREFIX ""
    C_VISIBILITY_PRESET hidden)
  if(APPLE)
    set_target_properties(${name} PROPERTIES SUFFIX ".dylib")
  endif()
  cogmaster_warnings(${name})
endfunction()

cogmaster_libretro_module(cogmaster_libretro)
if(COGMASTER_BUILD_TESTS)
  # Test only, never installed or packaged. The RGB565 build proves the
  # 16-bit path and the single-sample audio path; the GG build shows the
  # Game Gear card with no content, so its geometry and crop go through the
  # adapter without a user-facing option.
  cogmaster_libretro_module(cogmaster_libretro_rgb565)
  target_compile_definitions(cogmaster_libretro_rgb565 PRIVATE COGMASTER_LIBRETRO_RGB565)
  cogmaster_libretro_module(cogmaster_libretro_gg)
  target_compile_definitions(cogmaster_libretro_gg PRIVATE COGMASTER_LIBRETRO_TEST_GG)
  cogmaster_libretro_module(cogmaster_libretro_replay ${PROJECT_SOURCE_DIR}/tests/retroarch/checkpoint.c)
endif()

# Core information for RetroArch's info directory, with display_version
# taken from version.txt.
configure_file(cogmaster_libretro.info.in
  ${CMAKE_CURRENT_BINARY_DIR}/cogmaster_libretro.info @ONLY)

# RetroArch's layout: the module in cores/, its information file in info/.
# RUNTIME covers the Windows DLL.
install(TARGETS cogmaster_libretro
  LIBRARY DESTINATION cores COMPONENT libretro
  RUNTIME DESTINATION cores COMPONENT libretro)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/cogmaster_libretro.info
  DESTINATION info COMPONENT libretro)
