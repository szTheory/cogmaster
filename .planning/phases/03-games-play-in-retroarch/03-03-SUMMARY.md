---
phase: 03-games-play-in-retroarch
plan: "03"
subsystem: hosted-frontend-input
tags:
  - libretro
  - retroarch
  - input
  - screenshots
requires:
  - phase: 03-01
    provides: Owned SMS input fixture and independent coordinate oracle
  - phase: 03-02
    provides: Strict repository and hosted helper accounting
provides:
  - Released RetroArch replay input and independently correct scrolled screenshots
  - Production callback parity and recurring screenshot negative controls
  - Exact-SHA hosted proof across all existing platform lanes
affects:
  - 03-07
  - 03-10
  - 03-15
tech-stack:
  added: []
  patterns:
    - Test-only initial checkpoint forwarding with unchanged production input and video source
    - Independent pixel semantics alongside runner screenshot equality
    - Exact workflow_dispatch head SHA and completed step predicates
key-files:
  created:
    - tests/retroarch/checkpoint.c
    - tests/retroarch/replay.c
    - tests/retroarch/oracle.c
    - tests/policy/e2e_cases.cmake
  modified:
    - .github/workflows/ci.yml
    - libretro/CMakeLists.txt
    - tests/retroarch/run_retroarch.cmake
    - tests/retroarch/compare.c
    - tests/libretro/tests.cmake
    - tests/libretro/host.c
    - scripts/hygiene.sh
    - docs/testing.md
    - docs/conformance.md
key-decisions:
  - Use released frontend replay facilities rather than add an alternate input source.
  - Forward the existing core state visitor only in an uninstalled replay module for initial checkpoints.
  - Count the shared independent oracle as hosted helper code and register the gate in both CI and hygiene.
  - Keep GAME-04 pending because this plan proves the SMS port-one path rather than every pad and profile criterion.
requirements-completed:
  - GAME-07
coverage:
  - id: hosted-sms-input-scroll
    description: Released RetroArch boots the owned SMS game and receives press, hold, release and a second press through frontend replay callbacks, producing counter and scroll two
    requirement: GAME-07
    verification:
      - kind: e2e
        ref: https://github.com/szTheory/cogmaster/actions/runs/39480735490
        status: pass
    human_judgment: false
  - id: independent-screenshot-semantics
    description: Baseline and changed screenshots equal runner pixels and the independent coordinate oracle; wrong scroll and a changed pixel both reject
    requirement: GAME-07
    verification:
      - kind: integration
        ref: libretro.screenshot.oracle
        status: pass
      - kind: integration
        ref: libretro.screenshot.oracle_control
        status: pass
      - kind: integration
        ref: libretro.screenshot.pixel_control
        status: pass
      - kind: e2e
        ref: https://github.com/szTheory/cogmaster/actions/runs/39480735490
        status: pass
    human_judgment: false
  - id: unchanged-production-callbacks
    description: Production and replay modules run the same rich frontend sequence and independently expected pixels; production still refuses save states
    requirement: GAME-04
    verification:
      - kind: integration
        ref: libretro.host.content
        status: pass
      - kind: integration
        ref: libretro.host.replay
        status: pass
      - kind: integration
        ref: libretro.host.xrgb8888
        status: pass
    human_judgment: false
  - id: complete-hosted-helper-budget
    description: All hosted helpers plus the shared oracle total 300 physical lines and combined 301-line controls reject in CI and hygiene
    requirement: GAME-07
    verification:
      - kind: integration
        ref: policy.e2e_budget
        status: pass
      - kind: integration
        ref: policy.e2e_budget.self_test
        status: pass
      - kind: integration
        ref: policy.e2e_budget.shared_helper
        status: pass
    human_judgment: false
actuals:
  tokens: 11771
  tasks: 2
  commits: 6
plan_head_before: 907b101b5f41639277054a528613ea491431b8ff
plan_head_after: 1b7109ad773d1865fc2f4eb82e99fe8456a19995
duration: 24min
completed: 2026-10-20
status: complete
---

# Phase 03 Plan 03: Hosted SMS input and scrolling Summary

**RetroArch 1.22.2 replays real frontend input into the owned SMS game, and both released and scrolled screenshots match the runner and independent pixels within a complete 300-line hosted helper budget.**

## Performance

- Started: 2026-10-20T18:46:19Z
- Hosted acceptance: 2026-10-20T19:10:06Z
- Runtime: Codex
- Model: GPT-6
- Tasks: 2
- Implementation files changed: 29
- Actual tokens: ceiling of realized implementation diff characters divided by four, measured before summary/state metadata.

## Accomplishments

