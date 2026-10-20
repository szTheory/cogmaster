---
phase: 03-games-play-in-retroarch
plan: "10"
status: complete
subsystem: controller-input
tags:
  - pins
  - pause
  - reset
  - lag
  - libretro
requires:
  - phase: 03-09
    provides: Completed-field production adapter, GG native and SMS-mode controls, hosted playing evidence
provides:
  - Complete independent pad pins, TR outputs and export-SMS1-only physical Reset
  - Held-reconnect Pause suppression using saved absolute input history
  - SMS SOFT and GG POWER frontend Restart with visible guest residue assertions
  - Guest-derived runner lag_frames reports and independent per-pad fixture counters
affects:
  - 03-11
  - 03-14
  - phase-verification
tech-stack:
  added: []
  patterns:
    - Reuse saved absolute input records for device and edge history
    - Expose guest-observed input pins and restart residue through normal video callbacks
    - Count lag from completed core run results rather than host input polling
key-files:
  created:
    - tests/io/input_cases.h
    - tests/libretro/controls.h
  modified:
    - src/io.c
    - src/frame.c
    - include/cogmaster.h
    - libretro/libretro.c
    - runner/main.c
    - tests/roms/src/boot_to_play.asm
    - tests/fixtures/play.c
    - tests/runner/tests.cmake
    - docs/embedding.md
    - docs/runner.md
    - docs/conformance.md
key-decisions:
  - Reuse the existing serialized pending/current absolute records instead of adding redundant Pause arming state.
  - A missing poll or state callback is a disconnected input source, preserving reconnect suppression.
  - Preserve existing reset contracts and exercise actual frontend battery bytes and generation when plan 03-14 exposes them.
requirements-completed:
  - GAME-04
  - GAME-03
coverage:
  - id: independent-pad-pins
    description: Every pad bit, simultaneous opposing directions, TR outputs, region echo and physical Reset availability
    requirement: GAME-04
    verification:
      - kind: unit
        ref: io.pad_pins and io.port3f_region
        status: pass
      - kind: integration
        ref: libretro.controls.xrgb8888 and libretro.controls.rgb565
        status: pass
    human_judgment: false
  - id: pause-input-history
    description: Held and duplicate inputs, release and repress, disconnected and held reconnect, interleaved instances, state restoration and refusal before apply
    requirement: GAME-04
    verification:
      - kind: unit
        ref: io.pause_history and core.state.record_sweep
        status: pass
      - kind: unit
        ref: vdp.pause_nmi covers settled NTSC and PAL last-line ticks
        status: pass
    human_judgment: false
  - id: guest-lag-and-play
    description: Separate and overlapping pad sequences change counters, CRAM, scroll and frame hashes; only decoded enabled guest reads clear lag
    requirement: GAME-04
    verification:
      - kind: unit
        ref: io.lag_decode exhaustively checks all low port addresses on four profiles
        status: pass
      - kind: integration
        ref: fixtures.boot_to_play.input, fixtures.boot_to_play.pad2 and fixtures.boot_to_play.both_pads
        status: pass
      - kind: integration
        ref: runner.lag and runner.stop
        status: pass
    human_judgment: false
  - id: frontend-reset-device-contract
    description: Port-1 console ownership, immediate frame-boundary release, held reconnection, physical Reset versus SMS SOFT and GG POWER Restart
    requirement: GAME-04
    verification:
      - kind: integration
        ref: libretro.controls.xrgb8888 and libretro.controls.rgb565
        status: pass
    human_judgment: false
  - id: gg-start-and-aliases
    description: Native active-low Start is a level, SMS-mode Start queues Pause, and only native 00 plus C0/C1/DC/DD reads qualify
    requirement: GAME-03
    verification:
      - kind: unit
        ref: io.lag_decode and vdp.gg.ports
        status: pass
      - kind: integration
        ref: libretro.controls.xrgb8888, libretro.controls.rgb565 and fixtures.boot_to_play.gg.start
        status: pass
    human_judgment: false
