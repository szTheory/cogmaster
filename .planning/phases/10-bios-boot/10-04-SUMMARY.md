---
phase: 10-bios-boot
plan: "04"
subsystem: bus
tags:
  - bios
  - bus
  - arbitration
  - hot-path
status: complete

requires:
  - 10-03
provides:
  - struct cogmaster__bios (8 KiB image, size, crc32) as the last member of struct cogmaster and mem_slow beside io
  - cogmaster__bus_decode, recomputed at the $3E write and at the end of io_reset
  - one shared decoder (mem_slow_driven) behind the cold read path and cogmaster_peek, the closed-form BIOS and cartridge arbitration per machine
  - bios.arbitration and bios.slow_flag with their controls on the ci and wasm32 lanes
  - SMS-SYSTEM section 3 and section 9 item 6 amendments and the BIOS clause in the first fault row of docs/embedding.md
affects:
  - 10-05

actuals:
  tokens: 7200
  tasks: 2
  commits: 2

plan_head_before: a3f0c5e29779e51c4d6f759d4161af395ca81a7a
plan_head_after: f7bcbffd7e67f0aa2085e55e2e799dfa11bf3d38
commits: 2

tech-stack:
  added: []
  patterns:
    - "A derived flag the hot path loads and tests, while the cold decoder and peek decode from the registers directly, so a register written from outside the recompute sites still reads truthfully"
    - "An expected table typed by hand in the test, a measured table from the core, one comparator, and controls that run the comparator over every one-entry flip and three wrong whole tables"

key-files:
  created:
    - tests/bios/test_bios.c
    - tests/bios/tests.cmake
  modified:
    - src/bus.c
    - src/io.c
    - src/internal.h
    - tests/tests.cmake
    - tests/inventory/ci.txt
    - tests/inventory/wasm32.txt
    - docs/embedding.md
    - .planning/preparation/SMS-SYSTEM.md

key-decisions:
  - "mem_undriven is gone: the cold path and peek share mem_slow_driven, and the hot path is one load and test of m->mem_slow (named in two places in src/bus.c: the recompute in cogmaster__bus_decode and the test in mem_read)"
  - "With a BIOS attached, an undriven slot-area read with b5 or b7 clear returns bus.data | profile.bus_pullup (the expression cogmaster__io_open_bus returns) with no fault and no open_bus_reads bump; with no BIOS every undriven read is today's fault"
  - "The power-on value of $3E with a BIOS is not stated in SMS-SYSTEM section 3 or section 6, so nothing was labelled there; plan 05 adds the code comment and docs/bios.md"

requirements-completed: []

coverage:
  - id: F1
    description: "A read at a slot-area address returns the cartridge on an SMS1 and the AND of the BIOS and the cartridge on an SMS2 when both answer, the one that answers when only one does, and nothing driven otherwise, at $0000, $1FFF, $2000 and $C000, over the 16 combinations of $3E b7, b6, b5 and b3, with read and peek agreeing"
    requirement: BIOS-05
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci -R '^bios\\.arbitration$' exit 0; literal 2-by-4 value table and a literal fault column typed in the test"
        status: pass
    human_judgment: false
  - id: F2
    description: "The comparator rejects every one-entry flip of the 32 value and 32 fault entries, the columns swapped, the AND on both machines and the BIOS always winning, each naming the machine and combination, and a no-BIOS leg expects the fault for every b7 and b5 value"
    requirement: BIOS-05
    verification:
      - kind: unit
        ref: "bios.arbitration controls run in the same executable; one-shot mutation of the SMS2 branch of mem_slow_driven (AND replaced by the cartridge byte) made ctest -R '^bios\\.arbitration$' exit 8 with the message naming SMS2, restored exit 0"
        status: pass
    human_judgment: false
  - id: F3
    description: "m->mem_slow equals an independent recomputation after 4,096 random $3E writes on SMS1 and SMS2 with and without a BIOS and after io_reset, and a flag set stale by hand fails the comparator"
    requirement: BIOS-05
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci -R '^bios\\.slow_flag$' exit 0; deleting the decode call after the $3E write made it exit 8, deleting the decode call at the end of io_reset made it exit 8, each restored to exit 0"
        status: pass
    human_judgment: false
  - id: F4
    description: "With no BIOS attached the flag equals the old mask test bit for bit, so no hash, fault count or state byte moves, and tests/bus, tests/io, tests/vdp, tests/audio, tests/ports, testcard_pins and the frozen expected rows are unchanged"
    requirement: BIOS-05
    verification:
      - kind: integration
        ref: "cmake --workflow --preset ci exit 0 (605 tests); git diff --quiet over the unchanged paths exit 0; no removed line in expected.txt, expected.csv or manifest.txt exit 0; core.peek unchanged and green"
        status: pass
    human_judgment: false
  - id: F5
    description: "SMS-SYSTEM section 3 and section 9 item 6 carry the amended text, and the first row of the fault table gains the BIOS clause within 250 bytes"
    requirement: BIOS-05
    verification:
      - kind: other
        ref: "line-flattened grep for the new sentences exit 0, the old sentences present at origin/main exit 0 and gone exit 0; wc -c docs/embedding.md is 35616 (+95); ctest -R '^(docs\\.|policy\\.)' exit 0"
        status: pass
    human_judgment: false
  - id: F6
    description: "The hot path costs nothing: mem_read swaps one masked test for one load and test, and the perf gate stays within +1.00 percent on boot_to_play"
    requirement: BIOS-05
    verification:
      - kind: other
        ref: "measured at the plan 05 hosted dispatch (both runs= lines and delta_bp); not measured in this plan"
        status: unknown
    human_judgment: false
