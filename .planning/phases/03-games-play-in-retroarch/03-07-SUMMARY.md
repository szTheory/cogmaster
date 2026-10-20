---
phase: 03-games-play-in-retroarch
plan: "07"
status: complete
subsystem: video-geometry-and-fixtures
tags:
  - completed-fields
  - extended-modes
  - vdp-lines
  - portable-hashes
requires:
  - phase: 03-06
    provides: Independent IRQ sources, six counter sequences and fixed host deadlines
provides:
  - Completed pixel snapshots with coherent height, crop, backdrop and pixel aspect
  - Latched 192/224/240 rendering with extended name maps and PAL aspect
  - Owned guest rendering and timing matrix with independent full-field pixel oracle
affects:
  - 03-08
  - 03-09
  - 03-10
tech-stack:
  added: []
  patterns:
    - Final-physical-line tick-600 height latch independent of host deadlines
    - Separate bounded working and completed image stores
    - Guest numeric measurements judged externally against source-derived values
key-files:
  created:
    - tests/vdp/test_geometry.c
    - tests/fixtures/lines.c
    - tests/roms/src/vdp_lines.asm
    - tests/roms/src/vdp_lines.link
  modified:
    - src/internal.h
    - src/vdp.c
    - src/vdp_render.c
    - src/frame.c
    - src/instance.c
    - src/state.c
    - include/cogmaster.h
    - tests/vdp/tests.cmake
    - tests/vdp/test_timing.c
    - tests/core/test_state.c
    - tests/fixtures/tests.cmake
    - tests/cmake/hashes.cmake
    - tests/diag/expected.csv
    - tests/built/expected.txt
    - tests/inventory/ci.txt
    - tests/inventory/wasm32.txt
    - docs/hardware/video.md
    - docs/embedding.md
    - docs/conformance.md
key-decisions:
  - Latch height at final-physical-line tick 600; accesses at the tick observe the event first and miss that latch.
  - Retain separate working and completed 256x240 stores so extended active rows cannot overwrite the published image between fixed host deadlines.
  - Keep GAME-01 pending until hosted portable lanes compare the newly registered playing-frame hashes; GAME-02 now has its remaining mailbox criterion.
requirements-completed:
  - GAME-02
coverage:
  - id: completed-height-and-aspect
    description: NTSC and PAL 192/224/240 fields, final rows, 224/256 map wrap and exact PAL PAR
    requirement: GAME-01
    verification:
      - kind: unit
        ref: vdp.geometry.heights
        status: pass
      - kind: unit
        ref: vdp.geometry.coordinates
        status: pass
    human_judgment: false
  - id: completed-transition-coherence
    description: Every old/new height pair at latch-minus-one, latch and latch-plus-one on both TVs; live palette and backdrop cannot change a completed image
    requirement: GAME-01
    verification:
      - kind: unit
        ref: vdp.geometry.transitions
        status: pass
    human_judgment: false
  - id: bounded-frame-copy-and-state
    description: Full-field and crop canaries, invalid pitch no-write guarantees, fixed deadlines, allocation counts, hostile writes and validate-before-apply height restoration
    requirement: GAME-01
    verification:
      - kind: unit
        ref: vdp.geometry.buffers
        status: pass
      - kind: unit
        ref: vdp.geometry.hostile
        status: pass
      - kind: unit
        ref: vdp.geometry.state
        status: pass
    human_judgment: false
  - id: guest-rendering-matrix
    description: Six TV/height combinations check every field coordinate for tiles, both sprite sizes, zoom, SAT order, priority, flips, palette, mask, scroll locks and IRQ scroll bands
    requirement: GAME-01
    verification:
      - kind: integration
        ref: fixtures.vdp_lines.pixels.ntsc.192 through fixtures.vdp_lines.pixels.pal.240
        status: pass
      - kind: integration
        ref: runner.hashes_file
        status: pass
    human_judgment: false
  - id: guest-timing-measurements
    description: Eight numeric mailbox records per TV/height cover line and frame IRQ counts, entry H/V, two status reads and the visible V-counter 3F/40 boundary
    requirement: GAME-02
    verification:
      - kind: integration
        ref: fixtures.vdp_lines.mailbox.ntsc.192 through fixtures.vdp_lines.mailbox.pal.240
        status: pass
    human_judgment: false
  - id: fixture-negative-controls
    description: Wrong expected value, missing row and one-frame hang are rejected with named nonzero results
    requirement: GAME-02
    verification:
      - kind: integration
        ref: fixtures.vdp_lines.judge.wrong_value
        status: pass
      - kind: integration
        ref: fixtures.vdp_lines.judge.missing_row
        status: pass
      - kind: integration
        ref: fixtures.vdp_lines.judge.hang
        status: pass
    human_judgment: false
