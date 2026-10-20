---
phase: 03-games-play-in-retroarch
plan: "01"
subsystem: video-input-integration
tags:
  - mode4
  - input
  - libretro
  - fixtures
requires:
  - phase: 02
    provides: Bounded Z80, stamped VDP ports, mappers and mailbox fixtures
provides:
  - Production 192-line SMS background renderer with independent coordinate oracle
  - API v3 absolute input and bounded runner script reader
  - Content-loaded production libretro video and controller integration
affects:
  - 03-02
  - 03-03
  - 03-07
  - 03-10
tech-stack:
  added: []
  patterns:
    - Monotonic line rendering before stamped source mutations
    - Validate-before-apply absolute frame input
    - Independent pixel construction shared by test frontends
key-files:
  created:
    - src/vdp_render.c
    - runner/input.c
    - runner/input.h
    - tests/api/v3.c
    - tests/fixtures/play.c
    - tests/fixtures/play.h
    - tests/roms/src/boot_to_play.asm
    - tests/roms/src/boot_to_play.link
    - docs/hardware/video.md
  modified:
    - include/cogmaster.h
    - src/frame.c
    - src/state.c
    - src/io.c
    - libretro/libretro.c
    - tests/libretro/host.c
key-decisions:
  - Save emitted pixels with the rendering cursor so partially rendered fields restore exactly.
  - Keep script records in canonical field order and validate the complete record count before execution.
  - Seal the fixture mailbox after eight consecutive samples so CRC work cannot delay the input measurements.
  - Keep phase-wide game requirements pending until their later expansion and real-host checks are complete.
requirements-completed: []
coverage:
  - id: sms-background-tracer
    description: Owned CPU-executed SMS tiles, display-off backdrop and immutable completed lines
    requirement: GAME-01
    verification:
      - kind: unit
        ref: vdp.background
        status: pass
      - kind: integration
        ref: fixtures.boot_to_play
        status: pass
      - kind: integration
        ref: fixtures.boot_to_play.wrong_hash
        status: pass
    human_judgment: false
  - id: absolute-input-tracer
    description: First-boundary press, hold, release and second press with independent counter and scroll values
    requirement: GAME-04
    verification:
      - kind: unit
        ref: abi.api_v3
        status: pass
      - kind: unit
        ref: io.absolute_input
        status: pass
      - kind: integration
        ref: runner.input
        status: pass
      - kind: integration
        ref: fixtures.boot_to_play.input
        status: pass
      - kind: integration
        ref: fixtures.boot_to_play.mailbox
        status: pass
    human_judgment: false
  - id: production-libretro-tracer
    description: In-memory content, fresh input, descriptor claims and independently correct callback pixels equal to the runner
    requirement: GAME-07
    verification:
      - kind: integration
        ref: libretro.host.content
        status: pass
      - kind: integration
        ref: libretro.info_claims
        status: pass
    human_judgment: false
actuals:
  tokens: 21653
  tasks: 3
  commits: 7
plan_head_before: e56e63ee4efd3b56cc9791960ce04fcf7d3aa30f
plan_head_after: 9e361e391ca6aedf65a089ae9848ffc052060945
duration: 16min
completed: 2026-10-20
status: complete
---

# Phase 03 Plan 01: SMS play tracer Summary

**An owned ROM now draws Mode 4 tiles and responds to absolute pad input through the library, runner and production libretro module, with independently calculated pixels.**

## Performance

- Started: 2026-10-20T18:04:58Z
- Completed: 2026-10-20T18:20:00Z
- Runtime: Codex
- Model: GPT-6
- Tasks: 3
- Files changed: 42
- Actual tokens: ceiling of realized git diff bytes divided by four, measured before this metadata commit.

## Accomplishments

- The fixed, instance-owned renderer decodes four-plane tiles, names, palette selection, flips and scroll into the 192-line RGB444 field. Display-off uses the palette backdrop. Stamped writes cannot repaint completed lines. Cursor and emitted pixels participate in state validation and restoration.
- API v3 adds a 32-byte input struct and an appended lag result without changing legacy output writes. Pending levels latch at the next frame boundary; Pause is edge-detected. The fixed-memory script reader rejects malformed, missing and surplus records before running and supports LF, CRLF and a final unterminated record.
- The owned fixture records eight input samples and proves counts 1, 1, 1, 2 on press, hold, release and second press at frames 5–8. Its color counter and scroll change actual pixels. Independent coordinate construction, mailbox CRC/value checks and a changed-digest negative control keep production-path agreement from serving as its own oracle.
- The production libretro module loads in-memory content, polls once, submits fresh levels and advertises tested pad/none and descriptors. Every callback coordinate from frame 4 is checked against the independent oracle; the final native digest equals the runner. None devices and missing state or poll callbacks release pins. Existing card, RGB565 and both silence delivery paths remain tested.

## Task Commits

