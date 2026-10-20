---
phase: 03-games-play-in-retroarch
plan: "04"
subsystem: video-background-profiles
tags:
  - mode4
  - sms1
  - raster
  - state
requires:
  - phase: 03-03
    provides: Production SMS play path and bounded hosted screenshot helpers
provides:
  - Complete 192-line Mode 4 background flags, colours, wrapping and scroll locks
  - Monotonic dot intervals with saved line and frame latches
  - Append-only SMS1 profile with sourced address masks and retained bus byte
  - Bounded backdrop and explicit diagnostics for unsupported video modes
affects:
  - 03-05
  - 03-06
  - 03-07
  - 03-08
tech-stack:
  added: []
  patterns:
    - Exclusive raster coordinate before stamped source mutations
    - Shared revision facts and address helpers across one VDP renderer
    - Hand-calculated coordinates and independently constructed pattern address pairs
key-files:
  created:
    - tests/vdp/test_background.c
    - tests/vdp/tests.cmake
    - tests/api/v4.c
  modified:
    - src/vdp_render.c
    - src/vdp.c
    - src/internal.h
    - src/state.c
    - src/bus.c
    - src/instance.c
    - src/io.c
    - include/cogmaster.h
    - tests/fixtures/play.h
    - docs/hardware/video.md
    - docs/embedding.md
key-decisions:
  - Preserve RGB444 identity and serialize native hashes little-endian independently of host palette conversion.
  - Use the selected tick-600 next-line hscroll/display latch and final-physical-line per-frame vscroll latch, without claiming hardware measurement.
  - Follow the R6 register diagram gating A12/A11, correcting preparation's tile-bit typo to bits 7/6.
  - Keep GAME-01 pending because sprites, all heights and full fixture/platform criteria belong to later plans.
requirements-completed: []
coverage:
  - id: background-flags-and-colours
    description: All sixteen planar indices, all sixty-four SMS colours, tile bit eight, separate and combined flips, palette choice and priority-zero semantics
    requirement: GAME-01
    verification:
      - kind: unit
        ref: vdp.background.tiles
        status: pass
      - kind: unit
        ref: vdp.background.priority
        status: pass
    human_judgment: false
  - id: scrolling-and-empty-fields
    description: Horizontal and vertical wrap, fine-scroll backdrop, source-column vertical lock, top horizontal lock, left mask and display-off output
    requirement: GAME-01
    verification:
      - kind: unit
        ref: vdp.background.scroll
        status: pass
      - kind: unit
        ref: vdp.background.locks
        status: pass
      - kind: unit
        ref: vdp.background.empty
        status: pass
    human_judgment: false
  - id: immutable-raster-intervals
    description: Stamped CRAM, VRAM and backdrop writes preserve emitted dots; line and frame latches and partial-line state restore deterministically
    requirement: GAME-01
    verification:
      - kind: unit
        ref: vdp.background.intervals
        status: pass
      - kind: unit
        ref: vdp.background.latches
        status: pass
      - kind: unit
        ref: vdp.background.state_interval
        status: pass
    human_judgment: false
  - id: sms1-shared-profile
    description: SMS1 appends as value three; R2 through R6 address gates, console boot, reset and matching-profile states are checked independently
    requirement: GAME-01
    verification:
      - kind: unit
        ref: abi.api_v4
        status: pass
      - kind: unit
        ref: vdp.background.sms1_masks
        status: pass
      - kind: unit
        ref: vdp.background.sms1_state
        status: pass
      - kind: unit
        ref: core.load.profiles
        status: pass
      - kind: unit
        ref: io.open_bus
        status: pass
    human_judgment: false
  - id: bounded-unsupported-video
    description: Unsupported legacy and invalid text modes produce backdrop and a VDP fault while preserving bounded public frame delivery
    requirement: GAME-01
    verification:
      - kind: unit
        ref: vdp.background.unsupported
        status: pass
    human_judgment: false
  - id: native-field-identity
    description: Public frame pixels equal the independent fixture oracle and runner little-endian native hash, with original card controls retained
    requirement: GAME-01
    verification:
      - kind: integration
        ref: fixtures.boot_to_play
        status: pass
      - kind: integration
        ref: fixtures.boot_to_play.input
        status: pass
      - kind: integration
        ref: fixtures.boot_to_play.wrong_hash
        status: pass
      - kind: integration
        ref: libretro.host.content
        status: pass
      - kind: integration
        ref: runner.hash.sms
        status: pass
      - kind: integration
        ref: runner.hash.gg
        status: pass
    human_judgment: false
