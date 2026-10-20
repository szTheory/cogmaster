---
phase: 03-games-play-in-retroarch
plan: "05"
subsystem: video-sprites
tags:
  - mode4
  - sprites
  - sms1
  - collision
  - state
requires:
  - phase: 03-04
    provides: Monotonic raster intervals, background priority and shared SMS1 SAT/pattern address helpers
provides:
  - Eight bounded line-local sprite slots with saved shape and zoom state
  - Sprite composition, SAT ordering and SMS1 count-dependent horizontal zoom
  - Active-dot collision, ninth-match overflow and observe-then-clear status reads
affects:
  - 03-06
  - 03-07
  - 03-08
tech-stack:
  added: []
  patterns:
    - One next-line SAT scan with live pattern fetches only for unemitted dots
    - Independent planar and coordinate expectations through the public full-field API
key-files:
  created:
    - tests/vdp/test_sprites.c
  modified:
    - src/vdp_render.c
    - src/vdp.c
    - src/internal.h
    - src/state.c
    - include/cogmaster.h
    - tests/vdp/tests.cmake
    - tests/vdp/test_background.c
    - tests/bus/test_vdp.c
    - tests/roms/src/boot_to_play.asm
    - tests/built/expected.txt
    - tests/inventory/ci.txt
    - tests/inventory/wasm32.txt
    - docs/hardware/video.md
    - docs/embedding.md
key-decisions:
  - Prepare next-line SAT slots at tick 600, retaining at most eight matches from at most 64 entries and detecting a ninth match separately.
  - Use SMS1 horizontal doubling only when slot i+4 exists; double all selected source rows vertically when zoom is enabled.
  - Select first-active-dot overflow and shared-opaque-dot collision; a status read includes its current dot before clearing flags.
  - Keep GAME-01 and GAME-02 pending until later height, IRQ, fixture and platform criteria are evidenced.
requirements-completed: []
coverage:
  - id: sprite-shapes-and-clipping
    description: Independent 8x8 and tile-even 8x16 shapes, Y wrap and terminator, EC shift, clipped edges and two-axis zoom through the live full-field API
    requirement: GAME-01
    verification:
      - kind: unit
        ref: vdp.sprites.shapes
        status: pass
      - kind: unit
        ref: vdp.sprites.wrap_clip
        status: pass
      - kind: unit
        ref: vdp.sprites.zoom
        status: pass
    human_judgment: false
  - id: sprite-order-and-zoom-slots
    description: Earlier opaque SAT slot wins, transparent earlier dots reveal later slots, background priority and left mask work, SMS1 counts two/four/six/eight use the selected count-dependent rule
    requirement: GAME-01
    verification:
      - kind: unit
        ref: vdp.sprites.priority
        status: pass
      - kind: unit
        ref: vdp.sprites.sms1_zoom
        status: pass
      - kind: unit
        ref: vdp.sprites.gg_slots
        status: pass
    human_judgment: false
  - id: bounded-saved-line-work
    description: Full 64-entry scan retains eight slots, fixed-stamp write hammer cannot rescan or repaint, pattern mutations preserve emitted dots and selected slots survive partial-line state restoration
    requirement: GAME-01
    verification:
      - kind: unit
        ref: vdp.sprites.intervals
        status: pass
      - kind: unit
        ref: vdp.sprites.bounds_state
        status: pass
      - kind: unit
        ref: core.state.record_sweep
        status: pass
    human_judgment: false
  - id: active-dot-status
    description: Touching edges do not collide, shared opaque dots do, reads before/at/after an edge observe and clear flags, later overlaps reassert, and frame IRQ and control latch release on read
    requirement: GAME-02
    verification:
      - kind: unit
        ref: vdp.sprites.collision
        status: pass
      - kind: unit
        ref: vdp.sprites.sms1_collision
        status: pass
      - kind: unit
        ref: vdp.status
        status: pass
    human_judgment: false
  - id: overflow-and-exclusions
    description: Eight Y matches do not overflow, nine do independently of opacity and X; display-off, masked, clipped, transparent and inactive-dot controls exclude collision
    requirement: GAME-02
    verification:
      - kind: unit
        ref: vdp.sprites.overflow
        status: pass
      - kind: unit
        ref: vdp.sprites.inactive
        status: pass
    human_judgment: false
actuals:
  tokens: 8432
  tasks: 2
  commits: 5
plan_head_before: b93073248953954a0366f8121ba95a7d3c3deb85
plan_head_after: f148e804dfff341b06e6f81bf0e795fb568d9ac6
duration: 9min
completed: 2026-10-20
status: complete
---

# Phase 3 Plan 5: Bounded sprites and active-dot flags Summary

**The live Mode 4 field now composes eight saved SAT slots, SMS1 count-dependent zoom, ninth-match overflow and shared-opaque-dot collision.**

## Performance

- Runtime Codex, model GPT-6; two tasks, fifteen changed files.
- Duration is approximately nine minutes from the first recorded execution timestamp, 2026-10-20T19:35:24Z; initial context loading predates that timestamp.
- Actual tokens use realized patch characters divided by four, rounded up. The five commits are measured from the persisted plan ledger before this metadata commit.

## Accomplishments

