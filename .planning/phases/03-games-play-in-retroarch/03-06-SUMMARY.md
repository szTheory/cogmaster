---
phase: 03-games-play-in-retroarch
plan: "06"
status: complete
subsystem: video-timing
tags:
  - interrupts
  - counters
  - th-capture
  - state
requires:
  - phase: 03-05
    provides: Bounded sprite slots, monotonic active-dot flags and observe-before-clear status
provides:
  - Independent line reload counter and pending source with immediate IE gating
  - Six literal mode and TV V-counter sequences with independent rolling NTSC-240 phase
  - Qualified active-origin H translation and stable rising-TH capture
affects:
  - 03-07
  - 03-08
  - 03-09
tech-stack:
  added: []
  patterns:
    - Events at or before the stamped access precede IE writes and status reads
    - Independent interval tables and coordinate anchors rather than mirrored implementation formulas
    - Shared effective TH pin mixing for readback and capture
key-files:
  created:
    - tests/vdp/test_timing.c
  modified:
    - src/vdp.c
    - src/internal.h
    - src/state.c
    - src/io.c
    - include/cogmaster.h
    - tests/vdp/tests.cmake
    - tests/bus/test_vdp.c
    - tests/io/test_io.c
    - tests/inventory/ci.txt
    - tests/inventory/wasm32.txt
    - tests/diag/expected.csv
    - tests/roms/src/ports.asm
    - docs/hardware/video.md
    - docs/embedding.md
    - .planning/preparation/SMS-VDP.md
key-decisions:
  - Retain frame 607, line 608, latch 600 and V step 604; recompute either gated pending source immediately.
  - Keep the NTSC-240 rolling 263-count sequence independent of the fixed 262-line host deadline; mode-entry phase is a selected model.
  - Translate H origin by 32 ticks as an explicit inference; use shared effective TH pins for low-to-high capture and hold until capture or reset.
  - Keep GAME-02 pending until the later vdp_lines fixture mailbox criterion is demonstrated.
requirements-completed: []
coverage:
  - id: irq-edge-and-reload-ordering
    description: Frame and line edges, R10 zero/one/FF schedules, writes before/at/after reload, first postactive decrement and blank reload
    requirement: GAME-02
    verification:
      - kind: unit
        ref: vdp.timing.irq_edges
        status: pass
      - kind: unit
        ref: vdp.timing.irq_reload
        status: pass
      - kind: unit
        ref: vdp.timing.mode_edges
        status: pass
    human_judgment: false
  - id: irq-pending-cpu-and-state
    description: Disabled IE preserves pending, enabling or disabling recomputes INT, status clears both sources, owned IM1 loop counts six line interrupts and one frame interrupt, saved pending cannot disagree with CPU INT
    requirement: GAME-02
    verification:
      - kind: unit
        ref: vdp.timing.irq_pending
        status: pass
      - kind: integration
        ref: vdp.timing.irq_cpu
        status: pass
      - kind: unit
        ref: vdp.timing.irq_state
        status: pass
    human_judgment: false
  - id: all-v-counter-intervals
    description: Every entry and discontinuity of all six literal NTSC and PAL height tables at ticks 603, 604 and 605, including NTSC-240 rolling wrap
    requirement: GAME-02
    verification:
      - kind: unit
        ref: vdp.timing.v_ntsc_192
        status: pass
      - kind: unit
        ref: vdp.timing.v_ntsc_224
        status: pass
      - kind: unit
        ref: vdp.timing.v_ntsc_240
        status: pass
      - kind: unit
        ref: vdp.timing.v_pal_192
        status: pass
      - kind: unit
        ref: vdp.timing.v_pal_224
        status: pass
      - kind: unit
        ref: vdp.timing.v_pal_240
        status: pass
    human_judgment: false
  - id: translated-h-and-th-capture
    description: All 171 H values and four phases, SMS active 08 and GG crop 20 anchors, held capture, no recapture on falling or unchanged levels, Japanese readback mixing and native GG absence of TH
    requirement: GAME-02
    verification:
      - kind: unit
        ref: vdp.timing.hcounter
        status: pass
      - kind: unit
        ref: vdp.timing.th_capture
        status: pass
      - kind: unit
        ref: io.port3f_region
        status: pass
      - kind: unit
        ref: io.gg_ports
        status: pass
      - kind: unit
        ref: vdp.timing.th_edges
        status: pass
    human_judgment: false
  - id: rolling-phase-state-and-deadline
    description: Counter phase persists across fixed host frame deadlines and state restoration, invalid captured H gap values refuse before apply, reset releases capture
    requirement: GAME-02
    verification:
      - kind: unit
        ref: vdp.timing.deadlines
        status: pass
      - kind: unit
        ref: vdp.timing.counter_state
        status: pass
      - kind: integration
        ref: cmake --workflow --preset ci
        status: pass
    human_judgment: false
duration: 17min
completed: 2026-10-20
actuals:
  tokens: 15580
  tasks: 2
  commits: 4
plan_head_before: 1526b97c68c85d91c5aafb22fd88852ea6e8cf63
plan_head_after: 42e8cf171bb7561181cd2046576d9c3a1dd2b511
---

# Phase 3 Plan 6: Interrupt, Counter and TH Timing Summary

Independent frame/line pending IRQs, six V-counter sequences and qualified H-origin/TH capture timing with fixed host deadlines.

## Accomplishments