actuals:
  tokens: 12255
  tasks: 2
  commits: 4
commits: 4
plan_head_before: c99e537f7e3a51843911b65044636e9fe5f50517
plan_head_after: 4a85a8c065293b0e7eec1d98997be0d047e9d390
duration: 10min
completed: 2026-10-20
runtime: Codex
model: GPT-6
---

# Phase 03 Plan 10: Complete Input and Lag Summary

Independent controller pins and saved Pause history now drive normal guest execution, while frontend Restart and runner lag reporting follow the core's existing contracts.

## Accomplishments

- Mix TR output directions and levels over pad inputs, with Japanese output echo low. Preserve each of the six independent direction/button bits and both opposing-direction combinations. Only export SMS1 exposes physical Reset, owned by port 1; it never resets the CPU.
- Suppress a held reconnect until release and a new press. Existing serialized pending/current input records carry all necessary history. Interleaved instances, repeated held submissions, restored history and invalid input/state refusal have explicit tests. Existing NTSC/PAL NMI timing checks continue to pass without CPU stepping changes.
- Retain the shared controller decode as the sole source of lag credit. Exhaustively check all 256 low port addresses for export SMS, Japanese SMS, native GG and GG SMS mode. Peeks, disabled and undriven ports do not earn credit. A guest IN loop clears the frame flag; a halted guest does not.
- Extend the owned SMS fixture to keep independent pad counters. Separate and overlapping press/hold/release/repress schedules verify those counters and the unchanged visible colour/scroll arithmetic. The public oracle also asserts lag on each frame: setup frames 1–3 and mailbox-sealing frame 12 are lagged, all other frames execute controller reads. The SMS image pin changes intentionally; GG bytes and all visible picture oracles remain unchanged.
- Treat missing frontend callbacks as disconnected sources, poll once per frame, and keep console controls on port 1. A small guest turns input pins, boot counts and NMI counts into backdrop colours through the production video callback in both pixel formats. SMS Restart retains boot residue through SOFT; GG Restart clears it through POWER. Physical Select leaves the boot count unchanged.
- Report `lag_frames=N` after successful `--input` runs, counting completed run-result flags. No-read and read fixtures, disconnected pads and a signal stop after exactly three complete frames exercise the report. Behaviour revision advances to 9; the API, ABI and state format remain unchanged.

## Task Commits

1. Task 1 RED: `d9fe83b` — pin and reconnect assertions.
2. Task 1 GREEN: `c805328` — core pins/history and independent guest pad counters.
3. Task 2 RED: `b37a4d1` — frontend Restart/reconnect and missing lag output assertions.
4. Task 2 GREEN: `4a85a8c` — adapter connection/reset semantics, runner reporting and documentation.

The persisted ledger measures four implementation commits. The realized task diff is 49,018 characters, or 12,255 estimated tokens after rounding upward, across 23 files. Duration is approximately ten minutes including context loading; the first task commit was at 20:52:31 UTC.

## Verification Evidence

Commands ran from the repository root. Reports and assembled images remain ignored build artifacts; no evidence log is committed. These local commands have no hosted run URL.

