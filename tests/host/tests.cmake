# The host helpers: colour conversion outside the core's hash.
add_executable(cogmaster-test-host ${CMAKE_CURRENT_LIST_DIR}/test_palette.c)
target_link_libraries(cogmaster-test-host PRIVATE cogmaster-host)
set_target_properties(cogmaster-test-host PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
cogmaster_warnings(cogmaster-test-host)

# Fault prevented: a colour conversion that loses or swaps a channel, so the
# runner's PNG or the libretro frame no longer maps back to the core's frame.
cogmaster_add_test(NAME host.palette
  COMMAND cogmaster-test-host palette
  LABELS host portable TIMEOUT 10)

# The red-cyan composition for the 3-D view.
add_executable(cogmaster-test-eyes ${CMAKE_CURRENT_LIST_DIR}/test_eyes.c)
target_link_libraries(cogmaster-test-eyes PRIVATE cogmaster-host)
set_target_properties(cogmaster-test-eyes PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
cogmaster_warnings(cogmaster-test-eyes)

# Fault prevented: an anaglyph that swaps or drops a channel, or writes into a
# picture the core still owns, so the 3-D view is wrong or the frame changes.
# Timeout: 10 s floor; 2 x 0.08 s, slowest of 1 run, run 42978311844.
cogmaster_add_test(NAME host.eyes
  COMMAND cogmaster-test-eyes eyes
  LABELS host portable TIMEOUT 10)

# The pointer-to-aim mapping and the crosshair's dots for the light gun.
add_executable(cogmaster-test-aim ${CMAKE_CURRENT_LIST_DIR}/test_aim.c)
target_link_libraries(cogmaster-test-aim PRIVATE cogmaster-host)
set_target_properties(cogmaster-test-aim PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
cogmaster_warnings(cogmaster-test-aim)

# Fault prevented: a pointer mapped without the crop offset or off by one dot, an
# off-screen pointer clamped onto an edge pixel, or a crosshair written outside
# its points, so a light-gun title is aimed somewhere else than the player points.
# Timeout: 10 s floor; 2 x 0.07 s (be32), slowest of 9 runs, run 43015673654.
cogmaster_add_test(NAME host.aim
  COMMAND cogmaster-test-aim aim
  LABELS host portable TIMEOUT 10)

# The BIOS helper: table lookup, identification by digest and the bounded read.
# Two owned 8,192-byte files are written at configure time with
# file(CONFIGURE) (byte-exact on every host; it ends each file in one LF, so
# the bodies are 8,191 characters) and the first one's SHA-1 is computed by
# CMake, a digest implementation independent of the core's.
set(bios_dir ${PROJECT_BINARY_DIR}/host_bios)
file(MAKE_DIRECTORY ${bios_dir})
string(REPEAT "0123456789abcdef" 511 bios_body_a)
string(APPEND bios_body_a "0123456789abcde")
string(REPEAT "fedcba9876543210" 511 bios_body_b)
string(APPEND bios_body_b "fedcba987654321")
foreach(name IN ITEMS a b)
  set(path ${bios_dir}/owned_${name}.bin)
  file(CONFIGURE OUTPUT ${path} CONTENT "${bios_body_${name}}" NEWLINE_STYLE UNIX @ONLY)
  file(SIZE ${path} bios_size)
  if(NOT bios_size EQUAL 8192)
    message(FATAL_ERROR "host.bios: ${path} is ${bios_size} bytes, not 8192")
  endif()
endforeach()
file(SHA1 ${bios_dir}/owned_a.bin bios_owned_sha1)
add_executable(cogmaster-test-bios-host ${CMAKE_CURRENT_LIST_DIR}/test_bios.c)
target_link_libraries(cogmaster-test-bios-host PRIVATE cogmaster-host cogmaster::cogmaster)
set_target_properties(cogmaster-test-bios-host PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
cogmaster_warnings(cogmaster-test-bios-host)

# Fault prevented: a dump read past the 8,193-byte bound, a digest that names
# the wrong file or accepts the wrong size, a missing path reported as another
# error, or a recognition path that hashes the wrong bytes while the table
# lookup alone still passes.
# Timeout: 20 s floor; 2 x 0.07 s (wasm32), slowest of 9 runs, run 43074158512.
cogmaster_add_test(NAME host.bios
  COMMAND cogmaster-test-bios-host bios ${bios_dir} ${bios_dir}/owned_a.bin ${bios_owned_sha1}
    ${bios_dir}/owned_b.bin
  LABELS host portable TIMEOUT 20)