1. Task 1 RED: `52ff006` — planar pixel assertion fails on the previously blank field.
2. Task 1 GREEN: `f224096` — renderer, owned fixture and runner oracle.
3. Task 1 correction: `e5a37b2` — native RGB444 red-low channel order.
4. Task 2 RED: `15ecdc4` — executable API v3 contract assertion.
5. Task 2 GREEN: `7772f90` — absolute input, script grammar and frame-boundary fixture.
6. Task 3 RED: `e864c4e` — production module refuses valid owned content.
7. Task 3 GREEN: `9e361e3` — production content/input/render integration and truthful metadata.

## Verification Evidence

All commands ran from the repository root. Local checks have no hosted run URL; no logs are committed.

| Command | Exit | Result | Run URL |
|---|---|---|---|
| `cmake --workflow --preset ci` | 0 | 192 tests and 4 archive checks pass; exact inventory enforced | Not applicable, local |
| `cmake --workflow --preset hygiene` | 0 | All 23 hygiene, policy, formatting and analyzer checks pass | Not applicable, local |
| `ctest --test-dir build/hygiene -R '^hygiene.format$' --output-on-failure` | 0 | Formatting passes after final layout assertions | Not applicable, local |
| `git diff --check` | 0 | No whitespace errors | Not applicable, local |
| `cmake --workflow --preset wasm32` | 1 | Optional local configuration stopped because the expected Linux WASI compiler toolchain is unavailable | Not applicable, local |

The CI and wasm32 inventories include the new portable renderer, input and fixture checks. Cross-platform execution is left to the configured hosted lanes; no local wasm32 result is claimed.

## TDD Gate Compliance

Each task has a committed RED before implementation. Real CTest JUnit reports were classified with `node` invoking `check tdd-red-evidence`; all three returned `RED_EVIDENCE_OK` before GREEN. Records and reports remain ignored build artifacts.

- Rendering RED: `ctest --test-dir build/ci -R '^vdp.background$' --output-on-failure --output-junit tdd-render.xml`, exit 8. The target reached the planned assertion: pixel zero differed from expected red. This was a behavior failure, not discovery or setup failure. The subsequent RGB444 correction aligns red with the low nibble of the existing public format.
- Input RED: `ctest --test-dir build/ci -R '^abi.api_v3$' --output-on-failure --output-junit tdd-input.xml`, exit 8. The target executed and established absence of the planned versioned input contract. GREEN additionally exercises input validation and legacy result sizes.
- Libretro RED: `ctest --test-dir build/ci -R '^libretro.host.content$' --output-on-failure --output-junit tdd-libretro.xml`, exit 8. The runner and independent pixels passed first; the host reached its assertion that the valid owned image loads, and the production adapter refused it.
- Refactor: clang-format cleanup accompanies the verified GREEN commits; no separate behavior-free commit was needed.

## Deviations from Plan

### Auto-fixed Issues

1. **[Rule 1 — Bug] Corrected native channel order.** The public format and existing palette helper require red in the low nibble. Fixed renderer and independent construction, then reran CI. Commit: `e5a37b2`.
2. **[Rule 3 — Blocking] Expanded state test storage and bounded the pixel rejection sweep.** Saving emitted pixels exceeded the prior state test buffers and made repeated identical per-pixel range checks quadratic. Increased buffers and retained edge-pixel rejection checks plus complete byte-identical state round trips. Commit: `f224096`.
3. **[Rule 2 — Correctness] Versioned changed behavior/state and returned the actual backdrop.** Behavior revision 2 and state format 3 identify the new content behavior and larger state layout; the frame metadata now reports the rendered palette backdrop, with a public-frame assertion. Commit: `9e361e3`.
4. **[Rule 2 — Documentation] Updated the stale README status.** It previously said games cannot load. The short status now names the experimental background/input scope and silent audio. Commit: `9e361e3`.

The additional fixture oracle files and ABI/I/O test registrations are necessary companions to the planned checks. No external code, dependency, ROM bytes or new service was introduced. Assembled images remain in the ignored build tree and their source-derived digest is pinned in `tests/built/expected.txt`; the fixture manifest needs no new binary entry.

## Requirement Scope

This plan supplies the declared tracer contributions to GAME-01, GAME-04 and GAME-07. Those phase requirements remain pending: sprites/extended modes, broader controllers/regions/lag and the real RetroArch content session belong to later plans. The in-process libretro host is integration evidence, not the real-host session. Consequently `requirements-completed` is empty rather than overstating whole-phase completion.

## Deferred Issues

The optional local wasm32 command could not configure. It is recorded in `.planning/WINDOWS.md` for resolution from a hosted wasm32 run. Every required task `<verify>` ran and passed. No goal-blocking stub remains; expanded video/audio and real-host coverage are the existing later plans' scope.

## Next Plan Readiness

The renderer is the single content route to extend. Its line cursor samples a whole line before an active write; subsequent raster plans widen that scoped model. The fixed frame origin, deadline/overshoot checks, integer-only core and no-allocation tests remain green. Continue with plan 03-02 within the current execution step.

## Self-Check: PASSED

Nine created artifacts exist and all seven task commits are ancestors of HEAD. The measured ledger contains seven commits between the recorded before/after revisions. Required CI verification passed after the final source change.