- Scan once for the next active line at the selected tick-600 latch, at most 64 SAT entries and eight retained slots. A ninth Y match records overflow without a ninth rendered slot. All mutable state belongs to the instance.
- Render 8x8 and tile-even 8x16 shapes, negative Y wrap, D0 termination, EC left shift, nonwrapping X clipping, zoom, palette selection and SAT/background precedence through the same monotonic raster cursor.
- Apply SMS1 horizontal width only when i+4<count, including all requested count cases. GG sprites outside the visible LCD window still consume full-field slots.
- Observe collision at common opaque active dots, including sprites hidden by priority background. Exclude masked left pixels, transparent pixels, clipped coordinates and disabled/inactive display. Status reads include the current dot, clear flags and the control latch, and release the frame IRQ level; another genuine overlap may reassert collision.
- Save and validate selected slot coordinates/tiles/rows, scan/count bounds, zoom/shift and overflow state. Advance behavior revision to 4 and state format to 5; API 4 and ABI 1 remain unchanged.

## Task Commits

1. Task 1 RED: `e9afa45` — specify sprite shapes and slot precedence.
2. Task 1 GREEN: `31b8451` — render bounded sprite slots in the live field.
3. Task 2 RED: `31eb537` — specify sprite collision and overflow edges.
4. Task 2 GREEN: `6b20b6b` — assert sprite flags at active dot edges.
5. Version consistency fix: `f148e80` — identify sprite behavior and saved slot format.

## Execution Evidence

Commands ran from the repository root. Reports remain ignored build output, never committed logs.

| Command | Exit | Result |
|---|---|---|
| ctest --test-dir build/ci -R '^vdp.sprites.shapes$' --output-on-failure --output-junit red-sprites.xml | 8 | Named test executed and failed planned public-frame assertions: black instead of red/green sprite pixels |
| node "$HOME/.codex/gsd-core/bin/gsd-tools.cjs" check tdd-red-evidence build/ci/red-sprites.json --raw | 0 | RED_EVIDENCE_OK; no setup, discovery or build failure |
| ctest --test-dir build/ci -R '^vdp.sprites.collision$' --output-on-failure --output-junit red-sprite-status.xml | 8 | Executed edge assertions returned 1F instead of 3F at the intended shared opaque dot |
| node "$HOME/.codex/gsd-core/bin/gsd-tools.cjs" check tdd-red-evidence build/ci/red-sprite-status.json --raw | 0 | RED_EVIDENCE_OK; observed missing collision/read catch-up is the planned failure |
| cmake --workflow --preset ci | 0 | Task 1 and tracer rerun: 225 tests plus 4 package checks; final tree: 229 tests plus 4 package checks |
| cmake --workflow --preset hygiene | 0 | Final tree: all 32 checks, including source policy, formatting, analyzer and budgets |
| ctest --test-dir build/ci -R '^vdp.sprites\.' --output-on-failure | 0 | All twelve independently named sprite cases |
| ctest --test-dir build/ci -R '^docs.budgets$|^policy.e2e_budget$' -V | 0 | 449 repository-check lines / 4763 src lines; 299 hosted helper lines |
| git diff --check | 0 | No whitespace errors |

New registrations appear in both exact native and wasm32 inventories. The nofp inventory is unchanged from the plan base: its explicit early return registers ABI tests only, so sprite tests do not belong there. No new hosted dispatch was necessary for these deterministic core checks. The preceding phase baseline, [run 39480735490](https://github.com/szTheory/cogmaster/actions/runs/39480735490), predates this work and is not hosted proof of these commits.

## TDD Gate Compliance

Each task has RED before GREEN. Unchanged JUnit reports were accepted by the classifier; semantic inspection confirmed the named tests executed planned pixel/status assertions rather than failing compilation, setup or discovery. Task 1's automated tracer gate reran the full CI workflow before task 2. No refactor-only commit was needed.

## Decisions Made

Follow the selected sourced model rather than claim new hardware measurements: tick-600 next-line slot selection, count-dependent SMS1 zoom, first-active-dot overflow and observe-before-clear status ordering. Pattern bytes remain live only for unemitted dots. GAME-01 and GAME-02 remain pending because heights, complete interrupt timing and later fixture/platform criteria are outside this plan.

## Deviations from Plan

### Auto-fixed Issues

1. **[Rule 1 - Bug] Background-only test data accidentally doubled as a live SAT.** Enabling the missing renderer exposed tile data as sprite attributes in older background tests and the own play ROM. Give those cases a separate explicitly terminated SAT. Update the reproducible fixture digest to `8e39f03b9209eb37341a5cbb2ec8cd58ee7fb5f7b6dbd4675048afb1f89becce`; the independent pixel oracle and expected visible behavior stay unchanged. Files: `tests/vdp/test_background.c`, `tests/bus/test_vdp.c`, `tests/roms/src/boot_to_play.asm`, `tests/built/expected.txt`. Commit: `31b8451`; fixture, libretro and background checks pass.
2. **[Rule 2 - Missing critical functionality] Changed rendering and saved slots require version identifiers.** Increment behavior revision 4 and state format 5, documenting both without growing API/ABI. Existing API version and state header/refusal tests run in CI. Files: `include/cogmaster.h`, `docs/embedding.md`. Commit: `f148e80`.

## Issues Encountered

The GG coordinate helper initially requested the cropped frame while asserting full-field coordinates; it now explicitly requests FULL_FIELD. A source-policy check interpreted a numeric section range in a comment as a floating literal; citing the whole section avoids that ambiguity. No package installation, authentication gate or owner action was required.

## Next Phase Readiness

The following timing/height work can reuse saved slots, the monotonic dot cursor and status catch-up. This plan introduces no unresolved stub, skipped test or unrun planned verification. No new trust boundary or external surface was introduced. No tracked files were deleted.

## Self-Check: PASSED

The new sprite test and this summary exist. All five recorded commits are ancestors of HEAD; the persisted ledger measures five commits. All implementation changes were committed before this summary. No implementation file remains untracked or modified.
