---
phase: 11-pal-and-japanese-fm
plan: "06"
subsystem: fixtures
tags:
  - vdp
  - v-counter
  - pal
  - fixture
  - mailbox
status: complete

requires:
  - 11-05
provides:
  - an owned WLA-DX program that reads the V counter once per 228-cycle line over a whole frame on PAL and NTSC and reports seven closed-form records, built at 192, 224 and 240 lines and pinned by digest
  - 33 literal expectation rows derived from the documented jump intervals, judged on five SMS2 profiles and two first-model profiles
  - a recomputation test that rebuilds every row from the interval lists in CMake math
  - judge controls and a wrong-profile control, each shown able to fail
affects:
  - 11-07

actuals:
  tokens: 14000
  tasks: 2
  commits: 2

plan_head_before: 1e94441f74e9a6e64d7787660fa5c62d44325285
plan_head_after: 3fbd8ed9341b894a818d98f238cbcdb84992d50d
commits: 2

tech-stack:
  added: []
  patterns:
    - "A sampling loop of exactly one video line (228 cycles) meets each table entry once per frame, so the records are rotation invariant and need no model of the read tick"
    - "The expectation table is recomputed from the documented intervals in a second place (CMake math) and compared row by row, so a transcription error in the csv or the fixture fails"
    - "A one-cycle-longer loop is caught by a drift record: one more read 228 cycles after the last must equal the first"

key-files:
  created:
    - tests/roms/src/vdp_vtable.asm
    - tests/roms/src/vdp_vtable.link
  modified:
    - tests/fixtures/tests.cmake
    - tests/built/expected.txt
    - tests/roms/manifest.txt
    - tests/diag/expected.csv
    - tests/inventory/ci.txt
    - tests/inventory/wasm32.txt
    - docs/conformance.md

key-decisions:
  - "The TV verdict routine of the ports fixture is copied verbatim into vdp_vtable.asm with a comment naming it, so the ports image digest is untouched"
  - "Expectations come from the literal intervals of the V counter table (hex forms in the guide, decimal forms in the test, each conversion commented) and not from src/vdp.c; the core was never read for a value"
  - "The first-model rows use the 192 values and are checked by closed_form against the 192 interval lists of their TV"
  - "NTSC 240 is built and runs but is not registered or judged (the counter rolls); its drift record reads 0, as the rolling counter should"
  - "The closed-form case sits at the top of the fixtures script-mode block, before the image digest check, because it has no image"

requirements-completed: []