- Preserved the installed no-content card, runner equality and pixel control. The shared isolated launch driver also takes owned content and frontend replay arguments. Its screenshot configuration disables smoothing and shaders.
- The baseline records the released frontend's replay. A bounded own helper checks the measured v2 event layout and changes only RetroPad B on port zero to press, hold, release and press again. Playback delivers input through RetroArch's normal callback, without adding a production input hook. A test-only adapter forwards its three state entrypoints to the existing core state visitor for the frontend's mandatory initial checkpoint. It is neither installed nor included in release archives.
- Both baseline and changed screenshots must equal runner images and every coordinate of the shared independent oracle at counter/scroll zero and two. The changed input field visibly shifts two columns and changes the counter palette. Rich per-frame assertions remain in the fixture and in-process host, run against both modules. A synthetic equal-image/wrong-scroll control and an in-memory single-pixel control also run in portable CI.
- All six platform builds, both no-float lanes, ASan, big-endian, wasm32, hygiene, performance, hash equality, RetroArch and CI required pass on the accepted implementation SHA. Only the expected dispatch-inapplicable title/rehearsal/cleanup jobs skip.

## Task Commits

1. Task 1 RED: `8341d42` — agreeing screenshots with wrong independent scroll must reject.
2. Task 1 GREEN: `0f58326` — frontend replay, real checkpoint bridge, independent screenshot comparison and complete helper accounting.
3. Task 2: `8cfd500` — production/replay callback parity, recurring screenshot and helper controls, inventories and documentation.
4. Task 2 integration correction: `c9da107` — no-float inventory, manifested binary handling, MSVC stdio and nested emulator arguments.
5. Task 2 fixture correction: `ca94eb5` — explicit newline construction and exact byte assertions across platforms.
6. Task 2 WASI correction: `1b7109a` — full-field test scratch buffers outside the 64 KiB stack.

The persisted ledger measures six commits between the recorded revisions. Summary/state metadata is separate. No tracked files were deleted.

## Verification Evidence

Commands ran from the repository root. Logs and generated images remain ignored build output; no logs are committed.