| Command | Exit | Result |
|---|---:|---|
| `ctest --test-dir build/ci -R '^io\.(pad_pins|pause_history|lag_decode)$' --output-on-failure --output-junit red-input.xml` | 8 | Planned TR/Reset and held-reconnect assertions failed; existing decode passed. |
| `ctest --test-dir build/ci -R '^libretro.controls' --output-on-failure --output-junit red-controls.xml` | 8 | Both pixel formats exposed POWER-on-SMS and extra reconnect NMI. |
| `ctest --test-dir build/ci -R '^runner.lag$' --output-on-failure --output-junit red-lag.xml` | 8 | Successful guest run omitted the required lag report. |
| `node "$HOME/.codex/gsd-core/bin/gsd-tools.cjs" check tdd-red-evidence build/red-03-10-input.json --raw` | 0 | RED_EVIDENCE_OK. |
| `node "$HOME/.codex/gsd-core/bin/gsd-tools.cjs" check tdd-red-evidence build/red-03-10-controls.json --raw` | 0 | RED_EVIDENCE_OK. |
| `node "$HOME/.codex/gsd-core/bin/gsd-tools.cjs" check tdd-red-evidence build/red-03-10-lag.json --raw` | 0 | RED_EVIDENCE_OK. |
| `cmake --workflow --preset ci` | 0 | Final source: 293 tests and four packaged-release checks pass; exact CI inventory enforced. |
| `cmake --workflow --preset hygiene` | 0 | All 32 checks pass, including analyzer, format, hygiene and budgets. |
| `ctest --test-dir build/ci -R '^docs.budgets$\|^policy.e2e_budget$' -V` | 0 | 449 repository-check lines / 5,013 core lines; hosted helper total stays 300. |
| `git diff --check` | 0 | No whitespace errors. |

CI and wasm32 inventories contain all new applicable cases, and cross-compiling commands forward the existing emulator. No new hosted run was required. The previously accepted run https://github.com/szTheory/cogmaster/actions/runs/39494627790 remains evidence for preceding presentation/play work only, not these commits. GAME-03 was already complete; this plan preserves and expands its controls. GAME-04 now combines the earlier region/TV and hosted playing evidence with complete pins, edges and guest lag assertions.

## TDD Gate Compliance

Both tasks have committed RED before GREEN. Actual unchanged CTest JUnit reports returned RED_EVIDENCE_OK before implementation. Semantic inspection confirmed executed planned assertions, with no setup, syntax, discovery or load failures. The core tests failed on missing TR overrides, unsupported Reset and an extra held-reconnect NMI. The frontend tests failed on reset residue and callback reconnection; runner lag failed on absent output despite exit 0. No separate refactor was needed. Global TDD runtime gating and auto-advance were disabled. The automated core integration workflow passed before host expansion, and the final workflow reran all task checks.

## Deviations from Plan

### Auto-fixed Issues

1. **[Rule 2 - Correctness] Reuse saved input history and identify changed behavior.** The existing state visitor already serializes and validates both absolute records and the lag-read flag. They are sufficient for reconnect arming, so no redundant field or state-format change was added. Explicit corrupted-field/reject-before-apply and round-trip tests verify this. Behaviour revision 9 identifies changed pin semantics. Commits: `c805328`, `4a85a8c`.
2. **[Rule 3 - Blocking] Add focused test companions and widen the fixture branch.** Added `tests/io/input_cases.h` and `tests/libretro/controls.h` to keep existing modules bounded, extended the existing public fixture oracle, and changed the SMS backward branch to JP after its displacement exceeded JR range. The GG branch and image bytes remain unchanged. Commits: `d9fe83b`, `c805328`, `b37a4d1`, `4a85a8c`.
3. **[Rule 1 - Test bug] Account for the mailbox sealing frame.** Initial runner expectation counted three setup lag frames but omitted frame 12, occupied by the fixed-size mailbox CRC. A per-frame prefix run established the fourth lag frame. The independent public fixture oracle now asserts every frame flag; runner reporting sums those actual results. Commit: `4a85a8c`.

No external dependency, ROM bytes, new trust boundary, skipped verification, implementation stub or unresolved blocker was introduced. The preparation reset wording was already partly reconciled; updated the remaining Select/profile wording and documented reconnect semantics. As specified by the plan, the existing reset APIs are unchanged and the exact frontend battery-byte/generation preservation assertions are repeated in plan 03-14 when those surfaces exist.

## Self-Check: PASSED

Both new headers and this summary exist. All four listed implementation commits are ancestors of the recorded task head. The persisted ledger measures four commits between its base and task head. Final CI, release packaging, hygiene and whitespace checks passed after the final implementation changes.
