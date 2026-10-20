---
phase: 03-games-play-in-retroarch
plan: "09"
status: complete
subsystem: libretro-presentation
tags:
  - game-gear
  - libretro
  - geometry
  - pal
  - retroarch
requires:
  - phase: 03-08
    provides: Native Game Gear LCD, completed fields and owned Start-driven fixture
provides:
  - Live GG crop and pixel-aspect options independent of native emulation
  - Completed-field geometry notifications and exact bounded PAL silence
  - Exact-SHA real RetroArch native GG crop and input screenshot equality
affects:
  - 03-10
  - 03-11
  - phase-verification
tech-stack:
  added: []
  patterns:
    - Probe cartridge facts before applying explicit host profile overrides
    - Notify completed displayed geometry once inside retro_run, even when refused
    - Share hosted crop arithmetic and replay driver within the physical-line budget
key-files:
  created:
    - libretro/options.h
    - tests/libretro/presentation.h
  modified:
    - libretro/libretro.c
    - libretro/cogmaster_libretro.info.in
    - tests/libretro/host.c
    - tests/libretro/tests.cmake
    - tests/retroarch/replay.c
    - tests/retroarch/compare.c
    - tests/fixtures/play.h
    - tests/inventory/ci.txt
    - tests/inventory/nofp.txt
    - tests/vdp/test_sprites.c
    - tests/cmake/repository.sh
    - .github/workflows/ci.yml
    - docs/embedding.md
    - docs/testing.md
    - docs/conformance.md
key-decisions:
  - Preserve default GG 6:5 PAR and native crop; square and hidden full-field views affect only presentation.
  - Resolve automatic machine and region from the public cartridge probe before applying explicit load-time options.
  - Reuse the existing real-host driver, replay checkpoint module and shared coordinate oracle for GG Start input.
requirements-completed:
  - GAME-03
  - GAME-07
coverage:
  - id: gg-presentation
    description: Native 4:3 and square-pixel 10:9 LCD presentation, with selected PAR applied to full fields
    requirement: GAME-03
    verification:
      - kind: integration
        ref: libretro.presentation.xrgb8888 and libretro.presentation.rgb565
        status: pass
    human_judgment: false
  - id: completed-geometry-pal
    description: All completed heights, rejected geometry notifications, fixed storage and exact PAL sample totals
    requirement: GAME-03
    verification:
      - kind: integration
        ref: tests/libretro/presentation.h through cmake --workflow --preset ci
        status: pass
    human_judgment: false
  - id: native-gg-hosted
    description: Automatic native GG profile and Start-scripted crop equal runner and independent glyph pixels
    requirement: GAME-03
    verification:
      - kind: integration
        ref: libretro.host.content.gg and libretro.host.replay.gg
        status: pass
      - kind: e2e
        ref: https://github.com/szTheory/cogmaster/actions/runs/39494627790
        status: pass
    human_judgment: false
  - id: hosted-controls-preserved
    description: SMS input and scrolling, no-content equality and pixel negative control remain successful
    requirement: GAME-07
    verification:
      - kind: e2e
        ref: https://github.com/szTheory/cogmaster/actions/runs/39494627790
        status: pass
      - kind: other
        ref: policy.e2e_budget counts 300 physical lines including the shared oracle
        status: pass
    human_judgment: false
actuals:
  tokens: 12861
  tasks: 2
  commits: 8
commits: 8
plan_head_before: 4a3f47dabc420ef18bfe361fb30b0412d255204d
plan_head_after: 831f2044aeb3585aedf416b6f1f050d54cc6ee04
duration: 13min
completed: 2026-10-20
runtime: Codex
model: GPT-6
---

# Phase 03 Plan 09: GG/PAL Presentation and Hosted Crop Summary

**Completed-field libretro presentation preserves native GG identity while a real RetroArch Start replay matches every pixel of the runner's LCD crop.**

## Accomplishments

