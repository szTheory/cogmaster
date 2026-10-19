# The libretro core: its vendored header, its .info claims, and the core
# called in-process as a frontend calls it.
set(libretro_scripts ${CMAKE_CURRENT_LIST_DIR}/../cmake)

# Fault prevented: the vendored libretro.h edited in place, so the core no
# longer builds against the API its notice and digest describe.
cogmaster_add_test(NAME libretro.vendored
  COMMAND ${CMAKE_COMMAND} -P ${libretro_scripts}/vendored.cmake
  LABELS libretro TIMEOUT 10)
