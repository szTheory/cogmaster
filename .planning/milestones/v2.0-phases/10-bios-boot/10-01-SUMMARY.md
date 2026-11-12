---
phase: 10-bios-boot
plan: "01"
subsystem: firmware-legality
tags:
  - bios
  - release-scan
  - policy
  - digest-table
status: complete

requires: []
provides:
  - host/bios_table.inc, the 13 firmware digests of ASSET_POLICY section 4 as converted facts, checked against the doc both ways by policy.bios_table
  - release.firmware_scan, a pure CMake scan of the leg's three zips with eleven owned-byte controls
  - policy.bios_dir_reads, a guard on the BIOS directory variable name under tests, cmake and the workflows
  - ASSET_POLICY sections 4 and 5 and the THIRD-PARTY-NOTICES section for the table
affects:
  - 10-02
  - 10-03

actuals:
  tokens: 21000
  tasks: 2
  commits: 2

plan_head_before: 9d59bb7620bc4fcd1dddf3989086ae25c2ef8360
plan_head_after: f69406ee82917be068ce4c3211c96b43e8929a11
commits: 2

tech-stack:
  added: []
  patterns:
    - "A fixture registration file holds fixture builders and registrations only; every scan control builds its bytes and its injected digest from the same bytes at configure time"
    - "A scan reads its digest tables through cmake -E cat so a missing table is an empty table and fails as vacuous"

key-files:
  created:
    - host/bios_table.inc
    - tests/cmake/firmware_scan.cmake
    - tests/policy/firmware_release_cases.cmake
  modified:
    - ASSET_POLICY.md
    - THIRD-PARTY-NOTICES.md
    - tests/cmake/repository.sh
    - tests/policy/repository-checks.txt
    - tests/policy/tests.cmake
    - tests/inventory/ci.txt
    - tests/inventory/wasm32.txt

key-decisions:
  - "The scan extracts each zip beside its directory as DIR.extract, not one shared firmware_extract directory, because the eleven controls run in parallel and a shared scratch directory made two of them fail on a race"
  - "The needle is the 20 digest bytes with a comma or a closing brace after each (0xAA[,}]), so one search serves the table rows whose last byte is followed by a brace"
  - "The two digest tables are the lookup source and are not themselves digest-checked as files; manifest-listed files and loose files beside the zips are"
  - "A table that cannot be read is an empty table (cmake -E cat), so a missing table fails 'vacuous' like a row-less one"

requirements-completed: []

coverage:
  - id: D1
    description: "host/bios_table.inc holds the 13 rows of ASSET_POLICY section 4 sorted by file name, converted by script, and policy.bios_table compares the doc and the table in both directions naming the offending row, with a nibble-flipped doc control and a missing-row table control"
    requirement: BIOS-01
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci -R '^policy\\.bios_table' exit 0 (3 tests); one-shot nibble flip of the mpr-10883.rom row made policy.bios_table fail naming that file, git checkout restored it and the test passed"
        status: pass
    human_judgment: false
  - id: D2
    description: "release.firmware_scan stages exactly the leg's three zips, extracts them and fails naming the zip and member on a table or extra digest or on a firmware-class-plus-512 zip member with a byte-aligned NUL, and fails vacuous on an empty directory, a row-less or missing table, zero files or too few zips; it passes on the real leg and prints its tally"
    requirement: BIOS-01
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^release\\.firmware_scan$' -V exit 0 printing 3 zips, 21 files, 0 hits after cmake --workflow --preset ci packaged the zips"
        status: pass
    human_judgment: false
  - id: D3
    description: "Eleven controls prove the scan: whole, headered, member, cartdb and manifest fail naming the member; clean, text and loose pass; short, vacuous_dir and vacuous_table fail vacuous; each builds its bytes and injected digest from the same file and holds no real digest"
    requirement: BIOS-01
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci -R 'policy\\.firmware_scan' exit 0 (11 tests); mutating the aligned NUL match to an unaligned find failed policy.firmware_scan.text, dropping the zip-member scope failed policy.firmware_scan.loose, both restored and green"
        status: pass
    human_judgment: false
  - id: D4
    description: "policy.bios_dir_reads fails when the BIOS directory variable name appears under tests, cmake, CMakeLists.txt or the workflows, builds the name in pieces so it does not match its own source, and has a fixture control that names it"
    requirement: BIOS-01
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci -R '^policy\\.bios_dir_reads' exit 0 (2 tests)"
        status: pass
    human_judgment: false
  - id: D5
    description: "ASSET_POLICY sections 4 and 5 and THIRD-PARTY-NOTICES describe the table and the two scans as built, the notice carries an Applies to line for host/bios_table.inc and the plain Licence form, and no code file of include, src, runner, libretro or host/bios changed"
    requirement: BIOS-01
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^policy\\.notices$' exit 0; test -z over git diff --name-only origin/main...HEAD for include src runner libretro host/bios.[ch] exit 0; cmake -DROOT=$PWD -P tests/cmake/budgets.cmake exit 0 at 788 counted lines"
        status: pass
    human_judgment: false
