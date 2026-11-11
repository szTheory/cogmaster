# Owned fixtures and registrations for the firmware legality checks; no matching or scanning logic lives here.
set(fw ${PROJECT_BINARY_DIR}/policy/firmware)
file(REMOVE_RECURSE ${fw})
# Fixture text goes through file(CONFIGURE ... NEWLINE_STYLE UNIX) so Windows never writes CRLF.
function(fw_write path content)
  file(CONFIGURE OUTPUT ${path} CONTENT "${content}" NEWLINE_STYLE UNIX @ONLY)
endfunction()
function(fw_bytes out sha) # the 0x byte form of a SHA-1, as the digest tables write it
  string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," bytes "${sha}")
  string(REGEX REPLACE ",$" "" bytes "${bytes}")
  set(${out} "${bytes}" PARENT_SCOPE)
endfunction()
# Fault prevented: a failing control that passes on an unrelated error. Each control names its row or word.
function(fw_expect name regex)
  string(REPLACE ";" "$<SEMICOLON>" cmd "${ARGN}")
  # Timeout: 20 s floor; no hosted run yet, provisional.
  cogmaster_add_test(NAME ${name} COMMAND ${CMAKE_COMMAND} "-DCMD=${cmd}" -DCODE=1 "-DREGEX=${regex}"
    -P ${policy_scripts}/expect_fail.cmake LABELS policy TIMEOUT 20)
endfunction()
# Fault prevented: a mistyped digest that leaves a real dump unrecognised or unscanned. The fixture rows are the
# SHA-1 of three owned strings, so no real digest is written here. Alpha's digest begins with e, which the control flips.
foreach(fw_n alpha beta gamma)
  string(SHA1 fw_sha "owned firmware fixture ${fw_n}")
  fw_bytes(fw_bytes ${fw_sha})
  string(APPEND fw_doc "| Owned ${fw_n} (`${fw_n}.rom`) | 8,192 | `00000000` | `${fw_sha}` |\n")
  string(APPEND fw_tab "{\"${fw_n}.rom\", 8192u, {${fw_bytes}}},\n")
  if(fw_n STREQUAL alpha)
    string(REGEX REPLACE "^e" "f" fw_flip "${fw_sha}")
    set(fw_alpha ${fw_sha})
  endif()
endforeach()
string(REPLACE "${fw_alpha}" "${fw_flip}" fw_doc_flip "${fw_doc}")
string(REGEX REPLACE "\\{\"gamma[^\n]*\n" "" fw_tab_short "${fw_tab}")
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

# Release scan controls: owned bytes built at configure time with the shell every leg requires; each injected digest comes
# from the very file written, so none is a literal. Text is 8,704 bytes with space-newline pairs (hex 20 0a), no NUL, size read back.
find_program(fw_sh sh REQUIRED)
set(fw_src ${fw}/src)
fw_write(${fw_src}/clean.txt "owned clean member\n")
string(REPEAT "line ab \n\t" 870 fw_text)
fw_write(${fw_src}/text.txt "${fw_text}end\n")
file(SIZE ${fw_src}/text.txt fw_size)
if(NOT fw_size EQUAL 8704)
  message(FATAL_ERROR "firmware scan control text: ${fw_size} bytes, not 8704")
endif()
foreach(fw_n 8192 16384) # a copier-headered shape: 512 prefix bytes, then NULs
  execute_process(COMMAND ${fw_sh} -c "{ dd if=/dev/zero bs=512 count=1 | tr '\\000' A; dd if=/dev/zero bs=${fw_n} count=1; } 2>/dev/null > \"$0\"" ${fw_src}/head${fw_n}.bin COMMAND_ERROR_IS_FATAL ANY)