coverage:
  - id: C1
    description: "vdp_vtable.asm and its link file are built three times (192, 224, 240) as vdp_lines is; the three digests are in expected.txt and one comment line in the manifest; two clean builds give equal digests; no image is committed; include and src are unchanged"
    requirement: REG-02
    verification:
      - kind: integration
        ref: "cmake --build --preset ci exit 0; shasum -a 256 of the three images before and after removing the images and objects and rebuilding, cmp of the two listings exit 0; grep -c '^vdp_vtable_' tests/built/expected.txt prints 3; grep -c vdp_vtable tests/roms/manifest.txt prints 1; git ls-files tests/roms image-extension count 0 both here and at origin/main; git diff --quiet origin/main -- include src exit 0"
        status: pass
    human_judgment: false
  - id: C2
    description: "Each record read by hand on PAL 192, 224, 240, NTSC 192, 224 and the first model at 224 equals its literal from the interval lists (table below)"
    requirement: REG-02
    verification:
      - kind: integration
        ref: "build/ci/cogmaster-run build/ci/fixtures/vdp_vtable_<h>.sms --region pal|ntsc-u --frames 30 --mailbox exit 0 for 192 and 224 on both TVs and for 240 on PAL (loop exit 0); the same with --machine sms1 on the 224 image exit 0; observed values equal the literals in every cell"
        status: pass
    human_judgment: false
  - id: C3
    description: "The 228-cycle claim is not vacuous: one extra NOP in the padding makes record 07 read 0 on PAL and on NTSC; restored it reads 1 and the 192 digest equals the pinned one"
    requirement: REG-02
    verification:
      - kind: integration
        ref: "python edit .REPT 12 to .REPT 13, cmake --build --preset ci exit 0, build/ci/cogmaster-run build/ci/fixtures/vdp_vtable_192.sms --region pal|ntsc-u --frames 30 --mailbox prints 07=0000 on both; file restored from a copy, rebuilt, prints 07=0001 on both, shasum -a 256 equals the expected.txt line"
        status: pass
    human_judgment: false
  - id: C4
    description: "Thirty-three expectation rows judge records 01 to 07 on ntsc and pal (240 on pal only) and on ntsc-sms1 and pal-sms1; fixtures.vdp_vtable.closed_form recomputes count, sum, sum of squares, cyclic descents, maximum and minimum from the interval lists and compares 49 row lookups; twelve tests are registered with fixtures and portable labels and listed in both inventories"
    requirement: REG-02
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci -R '^fixtures\\.vdp_vtable' exit 0, 12 of 12 passed; ctest -N prints Total Tests: 12; cmake -DCASE=vtable_closed_form -DCSV=$PWD/tests/diag/expected.csv -P tests/fixtures/tests.cmake exit 0 printing 49 rows equal; grep -c '^fixtures\\.vdp_vtable\\.' prints 12 for ci.txt and for wasm32.txt; grep -c 'vdp_vtable_' tests/diag/expected.csv prints 33"
        status: pass
    human_judgment: false
  - id: C5
    description: "Negative controls: a changed interval end fails closed_form naming the record; a changed sum row fails the mailbox test and closed_form; the wrong-profile control fails when the judge is made to read the matching profile; a one-line change of the PAL 192 jump line in the core fails the PAL 192 mailbox test on records 02 and 03 and src is restored"
    requirement: REG-02
    verification:
      - kind: unit
        ref: "interval 0-242 to 0-243: ctest -R fixtures.vdp_vtable.closed_form exit 8 'vdp_vtable_192 pal 01 csv 0x0139 computed 0x13a'; csv row AF26 to AF27: exit 8 for mailbox.pal.192 ('record 02=AF26 is outside its row') and closed_form; control PROFILE pal to ntsc: ctest -R fixtures.vdp_vtable.control.wrong_profile exit 8; src/vdp.c 242u to 243u with a rebuild: ctest -R fixtures.vdp_vtable.mailbox.pal.192 exit 8 (record 02=AF5F, 03=460D outside their rows); every file restored, git diff --quiet origin/main -- include src exit 0, ctest -R '^fixtures\\.vdp_vtable' exit 0"
        status: pass
    human_judgment: false
  - id: C6
    description: "docs/conformance.md gains one short section (572 bytes added, file 31,549 bytes), counted check lines stay at 852, and the full ci and hygiene workflows pass"
    requirement: REG-02
    verification:
      - kind: integration
        ref: "wc -c < docs/conformance.md prints 31549; planning-identifier grep over added docs lines prints 0; cmake -DROOT=$PWD -P tests/cmake/budgets.cmake exit 0 (852 check lines, 9402 src lines); cmake --workflow --preset ci exit 0 (640 tests plus 5 release); cmake --workflow --preset hygiene exit 0 (72 tests)"
        status: pass
    human_judgment: false

duration: about 40 minutes
completed: 2026-11-11
---

# Phase 11 Plan 06: The V counter table fixture Summary

A self-authored Z80 program now samples the V counter once per 228-cycle line over a whole frame, on PAL (313 lines) and NTSC (262 lines) at 192, 224 and 240 lines, and reports seven closed-form records; every record equals a value derived from the documented jump intervals, and a second, independent recomputation in CMake guards the expectation table. REG-02 is not complete yet (the lane hashes and the hosted run are the next plan), so `requirements-completed` is empty.

## Accomplishments