---

# Phase 10 Plan 01: Digest table and release firmware scan Summary

The 13 firmware digests of ASSET_POLICY section 4 now live in `host/bios_table.inc` (converted by script, checked against the doc in both directions), and `release.firmware_scan` fails any release zip that holds a table digest, an extra digest, or a copier-headered firmware-class member, with eleven owned-byte controls and vacuous-scan guards, all before any BIOS code exists.

## Performance

- **Duration:** about 55 min, most of it the two full ci and hygiene workflows run under heavy machine load
- **Completed:** 2026-11-11
- **Tasks:** 2
- **Files:** 3 created, 7 modified

## Accomplishments

- `host/bios_table.inc`: a header comment citing `sms.cpp` at the pin, then 13 rows `{"name", Nu, {20 0x bytes}},` sorted bytewise by name, produced from ASSET_POLICY section 4 by a throwaway script that read only the doc (no firmware was opened by any step).
- `bios_table` and `bios_dir_reads` cases in `tests/cmake/repository.sh` (12 counted lines in all; `repository.cmake` gained no key). `policy.bios_table` and its two self-tests (a doc with one nibble flipped names `alpha.rom differs`, a table missing a row names `gamma.rom only in the doc`); `policy.bios_dir_reads` and its fixture self-test.
- `tests/cmake/firmware_scan.cmake` (39 counted lines): stages the three zips of the leg into `build/release/firmware_stage`, extracts into `firmware_stage.extract`, digest-checks every member, loose file and manifest-listed file against both tables and the extra list, applies the size-class NUL rule to zip members only, guards against vacuity and prints `firmware_scan: Z zips, F files, 0 hits`.
- `tests/policy/firmware_release_cases.cmake` (120 lines, fixture builders and registrations only): `release.firmware_scan` (label `release`) and the eleven `policy.firmware_scan.*` controls.
- ASSET_POLICY section 4 names the table, section 5 describes both scans as built (and no longer says planned); THIRD-PARTY-NOTICES gains "MAME BIOS digests" (BSD-3-Clause, plain `Licence:` form, `**Applies to:** \`host/bios_table.inc\``).

## Task Commits

1. Task 1, the table, the both-ways check, the dir-reads check, the notice and the policy sentence: `c7ef931`
2. Task 2, the release scan, its eleven controls, the registration and section 5: `f69406e`

## Evidence

All commands from the repository root; exit statuses are as observed.

- `cmake --workflow --preset ci`: exit 0 after task 1 and after task 2 (the first task 2 run failed two controls on a shared scratch directory race, fixed, see Deviations). `cmake --workflow --preset hygiene`: exit 0 after task 2; after task 1 the workflow's `hygiene.tree` timed out at 60 s under a machine load average near 26 (all 49 other tests passed), `sh scripts/hygiene.sh --tree` alone exited 0 and `ctest --test-dir build/hygiene -R '^hygiene\.tree$'` alone passed at 56.7 s.
- `ctest --test-dir build/ci -N -R '^policy\.bios_(table|dir_reads)'` prints `Total Tests: 5`; `ctest --test-dir build/ci -N -R firmware_scan` prints `Total Tests: 12`.
- `grep -cE` of the table row form prints 13 and `grep -E '^\{"' host/bios_table.inc | LC_ALL=C sort -c` exits 0.
- Nibble-flip control: a python edit changed one byte of the `mpr-10883.rom` row, `git diff --stat` showed 1 line changed, `ctest -R '^policy\.bios_table$'` failed printing `bios_table: mpr-10883.rom differs`; `git checkout -- host/bios_table.inc` restored it and the test passed.
- One-shot scan commands: `cmake -DSTAGE_FROM=$PWD/build/ci -DDIR=$PWD/build/ci/release/firmware_stage -DROOT=$PWD -DSUFFIX=macos-arm64 -DMIN_ZIPS=3 -P tests/cmake/firmware_scan.cmake` exit 0, `firmware_scan: 3 zips, 21 files, 0 hits`. Against the `whole` control directory with its own digest in `-DEXTRA_DIGESTS`: exit 1, output names `a.zip:clean.txt`; against the `clean` directory: exit 0.
- Mutation controls on the scan (each confirmed by `git diff --stat`, then restored from a saved copy and re-run green): the NUL test made an unaligned match failed `policy.firmware_scan.text`; the zip-member scope removed failed `policy.firmware_scan.loose`.
- `grep -Eq 'ENV\{|getenv' tests/cmake/firmware_scan.cmake` exit 1 and `grep -Eq 'string\(FIND' tests/cmake/firmware_scan.cmake` exit 1.
- `ctest --test-dir build/ci -R '^release\.' --output-on-failure` exit 0 (archive_contents sees only its own zips).
- Order evidence for plan 01: `test -z "$(git diff --name-only origin/main...HEAD -- include src runner libretro 'host/bios.[ch]')"` exit 0 after task 1 and after task 2.
- Pins: `git diff --quiet origin/main -- tests/io tests/vdp tests/bus tests/audio tests/ports tests/testcard_pins.cmake` exit 0.
- Notices: `grep -E '^Licence: BSD-3-Clause\.$' THIRD-PARTY-NOTICES.md` exit 0; the bold-Licence counts are 3 on both this tree and `origin/main` and the bare bold form count is 0; `ctest -R '^policy\.notices$'` exit 0. `grep -c planned ASSET_POLICY.md` prints 0 (origin/main: 2).
- Inventories: each of `tests/inventory/ci.txt` and `wasm32.txt` gained 17 lines and no deletion (`git diff --numstat origin/main`).
- No hosted run in this plan (the first dispatch is after plan 02).

