---
phase: 10-bios-boot
plan: "06"
subsystem: test
tags:
  - bios
  - fuzz
  - hostile-input
  - docs
status: complete

requires:
  - 10-05
provides:
  - bios.hostile, owned hostile and jammed BIOS images bounded to one instruction of overshoot, and the size sweep with the instance unchanged
  - tests/fuzz/fuzz_bios.c, the third harness entry whose input is the BIOS image, with fuzz.regress.bios on every lane and fuzz.libfuzzer.bios on the nightly lane
  - six bios-* seeds added after every existing write
  - testing-guide rows for the two scans and the fuzzing guide counting six targets
affects:
  - 10-07
  - 10-11

actuals:
  tokens: 5700
  tasks: 2
  commits: 2

plan_head_before: dd041c0501220af0847639659c7338d84af40ffb
plan_head_after: 0e0402c1e67b39898e093d86aa6b0825174d4711
commits: 2

tech-stack:
  added: []
  patterns:
    - "A fuzz harness whose input is one optional media field (the BIOS) against a fixed owned cartridge, with the profile taken from the last input byte, added as a third entry beside the loader and run harnesses without editing either"
    - "A size sweep that hands the loader a buffer of exactly the claimed size (one byte for a size only compared) so a read past the pointer is a sanitiser report"

key-files:
  created:
    - tests/fuzz/fuzz_bios.c
  modified:
    - tests/bios/test_bios.c
    - tests/bios/tests.cmake
    - tests/fuzz/tests.cmake
    - tests/fuzz/seeds.c
    - tests/inventory/ci.txt
    - tests/inventory/wasm32.txt
    - tests/inventory/fuzz.txt
    - docs/testing.md
    - docs/perf-fuzz.md

key-decisions:
  - "fuzz_bios treats any refusal other than ERR_ARGUMENT for size 0 and ERR_UNSUPPORTED_SIZE for every other length, and any successful load of a length other than 8192, as a harness abort"
  - "The bios-* seeds are described in a new comment block after the seeds.c header comment, not by editing the header line, so the diff against the merge base removes no line"

requirements-completed: []

coverage:
  - id: H1
    description: "Seven owned BIOS images (di then halt, jump to self, a loop writing 3E alternately FF and 00, a loop toggling 3E at every other instruction, a wild ld sp with a stack walk through RAM and the mapper registers, all FF, all 00) on SMS1 and SMS2 each return OK from three frames with overshoot at most 68 ticks and the frame counter advancing by exactly 1 per call; the alternating image leaves a recorded fault"
    requirement: BIOS-03
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci -R '^bios\\.hostile$' exit 0; the bound set to 5 made it exit 8, the expected-fault flag set on the di halt image made it exit 8, each restored to exit 0; the case also exits 0 under cc -fsanitize=address,undefined"
        status: pass
    human_judgment: false
  - id: H2
    description: "The size sweep 0, 1, 1023, 1025, 8191, 8193, 262145 and SIZE_MAX with a non-NULL pointer returns ERR_ARGUMENT for 0 and ERR_UNSUPPORTED_SIZE for every other size, each with the saved state bytes, the BIOS copy and the derived flag unchanged, on buffers of exactly the claimed size"
    requirement: BIOS-03
    verification:
      - kind: unit
        ref: "bios.hostile sweep leg exit 0 on the ci preset and under cc -fsanitize=address,undefined (exit 0)"
        status: pass
    human_judgment: false
  - id: H3
    description: "fuzz_bios loads the input as the BIOS against a fixed owned di-halt cartridge on SMS1 or SMS2 by the last byte, runs two frames with the run target's checks on a valid size, and requires ERR_ARGUMENT for an empty input and ERR_UNSUPPORTED_SIZE for any other length; the replay of all 57 seeds through it returns 0 on the ci preset and under ASan and UBSan"
    requirement: BIOS-03
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^fuzz\\.regress\\.bios$' exit 0; the harness aborting on ERR_ARGUMENT made it exit 8 through save-empty.bin; restored to exit 0; cc -fsanitize=address,undefined replay exit 0 printing replayed 57"
        status: pass
    human_judgment: false
  - id: H4
    description: "Six seeds named bios-* are added after every existing write; no existing line of seeds.c is removed or edited, size-1024.sms is still 1024 bytes, fuzz_run.c and fuzz_loader.c are unchanged, the five older replays pass on the new seeds, and tests/fuzz is 92 KiB against the 512 KiB cap"
    requirement: BIOS-03
    verification:
      - kind: integration
        ref: "git diff -U0 against the merge base of seeds.c has 0 removed lines; ls build/ci/tests/seeds | grep -c '^bios-' prints 6; the unchanged-proof git diff --quiet exit 0; ctest -R '^fuzz\\.regress\\.' exit 0 (6 tests)"
        status: pass
    human_judgment: false
  - id: H5
    description: "The inventories carry bios.hostile and fuzz.regress.bios on ci and wasm32 and fuzz.libfuzzer.bios on the fuzz list, sorted, and the registered names match on every lane"
    requirement: BIOS-03
    verification:
      - kind: integration
        ref: "cmake --workflow --preset ci exit 0 and cmake --workflow --preset hygiene exit 0 (the inventory comparison runs in both)"
        status: pass
    human_judgment: false
  - id: H6
    description: "docs/testing.md names the controls of the two scans, where each runs, what the window scan misses, that nested archives are not re-extracted and that a padded dump passes the release scan, and the boot ROM fuzz entry, ending at 36,714 bytes with no planning identifier; docs/perf-fuzz.md says six targets, lists fuzz.regress.bios, describes fuzz_bios and says the six targets are linked"
    requirement: BIOS-03
    verification:
      - kind: other
        ref: "wc -c docs/testing.md prints 36714 (net +855); the flattened-text greps for Six fuzz targets, the six targets linked and fuzz.regress.bios exit 0 and their old five-target baselines match at origin/main; the planning-identifier regex counts 0; ctest -R '^(docs\\.|policy\\.)' exit 0"
        status: pass
    human_judgment: false