- `tests/roms/src/vdp_vtable.asm` (288 lines, no table of V values, no byte run): sets mode 4 and the height bits, takes the TV verdict with the 31-cycle frame loop copied from the ports fixture (313 lines at a count of 2,048 or more, 262 at 1,800 or more, 0 otherwise), then reads `$7E` into RAM at `$C400` with interrupts off in an iteration of exactly 228 cycles (11 + 7 + 6 + 6 + 4 + 4 + 7 + 125 + 48 + 10, each instruction commented), takes one extra read 228 cycles after the last, and after the loop computes count, sum, sum of squares (shift and add), cyclic descents, maximum, minimum and the drift flag. The header comment names the fault it prevents.
- `tests/fixtures/tests.cmake`: a build loop beside the `vdp_lines` one (three link files by `file(GENERATE)`, `-D LINES_HEIGHT`), twelve test registrations, the new script-mode case `vtable_closed_form`, the control rows of the existing judge loop, and the wrong-profile control.
- `tests/diag/expected.csv`: 33 rows, source column `derived`, citation naming the V counter table section. Profile-independent records are `*` rows; the per-profile ones are `ntsc` and `pal` (PAL only for 240), plus `ntsc-sms1` and `pal-sms1` on the 224 image holding the 192 values.
- `tests/built/expected.txt` and `tests/roms/manifest.txt`: three digest lines and one comment line; nothing removed.
- `docs/conformance.md`: section "The V counter table fixture" (572 bytes).

Image digests (SHA-256): vdp_vtable_192 `2a0824da7ef48d2220e0291ced0fb49aa5969d2b968be9ffed98536bd7cf1c6d`, vdp_vtable_224 `fa8f02911fcc3dc812f21228918aa4244f4fce0aa890a644006666e6169baa58`, vdp_vtable_240 `9643218dafcf9b0d964c6d7da26c2004f6adf4b929e8bb1fc2f7a401ee9ba9e1`.

## By-hand run table (record, literal expectation, observed)

All six runs: `build/ci/cogmaster-run build/ci/fixtures/vdp_vtable_<h>.sms [--machine sms1] --region pal|ntsc-u --frames 30 --mailbox`, exit 0, status 00. Literals are the plan's decimals (hex in brackets); observed values are the printed hex.

| Run | 01 count | 02 sum mod 65,536 | 03 sum of squares mod 65,536 | 04 descents | 05 max | 06 min | 07 drift |
|---|---|---|---|---|---|---|---|
| PAL 192 | 313 (0139) / 0139 | 44,838 (AF26) / AF26 | 59,016 (E688) / E688 | 2 / 0002 | 255 / 00FF | 0 / 0000 | 1 / 0001 |
| PAL 224 | 313 / 0139 | 44,982 (AFB6) / AFB6 | 3,656 (0E48) / 0E48 | 2 / 0002 | 255 / 00FF | 0 / 0000 | 1 / 0001 |
| PAL 240 | 313 / 0139 | 43,390 (A97E) / A97E | 59,368 (E7E8) / E7E8 | 2 / 0002 | 255 / 00FF | 0 / 0000 | 1 / 0001 |
| NTSC 192 | 262 (0106) / 0106 | 33,933 (848D) / 848D | 5,635 (1603) / 1603 | 2 / 0002 | 255 / 00FF | 0 / 0000 | 1 / 0001 |
| NTSC 224 | 262 / 0106 | 34,029 (84ED) / 84ED | 48,547 (BDA3) / BDA3 | 2 / 0002 | 255 / 00FF | 0 / 0000 | 1 / 0001 |
| sms1 224 image, PAL | 313 / 0139 | 44,838 / AF26 | 59,016 / E688 | 2 / 0002 | 255 / 00FF | 0 / 0000 | 1 / 0001 |
| sms1 224 image, NTSC | 262 / 0106 | 33,933 / 848D | 5,635 / 1603 | 2 / 0002 | 255 / 00FF | 0 / 0000 | 1 / 0001 |

Every observed value equals its literal. The hex of each decimal is also produced by the CMake `math(EXPR)` in closed_form. NTSC 240 (not judged, the counter rolls) reads 0106, 7EA6, F6BA, 0002, 00FF, 0000 and a drift of 0000, which is what a counter that rolls through 263 values against a 262-line frame should show.