duration: 14min
completed: 2026-10-20
actuals:
  tokens: 17246
  tasks: 2
  commits: 4
plan_head_before: 07bbf2d39b7b1c6b9cb0f385bbafe42f70bf6042
plan_head_after: e878e2f7bcc731ace83d3812606aa01bb5ba8f92
---

# Phase 3 Plan 7: Completed Fields and Guest Video Matrix Summary

Coherent completed 192/224/240-line snapshots, extended name tables, PAL pixel aspect and an owned guest fixture with independent full-field rendering and timing checks.

## Accomplishments

- Both frame and information queries describe the same completed image, including initial/reset black 192-line output. Publish pixels, height, crop and backdrop at the height-specific frame event; retain the result while a following raster is partially drawn. NTSC/PAL host deadlines and bounded instruction overshoot remain unchanged.
- Select height at final-physical-line tick 600, with event-before-access ordering. Extended maps use R2 bits 3–2 plus $0700, 32x32 names and 256-pixel vertical wrap. The 192-line map retains 224-pixel wrap. $D0 terminates sprites only in 192-line mode. PAL reports 2950000:2128137; NTSC and GG retain their own ratios.
- Save working/completed pixels, height latch and completed metadata; validate allowed heights and SMS1 fallback before applying. Behavior revision 7 and state format 8 reflect the changed behavior/layout. No API or ABI layout changes.
- Build three variants of the same `vdp_lines` assembly source, run each on NTSC and PAL, compare every pixel with an independent diagonal/solid-colour construction, and judge eight guest numeric measurements. Preserve the separate `boot_to_play` oracle unchanged. Register image digests and NTSC/PAL playing hashes in the portable lane comparison.

## Task Commits

1. Task 1 RED: `ba80800` — assert completed field heights and PAL aspect.
2. Task 1 GREEN: `1c0672c` — publish coherent completed fields at every SMS height.
3. Task 2 RED: `db3f57d` — assert guest-driven extended field output.
4. Task 2 GREEN: `e878e2f` — prove rendered heights and IRQ timing through vdp_lines.

## Execution Evidence

Commands ran from the repository root; generated reports, mailbox output and ROM bytes remain ignored build output.

| Command | Exit | Result |
|---|---|---|
| ctest --test-dir build/ci -R '^vdp.geometry.heights$' --output-on-failure --output-junit geometry-red.xml | 8 | Named height/PAL aspect assertions executed and failed against 192-line output and 8:7 PAL |
| node "$HOME/.codex/gsd-core/bin/gsd-tools.cjs" check tdd-red-evidence build/ci/geometry-red.json --raw | 0 | RED_EVIDENCE_OK; planned assertions failed, no build/load/discovery fault |
| ctest --test-dir build/ci -R '^fixtures.vdp_lines.pixels.ntsc.240$' --output-on-failure --output-junit lines-red.xml | 8 | Public API guest baseline executed but produced 192 rather than the required 240 lines |
| node "$HOME/.codex/gsd-core/bin/gsd-tools.cjs" check tdd-red-evidence build/ci/lines-red.json --raw | 0 | RED_EVIDENCE_OK; guest field contract failure inspected semantically |
| cmake --workflow --preset ci | 0 | Task 1 and tracer rerun: 252 tests plus four package checks; final task 2: 267 tests plus four package checks |
| cmake --workflow --preset hygiene | 0 | All 32 final checks, including format, analyzer, source policy and budgets |
| ctest --test-dir build/ci -R '^docs.budgets$\|^policy.e2e_budget$' -V | 0 | 449 repository-check lines / 4909 src lines; 299 hosted helper lines |
| build/ci/cogmaster-run --region pal build/ci/fixtures/vdp_lines_240.sms --frames 30 --mailbox | 0 | Eight records, valid CRC; seven line IRQs, four frame IRQs, H=0036, V=00F1, status 009F/001F, V boundary 003F/0040 |
| node "$HOME/.codex/gsd-core/bin/gsd-tools.cjs" check evaluation-scope --plan 03-07 --commits-only --raw | 0 | All four plan commits resolved on this branch |
| git diff --check | 0 | No whitespace errors |

