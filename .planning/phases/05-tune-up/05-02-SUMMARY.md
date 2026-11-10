---
phase: 05-tune-up
plan: "02"
subsystem: tests
tags: [tests, retroarch, budget, ci]
status: complete

requires:
  - phase: 04-sound
    provides: the hosted RetroArch screenshot comparator and its oracle header at 299 of 300 lines
provides:
  - a recorded BMP orientation measurement of what sips writes
  - one comparator file with the BMP reader folded in, in place of two
  - a shorter oracle header using pragma once
  - 27 lines of headroom under the hosted helper budget, 273 of 300
affects: [05-14, phase 7 hosted save round trip]

actuals:
  tokens: 9000
  tasks: 2
  commits: 1

plan_head_before: 8502e9300348c3363d77e2686c50bda8e60fc978
plan_head_after: 6b87178e71d9f1e6e4252666d63fb61fb47d6865
commits: 1

tech-stack:
  added: []
  patterns:
    - "A test helper that returns its buffer (NULL on refusal) needs no status code and no out-parameter for the buffer"

key-files:
  created: []
  modified:
    - tests/retroarch/compare.c
    - tests/fixtures/play.h
    - tests/libretro/tests.cmake
    - .github/workflows/ci.yml
  deleted:
    - tests/retroarch/bmp.c

key-decisions:
  - "The top-down branch of the BMP reader stays: sips writes a negative height field (-192), so it is the branch the hosted job actually reads"
  - "The reader checks for 70 bytes before parsing, so the parser reads the header without its own length test"

patterns-established:
  - "Measure a converter's output orientation before deleting a branch that handles it"

requirements-completed: [TUNE-05]

coverage:
  - id: D1
    description: "The BMP orientation sips writes is measured, and the top-down branch is kept or removed on that measurement"
    requirement: TUNE-05
    verification:
      - kind: other
        ref: "build/ci/cogmaster-run --frames 60 --png 60:r.png then sips -s format bmp then od -A d -t d4 -j 22 -N 4 printed -192 (every command exit 0); the plan verify awk exits 0"
        status: pass
    human_judgment: false
  - id: D2
    description: "policy.e2e_budget prints 273 lines under tests/retroarch, at most 274 and 27 under the 300 cap"
    requirement: TUNE-05
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci -R policy.e2e_budget (passed) and cmake -DDIR=tests/retroarch -DCHECK=e2e_budget -P tests/cmake/repository.cmake printed 273"
        status: pass
    human_judgment: false
  - id: D3
    description: "The comparator prints every pixel equal for equal images, bottom-up or top-down, fails its flip control naming pixel 7,3, and exits 2 with a cannot-read message for a missing or truncated file"
    requirement: TUNE-05
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R libretro.screenshot (7 of 7 passed, including oracle_control and pixel_control); compare r.bmp r.bmp exit 0; expect_fail.cmake with --flip and first difference at 7,3 exit 0; missing and 60-byte files exit 2; a row-reversed bottom-up copy of the same image compares equal exit 0"
        status: pass
    human_judgment: false
  - id: D4
    description: "The libretro.screenshot target and the hosted job build the comparator from compare.c alone, warning-free under cogmaster_warnings and under -Wall -Wextra -Werror"
    requirement: TUNE-05
    verification:
      - kind: other
        ref: "test ! -e tests/retroarch/bmp.c and no git grep hit for bmp.c under tests and .github (exit 0); cc -std=c17 -O2 -Wall -Wextra -Werror -o compare tests/retroarch/compare.c (exit 0); actionlint with the ubuntu-26.04 label ignore (exit 0); the same source under -fsanitize=address,undefined ran clean"
        status: pass
    human_judgment: false
  - id: D5
    description: "The oracle header carries a single pragma once guard and the full local gates stay green"
    requirement: TUNE-05
    verification:
      - kind: integration
        ref: "grep -q '#pragma once' tests/fixtures/play.h (exit 0); cmake --workflow --preset hygiene exit 0 (hygiene.format included); cmake --workflow --preset ci exit 0, 399 of 399 tests passed"
        status: pass
    human_judgment: false
---

# Phase 5 Plan 02: Comparator Merge Summary

**The BMP reader now lives inside the screenshot comparator, bmp.c is gone, and the hosted RetroArch helper set drops from 299 to 273 lines with every message and exit code unchanged.**