- Added truthful v2 options, with v1 and legacy descriptors, for machine, region/timing, GG mode, pixel aspect and view. Emulation overrides apply only on content load; aspect/view update on the next frame. The public cartridge probe supplies automatic machine/region facts.
- Kept the GG default 6:5 PAR and native 160×144 crop; square pixels give native DAR 10:9. Full-field viewing exposes normally hidden pixels without selecting SMS compatibility. SMS uses its documented NTSC or PAL PAR.
- Allocated both conversion buffers at maximum geometry during load. Completed dimensions and aspect notify once from `retro_run`; a false frontend return still delivers video and audio. PAL silence storage holds 966 stereo frames, with exact rational cumulative samples through both callbacks.
- Added reference-instance pixel tests for both pixel formats, all heights, GG native/SMS modes, live presentation changes, refused geometry and load-time overrides. The actual GG fixture exercises production and replay modules with Start and released/disconnected callback cases.
- Reused the hosted driver for native GG baseline/input screenshots. The shared arithmetic oracle checks field origin (48,24), and the existing pixel control rejects a changed pixel. Simplifying the diamond oracle to its equivalent diagonal equation keeps all hosted code, including the shared oracle, at exactly 300 physical lines.

## Task Commits

1. **Task 1 — completed geometry and pacing:** `8629ace` RED option registration, `0feea28` GREEN descriptors, `eeb4731` RED completed geometry/PAL, `915c66c` GREEN presentation and pacing.
2. **Task 2 — hosted native GG crop:** `95b539e` shared hosted GG comparison, `96987b1` portable inventory/MSVC integration repairs, `c6e8ebe` automatic GG probe and real-fixture regression coverage, `831f204` MSVC unsigned button comparison.

The measured task diff is 51,443 characters, or 12,861 estimated tokens after rounding upward. Eight commits are measured from the persisted plan ledger to the recorded task head. The recorded task-commit interval is 20:35:06–20:48:01 UTC; initial context reading preceded the first commit. Nineteen files changed.

## Verification Evidence

Commands ran from the repository root; logs and images remain ignored build artifacts.

| Command | Exit | Result |
|---|---:|---|
| `cmake --workflow --preset ci` | 0 | Final corrected source passed 286 CI tests and 4 packaged-release checks. |
| `cmake --workflow --preset hygiene` | 0 | 32 checks passed; latest exact-SHA hosted hygiene also passed. |
| `cmake -DDIR=tests/retroarch -DCHECK=e2e_budget -P tests/cmake/repository.cmake` | 0 | 300 physical lines including the shared oracle. |
| `git push origin "${phase_sha}:refs/heads/phase/03-games-play-in-retroarch"` | 0 | Pushed implementation SHA `831f2044aeb3585aedf416b6f1f050d54cc6ee04`. |
| `gh workflow run ci.yml --ref phase/03-games-play-in-retroarch` | 0 | Dispatched the existing workflow for that branch/SHA. |
| `gh run list --workflow ci.yml --branch phase/03-games-play-in-retroarch --event workflow_dispatch --commit "$phase_sha" --json databaseId,headSha,event,url` | 0 | Selected run `39494627790`, exact SHA and event. Discovery was repeated after delayed indexing. |
| `gh run watch 39494627790 --exit-status --interval 10` | 0 | Entire workflow completed successfully. |
| `gh run view 39494627790 --json headSha,event,status,conclusion,jobs,url` followed by the plan's `jq -e` predicate | 0 | Exact SHA, workflow_dispatch, whole run and RetroArch job successful; all four screenshot/control predicates successful, none missing or skipped. |
| `gh run download 39494627790 --name retroarch-owned-screenshots --dir build/hosted-03-09-screenshots` | 0 | Downloaded only owned fixture images; agent viewed GG baseline, input and expected-input PNGs. |
| `sips -g pixelWidth -g pixelHeight build/hosted-03-09-screenshots/gg-input.png` | 0 | Native dimensions 160×144. Diamond/bar glyphs fill the LCD crop; no blank or misplaced window. |
| `git diff --check` | 0 | No whitespace errors. |