## Budgets

- `cmake -DROOT=$PWD -P tests/cmake/budgets.cmake`: exit 0, **788 counted check lines** / 9237 src lines (baseline 737; repository.sh +12, firmware_scan.cmake +39; the plan cap is 791). Remaining allowance of the phase: 847 - 788 = 59 for plan 02 (54) and plan 10 (2).
- `tests/policy/firmware_release_cases.cmake` is 120 lines (cap 120); both new check/behavior files have their `repository-checks.txt` rows.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Shared scratch directory made two controls race**
- **Found during:** Task 2, first `cmake --workflow --preset ci`
- **Issue:** the scan extracted into `<DIR>/../firmware_extract`; all controls have DIRs under one parent and ctest runs them in parallel, so one control's `REMOVE_RECURSE` removed another's members (`policy.firmware_scan.headered` and `.clean` failed with "file SHA1 failed to read file").
- **Fix:** the scratch directory is `<DIR>.extract`, unique per staging directory, still a sibling under DIR's parent; the label regex follows.
- **Files modified:** tests/cmake/firmware_scan.cmake
- **Verification:** `ctest -j8 -R firmware_scan` three times, 12 of 12 each; `cmake --workflow --preset ci` exit 0.
- **Commit:** f69406e (the fix landed before the task commit)

**2. [Rule 1 - Bug] CMake `^` re-anchors in a global replace**
- **Found during:** Task 2, the `manifest` control (it passed instead of failing)
- **Issue:** `list(TRANSFORM ... REPLACE "^([^\t]*).*$" ...)` matched a second, empty time at the end of each element (CMake re-anchors `^` after each match), so manifest paths were mangled and never scanned.
- **Fix:** `[^\t]+` so an empty match cannot recur.
- **Files modified:** tests/cmake/firmware_scan.cmake
- **Verification:** the `manifest` control fails naming `m.bin`; the real run now counts the manifest's `sst-sample.bin` (21 files).
- **Commit:** f69406e

### Interpretations of the plan (no code behaviour lost)

- `host/bios_table.inc` and `src/cartdb.inc` are read as the digest source and are not themselves hashed as scanned files (a table's own digest can never be a digest in it); manifest-listed files that exist and loose files in the staging directory are hashed. The zips themselves in the staging directory are hashed too, as part of the recursive listing, and counted in F.
- The needle form is `0xAA[,}]0xBB[,}]...` (a regular expression that accepts the comma or the closing brace after each byte) so the last byte of a table row, which is followed by a brace, matches without a normalising pass; every digest byte is still compared exactly.
- A missing digest table is read as empty through `cmake -E cat` and fails `vacuous`; a missing `tests/roms/manifest.txt` fails with CMake's own read error (nothing is scanned either way).
- Control names use the plan's names with the `policy.firmware_scan.` prefix; `headered` uses an 8,704 byte member and `member` a 16,896 byte one (the plan gave both the same shape), so two firmware size classes are exercised. The `loose` control places the same 16,896 bytes beside a clean zip.
- The wasm32 build root holds no packaged zips (that lane runs only `portable` tests), so `release.firmware_scan` is registered there with the same text and listed in `wasm32.txt` but never run; the label `release` keeps it out of the `ci` test step and it runs in the `ci-release` step after packaging.

**Total deviations:** 2 auto-fixed bugs, 0 architectural. **Impact:** none on the plan's outputs.

## Known risks carried to plan 02 and the first dispatch

- `hygiene.tree` sits near its 60 s limit on this Mac under load (56.7 s alone); the hosted Linux lane is the evidence.
- The size-class rule could match a real zip member of exactly 1,536, 8,704, 16,896, 131,584 or 262,656 bytes holding a NUL in its first 4,096 bytes (for instance a static library of that exact size); on this leg's three zips it did not (3 zips, 21 files, 0 hits). The six hosted legs at dispatch 1 are the evidence for the others.
- Every new test carries an unsettled `# Timeout: 20 s floor; no hosted run yet, provisional.` comment for plan 02 to settle from the first hosted times.

## Known Stubs

None.

## Threat Flags

None.

## Self-Check: PASSED

- host/bios_table.inc, tests/cmake/firmware_scan.cmake and tests/policy/firmware_release_cases.cmake exist.
- Commits c7ef931 and f69406e are ancestors of HEAD (`git merge-base --is-ancestor` exit 0 for both).
