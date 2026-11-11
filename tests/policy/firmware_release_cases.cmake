# Owned fixtures and registrations for the firmware legality checks; no matching or scanning logic lives here.
set(fw ${PROJECT_BINARY_DIR}/policy/firmware)
file(REMOVE_RECURSE ${fw})
# Fixture text goes through file(CONFIGURE ... NEWLINE_STYLE UNIX) so Windows never writes CRLF.
function(fw_write path content)
  file(CONFIGURE OUTPUT ${path} CONTENT "${content}" NEWLINE_STYLE UNIX @ONLY)
endfunction()
# Fault prevented: a failing control that passes on an unrelated error. Each control names its row or word.
function(fw_expect name regex)
  string(REPLACE ";" "$<SEMICOLON>" cmd "${ARGN}")
  # Timeout: 20 s floor; no hosted run yet, provisional.
  cogmaster_add_test(NAME ${name} COMMAND ${CMAKE_COMMAND} "-DCMD=${cmd}" -DCODE=1 "-DREGEX=${regex}"
    -P ${policy_scripts}/expect_fail.cmake LABELS policy TIMEOUT 20)
endfunction()

# Fault prevented: a mistyped digest that leaves a real dump unrecognised or unscanned. The fixture rows are the
# SHA-1 of three owned strings, so no real digest is written here.
foreach(fw_n alpha beta gamma)
  string(SHA1 fw_sha "owned firmware fixture ${fw_n}")
  string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," fw_bytes "${fw_sha}")
  string(REGEX REPLACE ",$" "" fw_bytes "${fw_bytes}")
  string(REGEX REPLACE "^f" "0" fw_flip "${fw_sha}")
  if(fw_flip STREQUAL fw_sha)
    string(REGEX REPLACE "^." "f" fw_flip "${fw_sha}")
  endif()
  string(APPEND fw_doc "| Owned ${fw_n} (`${fw_n}.rom`) | 8,192 | `00000000` | `${fw_sha}` |\n")
  string(APPEND fw_tab "{\"${fw_n}.rom\", 8192u, {${fw_bytes}}},\n")
  if(fw_n STREQUAL alpha)
    set(fw_doc_flip "| Owned alpha (`alpha.rom`) | 8,192 | `00000000` | `${fw_flip}` |\n")
  else()
    string(APPEND fw_doc_flip "| Owned ${fw_n} (`${fw_n}.rom`) | 8,192 | `00000000` | `${fw_sha}` |\n")
  endif()
  if(NOT fw_n STREQUAL gamma)
    string(APPEND fw_tab_short "{\"${fw_n}.rom\", 8192u, {${fw_bytes}}},\n")
  endif()
endforeach()
fw_write(${fw}/doc.md "${fw_doc}")
fw_write(${fw}/doc_flip.md "${fw_doc_flip}")
fw_write(${fw}/table.inc "${fw_tab}")
fw_write(${fw}/table_short.inc "${fw_tab_short}")
set(fw_check -DCHECK=bios_table -P ${policy_scripts}/repository.cmake)
# Timeout: 20 s floor; no hosted run yet, provisional.
cogmaster_add_test(NAME policy.bios_table
  COMMAND ${CMAKE_COMMAND} -DDOC=${PROJECT_SOURCE_DIR}/ASSET_POLICY.md -DOTHER=${PROJECT_SOURCE_DIR}/host/bios_table.inc ${fw_check}
  LABELS policy TIMEOUT 20)
fw_expect(policy.bios_table.self_test.doc "bios_table: alpha\\.rom differs"
  ${CMAKE_COMMAND} -DDOC=${fw}/doc_flip.md -DOTHER=${fw}/table.inc ${fw_check})
fw_expect(policy.bios_table.self_test.table "bios_table: gamma\\.rom only in the doc"
  ${CMAKE_COMMAND} -DDOC=${fw}/doc.md -DOTHER=${fw}/table_short.inc ${fw_check})

# Fault prevented: a skip-if-absent test whose path comes from the BIOS directory variable. The name is built in
# pieces so this file does not match its own check.
set(fw_var COGMASTER_BIOS)
string(APPEND fw_var _DIR)
fw_write(${fw}/reads/tests/read.cmake "# reads ${fw_var} here\n")
# Timeout: 20 s floor; no hosted run yet, provisional.
cogmaster_add_test(NAME policy.bios_dir_reads
  COMMAND ${CMAKE_COMMAND} -DROOT=${PROJECT_SOURCE_DIR} -DCHECK=bios_dir_reads -P ${policy_scripts}/repository.cmake
  LABELS policy TIMEOUT 20)
fw_expect(policy.bios_dir_reads.self_test "bios_dir_reads: [^\n]*read\\.cmake"
  ${CMAKE_COMMAND} -DROOT=${fw}/reads -DCHECK=bios_dir_reads -P ${policy_scripts}/repository.cmake)