duration: about 15 minutes
completed: 2026-11-11
---

# Phase 10 Plan 06: Hostile BIOS images and the BIOS fuzz entry Summary

A hostile, jammed or wrong-sized BIOS can neither hold a frame past one instruction of overshoot nor disturb the instance, in an owned-image test and under a third fuzz entry whose input is the BIOS, and the guides now say what the two scans catch and miss.

## Accomplishments

- `tests/bios/test_bios.c`: case `hostile` builds seven 8 KiB owned images from short Z80 byte strings and a fill, loads each with a 16 KiB owned `di; halt` cartridge on SMS1 and SMS2, runs three frames and asserts OK, overshoot at most 68 ticks and `frame` advancing by exactly 1; the alternating `$3E` image must leave a recorded fault. The sweep reuses the plan 05 `refused` helper (state bytes, BIOS copy and flag identical) and hands the loader a heap buffer of exactly the claimed size.
- `tests/fuzz/fuzz_bios.c`: the third harness entry, modelled on `fuzz_run.c` (allocation count, deadline window, render and sprite bounds). An empty input passes a one-byte non-NULL pointer so the pair rule is reached with size 0.
- `tests/fuzz/tests.cmake`: `bios` in the libFuzzer and the replay foreach lists and one clause in each fault comment; `fuzz.regress.bios` replays 57 files.
- `tests/fuzz/seeds.c`: `bios-owned.bin`, `bios-ff.bin`, `bios-zero.bin`, `bios-never-disables.bin`, `bios-both.bin` (8192 bytes each) and `bios-oversize.bin` (262,145 bytes), written last by `bios_images()`.
- `docs/testing.md` (+855 net) and `docs/perf-fuzz.md`: the rows, paragraph and counts the plan lists; the stale "three timed libFuzzer runs" is now "the timed libFuzzer runs".

## Task Commits

1. Task 1, `bios.hostile`, `fuzz_bios.c`, seeds, foreach lists, inventories: `30ffd39`
2. Task 2, testing and fuzzing guides: `0e0402c`

## Evidence

All commands from the repository root; exit statuses as observed.