---

# Phase 10 Plan 04: Bus decode and BIOS arbitration Summary

The bus decodes the documented BIOS and cartridge arbitration per machine (SMS1 cartridge wins, SMS2 ANDs, the BIOS answering only `$0000`-`$1FFF`) behind a derived `mem_slow` flag, with one shared decoder for the cold read path and `cogmaster_peek`, and an independent closed-form table with controls proves it. Nothing loads a BIOS yet, so every existing hash is unchanged.

## Performance

- **Duration:** about 40 minutes
- **Completed:** 2026-11-11
- **Tasks:** 2
- **Files:** 2 created, 8 modified

## Accomplishments

- `src/internal.h`: `COGMASTER__BIOS_SIZE` (8192), `uint8_t mem_slow` after `io`, `struct cogmaster__bios { rom[8192]; size; crc32; } bios` as the last member (no hot offset moves), `cogmaster__bus_decode` declared.
- `src/bus.c`: `cogmaster__bus_decode` sets the flag to `(c & $50) != 0 || (bios.size != 0 && !(c & $08))`; `mem_slow_driven` decodes from `io.mem_ctrl` and `bios.size` only and replaces `mem_undriven`; `mem_read` is `if (m->mem_slow != 0) return mem_read_slow(...)` else `mem_driven`; `mem_read_slow` returns the decoder's value, the no-fault open-bus value for an empty enabled slot with a BIOS attached, or the fault as before; `cogmaster_peek` calls the decoder. `bus_write` and `io_glasses_write` are not edited.
- `src/io.c`: two added lines, `cogmaster__bus_decode(m)` after `io->mem_ctrl = v;` and at the end of `io_reset`. No banned title-table token added.
- `tests/bios/`: `bios.arbitration` (16 combinations, 4 probes, both machines, read against peek, a no-BIOS leg, 64 one-entry flips, three whole-table controls) and `bios.slow_flag` (4,096 random `$3E` writes on four legs, a reset leg, a stale-flag control). Registered with `bios portable`, timeout 20 s provisional (plan 05 settles it from a hosted run), on ci and wasm32.
- `.planning/preparation/SMS-SYSTEM.md` section 3 (arbitration sentence replaced, empty-slot rule added, Game Gear bullet extended) and section 9 item 6 (the disassembly clause deleted; item 6 now reads "(6) Whether the GG BIOS stores $3E at $C000, and the VDP "after BIOS" register image are unknown.", re-read once after the edit and it still reads as a sentence). `docs/embedding.md` first fault row gained the BIOS clause (+95 bytes, 35,616 of the 35,771 allowance).

## Task Commits

1. Task 1, the flag, decoder, two recompute sites and the two tests: `62068d6`
2. Task 2, the SMS-SYSTEM amendments and the fault-row sentence: `f7bcbff`

## Evidence

All commands from the repository root; exit statuses as observed.

