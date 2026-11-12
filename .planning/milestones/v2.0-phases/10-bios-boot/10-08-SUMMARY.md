---
phase: 10-bios-boot
plan: "08"
subsystem: host
tags:
  - bios
  - host-helper
  - recognition
  - bounded-read
status: complete

requires:
  - 10-01
  - 10-07
provides:
  - host/bios.c and host/bios.h, the one BIOS helper the runner and the libretro adapter share, built into cogmaster-host
  - cogmaster__bios_rows, a pure lookup, identify by whole-file SHA-1 and size, and one read bounded at 8,193 bytes
  - host.bios with controls on the real bytes-to-digest-to-name path
  - the provenance sentence and the environment example comment that match the code
affects:
  - 10-09
  - 10-10
  - 10-11

actuals:
  tokens: 9000
  tasks: 2
  commits: 2

plan_head_before: a037620de4b9c3c38186db5f4ec531f0248f86dd
plan_head_after: f4cb536
commits: 2

tech-stack:
  added: []
  patterns:
    - "A host helper that takes its recognition table as an argument, so the unit test injects its own rows and no test-only bypass exists in the recognition path"
    - "A bounded read of one fread of cap bytes, checked with sentinel bytes after the buffer, never a loop to the end of the file"
    - "An owned file written at configure time with file(CONFIGURE) and its SHA-1 computed there by file(SHA1), an implementation independent of the core's"

key-files:
  created:
    - host/bios.c
    - host/bios.h
    - tests/host/test_bios.c
  modified:
    - CMakeLists.txt
    - tests/host/tests.cmake
    - tests/inventory/ci.txt
    - tests/inventory/wasm32.txt
    - PROVENANCE.md
    - .env.example

key-decisions:
  - "The core's SHA-1 is reused through ../src/sha1.h and host/bios.c includes ../runner/path.h for the open, so no second digest or path implementation exists; the helper prints nothing"
  - "cogmaster-host is not linked to the core in CMake: the runner and the libretro module already link it, a second link produced duplicate-library linker warnings, and only the test executable needed the core on its link line"
  - "Owned test files are 8,191 characters plus the one LF that file(CONFIGURE) always writes, giving exactly 8,192 bytes"

requirements-completed: []

coverage:
  - id: H1
    description: "The helper recognises by whole-file SHA-1 and exact size, for a label only: each of the 13 built-in rows identifies to itself, one flipped digest bit, size plus one and an empty table each identify nothing, and nothing in the helper selects a machine, region or FM, prints a path or prints a byte"
    requirement: BIOS-02
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci -R '^host\\.bios$' exit 0; grep for printf, puts, fputs and perror over host/bios.c matches nothing"
        status: pass
    human_judgment: false
  - id: H2
    description: "The real identify path is proved with injected tables: the published SHA-1 vector of abc fills sha1_out with its 20 bytes and names a one-row table; abd, a flipped nibble and size 4 identify nothing; an owned 8,192-byte file read through the helper and hashed by the core equals the digest CMake computed independently, and a flipped nibble, size plus one, a second owned file and an empty table identify nothing"
    requirement: BIOS-02
    verification:
      - kind: unit
        ref: "host.bios exit 0; identify hashing one byte fewer made the vector leg and the owned-file leg exit non-zero while the lookup legs stayed green; restored to exit 0"
        status: pass
    human_judgment: false
  - id: H3
    description: "The read is bounded and classified: an absent path is absent, a path below a regular file is absent on POSIX, a directory and a NULL path are the other-error status, a 16 KiB file and a file of exactly 8,193 bytes return 8,193 with the sentinel after them untouched, an 8,192-byte file returns 8,192, an empty file is ok with zero bytes, and a read of /dev/zero stops at 8,193 where the device exists"
    requirement: BIOS-02
    verification:
      - kind: unit
        ref: "host.bios exit 0; the read length changed to 8194 made it exit non-zero naming the bound and the guard byte; restored to exit 0; the same test exit 0 under cc -fsanitize=address,undefined"
        status: pass
    human_judgment: false
  - id: H4
    description: "The lookup compares the size as well as the digest: with the size comparison removed the size-plus-one control of every row fails"
    requirement: BIOS-02
    verification:
      - kind: unit
        ref: "host.bios exit non-zero with the size comparison replaced by a no-op and a successful rebuild; restored to exit 0"
        status: pass
    human_judgment: false
  - id: H5
    description: "The build adds only host/bios.c to cogmaster-host; no runner or libretro file has changed in the phase; host.bios is in the ci and wasm32 inventories with sorted labels; the ci and hygiene workflows stay green; no existing pin moved"
    requirement: BIOS-02
    verification:
      - kind: integration
        ref: "cmake --workflow --preset ci exit 0 and cmake --workflow --preset hygiene exit 0; test -z over git diff --name-only origin/main...HEAD -- runner libretro exit 0; git diff --quiet against the merge base over tests/io tests/vdp tests/bus tests/audio tests/ports tests/testcard_pins.cmake exit 0"
        status: pass
    human_judgment: false
  - id: H6
    description: "PROVENANCE.md section 6 says the dump named by --bios FILE or found in the libretro system folder is what a BIOS boot runs, recognised by digest for its label only, and that the directory variable is read only by the local window scan; .env.example says the same of the variable; no planning identifier appears"
    requirement: BIOS-02
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^(docs\\.|policy\\.)' exit 0; ctest -R '^policy\\.bios_dir_reads$' exit 0; cmake --workflow --preset hygiene exit 0; the greps named by the plan for the new and removed sentences and for planning identifiers exit as expected"
        status: pass
    human_judgment: false