actuals:
  tokens: 14394
  tasks: 2
  commits: 4
plan_head_before: 7e6b7ad572bb6cfee1be422e601b2da5e501a11a
plan_head_after: 03227608041c89e818a69e7cadc113ddfde26cfa
duration: 16min
completed: 2026-10-20
status: complete
---

# Phase 3 Plan 4: Complete background rendering and SMS1 tile behavior Summary

**One Mode 4 renderer now emits immutable raster intervals with complete background semantics and shared SMS1 address and bus rules.**

## Performance

- Duration: 16 minutes, runtime Codex, model GPT-6.
- Started: 2026-10-20T19:16:12Z.
- Completed: 2026-10-20T19:31:49Z.
- Tasks: 2; changed files: 22.
- Actual tokens use realized patch characters divided by four, rounded up; the four task commits are measured from the persisted plan ledger before this metadata commit.

## Accomplishments

- Render all name attributes, planar colours, exact SMS-to-RGB444 expansion, priority-zero rules, 32×28 scroll wrapping, fine backdrop, locks and left mask.
- Catch up only unrendered dots before mutation. Save both raster cursor components, R8/display line latches, R9 frame latch and emitted pixels; restored partial lines continue identically.
- Append SMS1=3 with AUTO=0, SMS2=1 and GG=2 unchanged. Shared profile facts select name, pattern, SAT and sprite address masks and the undriven data-bus policy.
- Save and restore the last driven bus byte. SMS1 undriven memory/port reads retain it; SMS2/GG still return $FF. Unsupported enabled display modes emit backdrop and an explicit VDP fault within the fixed deadline.
- Keep API v1–v3 programs frozen in the final tree and add the mandatory API v4 program, including a first-released-size media caller.

## Task Commits

1. Task 1 RED: `de49caf` — specify background coordinates and raster latches.
2. Task 1 GREEN: `3f91b30` — complete Mode 4 backgrounds and raster latches.
3. Task 2 RED: `c042876` — specify appended SMS1 machine profile.
4. Task 2 GREEN: `0322760` — add shared SMS1 address and bus profile facts.

## Verification Evidence

All commands ran from the repository root; generated reports and mutation scripts remain ignored build output, never committed logs.

| Command | Exit | Result |
|---|---|---|
| `ctest --test-dir build/ci -R '^vdp.background\.' --output-on-failure --output-junit red-background.xml` | 8 | Six cases executed; four failed on missing fine-scroll, locks, intervals and latch assertions |
| `node "$HOME/.codex/gsd-core/bin/gsd-tools.cjs" check tdd-red-evidence build/ci/red-background.json --raw` | 0 | RED_EVIDENCE_OK for vdp.background.scroll; actual 000 versus expected backdrop F00 |
| `ctest --test-dir build/ci -R '^abi.api_v3$' --output-on-failure --output-junit red-sms1.xml` | 8 | SMS1 load assertion returned profile rejection 6 instead of success 0 |
| `node "$HOME/.codex/gsd-core/bin/gsd-tools.cjs" check tdd-red-evidence build/ci/red-sms1.json --raw` | 0 | RED_EVIDENCE_OK for executed profile-acceptance assertion |
| `cmake --workflow --preset ci` | 0 | Task 1: 213 tests plus 4 package checks; task 2 final tree: 217 tests plus 4 package checks |
| `cmake --workflow --preset hygiene` | 0 | 32 checks, including formatting, analyzer, source budgets and hosted helper budget |
| `python3 build/background-mutations.py` | 0 | All 18 individual production mutations built and failed their intended pixel/address assertions with exit 1; source restored and rebuilt |
| `ctest --test-dir build/ci -R '^docs.budgets$|^policy.e2e_budget$' -V` | 0 | 449 repository-check lines / 4638 src lines; 299 hosted helper lines |
| `git diff --check` | 0 | No whitespace errors |

