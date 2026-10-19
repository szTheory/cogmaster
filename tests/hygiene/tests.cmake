# Hygiene tests, registered only by the hygiene preset (COGMASTER_HYGIENE).
set(hygiene_scripts ${CMAKE_CURRENT_LIST_DIR}/../cmake)

# Fault prevented: a personal path or address, an unlisted binary, a game
# image or GPL text in any file git would track, ignored build output aside.
cogmaster_add_test(NAME hygiene.tree
  COMMAND ${COGMASTER_SH} ${CMAKE_SOURCE_DIR}/scripts/hygiene.sh --tree
  LABELS hygiene TIMEOUT 60)

# Fault prevented: a tree scan that passes whatever the tree holds. A
# throwaway repository holds a copy of scripts/hygiene.sh and one untracked,
# not ignored file whose text is a home directory path, which must be named.
# The path is built in pieces, so this file never holds one and
# hygiene.tree stays green.
set(hygiene_tree ${CMAKE_BINARY_DIR}/hygiene/tree_fixture)
file(REMOVE_RECURSE ${hygiene_tree})
file(MAKE_DIRECTORY ${hygiene_tree}/scripts)
file(COPY ${CMAKE_SOURCE_DIR}/scripts/hygiene.sh DESTINATION ${hygiene_tree}/scripts)
set(hygiene_home "/Use")
string(APPEND hygiene_home "rs/someone/projects/cogmaster")
file(WRITE ${hygiene_tree}/notes.txt "built in ${hygiene_home}\n")
execute_process(COMMAND ${GIT_EXECUTABLE} init -q ${hygiene_tree} RESULT_VARIABLE hygiene_rc)
if(NOT hygiene_rc EQUAL 0)
  message(FATAL_ERROR "hygiene: git init ${hygiene_tree} failed")
endif()
set(hygiene_cmd ${COGMASTER_SH} -c "cd \"$1\" && sh scripts/hygiene.sh --tree" sh ${hygiene_tree})
string(REPLACE ";" "$<SEMICOLON>" hygiene_cmd "${hygiene_cmd}")
cogmaster_add_test(NAME hygiene.tree.self_test
  COMMAND ${CMAKE_COMMAND} "-DCMD=${hygiene_cmd}" -DCODE=1 "-DREGEX=hygiene: home directory path"
    -P ${hygiene_scripts}/expect_fail.cmake
  LABELS hygiene TIMEOUT 60)

# Fault prevented: code that drifts from .clang-format. Every tracked C file
# but the vendored libretro.h, which is never edited.
cogmaster_add_test(NAME hygiene.format
  COMMAND ${COGMASTER_SH} -c "cd \"$1\" && \"$2\" ls-files -z '*.c' '*.h' \
':!:libretro/libretro.h' | xargs -0 \"$3\" --dry-run --Werror" sh
    ${CMAKE_SOURCE_DIR} ${GIT_EXECUTABLE} ${COGMASTER_CLANG_FORMAT}
  LABELS hygiene TIMEOUT 60)

# Fault prevented: floating point in the core's text. The self test scans a
# fixture with six float lines beside two lines that only look like them.
cogmaster_add_test(NAME hygiene.no_float
  COMMAND ${CMAKE_COMMAND} "-DROOTS=${CMAKE_SOURCE_DIR}/src$<SEMICOLON>${CMAKE_SOURCE_DIR}/include"
    -P ${hygiene_scripts}/no_float.cmake
  LABELS hygiene TIMEOUT 30)
set(hygiene_fixture ${CMAKE_BINARY_DIR}/hygiene/no_float_fixture)
file(WRITE ${hygiene_fixture}/sample.c "int y = 0x1E5;\nint double_free = 2;\nx = 1.5;\n"
  "z = 0x1p3;\nw = 1e3;\ndouble d;\n#include <math.h>\n#include <float.h>\n")
set(hygiene_cmd ${CMAKE_COMMAND} -DROOTS=${hygiene_fixture} -P ${hygiene_scripts}/no_float.cmake)
string(REPLACE ";" "$<SEMICOLON>" hygiene_cmd "${hygiene_cmd}")
cogmaster_add_test(NAME hygiene.no_float.self_test
  COMMAND ${CMAKE_COMMAND} "-DCMD=${hygiene_cmd}" -DCODE=1
    "-DREGEX=sample.c:3: x = 1.5.*sample.c:8: #include <float.h>.*no_float: 6 lines"
    -P ${hygiene_scripts}/expect_fail.cmake
  LABELS hygiene TIMEOUT 30)

# Fault prevented: a defect the clang static analyzer can see in the core.
# The Annex K check is off: the core may call only the C memory functions
# (abi.undefined), and the _s variants it asks for are optional in C11 and
# absent from glibc.
cogmaster_add_test(NAME hygiene.analyzer
  COMMAND ${COGMASTER_SH} -c "cd \"$1\" && \"$2\" -p \"$3\" --quiet \
-checks='-*,clang-analyzer-*,-clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling' \
--warnings-as-errors='*' src/*.c" sh
    ${CMAKE_SOURCE_DIR} ${COGMASTER_CLANG_TIDY} ${CMAKE_BINARY_DIR}
  LABELS hygiene TIMEOUT 120)

# Fault prevented: a pins.txt row that scripts/fetch-pin.sh would misread,
# or that downloads over plain HTTP or with a truncated digest. The self test
# runs scripts/fetch-pin.sh on a row whose digest is wrong for its file,
# which must stop before DEST is touched.
cogmaster_add_test(NAME policy.pins
  COMMAND ${CMAKE_COMMAND} -DPINS=${CMAKE_SOURCE_DIR}/pins.txt -P ${hygiene_scripts}/pins.cmake
  LABELS hygiene policy TIMEOUT 10)
set(hygiene_pins ${CMAKE_BINARY_DIR}/hygiene/pins_fixture)
file(REMOVE_RECURSE ${hygiene_pins})
file(WRITE ${hygiene_pins}/payload.txt "not the pinned bytes\n")
string(REPEAT "0" 64 hygiene_zeros)
file(WRITE ${hygiene_pins}/pins.txt "payload 1 file://${hygiene_pins}/payload.txt ${hygiene_zeros}\n")
set(hygiene_check [=[
PINS=$1 sh "$2" payload "$3"
rc=$?
if [ -e "$3" ]
then
  echo "policy.pins.self_test: $3 was created before the digest matched"
  exit 3
fi
exit "$rc"
]=])
set(hygiene_cmd ${COGMASTER_SH} -c "${hygiene_check}" sh ${hygiene_pins}/pins.txt
  ${CMAKE_SOURCE_DIR}/scripts/fetch-pin.sh ${hygiene_pins}/dest)
string(REPLACE ";" "$<SEMICOLON>" hygiene_cmd "${hygiene_cmd}")
cogmaster_add_test(NAME policy.pins.self_test
  COMMAND ${CMAKE_COMMAND} "-DCMD=${hygiene_cmd}" -DCODE=1 "-DREGEX=payload: sha256 mismatch"
    -P ${hygiene_scripts}/expect_fail.cmake
  LABELS hygiene policy TIMEOUT 30)
