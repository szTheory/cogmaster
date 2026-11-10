---
phase: 06-korean-cartridges-play
plan: "04"
subsystem: mapper
tags:
  - mapper
  - mapper-ext
  - state
  - perf
status: complete

requires:
  - phase: 06-korean-cartridges-play
    provides: plan 06-02 public board constants COGMASTER_BOARD_KOREAN to COGMASTER_BOARD_4PAK and the format number bump
provides:
  - src/mapper_ext.c with one default arm per function, reached by tail call from the default arm of each mapper.c switch
  - struct cogmaster__mapper grown to 12 bytes with bank8[6] and raw[2], visited by the state code only for boards 4 and up
  - internal board enum values for the six Korean and multicart boards
  - boot_to_play Ir/frame on the phase branch measured against main, delta_bp=0
affects: [06-06, 06-07, 06-11]

actuals:
  tokens: 1800
  tasks: 2
  commits: 1

plan_head_before: 1fb7ca8ccd270634cf0914a599603f66eb6226ba
plan_head_after: c75ed9bfa95014e2b5f8412a248c66df1fe9e5fa
commits: 1

tech-stack:
  added: []
  patterns:
    - "One switch arm per board in a second translation unit, no function pointer table, so the library keeps no relocated data"
    - "Bank state for later boards saved only when the board needs it, so existing states keep their fields"

key-files:
  created:
    - src/mapper_ext.c
  modified:
    - src/mapper.c
    - src/internal.h
    - src/state.c
    - CMakeLists.txt

key-decisions:
  - "The reset's leading clear is a memset of the whole mapper struct, so bank8 and raw start at zero for every board"
  - "bank8 fits a byte because the largest accepted image is 64 pages, so at most 128 banks of 8 KiB"

requirements-completed: []

coverage:
  - id: C1
    description: "The Sega, Codemasters and ROM-only arms in src/mapper.c are unchanged; each switch default tail-calls a mapper_ext.c function and no if precedes either switch"
    requirement: MAP-08
    verification:
      kind: other
      ref: "git diff -U0 origin/main -- src/mapper.c exit 0; added lines other than mapext, memset, string.h and break count 0; removed lines naming an existing arm count 0; grep -c cogmaster__mapext_ src/mapper.c prints 3"
      status: pass
    human_judgment: false
  - id: C2
    description: "struct cogmaster__mapper holds bank8[6] and raw[2]; the state visitor reads them only for boards 4 and up, each bank8 below twice the page count, and the size tripwire reads 12"
    requirement: MAP-07
    verification:
      kind: unit
      ref: "ctest --test-dir build/ci -R core.state exit 0 (part of the 84-test selection in task 1)"
      status: pass
    human_judgment: false
  - id: C3
    description: "Boards 0 and ROM-only behave as before through mapper_ext.c: reads past the image give $FF, ROM writes record rom_write, $FFFC-$FFFF writes do nothing"
    requirement: MAP-07
    verification:
      kind: integration
      ref: "ctest --test-dir build/ci -R '^(mapper\\.|battery\\.|core\\.state|fault\\.|fixtures\\.|abi\\.(globals|undefined|symbols)$|libretro\\.host\\.)' exit 0, 84 of 84 passed"
      status: pass
    human_judgment: false
  - id: C4
    description: "boot_to_play Ir/frame on the phase branch is within +1.00 percent of main: 12302847 against 12302839, delta_bp=0, with both dispatch runs green"
    requirement: MAP-08
    verification:
      kind: other
      ref: "gh workflow run ci.yml on the branch and on main, gh run watch --exit-status exit 0 for both; run URLs under Perf delta"
      status: pass
    human_judgment: false

duration: 40 min
completed: 2026-11-10
---

# Phase 6 Plan 04: Mapper seam Summary

A second translation unit now holds every board outside the Sega, Codemasters and ROM-only arms, reached by a tail call from each mapper switch default, with the 8 KiB bank state saved only for boards 4 and up; the gated boot_to_play workload costs 8 instructions per frame more than main (0 bp).

## Accomplishments

- `src/mapper_ext.c` carries `cogmaster__mapext_read`, `_write` and `_reset`, each with only its `default:` arm, reproducing the old default behaviour exactly (read gives $FF, ROM writes fault, $FFFC-$FFFF do nothing). Plans 06-06 and 06-07 add the board arms.
- `src/mapper.c` default arms call the ext functions; the reset clears the whole struct. Nothing sits before either `switch (m->cart.board)`.
- `struct cogmaster__mapper` is 12 bytes; the state visitor adds `mapper_bank8` (0 to 2*pages-1) and `mapper_raw` only when `board >= COGMASTER__BOARD_KOREAN`.
- The internal board enum gained the six public Korean and multicart values.

## Mapper diff against origin/main

The whole diff of `src/mapper.c` (recorded here because it is the evidence for the unchanged arms):

```
+#include <string.h>
-    p->ctrl = 0u;
+    memset(p, 0, sizeof *p);
-        p->bank[0..2] = 0u;                      (reset default)
+        cogmaster__mapext_reset(m);
-        return 0xFFu;                            (read default)
+        return cogmaster__mapext_read(m, addr);
-        the ROM-write fault under addr < 0xC000  (write default, moved verbatim to mapper_ext.c)
+        cogmaster__mapext_write(m, addr, v);
```

## Perf delta

Command: `gh workflow run ci.yml --ref phase/06-korean-cartridges-play` and `gh workflow run ci.yml --ref main`, then `gh run watch <id> --exit-status` (exit 0 for both), then `gh run view <id> --log` and the `perf control boot_to_play` line.

- Branch run: https://github.com/szTheory/cogmaster/actions/runs/42781709144, head c75ed9bfa95014e2b5f8412a248c66df1fe9e5fa, conclusion success
  `perf control boot_to_play runs=12302847,12302847,12302847 spread=0`
- Main run: https://github.com/szTheory/cogmaster/actions/runs/42781713344, head 8c97f8e1d073d22b69e615f37c917886d06d73eb
  `perf control boot_to_play runs=12302839,12302839,12302839 spread=0`
- Main head SHA for reuse while origin/main has not moved: 8c97f8e1d073d22b69e615f37c917886d06d73eb (value 12302839).
- H=12302847, B=12302839, delta_bp=0 (shell integer arithmetic; 8 instructions per frame, under 0.007 percent). The limit is 100. `tests/perf/allow.txt` is untouched: `git diff --quiet origin/main -- tests/perf/allow.txt` exits 0 (the branch changed nothing there).

## Deviations from Plan

None - plan executed exactly as written. The default arm needed no tail-call disassembly check since delta_bp was far under the limit.

## Issues Encountered

None.

## Known Stubs

None. The ext functions have only a default arm by design; boards 4 and up are not yet reachable, which plans 06-06 and 06-07 resolve.

## Next Phase Readiness

Ready for the next plan in wave 2. Plans 06-06 and 06-07 add a case per board in `src/mapper_ext.c` and the shared 8 KiB read; no unused helper was added now (the build uses -Werror).

## Plan verification

- `cmake --workflow --preset ci` exit 0; `cmake --workflow --preset hygiene` exit 0.
- `git diff --quiet origin/main -- tests/audio tests/testcard_pins.cmake` exit 0; no line removed or added under `tests/built`.

## Self-Check: PASSED

src/mapper_ext.c found; commit c75ed9b is an ancestor of HEAD; both run URLs recorded.