The mutation probes cover tile bit 8, each flip, palette, priority and zero priority, both wrapping axes, fine backdrop, both locks, left mask, display, and SMS1 R2/R3/R4/R5/R6 gates. No second renderer supplies expectations. Task 1's tracer feedback gate reran its complete CI workflow before expansion. Native hashes retain explicit low-byte-first serialization in the runner and independent fixture oracle; all native test-card controls remain passing.

Hosted baseline from the preceding plan: [run 39480735490](https://github.com/szTheory/cogmaster/actions/runs/39480735490). That run predates this plan and is not presented as hosted proof of these commits. New cases are registered in both exact native and wasm32 inventories for the continuing phase's hosted run.

## TDD Gate Compliance

Both tasks have observed RED followed by GREEN commits. The two unchanged JUnit reports were accepted by the classifier and inspected semantically: targets executed and failed planned behavior assertions, with no compilation, discovery or setup failures authorizing implementation. The SMS1 acceptance assertion moves into API v4 in the final tree to preserve frozen earlier caller programs. No refactor-only commit was needed.

## Decisions Made

- Follow the selected tick-600 hscroll/display and per-frame vscroll model; its precision remains a sourced model choice, not a new hardware claim.
- Follow SMS Power's R3/R4 plane-specific masks and R6 address diagram. R6 gates tile bits 7/6, while bit 2 supplies tile bit 8.
- Retain empty-background colour zero separately from backdrop: no SAT data contributes no sprite pixels, but an enabled zero tile still shows CRAM[0]. Display-off, masking and fine-scroll gaps show the selected backdrop.
- GAME-01 remains incomplete until later sprite, height, fixture and cross-platform criteria finish.

## Deviations from Plan

### Auto-fixed Issues

1. **[Rule 1 - Bug] Shared play oracle assumed wrapped tile pixels in the fine-scroll gap.** Corrected its independent coordinate construction to select fixture backdrop CRAM[16] there. The shared hosted helper remains 299 lines. Files: `tests/fixtures/play.h`; commit `3f91b30`; verified by fixture, libretro and screenshot tests.
2. **[Rule 2 - Missing critical functionality] Public additions require an API-v4 caller and append-only diagnostic values.** Added `tests/api/v4.c` and registration, retained earlier API callers, and moved invalid machine probes from 3 to 4 in the core/state tests. Behaviour revision 3 and state format 4 identify the changed frame rules and saved cursor/latches/data byte. Files: public header, API/ABI tests, inventories, core/state tests and embedding documentation; commits `3f91b30`, `0322760`.
3. **[Rule 2 - Missing critical functionality] SMS1 undriven reads require the actual retained data-bus byte.** Extended `src/bus.c` and the existing visitor rather than infer a byte from the current opcode. Every fetch/read/write drives the instance byte; observational peeks preserve it. Verified by io.open_bus and matching-profile state tests; commit `0322760`.
4. **[Rule 1 - Source correction] Preparation named the wrong upper R6 mask bit.** Rechecked the public register diagram and corrected `.planning/preparation/SMS-VDP.md` to A12/A11, tile bits 7/6. All four mask combinations have independent expected addresses; commit `0322760`.

## Issues Encountered

The first full SMS1 workflow exposed the required versioned API file and two older invalid-profile probes using the newly valid value 3. These were corrected within task 2. No package installation, authentication gate or owner action was needed.

## Next Phase Readiness

The sprite plan can consume the shared SAT, sprite-pattern and plane-address helpers and background palette/priority result. R5/R6 address behavior is already checked; sprite composition and its pixel consequences remain that next plan's work. No legacy renderer or host override spelling is introduced here. No unresolved stub prevents this plan's background goal.

## Self-Check: PASSED

All three created files and this summary exist. All four recorded task commits are ancestors of HEAD. The tree had no uncommitted implementation changes after task 2. No tracked files were deleted.