## Performance

- **Tasks:** 2 of 2, 1 commit (task 1 is a measurement and commits nothing)
- **Files:** 5 touched (1 deleted)

## Accomplishments

- Measured what `sips` writes before deleting anything. The hosted job converts both PNGs with `sips -s format bmp`; on a runner PNG the signed height field at offset 22 is negative, so the converter writes top-down BMPs and the top-down branch is kept.
- Folded `bmp.c` into `compare.c`: one static reader (`compare_read`), one static parser (`compare_bmp`) that returns the pixel buffer or NULL, and a four-byte little-endian helper (`compare_le32`). The two `bmp_read` prototypes, the `<stdint.h>` include and the separate status codes are gone.
- `cogmaster-test-screenshot` and the hosted job's `cc` line name `compare.c` only.
- `tests/fixtures/play.h` uses `#pragma once` in place of a three-line guard.

## Measured BMP orientation

Commands run from the repository root with scratch files under `$TMPDIR`:

| Command | Exit | Output |
|---------|------|--------|
| `build/ci/cogmaster-run --frames 60 --png 60:$TMPDIR/r.png` | 0 | wrote r.png |
| `sips -s format bmp $TMPDIR/r.png --out $TMPDIR/r.bmp` | 0 | wrote r.bmp |
| `od -A d -t d4 -j 22 -N 4 $TMPDIR/r.bmp` | 0 | `-192` |
| the plan's verify awk on that output | 0 | height parses as a signed integer |

A negative height means sips writes top-down, so the top-down branch stays (the plan allowed removal only for a positive height). The bottom-up path is still exercised: a row-reversed bottom-up copy of the same BMP compares equal to the original.

## Measured e2e count

Printed by `cmake -DDIR=tests/retroarch -DCHECK=e2e_budget -P tests/cmake/repository.cmake`:

- Before: `e2e_budget: 299 lines under tests/retroarch, at most 300`
- After: `e2e_budget: 273 lines under tests/retroarch, at most 300`

The plan projected 271 with the top-down branch kept (270 if removed); the two extra lines come from clang-format 21 wrapping the 64-bit-safe header test and the two-declarator buffer line. 273 is within the 274 target and 27 under the cap.

## Task Commits

1. **Task 1: Measure the BMP orientation sips writes** - no commit by design; recorded above.
2. **Task 2: Fold the BMP reader into compare.c and reclaim lines** - `6b87178` (test)

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] Reader length guard moved out of the parser**
- **Found during:** Task 2
- **Issue:** The plan folded the parser's `n < 70` test into the single validity condition, but the header fields are read in the declarations ahead of that condition, so a short file would read past the buffer.
- **Fix:** `compare_read` only parses a buffer of at least 70 bytes (`end >= 70`), so the parser may read the header unconditionally. A 60-byte file exits 2 with the cannot-read message, and the sanitizer build ran clean on it.
- **Files modified:** tests/retroarch/compare.c
- **Commit:** 6b87178

**2. [Process] Plan-level two-step commit corrected**
- **Found during:** Task 2
- **Issue:** The first commit staged only the deletion because `git add` on the already-staged removed path failed and aborted the whole add.
- **Fix:** Staged the four modified files and amended the local, unpushed commit, so the task is one commit as planned.
- **Commit:** 6b87178

**Total deviations:** 1 auto-fixed (1 blocking), 1 process note. **Impact:** none on behaviour; the count is 273 rather than the projected 271.

## Known Stubs

None.

## Threat Flags

None. The change is test code only and adds no network, auth or file-access surface.

## Evidence

- The hosted RetroArch job was not dispatched: the plan names no `gh workflow run`, and the only hosted-side change is the `cc` line, which was reproduced locally (`cc -std=c17 -O2 -Wall -Wextra -Werror -o compare tests/retroarch/compare.c`, exit 0) and passes actionlint. The first hosted run on the phase branch will exercise it.
- `cmake --workflow --preset hygiene` exit 0; `cmake --workflow --preset ci` exit 0 with 399 of 399 tests passed.

## Next

Ready for the next plan in the phase (05-03).

## Self-Check: PASSED

- tests/retroarch/compare.c found; tests/retroarch/bmp.c absent as intended
- commit 6b87178 is an ancestor of HEAD