## Evidence and negative controls

- Verify command of task 1 exit 0 (pal and ntsc-u at 192 and 224, then `ctest -R '^(fixtures\.|policy\.|hygiene\.)'` 124 of 124).
- Verify command of task 2: `ctest -R '^fixtures\.vdp_vtable'` 12 of 12, `cmake --workflow --preset ci` exit 0 (640 tests, release 5 of 5), `cmake --workflow --preset hygiene` exit 0 (72 tests). Slowest local test 0.16 s (closed_form); the timeout comments say 20 s floor, provisional.
- Two-build digest equality: `shasum -a 256` listing, then the three images and objects removed and rebuilt, second listing, `cmp` exit 0.
- Drift control (recorded): padding `.REPT 12` to `.REPT 13` by python on the untracked source (so `git diff --stat` has no old version to show; the edit is the single token), rebuild exit 0, record 07 = 0000 on PAL and NTSC; restored from a saved copy, rebuild exit 0, record 07 = 0001 on both, 192 digest equals expected.txt.
- Closed form control: interval `0-242` to `0-243` in the test literal, `ctest -R fixtures.vdp_vtable.closed_form` exit 8, `vtable closed form: vdp_vtable_192 pal 01 csv 0x0139 computed 0x13a`; restored, exit 0.
- Row control: csv row `vdp_vtable_192,02:...,pal,AF26` to `AF27`, ctest on `mailbox.pal.192` and `closed_form` exit 8, messages `record 02=AF26 is outside its row 02:sum_mod_65536 (AF27)` and `csv 0xAF27 computed 0xaf26`; restored, 12 of 12.
- Profile control: the wrong-profile control's `PROFILE pal` changed to `ntsc` (so the judge reads the matching profile and passes), `ctest -R fixtures.vdp_vtable.control.wrong_profile` exit 8; restored, exit 0. This shows the control is the thing that proves the judge reads the profile.
- Non-tautology control against the core: `src/vdp.c` PAL 192 jump line `242u` changed to `243u`, rebuild exit 0, `ctest -R fixtures.vdp_vtable.mailbox.pal.192` exit 8 with `record 02=AF5F` and `record 03=460D` outside their rows; `src/vdp.c` restored from a saved copy, `git diff --quiet origin/main -- include src` exit 0, rebuild, `ctest -R '^fixtures\.vdp_vtable'` exit 0. The fixture therefore fails if the core's table is wrong, and no expectation came from `src/vdp.c`.
- Acceptance greps: expected.txt 3 lines, manifest 1, nothing removed against origin/main (0), image-extension files in `tests/roms` 0 here and at origin/main, hex byte runs of 256 or more in added test lines 0, inventories 12 and 12 with 0 removed, expected.csv `vdp_vtable_` rows 33, planning-identifier grep over added docs lines 0.
- Counted check code: `cmake -DROOT=$PWD -P tests/cmake/budgets.cmake` exit 0, 852 check lines / 9,402 src lines, unchanged from the end of plan 05 (this plan added 0 counted lines). Docs bytes: conformance.md 30,977 to 31,549 (+572, section cap 700, file cap 32,400).
- Container compile: not applicable, no C file touched (assembly, CMake and csv only).
- No hosted run was dispatched in this plan, so there is no run URL; the hosted proof belongs to the next plan.

## Deviations from Plan

None in scope. One draft defect found and fixed before the task commit: the first form of `closed_form` skipped the 224 rows (its pair loop was wrong) and reported 35 checked rows; the expected count of 49 (seven records on seven rom and profile pairs) exposed it, and the committed form checks all 49.

## Known Stubs

None.

## Threat Flags

None. The new surface is an owned assembly source and a test case; no network, auth or schema change.

## Self-Check: PASSED

- Files found: tests/roms/src/vdp_vtable.asm, tests/roms/src/vdp_vtable.link.
- Commits found: 89cfaa8, 3fbd8ed (ancestors of HEAD).