- Keep frame pending at tick 607, line counter/test at 608, next-line latch at 600 and V visibility at 604. R10 writes affect reload only. Each pending source is independent and immediately gated by its IE; status sees then clears both sources. An owned interrupt-driven loop records six line interrupts and one frame interrupt.
- Check every entry in six independent literal counter tables at edge-minus-one, edge and edge-plus-one. NTSC-240 saves a rolling 263-count phase independently of the unchanged 262-line host frame deadline; PAL stays 313 lines. SMS1 unsupported extended combinations retain explicit 192-line timing fallback.
- Translate active-origin H by 32 ticks, explicitly identified as a coordinate/sub-dot inference. Independent intervals give SMS active HC08, GG left HC20, jump at active tick 560 and wrap at 652. Shared TH readback/capture mixing captures low-to-high transitions and holds until another capture/reset; native GG has no TH capture.
- Validate saved pending against CPU INT, preserve rolling phase and held H, and reject impossible H gap values before applying state. Behavior revision is 6 and state format 7; API/ABI layouts remain unchanged.

## Task Commits

1. Task 1 RED: `eb1d730` — assert line IRQ reload and pending races.
2. Task 1 GREEN: `cfa2bd0` — deliver line IRQs and pending source races.
3. Task 2 RED: `3a9d3ac` — assert counter intervals and TH capture.
4. Task 2 GREEN: `42e8cf1` — model mode counters and held TH captures.

## Execution Evidence

Commands ran from the repository root. Reports/evidence records are ignored build output, never committed logs.

| Command | Exit | Result |
|---|---|---|
| ctest --test-dir build/ci -R '^vdp.timing.irq_pending$' --output-on-failure --output-junit timing-red.xml | 8 | Named planned IE1-enable assertion executed and returned INT=0 instead of 1 |
| node "$HOME/.codex/gsd-core/bin/gsd-tools.cjs" check tdd-red-evidence build/ci/timing-red.json --raw | 0 | RED_EVIDENCE_OK from the actual unchanged JUnit report |
| ctest --test-dir build/ci -R '^vdp.timing.hcounter$' --output-on-failure --output-junit counter-red.xml | 8 | Named planned H interval assertions executed; old active-origin count was 00 instead of 08 |
| node "$HOME/.codex/gsd-core/bin/gsd-tools.cjs" check tdd-red-evidence build/ci/counter-red.json --raw | 0 | RED_EVIDENCE_OK from the actual unchanged JUnit report |
| cmake --workflow --preset ci | 0 | Task 1 and tracer rerun: 234 tests plus 4 package checks; final implementation: 246 tests plus 4 package checks |
| cmake --workflow --preset hygiene | 0 | All 32 final checks, including formatting, analyzer, source policy and budgets |
| ctest --test-dir build/ci -R '^docs.budgets$|^policy.e2e_budget$' -V | 0 | 449 repository-check lines / 4864 src lines; 299 hosted helper lines |
| git diff --check | 0 | No whitespace errors |

All 17 timing registrations appear in both exact native and wasm32 inventories. No optional local WASI run was attempted without its toolchain. Deterministic core checks required no new hosted dispatch; the preceding phase baseline [run 39480735490](https://github.com/szTheory/cogmaster/actions/runs/39480735490) predates this work and is not hosted evidence of these commits. Final phase-wide hosted verification remains with the orchestrator.

## TDD Gate Compliance

Both tasks have RED before GREEN. Classifier acceptance was followed by semantic inspection: the named target executed and failed its intended IRQ or H-coordinate assertion, without setup/discovery/build errors. Task 1's full automated tracer verification ran again before task 2. No refactor-only commit was necessary.

## Decisions Made

Preserve the locked 607/608 choices and distinguish their confidence from approximate latch/V positions. The H translation, mode-entry rolling phase and software-driven TH edge model are selected, confidence-qualified behavior; automated agreement is not a hardware measurement. Tests use independent interval tables and literal anchors. GAME-02 remains pending because its full requirement also names the later `vdp_lines` fixture mailbox proof.

## Deviations from Plan

### Auto-fixed Issues

1. **[Rule 1 - Bug] Existing ports fixture assumed H stayed live after its region probe.** Once TH capture exists, the earlier low-to-input transition holds H on SMS. Its later two reads therefore differ by zero; GG has no TH and retains the independent 11–12-count live difference. Correct the per-profile oracle and assembly comments. No fixture ROM bytes or fixture digests changed. Files: `tests/diag/expected.csv`, `tests/roms/src/ports.asm`. Commit: `42e8cf1`.
2. **[Rule 2 - Missing critical functionality] Version identifiers and embedding prose must describe changed behavior and saved fields.** Advance behavior/state to 5/6 for task 1 and 6/7 for task 2; document the final values. Files: `include/cogmaster.h`, `docs/embedding.md`. Commits: `cfa2bd0`, `42e8cf1`.

## Issues Encountered

An IRQ-specific enabled-display test initially populated an unterminated zero SAT and observed legitimate sprite overflow. Give it an explicitly terminated SAT to isolate IRQ flags. The preparation PAL-240 shorthand subtracted 56 after line 266, contradicting its own literal D2 table; the corrected derivation subtracts 57. Reconcile stale preparation line-191/frame-edge and shared-607 prose with the selected active-origin model. No package install, authentication gate or owner action was needed.

## Next Phase Readiness

Height/rendering work can reuse selected timing modes, fixed host deadlines and saved rolling phase. Extended counter behavior does not claim the later extended renderer or GG LCD model. No unresolved stub, skipped test or unrun planned verification was introduced. No new trust boundary or external surface was introduced; no tracked file was deleted.

## Self-Check: PASSED

The timing test exists and all four recorded commits are ancestors of HEAD. The persisted ledger measures four implementation commits and the recorded base/head match that measurement. All implementation files were committed before this summary; no implementation change remains untracked or modified.
