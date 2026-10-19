# Every subsystem's tests. Each file is include()d, never add_subdirectory()d,
# so the top-level directory's TESTS property holds every name and the
# inventory check in cmake/CogmasterTests.cmake sees them all.
include(${CMAKE_CURRENT_LIST_DIR}/testcard_pins.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/core/tests.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/host/tests.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/runner/tests.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/perf/tests.cmake)
if(COGMASTER_BUILD_LIBRETRO)
  include(${CMAKE_CURRENT_LIST_DIR}/libretro/tests.cmake)
endif()