endforeach()
function(fw_zip name zip) # a directory holding the zip of the ARGN files, or empty
  file(MAKE_DIRECTORY ${fw}/${name})
  if(ARGN)
    execute_process(COMMAND ${CMAKE_COMMAND} -E tar cf ${fw}/${name}/${zip} --format=zip ${ARGN} WORKING_DIRECTORY ${fw_src} COMMAND_ERROR_IS_FATAL ANY)
  endif()
endfunction()
function(fw_root name bios cartdb) # a repository holding two tables and a manifest that lists m.bin
  fw_write(${fw}/r_${name}/host/bios_table.inc "${bios}")
  fw_write(${fw}/r_${name}/src/cartdb.inc "${cartdb}")
  fw_write(${fw}/r_${name}/tests/roms/manifest.txt "m.bin\tfixture\n")
  fw_write(${fw}/r_${name}/m.bin "owned manifest file\n")
endfunction()
function(fw_scan name root member regex)
  fw_zip(${name} a.zip ${member})
  set(cmd ${CMAKE_COMMAND} -DDIR=${fw}/${name} -DROOT=${fw}/r_${root} ${ARGN} -P ${policy_scripts}/firmware_scan.cmake)
  if(regex MATCHES "^[0-9]")
    # Timeout: 20 s floor; no hosted run yet, provisional.
    cogmaster_add_test(NAME policy.firmware_scan.${name} COMMAND ${cmd} LABELS policy TIMEOUT 20)
    set_tests_properties(policy.firmware_scan.${name} PROPERTIES PASS_REGULAR_EXPRESSION "firmware_scan: ${regex}")
  else()
    fw_expect(policy.firmware_scan.${name} "firmware_scan:[ \n]+${regex}" ${cmd})
  endif()
endfunction()
fw_bytes(fw_ob ${fw_sha})
file(SHA1 ${fw_src}/clean.txt fw_clean)
fw_bytes(fw_cb ${fw_clean})
fw_root(ok "${fw_tab}" "{.sha1 = {${fw_ob}}, .bytes = 1u},\n")
fw_root(empty "/* no rows */\n" "/* no rows */\n")
fw_root(cartdb "${fw_tab}" "{.sha1 = {${fw_cb}}, .bytes = 19u},\n")
file(SHA1 ${fw}/r_ok/m.bin fw_msha)
fw_scan(whole ok clean.txt "a\\.zip:clean\\.txt" -DEXTRA_DIGESTS=${fw_clean})
fw_scan(headered ok head8192.bin "a\\.zip:head8192\\.bin")
fw_scan(member ok head16384.bin "a\\.zip:head16384\\.bin")
fw_scan(cartdb cartdb clean.txt "a\\.zip:clean\\.txt")
fw_scan(manifest ok clean.txt "[^ \n]*m\\.bin" -DEXTRA_DIGESTS=${fw_msha})
fw_scan(short ok clean.txt "vacuous" -DMIN_ZIPS=3)
fw_zip(short b.zip clean.txt)
fw_scan(vacuous_dir ok "" "vacuous")
fw_scan(vacuous_table empty clean.txt "vacuous")
fw_scan(clean ok clean.txt "1 zips, [1-9][0-9]* files, 0 hits")
fw_scan(text ok text.txt "1 zips, [1-9][0-9]* files, 0 hits")
fw_scan(loose ok clean.txt "1 zips, [1-9][0-9]* files, 0 hits")
file(COPY ${fw_src}/head16384.bin DESTINATION ${fw}/loose)
# Timeout: 20 s floor; no hosted run yet, provisional.
cogmaster_add_test(NAME release.firmware_scan
  COMMAND ${CMAKE_COMMAND} -DSTAGE_FROM=${PROJECT_BINARY_DIR} -DDIR=${PROJECT_BINARY_DIR}/release/firmware_stage
    -DROOT=${PROJECT_SOURCE_DIR} -DSUFFIX=${COGMASTER_OS}-${COGMASTER_ARCH} -DMIN_ZIPS=3 -P ${policy_scripts}/firmware_scan.cmake
  LABELS release TIMEOUT 20)