All 21 new plan test names appear in both exact native and wasm32 inventories. Native fixture checks forward cross-compiling emulators through the existing registration/command patterns. This plan did not dispatch hosted CI. The preceding [run 39480735490](https://github.com/szTheory/cogmaster/actions/runs/39480735490) is baseline evidence only, not evidence of these commits. GAME-01 remains pending for the phase's hosted cross-platform hash comparison; all planned local verification ran. GAME-02 combines the prior timing/status unit evidence with this plan's completed fixture criterion.

## TDD Gate Compliance

Both tasks have RED before GREEN. Task 1 failed the intended public height/aspect assertions. Task 2 first exercised the existing 192-line guest tracer as its RED baseline, then replaced it with the new guest fixture and expanded to the six TV/height cases and independent pixel oracle. Both unchanged JUnit reports were machine-classified and semantically inspected. The full task-1 tracer verification passed again before task 2. No refactor-only commit was needed.

## Decisions Made

The exact height latch is a documented coherent field-boundary model, not a new hardware claim. Keep separate working/completed buffers, each bounded by 256x240, because extended fields and fixed host frame deadlines overlap. Save both rather than attempting to reconstruct pixels from later register values. Guest handler H has only the source-derived CPU-tail tolerance; other mailbox expectations are exact. NTSC-240 rolling counter phase stays independent of the host frame length.

## Deviations from Plan

### Auto-fixed Issues

1. **[Rule 2 - Missing critical functionality] Version identifiers must describe changed output and saved fields.** Advance behavior/state to 7/8 in `include/cogmaster.h` and document them. Commit: `1c0672c`.
2. **[Rule 3 - Blocking issue] Existing state-test capacity and pixel sampling assumed one image.** Increase static state buffers and field capacity in `tests/core/test_state.c`; keep the two pixel streams contiguous and sample each stream's RGB444 bounds rather than unintentionally testing every alternating pixel. Existing state tests retain their meaningful boundary and refusal checks. Commit: `1c0672c`.
3. **[Rule 3 - Blocking issue] Existing synthetic timing setup skipped the new latch.** Cross final-line tick 600 explicitly before checking extended IRQ/counter tables in `tests/vdp/test_timing.c`. Commit: `1c0672c`.
4. **[Rule 2 - Missing critical functionality] Portable hash collection previously listed fixture image bytes but omitted playing fields.** Add NTSC/PAL playing hashes for both `boot_to_play` and all `vdp_lines` variants in `tests/cmake/hashes.cmake`. Commit: `e878e2f`.

The independent fixture oracle is a small test-only companion, `tests/fixtures/lines.c`; no renderer helper is shared with it. No dependency, owner action or architectural decision was needed.

## Issues Encountered

The initial exact-inventory configure correctly rejected the newly registered test until its inventory entry was added. The enlarged state exposed an alternating-pixel sweep in the test harness; grouping image streams restored proportional runtime. A warning-as-error caught a test framebuffer pointer type and was corrected before verification. All were resolved; no unrelated source issue was changed.

## Next Phase Readiness

The following GG/LCD and host geometry work can consume the completed descriptor and selected latch. Native GG 240-line LCD freeze remains the already-planned following work; this SMS-height plan does not claim it. No unresolved stub, skipped test or unrun planned verification was introduced. No new core trust boundary, external surface or tracked-file deletion was introduced.

## Self-Check: PASSED

All four created files exist, all four task commits are ancestors of HEAD, and evaluation-scope resolves them on this branch. The persisted plan ledger measures four implementation commits between the recorded base and head. Final CI and hygiene passed after the last implementation changes. All implementation changes were committed before this summary.