| Command | Exit | Result | Run URL |
|---|---|---|---|
| `cmake --workflow --preset ci` | 0 | 205 tests and four archive checks pass with exact inventories | Local; same preset also passes in the hosted run below |
| `cmake --workflow --preset hygiene` | 0 | 32 checks pass, including manifested/unlisted binary controls and complete hosted helper controls | Local; hosted hygiene also passes |
| `cmake -DROOT=. -P tests/cmake/budgets.cmake` | 0 | 449 counted check lines / 4,516 core lines, strictly below one tenth | Local |
| `cmake -DCHECK=e2e_budget -DDIR=tests/retroarch -P tests/cmake/repository.cmake` | 0 | 300 physical lines including the shared oracle | Local |
| `cmake -DCHECK=check_junit -DJUNIT=build/ci/junit.xml -DINVENTORY=tests/inventory/ci.txt -DEXCLUDE=release -P tests/cmake/repository.cmake` | 0 | Exact executed inventory without failure or skip | Local |
| `git push origin 1b7109ad773d1865fc2f4eb82e99fe8456a19995:refs/heads/phase/03-games-play-in-retroarch` | 0 | Exact accepted implementation pushed | [Accepted run](https://github.com/szTheory/cogmaster/actions/runs/39480735490) |
| `gh workflow run ci.yml --ref phase/03-games-play-in-retroarch` | 0 | Existing workflow dispatched | [Accepted run](https://github.com/szTheory/cogmaster/actions/runs/39480735490) |
| `gh run list --workflow ci.yml --branch phase/03-games-play-in-retroarch --event workflow_dispatch --commit 1b7109ad773d1865fc2f4eb82e99fe8456a19995 --json databaseId,headSha,event,status,url` | 0 | Indexed run selected by exact event and SHA; delayed discovery was repeated | [Accepted run](https://github.com/szTheory/cogmaster/actions/runs/39480735490) |
| `gh run watch 39480735490 --exit-status --interval 10` | 0 | Whole workflow completed successfully | [Accepted run](https://github.com/szTheory/cogmaster/actions/runs/39480735490) |
| `gh run view 39480735490 --json headSha,event,status,conclusion,jobs,url` followed by the plan's `jq -e --arg sha 1b7109ad773d1865fc2f4eb82e99fe8456a19995` predicate | 0 | Exact SHA, dispatch event, successful complete workflow/job and each required screenshot/control step verified | [Accepted run](https://github.com/szTheory/cogmaster/actions/runs/39480735490) |
| `gh run download 39480735490 --name retroarch-owned-screenshots --dir build/hosted-03-03-final-screenshots` | 0 | Downloaded only owned screenshots and inspected baseline, changed field and card | [Accepted run](https://github.com/szTheory/cogmaster/actions/runs/39480735490) |
| `git diff --check` | 0 | No whitespace errors | Local |

Accepted SHA: `1b7109ad773d1865fc2f4eb82e99fe8456a19995`. The completed successful RetroArch steps are **The screenshot equals the runner's frame**, **The compare's negative control**, and **SMS input and scrolling screenshot equality**. No absent or skipped predicate was accepted. The agent viewed the accepted run's PNGs: the baseline repeating diamond/bar field, the shifted green counter field, and the existing colour card agree with the independent semantics. Both equality checks and the changed-frame pixel control executed in the hosted SMS step.

Earlier dispatched runs were not accepted: [39479027713](https://github.com/szTheory/cogmaster/actions/runs/39479027713) exposed inventory, MSVC, binary scanning and emulator quoting issues; [39479637694](https://github.com/szTheory/cogmaster/actions/runs/39479637694) reached the Windows newline fixture failure; [39480181725](https://github.com/szTheory/cogmaster/actions/runs/39480181725) passed RetroArch but failed the whole-workflow predicate because wasm test scratch overflowed its stack. Every implementation correction received fresh pushed-SHA evidence. No owner verification was requested.

## TDD Gate Compliance

Task 1 committed RED before implementation. The actual CTest JUnit report and JSON record are ignored build artifacts. `node` invoking `check tdd-red-evidence build/red-screenshot.json --raw` returned `RED_EVIDENCE_OK`.

- RED command: `ctest --test-dir build/ci -R '^libretro.screenshot.' --output-on-failure --output-junit tdd-screenshot.xml`, exit 8. The synthetic fixture passed; `libretro.screenshot.oracle_control` executed its planned assertion and found that identical unscrolled images were accepted despite expecting counter/scroll two. This was the missing semantic check, not discovery, syntax, parser or fixture failure.
- GREEN: the comparator applies the independent oracle as well as image equality; wrong-scroll equal images reject. The full CI workflow passed after the implementation commit and was rerun as the tracer feedback gate before task 2 expansion.
- Refactor: redundant launch/configuration prose and the separate on-disk mutation path were simplified while retaining bounded bitmap parsing and the specific pixel assertion. No separate behavior-free commit was needed. Global TDD mode remained disabled.

## Deviations from Plan

1. **[Rule 3 - Blocking] Real initial checkpoints for frontend replay.** Released RetroArch records the owned game but refuses playback when its initial checkpoint cannot deserialize; a zero-byte acceptance experiment did not satisfy its loader. The permitted uninstalled bridge therefore forwards the existing state visitor and compiles the adapter input/video source unchanged. `libretro/CMakeLists.txt`, `tests/retroarch/checkpoint.c`, replay helper and workflow changes ship in `0f58326`. Production save-state support remains unchanged and is tested. The recorded format was measured from the released binary; no emulator source was opened for its implementation.
2. **[Rule 2 - Correctness] Complete size protection in CI.** The existing hosted gate was hygiene-only and did not count the now-shared independent oracle. Shared registration places it in both lanes, counts the oracle, and rejects combined 301-line helpers. BMP decoding is restricted to the uncompressed RGB24 output actually produced by `sips`; the negative control flips a channel in memory. Files include shared scanner/registration, inventories and hosted helpers. Commits: `0f58326`, `8cfd500`.
3. **[Rule 3 - Blocking] Targeted prior-plan integration corrections from actual hosted evidence.** The orchestrator explicitly assigned these blockers to this plan after the first run. Added the registered API v3 test to the no-float inventory; separated manifested binary bytes from private-text scans with both positive and unlisted controls; retained portable ISO C stdio on MSVC; preserved multi-argument emulators through the nested wrong-hash check; made LF/CRLF fixture output explicit with exact byte assertions; and moved only two test pixel scratch arrays to function-static storage, matching existing test practice. No core stack setting, production rendering or gate was weakened. Commits: `c9da107`, `ca94eb5`, `1b7109a`.

The three deviations are recorded and fixed in the broken-windows ledger. Existing entry 1 is also fixed because the accepted hosted wasm32 workflow actually executed successfully. There are no remaining open ledger entries, known stubs, skipped required checks or deferred issues from this plan.

## Requirement Scope

GAME-07's concrete integrated frontend criterion is complete. GAME-04 remains pending for its two-pad, GG Start, region/TV and lag coverage in later plans. Retain the plan's **FLAGGED unresolved edge GAME-07/unclassified** as provenance: the generic deterministic probe supplied no domain-specific edge; this plan demonstrates the explicit D-26/D-27 engineering checks and does not claim that generic flag itself was resolved. Hardware-model confidence is unchanged.

## Self-Check: PASSED

All four created implementation artifacts exist. All six task commits are ancestors of HEAD; the persisted ledger measures six commits. Accepted exact-SHA hosted predicates, all platform lanes, local inventories and both strict budgets passed. The accepted screenshots were inspected by the agent.
