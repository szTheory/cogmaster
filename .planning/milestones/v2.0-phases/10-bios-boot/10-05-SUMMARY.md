---
phase: 10-bios-boot
plan: "05"
subsystem: load
tags:
  - bios
  - load
  - boot
  - state
  - hosted-dispatch
status: complete

requires:
  - 10-04
provides:
  - cogmaster_load reads media.bios and media.bios_size by size tag, validates in the stated order, keeps an instance-owned copy and CRC-32
  - cogmaster__boot_power_on, chosen by load and by both reset kinds when a BIOS is attached
  - state header fields boot (53) and bios_crc32 (54), six reserved bytes left at 58, and the flag recomputed after a state load
  - bios.load, bios.state and the load, reset and state-load legs of bios.slow_flag
  - embedding guide text for the media fields, the exact-size rule, the statuses and API 14
  - hosted dispatch D2, all lanes green, perf delta against a fresh main dispatch
affects:
  - 10-06
  - 10-07

actuals:
  tokens: 7800
  tasks: 3
  commits: 6

plan_head_before: 0734275eecb705673fdef22486a0f620a361daf0
plan_head_after: 356b5579908ca757a138238ba64107078de9d457
commits: 6

tech-stack:
  added: []
  patterns:
    - "A window detector that indexes every 32-byte window of a self-authored image by its first four bytes in a 16,384-slot table and scans the state with one lookup per byte, proven by a control image that copies 64 of its own bytes to work RAM"
    - "A load that reads an optional input pair by size tag with constant-length copies, validates pair, size and content before the probe, and replaces the instance-owned copy only after the last fallible step"

key-files:
  created: []
  modified:
    - src/instance.c
    - src/boot.c
    - src/state.c
    - src/internal.h
    - tests/bios/test_bios.c
    - tests/bios/tests.cmake
    - tests/core/test_state.c
    - docs/embedding.md
    - tests/inventory/ci.txt
    - tests/inventory/wasm32.txt

key-decisions:
  - "A BIOS in the media is the request to boot from it: load and both reset kinds call cogmaster__boot_power_on when bios.size is set, else the fast-boot hand-off, so no fast-boot value moved"
  - "The state binds the boot mode and the BIOS CRC-32 from the reserved header bytes; BIOS bytes are never visited by a state pass, and state_load recomputes mem_slow after APPLY"
  - "The four bios test timeouts stay at the 20 s floor: the slowest hosted time over nine lanes was 0.57 s"

requirements-completed:
  - BIOS-05