Accepted run: https://github.com/szTheory/cogmaster/actions/runs/39494627790

The accepted RetroArch job explicitly passed **Game Gear native crop screenshot equality**, **SMS input and scrolling screenshot equality**, **The screenshot equals the runner's frame**, and **The compare's negative control**. Its GG baseline and Start press/hold/release/repeated-press images equal runner pixels and the independent coordinate oracle. No owner verification is needed.

The same run passed six platform builds, the seven-file required hash comparison including big-endian, both no-floating-point lanes, sanitizers and wasm32's report-only comparison. In particular, its playing SMS and extended-height fixture hashes provide the hosted cross-platform evidence previously pending for GAME-01; this plan marks only its assigned GAME-03/GAME-07 requirements complete.

## TDD Gate Compliance

`libretro.host.gg` first failed its planned option-registration assertion (0 registrations versus 1). `libretro.presentation.xrgb8888` then failed the planned geometry, aspect and PAL sample assertions. Both actual CTest JUnit reports were classified by `check tdd-red-evidence`, returning RED_EVIDENCE_OK before their GREEN implementations. Semantic inspection confirmed executed behavior assertions rather than setup, syntax or load failures. Their separate RED commits precede GREEN commits. A later automatic-GG regression was also observed RED before applying the probe fix. No separate speculative refactor was needed.

The tracer's automated end-to-end verification passed before expansion. Global TDD runtime gating and auto-advance were disabled; no blocking human checkpoint applied.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] Use the consolidated repository check and companion files.**
- The planned `tests/cmake/info_claims.cmake` had already been consolidated into `tests/cmake/repository.sh`. Updated its claims mapping there. Added small option/test headers to keep modules bounded, and advertised the region option needed to select truthful PAL behavior. Existing hosted content parameterization and configuration required no duplicate driver or configuration code.
- Commits: `0feea28`, `eeb4731`, `915c66c`.

**2. [Rule 3 - Blocking] Repair cross-platform integration discovered by exact-SHA CI.**
- The no-floating-point inventory lacked API v4/v5 tests from preceding plans. MSVC rejected an earlier sprite test's constant-truncating cast; replaced it with the equivalent byte mask. The new GG button-selector comparison also needed an explicit unsigned cast for MSVC.
- Commits: `96987b1`, `831f204`.
- Initial hosted runs were not accepted. A transient macOS x64 state sweep timeout in run `39494115375` passed on subsequent exact-SHA runs without changing its test or deadline. Local nofp configuration exits 1 because it requires GCC rather than AppleClang; both actual hosted GCC lanes passed in the accepted run.

**3. [Rule 1 - Bug] Resolve automatic GG profile before loading content.**
- Running the released local RetroArch binary showed a 256×192 image for the GG fixture: the library's AUTO contract defaults to SMS2. The adapter now uses `cogmaster_probe` machine/region facts before explicit options. Added an automatic-header reference case and owned GG content/replay tests. The corrected local Start replay produced 160×144 and every pixel equaled the runner; the accepted hosted run independently repeats that proof.
- Commit: `c6e8ebe`.

## Decisions Made

Preserve the released GG presentation default and separate emulation overrides from live presentation. Reuse the public probe rather than duplicating header parsing. Keep own-fixture screenshot uploads and one shared bounded driver; no new dependency or source admission was needed.

## Next Plan Readiness

GAME-03's full native GG/ports/Start/SMS-mode and hosted-crop criterion is now covered with plan 03-08's subsystem evidence. GAME-07 remains proven. Plan 03-10 can proceed with its input/reset and battery adapter work. No implementation stub, skipped planned check, new dependency, ROM bytes or unresolved execution blocker remains.

## Self-Check: PASSED

Both newly created headers and this summary exist. All eight listed task commits are ancestors of the recorded task head. The persisted ledger yields eight commits; task head matches the accepted hosted SHA. The working diff passes `git diff --check`, and the agent inspected the hosted owned screenshots.