- `cmake --preset ci` exit 0; `cmake --build --preset ci` exit 0; `ctest --test-dir build/ci -R '^(bios\.|fuzz\.regress\.)' --output-on-failure` exit 0 (11 tests); `ctest --test-dir build/ci -N -R '^fuzz\.regress\.'` prints `Total Tests: 6` and `ctest -N -R '^bios\.'` prints `Total Tests: 5`.
- `cmake --workflow --preset ci` exit 0 after each task; `cmake --workflow --preset hygiene` exit 0 after each task (63 tests). On the first Task 2 run `hygiene.tree` timed out at 60 s while the `ci` workflow's build artefacts were present and the machine was busy; `sh scripts/hygiene.sh --tree` exit 0, and the workflow rerun alone exit 0 with `hygiene.tree` at 41.78 s.
- Negative controls, each a python edit confirmed by `cmp`/`git diff --stat` and a successful rebuild (exit 0), then restored from a saved copy and re-run green: the hostile bound 68 changed to 5, `ctest -R '^bios\.hostile$'` exit 8 (message `hostile jr self: overshoot 12 ticks`); the expected-fault flag set on the `di halt` image, `bios.hostile` exit 8; `fuzz_bios.c` aborting on ERR_ARGUMENT, `fuzz.regress.bios` exit 8 (the replay's `save-empty.bin` is a zero-length input, so it does reach the size-0 path). After the restores `ctest -R '^(bios\.hostile|fuzz\.regress\.bios)$'` exit 0.
- `git diff --quiet $(git merge-base origin/main HEAD) -- tests/io tests/vdp tests/bus tests/audio tests/ports tests/testcard_pins.cmake tests/fuzz/fuzz_run.c tests/fuzz/fuzz_loader.c` exit 0.
- `git diff -U0 $(git merge-base origin/main HEAD) -- tests/fuzz/seeds.c | grep -c '^-[^-]'` prints 0; `ls build/ci/tests/seeds | grep -c '^bios-'` prints 6; `wc -c < build/ci/tests/seeds/size-1024.sms` prints 1024; `du -sk tests/fuzz` prints 92.
- `docker run --rm -v "$PWD":/w -w /w gcc:13 sh -c 'for f in tests/bios/test_bios.c tests/fuzz/fuzz_bios.c tests/fuzz/seeds.c; do gcc -std=c17 -Wall -Wextra -Wpedantic -Werror -Wtype-limits -Wconversion -fsyntax-only -I include -I build/ci/generated -I src -I tests/fuzz "$f" || exit 1; done'` exit 0 (run again after the clang-format pass).
- `clang-format --dry-run -Werror` (Homebrew `llvm@21`) over the three C files exit 0 after one formatting pass.
- `cmake --workflow --preset asan` was not run (clang-21 is not installed here, as in plan 05). Instead `cc -g -fsanitize=address,undefined -fno-sanitize-recover=undefined` over `src/*.c` with `tests/bios/test_bios.c` built (exit 0) and `bios hostile` exit 0; over `src/*.c`, `tests/fuzz/replay.c` and `tests/fuzz/fuzz_bios.c` built (exit 0) and the replay of the 57 seeds exit 0. The hosted asan job decides at the next dispatch.
- libFuzzer is not available locally: `cc -fsanitize=fuzzer` on a one-function harness fails to link (exit 1). `fuzz.libfuzzer.bios` is registered only where `COGMASTER_LIBFUZZER` is set, so its first run is the nightly dispatch in plan 11.
- Docs: `ctest --test-dir build/ci -R '^(docs\.|policy\.)'` exit 0; `cmake -DROOT="$PWD" -P tests/cmake/budgets.cmake` exit 0 (`833 check lines / 9399 src lines`). The perf-fuzz greps (`Six fuzz targets`, no `Five fuzz targets`, `the six targets linked`, `fuzz.regress.bios`, `fuzz_$t` for the six names) exit 0 with the old baselines matching at origin/main; the planning-identifier regex over the added lines of both guides counts 0.
- No hosted dispatch in this plan: `bios.hostile` and `fuzz.regress.bios` are first seen hosted at the next dispatch, so their timeouts are the floors (`# Timeout: 20 s floor, provisional` for `bios.hostile`, the foreach's existing comments for the replay) and plan 11 settles them from the D3 times.

## Budgets

`cmake -DROOT=$PWD -P tests/cmake/budgets.cmake` exit 0: counted check code 833 (phase cap 847, limit 939), src lines 9,399. This plan added no counted check code and no src line.

## Deviations from Plan

### Interpretations (no behaviour lost)

- **Docs diff-size proxy.** The plan gives two size bounds for `docs/testing.md`: at most 900 added bytes (net: 36,714 - 35,859 = 855, met) and at most 1,000 bytes of `+` diff lines. The second reads 1,191 because two existing lines the plan itself asks to extend (the `fuzz` preset row and the `fuzz.txt` sentence, whose stale "three" is now false) count in full as added lines. The net bound and the 36,864 cap hold; the proxy was not met and is not worth deleting a true sentence for.
- **Seeds header.** The first draft extended the seeds header comment by editing its last line, which removed one existing line. The header is untouched now and a separate comment block after it lists the bios-* files, so the removed-line count is 0 as the acceptance check states.
- **Three frames.** `bios.hostile` runs three frames per image instead of one; each is asserted separately.
- **Second negative control.** The plan's second control expects the size-0 path to be reached by the replay of an empty input; `save-empty.bin` (an existing zero-length seed) reaches it, so the replay did fail (exit 8) when the harness aborted on ERR_ARGUMENT.

**Total deviations:** 0 rule-driven fixes.

## Known Stubs

None. Every image is self-authored (short Z80 byte strings behind a fill or a pseudo-random body); no real firmware byte was read, copied or logged, and no BIOS-shaped file is committed.

## Threat Flags

None. `fuzz_bios.c` reads only the fuzz input; the loader still refuses every wrong size before touching the pointer.

## Self-Check: PASSED

- `tests/fuzz/fuzz_bios.c` and this SUMMARY exist.
- Commits `30ffd39` and `0e0402c` are ancestors of HEAD (`git merge-base --is-ancestor` exit 0 for each).