- `cmake --preset ci` exit 0; `cmake --build --preset ci` exit 0; `ctest --test-dir build/ci -R '^(bios\.|bus\.|fault\.|io\.|ports\.|core\.|vdp\.|audio\.)'` exit 0 (179 tests) at Task 1.
- `ctest --test-dir build/ci -N -R '^bios\.'` prints `Total Tests: 2`; `ctest -N -R '^core\.peek'` prints `Total Tests: 2` (the existing peek tests, unchanged and green).
- `cmake --workflow --preset ci` exit 0 (605 tests, plus 5 release) after each task; `cmake --workflow --preset hygiene` exit 0 (63 tests) after each task, first try (no `hygiene.tree` timeout this time).
- Unchanged proof: `git diff --quiet $(git merge-base origin/main HEAD) -- tests/io tests/vdp tests/bus tests/audio tests/ports tests/testcard_pins.cmake tests/fuzz/fuzz_run.c tests/fuzz/fuzz_loader.c` exit 0; `! git diff $(git merge-base origin/main HEAD) -- tests/built/expected.txt tests/diag/expected.csv tests/roms/manifest.txt | grep -q '^-[^-]'` exit 0.
- io.c: the added lines of `git diff -U0 $(git merge-base origin/main HEAD) -- src/io.c` match no banned token (count 0); `ctest -R '^policy\.no_title_table'` exit 0.
- Flag mentions: `grep -n 'm->mem_slow' src/bus.c` prints line 131 (in `cogmaster__bus_decode`) and line 190 (in `mem_read`) only; the decoder and `cogmaster_peek` never read the flag (the bare `grep -n mem_slow` also matches the decoder's own name `mem_slow_driven` and one comment).
- Negative controls, each by python edit with a successful rebuild (exit 0) and confirmed by the red result, then restored from a saved copy and re-run green: SMS2 branch AND replaced by the cartridge byte, `ctest -R '^bios\.arbitration$'` exit 8, message `arbitration: SMS2 b7=0 b6=0 b5=0 b3=0 value at probe 0: got 15, want 3`, restored exit 0; decode call after the `$3E` write deleted, `ctest -R '^bios\.slow_flag$'` exit 8, restored exit 0; decode call at the end of `io_reset` deleted, `bios.slow_flag` exit 8, restored exit 0.
- Container compile: `docker run --rm -v "$PWD":/w -w /w gcc:13 sh -c 'for f in src/bus.c src/io.c tests/bios/test_bios.c; do gcc -std=c17 -Wall -Wextra -Wpedantic -Werror -Wtype-limits -Wconversion -fsyntax-only -I include -I build/ci/generated -I src $f || exit 1; done'` exit 0.
- `clang-format --dry-run -Werror` (Homebrew `llvm@21`) over the four changed C files exit 0 (after one `-i` pass over the new test).
- `cmake --workflow --preset asan` exit 1 at configure (clang-21 is not installed here); instead `cc -fsanitize=address,undefined -fno-sanitize-recover=undefined` over `src/*.c` and `tests/bios/test_bios.c` built (exit 0) and both cases `arbitration` and `slow_flag` exit 0. The hosted asan job of the next dispatch decides.
- `cmake -DROOT=$PWD -P tests/cmake/budgets.cmake` exit 0: **833 counted check lines** / 9,313 src lines (src grew 70, the counted figure is unchanged; phase cap 847).
- Flattened-grep checks of the SMS-SYSTEM edit (new sentences present, old sentences present at `origin/main` and absent now): all exit 0. `wc -c < docs/embedding.md` prints 35616; the planning-ID regex over its added lines counts 0.

## Hosted dispatch

None in this plan's scope; none was run. `gh run list --workflow ci.yml` at the end shows the orchestrator's health probe on main (run https://github.com/szTheory/cogmaster/actions/runs/43056313541, in progress at the time) after the failed main run 43053345486 and the three failed branch runs of plan 02, which failed at the apt step on runner DNS. The workflow was not changed. Still open and not testable here: plan 02's all-legs criterion, the be32 and wasm32 timeouts for the plan 01 and plan 02 tests, the nofp lane line `abi.api_v14` (unproven until a hosted run), and the wasm32 and ci lanes for `bios.arbitration` and `bios.slow_flag` (registered with identical names on both inventories, ci passes locally).

## Deviations from Plan

None - plan executed as written, with these interpretations (no behaviour lost):

- The "literal fault-count column" holds, per machine and combination, the number of the four probes that fault; with a BIOS it is 0 everywhere but combination 14 (1) and 15 (3), and with no BIOS 3 where b6 is set. The same row is typed for both machines.
- The value table is indexed by the kind `2 * b6 + b3` (the plan's "2-by-4" rows), and the comparator maps each of the 16 combinations onto its kind, which is how b7 and b5 are proved not to change a value.
- The probes set the retained bus byte to `$00` before each read, so every undriven expectation is a literal (`$00` on SMS1, `$FF` on SMS2).
- The plan asked to mark the power-on `$3E` value `$E0` in SMS-SYSTEM if section 3 or 6 states it; neither does, so nothing was added there (plan 05 adds the code comment and docs/bios.md).

**Total deviations:** 0.

## Residuals (stated in the plan)

- `state_load` and `boot_power_on` do not yet recompute the flag (plan 05). `grep -n mem_ctrl tests/core/test_state.c tests/ports/test_state.c` shows only a name list and a read of the loaded value (`0xAF`/`0xA8`), no state with b6 or b4 set, so no existing test can see it.
- The hot-path cost (`delta_bp` against a fresh main dispatch) is measured at plan 05's dispatch (F6 status unknown until then).

## Known Stubs

None. The BIOS image is attached only by tests in this plan; nothing loads one until plan 05.

## Threat Flags

None. No new network, file or trust-boundary surface; the BIOS bytes are read only from the instance and never from outside it here.

## Self-Check: PASSED

- tests/bios/test_bios.c, tests/bios/tests.cmake and this SUMMARY exist.
- Commits 62068d6 and f7bcbff are ancestors of HEAD (`git merge-base --is-ancestor` exit 0 for each).