coverage:
  - id: L1
    description: "A BIOS pair is validated before any state change in the stated order: a pair with one side missing is ERR_ARGUMENT, a size other than 8192 (1, 1023, 1024, 8191, 8193, 16384, 131072, 262144) is ERR_UNSUPPORTED_SIZE, the test card is ERR_NO_CONTENT, a Game Gear is ERR_PROFILE_REJECTED, and an unprobeable cartridge keeps its own status; every refusal leaves the state bytes, the BIOS copy and the flag identical"
    requirement: BIOS-05
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci -R '^bios\\.load$' exit 0; removing the Game Gear refusal made it fail, changing 8192 to 8191 made it exit 8, each restored to exit 0"
        status: pass
    human_judgment: false
  - id: L2
    description: "The instance owns its copy: the caller's buffer is freed before frames run (sanitiser clean), a load without a BIOS zeroes it and restores the fast-boot values, a size tag of 56 or 32, one covering only the pointer, or one byte short of the struct reads both fields as absent, and the whole struct reads them"
    requirement: BIOS-05
    verification:
      - kind: unit
        ref: "bios.load ownership leg exit 0 on the ci preset and under cc -fsanitize=address,undefined (exit 0)"
        status: pass
    human_judgment: false
  - id: L3
    description: "After a BIOS load on SMS1 and SMS2 the Z80 is at address 0 with AF and SP all ones, IM 0, IFF 0 and every other register zero, work RAM is zero, port 3E is the derived constant with the cartridge off and the BIOS on, the mapper is at 0, 0, 1, 2, and address 0 reads the BIOS; a soft reset keeps RAM, VRAM and CRAM and a power reset clears them, both re-entering address 0"
    requirement: BIOS-05
    verification:
      - kind: unit
        ref: "bios.load power-on leg over three passes (load, soft reset, power reset) on both machines, exit 0"
        status: pass
    human_judgment: false
  - id: L4
    description: "A state saved under fast boot loads into fast boot and is refused by a BIOS instance, a state saved under one BIOS loads into it and is refused by fast boot and by another BIOS, each refusal changing nothing; the boot byte sits at offset 53, the CRC-32 at 54 to 57, six zero bytes follow; test_rejects_header moved to offset 58 with six reserved bytes and refuses a flipped boot byte and each flipped CRC byte"
    requirement: BIOS-05
    verification:
      - kind: unit
        ref: "ctest -R '^bios\\.state$' and ctest -R '^core\\.state' exit 0 (5 tests); making bios_crc32 a constant 0 made bios.state exit 8, restored exit 0"
        status: pass
    human_judgment: false
  - id: L5
    description: "No 32-byte window of a self-authored high-entropy image that never copies itself to work RAM occurs in a state saved after three frames on SMS1 and SMS2, and the control image that copies 64 of its own bytes shows at least 33 windows, so the detector is proven; the test comment states that the claim no BIOS byte in a state is false for a real run"
    requirement: BIOS-05
    verification:
      - kind: unit
        ref: "bios.state exit 0; replacing the control's ldir by two nops made it exit 1 with the message that the detector found 0 windows, restored exit 0"
        status: pass
    human_judgment: false
  - id: L6
    description: "mem_slow equals its independent recomputation after a load, a load that flips the BIOS, a soft and a power reset, and a state load whose saved port 3E is any of six values, in both directions and with and without a BIOS (these legs with plan 04's two make four sites)"
    requirement: BIOS-05
    verification:
      - kind: unit
        ref: "bios.slow_flag exit 0; deleting the decode call after the state load APPLY made it exit 8, restored exit 0"
        status: pass
    human_judgment: false
  - id: L7
    description: "Fast boot is unchanged: every existing test passes, tests/io, tests/vdp, tests/bus, tests/audio, tests/ports, the test card pins and the fuzz sources are unchanged against the merge base and no line was removed from expected.txt, expected.csv or manifest.txt"
    requirement: BIOS-05
    verification:
      - kind: integration
        ref: "cmake --workflow --preset ci exit 0 (607 tests, 5 release); cmake --workflow --preset hygiene exit 0 (63 tests); the unchanged-proof commands exit 0"
        status: pass
    human_judgment: false
  - id: L8
    description: "Hosted dispatch D2 is green on every lane with the bus change, the 8 KiB instance growth and the new state header (wasm32, MSVC, be32, nofp, asan, perf, hash-equality), and the perf delta on boot_to_play against a fresh main dispatch is at most 100 basis points"
    requirement: BIOS-05
    verification:
      - kind: integration
        ref: "https://github.com/szTheory/cogmaster/actions/runs/43062734904 conclusion success on 076f13b; fresh main run https://github.com/szTheory/cogmaster/actions/runs/43063808940 success; perf dispatch boot_to_play H=12288944 B=12303282 delta_bp=-11"
        status: pass
    human_judgment: false
  - id: L9
    description: "docs/embedding.md names the two media fields, the exact-size rule, the four statuses, the 72-byte struct and API 14, and points at the BIOS guide, ending at 36,379 bytes with no planning identifier"
    requirement: BIOS-05
    verification:
      - kind: other
        ref: "wc -c docs/embedding.md prints 36379 (+763 over the start of this plan); the planning-identifier regex over its added lines counts 0; ctest -R '^docs\\.' exit 0"
        status: pass
    human_judgment: false
  - id: L10
    description: "The four bios tests have measured timeouts: the slowest hosted time over nine lanes (be32 under qemu and wasm32 included) was 0.57 s, so each keeps the 20 s floor, and the word provisional is gone from the file"
    requirement: BIOS-05
    verification:
      - kind: other
        ref: "grep -c provisional tests/bios/tests.cmake prints 0; the timeout-comment regex matches 4 lines, each naming run 43062734904"
        status: pass
    human_judgment: false

duration: about 65 minutes
completed: 2026-11-11
---

# Phase 10 Plan 05: Load, power-on boot, state binding and hosted dispatch D2 Summary

`cogmaster_load` now accepts an 8 KiB Master System BIOS under the exact validation order, copies it into the instance and powers the machine on at address 0 with the BIOS visible; a state binds to its boot mode and BIOS CRC-32; every recompute site of the bus flag is covered; and the first hosted dispatch with the bus change and the 8 KiB growth is green on every lane with a perf delta of -11 basis points.

## Performance

- **Duration:** about 65 minutes
- **Completed:** 2026-11-11
- **Tasks:** 3
- **Files:** 0 created, 10 modified

## Accomplishments