duration: about 45 minutes
completed: 2026-11-11
---

# Phase 10 Plan 08: The BIOS helper and its unit test Summary

One host helper now recognises a BIOS dump by whole-file SHA-1 and size for its label only and reads it with a hard 8,193-byte bound and no output, with its recognition table injected in `host.bios` so the bytes-to-digest-to-name path is exercised for real before the runner and the adapter use it.

## Accomplishments

- `host/bios.h` and `host/bios.c`: `cogmaster__bios_rows` returns the 13-row table (`host/bios_table.inc` included between the array braces); `cogmaster__bios_lookup` is pure over a row pointer and a count; `cogmaster__bios_identify` always fills the digest with the core's SHA-1 (`../src/sha1.h`) and looks it up in the table it is given; `cogmaster__bios_read` opens through `cogmaster__path_open`, does one `fread` of 8,193 bytes, maps ENOENT and ENOTDIR to absent and every other failure (including a directory that opens and then fails to read) to the error status. The file prints nothing.
- `tests/host/test_bios.c` and `host.bios` (labels host portable): the table, the abc vector, the owned-file leg against CMake's own SHA-1, and the read cases with sentinel bytes after the buffer. The test builds its read files at run time from a counter; the two 8,192-byte files and the digest are made at configure time.
- `PROVENANCE.md` section 6 and `.env.example` now say what the code does.

## Task Commits

1. Task 1, the helper, the build line, `host.bios` and the inventories: `4adf6ed`
2. Task 2, the provenance sentence and the example comment: `f4cb536`

## Evidence

All commands from the repository root; exit statuses as observed. No hosted dispatch in this plan.

