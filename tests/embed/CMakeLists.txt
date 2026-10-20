# A C-only project that embeds cogmaster with add_subdirectory, as a host
# using FetchContent would, with the tests, the runner and the libretro core
# off. The test embed.subdirectory builds it and runs it. Configuring fails if
# embedding enabled C++, defined any target besides the library and its
# alias, or registered cogmaster's tests.
cmake_minimum_required(VERSION 3.25)
project(embed C)

if(NOT DEFINED COGMASTER_SOURCE_DIR)
  message(FATAL_ERROR "embed: set COGMASTER_SOURCE_DIR")
endif()
set(COGMASTER_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(COGMASTER_BUILD_RUNNER OFF CACHE BOOL "" FORCE)
set(COGMASTER_BUILD_LIBRETRO OFF CACHE BOOL "" FORCE)
add_subdirectory(${COGMASTER_SOURCE_DIR} cogmaster)

get_property(languages GLOBAL PROPERTY ENABLED_LANGUAGES)
if("CXX" IN_LIST languages)
  message(FATAL_ERROR "embed: cogmaster enabled C++ in an embedding project")
endif()
foreach(target IN ITEMS cogmaster-run cogmaster_libretro cogmaster-test-core cogmaster-host)
  if(TARGET ${target})
    message(FATAL_ERROR "embed: cogmaster defined the target ${target}")
  endif()
endforeach()
get_property(targets DIRECTORY ${COGMASTER_SOURCE_DIR} PROPERTY BUILDSYSTEM_TARGETS)
if(NOT targets STREQUAL "cogmaster")
  message(FATAL_ERROR "embed: cogmaster added the targets '${targets}', not 'cogmaster'")
endif()
if(NOT TARGET cogmaster::cogmaster)
  message(FATAL_ERROR "embed: cogmaster did not add the alias cogmaster::cogmaster")
endif()
get_property(tests DIRECTORY ${COGMASTER_SOURCE_DIR} PROPERTY TESTS)
if(tests)
  message(FATAL_ERROR "embed: cogmaster registered tests in an embedding project: ${tests}")
endif()

add_executable(embed ${CMAKE_CURRENT_SOURCE_DIR}/../consumer/main.c)
target_link_libraries(embed PRIVATE cogmaster::cogmaster)