- `src/instance.c`: `tagged_bios` reads the pointer and size_t only when the size tag covers the end of `bios_size`; `cogmaster_load` checks the pair, the exact 8192 size and the test card before the probe, the Game Gear right after the machine resolves, and replaces `m->bios` (fixed-size memset, constant-length `memcpy` of 8192 bytes, CRC-32 with the cartridge's function) only after `cart_install`, so a failed load leaves it untouched. `cogmaster_load` and `cogmaster_reset` choose `cogmaster__boot_power_on` when `bios.size != 0`, else the unchanged hand-off.
- `src/boot.c`: `cogmaster__boot_power_on(m, power)` zeroes the Z80 except AF and SP all ones, clears work RAM on power, writes no hand-off byte, resets mapper and I/O, sets port 3E to the derived constant and recomputes the flag. The code comment says the constant is derived (MAME builds the same, ares enables only the BIOS), not measured, and that the 0, 0, 1, 2 mapper hand-off is exact up to 48 KiB and an approximation after a larger checksum.
- `src/state.c`: header fields `boot` (53) and `bios_crc32` (54), reserved bytes now 58 to 63; `cogmaster__bus_decode` after the state load APPLY. This closes the carry-over from plan 04: `state_load` and `boot_power_on` both recompute `mem_slow`, and the plan's scope already named both, so it is not a deviation.
- `tests/bios/test_bios.c`: cases `load` (refusals, ownership, tags, power-on and both resets) and `state` (binding and the window detector with its control), and the `slow_flag` legs for load, both resets and state load. `tests/core/test_state.c`: `test_rejects_header` at offset 58 and six bytes plus the boot and CRC refusals, and the two names added to the field list.
- `docs/embedding.md`: the media fields, the rule, the statuses, API 14 and the reset clause.

## Task Commits

1. Task 1, load validation, ownership, power-on and `bios.load`: `70e5691`
2. Task 2, state binding, the last recompute legs, `bios.state`: `13ace65`; embedding text: `81bd971`
3. Task 3, MSVC fix found by the first dispatch: `796b7f4`; timeouts from hosted times: `076f13b` (provisional comment from run 43060784025) then `356b557` (settled from run 43062734904)

## Evidence

All commands from the repository root; exit statuses as observed.

- `cmake --preset ci` exit 0; `cmake --build --preset ci` exit 0; `ctest --test-dir build/ci -R '^(bios\.|core\.|bus\.|io\.|ports\.|fuzz\.regress\.|abi\.)'` exit 0 (128 tests) at Task 1; `ctest -R '^(bios\.|core\.|ports\.|fuzz\.regress\.|docs\.|abi\.|policy\.)'` exit 0 (168 tests) at Task 2.
- `ctest --test-dir build/ci -N -R '^bios\.'` prints `Total Tests: 4` (3 after Task 1); `ctest -N -R '^core\.state'` prints `Total Tests: 5`, all exit 0.
- `cmake --workflow --preset ci` exit 0 (607 tests, plus 5 release) and `cmake --workflow --preset hygiene` exit 0 (63 tests, no `hygiene.tree` timeout) on the final local head, and after each task.
- Negative controls, each a python edit with a successful rebuild (exit 0), then restored from a saved copy and re-run green: Game Gear refusal removed, `bios.load` reported Failed; 8192 changed to 8191, `bios.load` exit 8; decode call after the state load deleted, `bios.slow_flag` exit 8; `bios_crc32` made a constant 0, `bios.state` exit 8; the control image's `ldir` replaced by two nops, `bios.state` exit 1 with `state: the detector found 0 windows of the copying image, wanted 33`. After the last restore `ctest -R '^bios\.'` exit 0.
- `grep -n 'memcpy' src/instance.c | grep -c 'bios_size'` prints 0 (the first draft matched once; the read of the size field now goes through locals); `ctest -R '^abi\.undefined$'` exit 0.
- The planning-identifier and long-hex regex over the added lines of `src/instance.c` and `src/boot.c` counts 0.
- Container compile `docker run --rm -v "$PWD":/src -w /src gcc:13 sh -c 'for f in src/instance.c src/boot.c src/state.c tests/core/test_state.c tests/bios/test_bios.c; do gcc -std=c17 -Wall -Wextra -Wpedantic -Werror -Wtype-limits -Wconversion -fsyntax-only -I include -I build/ci/generated -I src "$f" || exit 1; done'` exit 0 (run per task for that task's files).
- `clang-format --dry-run -Werror` (Homebrew `llvm@21`) over the changed C files exit 0.
- `cmake --workflow --preset asan` exit 1 at configure (clang-21 is not installed here); `cc -fsanitize=address,undefined -fno-sanitize-recover=undefined` over `src/*.c` and `tests/bios/test_bios.c` built (exit 0) and the four cases `arbitration`, `slow_flag`, `load`, `state` exit 0. The hosted asan job is green on every dispatch that reached it (below).
- Supporting perf measurement, not the gate: valgrind is not available on this Mac, so the Cachegrind count cannot be taken locally. Wall time instead: `cogmaster-run --bench 6000` on `boot_to_play.sms`, a Release build of `origin/main` (from `git archive`) against the head build, six alternating pairs, p50 per frame in microseconds main 334, 332, 332, 328, 333, 344 and head 328, 329, 327, 333, 335, 340: no visible difference.

## Hosted dispatch D2

Branch `phase/10-bios-boot`, pushed with `git push origin phase/10-bios-boot`, dispatched with `gh workflow run ci.yml --ref phase/10-bios-boot`, each run found by head SHA with `gh run list --workflow ci.yml --branch phase/10-bios-boot --event workflow_dispatch --json databaseId,headSha` and watched to completion. Every attempt:

1. https://github.com/szTheory/cogmaster/actions/runs/43059318250 on `81bd971`: failure. `build (windows-x64)` and `build (windows-arm64)` failed to compile `tests/bios/test_bios.c` (MSVC C4132, a `const` zero-filled array without an initialiser); perf failed and be32 was cancelled at their apt step (runner DNS `Temporary failure resolving 'azure.archive.ubuntu.com'`); wasm32, retroarch and hash-equality skipped behind the builds. Fixed in `796b7f4` (the array is no longer `const`).
2. https://github.com/szTheory/cogmaster/actions/runs/43060784025 on `796b7f4`: failure. All six builds, asan, both nofp and retroarch succeeded (the Windows legs now compile); perf failed and be32 and wasm32 were cancelled at their apt step on the same DNS error. The workflow was not changed.
3. https://github.com/szTheory/cogmaster/actions/runs/43062734904 on `076f13ba02bd183e5941f0453c6356b5a937a4a7`: **success**, every job. The runner DNS had recovered (about 12 minutes after attempt 2). The dispatched head differs from the final head `356b557` in `tests/bios/tests.cmake` only, comments: `git diff --stat 076f13b HEAD -- . ':(exclude).planning'` lists that one file.

Fresh main dispatch (origin/main is `202f107bf1e888d197ed090f34b01cce73e7185c`, the merge base too): https://github.com/szTheory/cogmaster/actions/runs/43063808940, success, head SHA equal to `git rev-parse origin/main`. Two earlier main runs on the same SHA (43053345486, 43056313541) failed at the apt step and are not used.

Per-job conclusions of run 43062734904, all success except the two intentionally skipped: `changes`, `hygiene`, `build (linux-x64)`, `build (linux-arm64)`, `build (macos-x64)`, `build (macos-arm64)`, `build (windows-x64)` (MSVC), `build (windows-arm64)` (MSVC), `asan`, `nofp (ubuntu-26.04)`, `nofp (ubuntu-26.04-arm)`, `be32`, `wasm32`, `perf`, `retroarch`, `hash-equality`, `CI required`; `title`, `rehearsal` and `cleanup` skipped (not part of a dispatch).

The perf lines, verbatim from the `perf` job logs:

- branch run 43062734904: `perf control boot_to_play runs=12288944,12288944,12288944 spread=0`
- main run 43063808940: `perf control boot_to_play runs=12303282,12303282,12303282 spread=0`

`H=12288944`, `B=12303282`, `delta_bp=$(( (H - B) * 10000 / B ))` in shell integer arithmetic prints -11. Both spreads are 0, so H and B are the first value of each line.

perf dispatch boot_to_play H=12288944 B=12303282 delta_bp=-11

The remedy of moving `bios` and `mem_slow` to the end of the struct was not needed (`bios` is already last and `mem_slow` sits beside `io`), and `tests/perf/allow.txt` is unchanged (`git diff --quiet origin/main -- tests/perf/allow.txt` exit 0).

What the green run settles from the open list:

- Plan 02's all-legs criterion: every leg is green on one run, `hash-equality` and the exact inventories on be32 (513 tests) and wasm32 (476 tests, "Every test ran" step) included.
- The instance growth on wasm32: `quality: size.wasm measured=424028 ceiling=1037312`.
- `abi.api_v14` Passed on be32 (0.03 s) and wasm32 (0.03 s) in run 43062734904 and on both nofp lanes in run 43060784025 (the nofp inventory is 24 tests, 100 percent passed).
- `bios.*` on wasm32 passed in 0.30, 0.38, 0.21 and 0.19 s (arbitration, slow_flag, load, state), and on be32 under qemu in 0.04, 0.57, 0.47 and 0.55 s.
- Timeouts: the policy, hygiene and release tests of plans 01 and 02 do not appear in the be32 or wasm32 logs (`grep -c 'policy\.'` prints 0 for both), so no be32 or wasm32 time exists or is needed for them. The four bios tests: slowest over nine lanes of run 43062734904 (six builds, asan, be32, wasm32) 0.30, 0.57, 0.47 and 0.55 s, so `max(20, ceil(2 x S))` is the 20 s floor for each; `tests/bios/tests.cmake` carries `# Timeout: 20 s floor; 2 x <S> s, slowest of 9 runs, run 43062734904.` four times.

## Unchanged proof

- `git diff --quiet $(git merge-base origin/main HEAD) -- tests/io tests/vdp tests/bus tests/audio tests/ports tests/testcard_pins.cmake tests/fuzz/fuzz_run.c tests/fuzz/fuzz_loader.c` exit 0.
- `! git diff $(git merge-base origin/main HEAD) -- tests/built/expected.txt tests/diag/expected.csv tests/roms/manifest.txt | grep -q '^-[^-]'` exit 0 (no removed line).
- `git diff --quiet origin/main -- tests/perf/allow.txt` exit 0.

## Budgets

`cmake -DROOT=$PWD -P tests/cmake/budgets.cmake` exit 0: `repository: 833 check lines / 9399 src lines, strict one tenth`. After plans 03 to 05 the src line count is 9,399; the limit is the largest counted value with counted x 10 below that count, 939, and the counted total is 833 (phase cap 847, plan cap 845). This plan added no counted check code.

## Phase notes

- The embedding guide edits are split between plans 04 and 05 to keep each plan at ten files; this plan grew `docs/embedding.md` from 35,616 to 36,379 bytes (+763 net; the added-lines diff counts 1,117 bytes because three existing lines were rewritten), under the 950-byte plan allowance and the 36,864 cap.
- The power-on port 3E value `$E0`, the Z80 stack pointer `$FFFF` and the 0, 0, 1, 2 mapper hand-off are conventions with indirect evidence, not measured BIOS values; the code comment and the BIOS guide of plan 10 say so.
- The perf remedy was not used. A real BIOS is not involved in any test here (hand-off equivalence and the assembled image are plan 07).
- The BIOS-03 requirement completes in plan 07 with the equivalence check; this plan lists only BIOS-05 as completed.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] MSVC C4132 on a const zero-filled reference array**
- **Found during:** Task 3 (first hosted dispatch, run 43059318250)
- **Issue:** `static const uint8_t zero[8192];` in the ownership leg of `bios.load` does not compile under MSVC (`const` object should be initialised), failing both Windows build legs. GCC 13 with the portability flags and Apple clang accepted it.
- **Fix:** the array is `static uint8_t zero[...]` (implicitly zero-initialised, never written).
- **Files modified:** `tests/bios/test_bios.c`
- **Commit:** `796b7f4`

### Interpretations (no behaviour lost)

- The first draft of `tagged_bios` made `grep -n memcpy src/instance.c | grep -c bios_size` print 1 (a constant-length copy of the size field); it now reads through locals so the plan's check prints 0.
- The task 1 commit was made with `src/state.c` temporarily reverted (saved as a patch and re-applied for task 2), so each commit's tree keeps the state tests green as the plan states.
- The window detector indexes windows in a hash table instead of striding, so every one of the 8,161 offsets is checked with no stride and a scan costs one lookup per state byte; it took 0.55 s on be32 under qemu.

**Total deviations:** 1 (Rule 1).

## Known Stubs

None. The BIOS images in these tests are self-authored (a Z80 `di` and jump to itself, or a short copy loop, behind a pseudo-random body); no BIOS-shaped file is committed and no real firmware byte was read, copied or logged.

## Threat Flags

None beyond the plan's model: `cogmaster_load` reads exactly 8,192 bytes from a caller pointer, but only after the pair and the exact size are validated and only when the size tag covers the field.

## Self-Check: PASSED

- `src/boot.c`, `src/instance.c`, `src/state.c`, `tests/bios/test_bios.c` and this SUMMARY exist.
- Commits `70e5691`, `13ace65`, `81bd971`, `796b7f4`, `076f13b` and `356b557` are ancestors of HEAD (`git merge-base --is-ancestor` exit 0 for each).