- `cmake --preset ci` exit 0, `cmake --build --preset ci` exit 0, `ctest --test-dir build/ci -R '^host\.' --output-on-failure` exit 0 (4 tests); `ctest --test-dir build/ci -N -R '^host\.'` prints `Total Tests: 4`; `grep -c 'cogmaster__bios_identify' tests/host/test_bios.c` prints 9.
- Negative controls, each a python edit confirmed by `diff` against a saved copy (the file is untracked until committed, so `git diff` shows nothing) and by a successful rebuild (exit 0), then restored and re-run green: (1) the read length changed to 8194, `ctest -R '^host\.bios$'` exit 8 printing `n == expect failed: 8194 != 8193` and the guard-byte failure, restored exit 0; (2) the size comparison removed from the lookup, exit 8 printing the size-plus-one control failing for every row, restored exit 0; (3) identify hashing one byte fewer, exit 8 failing the abc vector, its digest and the owned-file legs while the table legs stayed green, restored exit 0 (file byte-identical to the saved copy). The first attempt at control 2 did not compile (an unused parameter under `-Werror`) and the stale binary reported control 1's result, which is why every result above is tied to a build exit 0.
- `grep -En 'printf|puts|fputs|perror' host/bios.c` matches nothing (exit 1, so the helper prints nothing); `grep -En 'bios_identify\(' host/bios.h` shows the table-pointer parameters.
- `test -z "$(git diff --name-only origin/main...HEAD -- runner libretro)"` exit 0.
- `docker run --rm -v "$PWD":/src -w /src gcc:13 sh -c 'for f in host/bios.c tests/host/test_bios.c; do gcc -std=c17 -Wall -Wextra -Wpedantic -Werror -Wtype-limits -Wconversion -Wsign-conversion -Wshadow -Wcast-qual -fsyntax-only -I include -I build/ci/generated -I src -I host -I runner "$f" || exit 1; done'` exit 0; `clang-format --dry-run --Werror` (Homebrew `llvm@21`) over the three new C files exit 0; `cc -g -fsanitize=address,undefined -I host -I src -I runner host/bios.c src/sha1.c tests/host/test_bios.c` built and `bios` run with the built paths exit 0. The asan preset was not run (clang-21 is not installed here).
- `cmake --workflow --preset ci` exit 0 (622 tests) and `cmake --workflow --preset hygiene` exit 0 (63 tests) after Task 1; both exit 0 again after Task 2. `ctest --test-dir build/ci -R '^(host\.|runner\.|libretro\.|policy\.|docs\.)' -j4` exit 0 (188 tests).
- Task 2 greps: `grep -c 'read only by the local window scan' PROVENANCE.md` prints 1; the removed sentence is gone (exit 0); `grep -c 'library does not load BIOS files' .env.example` prints 0; `grep -c 'firmware-window' .env.example` prints 1; the planning-identifier grep over the added lines prints 0.
- Ordering of the plans is not asserted here; `git merge-base --is-ancestor 4adf6ed HEAD` exit 0.

## Unchanged proof

- `git diff --quiet $(git merge-base origin/main HEAD) -- tests/io tests/vdp tests/bus tests/audio tests/ports tests/testcard_pins.cmake` exit 0.

## Budgets

`cmake -DROOT=$PWD -P tests/cmake/budgets.cmake` exit 0: counted check code 833 (phase cap 847), src lines 9,399. This plan added no counted check code (host code, the test and `tests/host/tests.cmake` are not counted) and no src line; the new helper is under `host/`.

## Deviations from Plan

### Interpretations and measured corrections (no behaviour lost)

- **Owned files are 8,191 characters plus the CMake newline.** The plan asks for a newline-free body of exactly 8,192 characters. `file(CONFIGURE ... CONTENT ...)` always ends the file in one LF, so a 8,192-character body gave 8,193 bytes and the plan's own size check failed configure. The bodies are now 511 repeats of a 16-character pattern plus 15 characters; the file is 8,192 bytes with a final LF, the size check passes, and the digest CMake computes is over those exact bytes.
- **No link from `cogmaster-host` to the core.** The plan allows a PRIVATE link only if the consumers need it. The runner and the libretro module already link the core; adding the link duplicated the library on their link lines and the macOS linker warned. Only the test executable needed the core, so it links `cogmaster-host` then `cogmaster::cogmaster` itself and the build file changes by the source name alone.
- **`/dev/zero` read is not run on WASI.** The plan names POSIX only; the WASI lane has no such device, so the case is guarded by `!defined(_WIN32) && !defined(__wasi__)`. The path below a regular file (the not-a-directory status) is also run on POSIX only.
- **Extra read cases.** A file of exactly 8,193 bytes and a NULL path were added; the test also checks that the bytes read equal the counter the file was written from, and that the owned file starts with its known pattern.

**Total deviations:** 0 rule-driven fixes; 4 interpretations recorded above. **Impact:** none on the requirement.

## Known Stubs

None. No real firmware byte was read, copied, logged or compared; the digests the test uses are a published test vector, the table rows' own digests and a digest CMake computed over owned bytes.

## Threat Flags

None. The helper opens one caller-named path read-only, writes only into the caller's buffer up to 8,193 bytes and prints nothing.

## Phase notes

- The two small text amendments (the provenance sentence and the environment example comment) were moved here from the closing plan to keep every plan at ten files or fewer.
- The `host.bios` timeout is an unsettled 20 s floor (marked provisional); plan 11 settles it from the dispatch after plan 10.
- The runner (plan 09) and the adapter (plan 10) have not changed yet and do not use the helper.

## Self-Check: PASSED

- `host/bios.c`, `host/bios.h`, `tests/host/test_bios.c` and this SUMMARY exist.
- Commits `4adf6ed` and `f4cb536` are ancestors of HEAD (`git merge-base --is-ancestor` exit 0 for each).
