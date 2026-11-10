---
phase: 05-tune-up
plan: "01"
subsystem: docs
tags: [docs, perf, ci, policy]

requires:
  - phase: 04-sound
    provides: the perf job's workflow_dispatch control mode and the docs files at their cap
provides:
  - two zero-spread perf control runs on main, recorded before any arming work
  - docs/hosted.md, docs/perf-fuzz.md, docs/saves.md and docs/boards.md, moved verbatim out of testing.md and embedding.md
  - docs.api_names reading a list of guide files, with a two-guide self test
affects: [05-02, 05-08, 05-14, phase 6 boards docs, phase 7 saves docs]

actuals:
  tokens: 12600
  tasks: 2
  commits: 1

plan_head_before: ee7690475280101b600b8c3b9359a09c74d9a8c9
plan_head_after: fc56cee41de8bb4f1b79f630d13bc5254c58d08f
commits: 1

tech-stack:
  added: []
  patterns:
    - "A repository check that compares against several guide files takes them as a newline-separated DOC list and lets awk open them through ARGV"
    - "A semicolon inside one element of a self-test command list is written as backslash plus the semicolon generator expression"

key-files:
  created:
    - docs/hosted.md
    - docs/perf-fuzz.md
    - docs/saves.md
    - docs/boards.md
  modified:
    - docs/testing.md
    - docs/embedding.md
    - docs/audio.md
    - docs/conformance.md
    - tests/cmake/repository.sh
    - tests/policy/tests.cmake

key-decisions:
  - "The opening battery-test paragraph of testing.md moved to docs/saves.md under a Testing saves heading: it is about saves, it sat above the page's own introduction, and it frees 1.5 KB"
  - "The api_names awk adds the DOC paths to its own ARGV in BEGIN instead of looping in shell, so the change is two awk lines and one trimmed argument"
  - "Pointers left behind in the source guides are plain sentences, not headings, so the moved section titles exist in one file only"

patterns-established:
  - "Move whole sections verbatim, then fix only the cross-links the move breaks"

requirements-completed: []

coverage:
  - id: D1
    description: "Two perf control runs on current main each print a perf control boot_to_play line with spread 0, and both run URLs are recorded here before any arming work"
    requirement: TUNE-02
    verification:
      - kind: e2e
        ref: "gh run view 42752737395 --log and gh run view 42752770307 --log, each grep for the control line with spread=0 (exit 0 each); hosted runs https://github.com/szTheory/cogmaster/actions/runs/42752737395 and https://github.com/szTheory/cogmaster/actions/runs/42752770307"
        status: pass
    human_judgment: false
  - id: D2
    description: "Every docs/*.md is under 38,912 bytes, so none is within 2 KiB of the 40,960-byte cap; the largest is conformance.md at 29,625 bytes"
    requirement: TUNE-05
    verification:
      - kind: unit
        ref: "for f in docs/*.md loop of wc -c against 38912 (exit 0) and ctest --test-dir build/ci -R '^docs\\.' (8 of 8 passed)"
        status: pass
    human_judgment: false
  - id: D3
    description: "docs.api_names reads embedding.md, saves.md and boards.md together, and its self test proves the list is honoured: the second guide's message shows only when the second file is read"
    requirement: TUNE-05
    verification:
      - kind: unit
        ref: "tests/policy/tests.cmake docs.api_names and docs.api_names.self_test (passed in the ci preset); the same command with only the first guide prints one message, with both prints two"
        status: pass
    human_judgment: false
  - id: D4
    description: "The topic files hold the moved sections, the source guides no longer have them, and every docs cross-link to a moved section resolves"
    requirement: TUNE-05
    verification:
      - kind: other
        ref: "grep for the four moved headings in the new files (exit 0), grep for them in testing.md and embedding.md (exit 1 each), git grep of the old testing.md anchors in docs (exit 1)"
        status: pass
    human_judgment: false
  - id: D5
    description: "docs/testing.md Budgets names embedding.md, saves.md and boards.md as the guides docs.api_names reads"
    requirement: TUNE-05
    verification:
      - kind: other
        ref: "awk extraction of the Budgets section piped to grep for saves.md and boards.md (exit 0)"
        status: pass
    human_judgment: false
---

# Phase 5 Plan 1: Control runs and docs split Summary

Two zero-spread perf control runs on main (12302839 instructions per frame, three times each) and the testing and embedding guides split into four topic files, with `docs.api_names` reading all three public-name guides.

## Perf control runs

- https://github.com/szTheory/cogmaster/actions/runs/42752737395
- perf control boot_to_play runs=12302839,12302839,12302839 spread=0
- https://github.com/szTheory/cogmaster/actions/runs/42752770307
- perf control boot_to_play runs=12302839,12302839,12302839 spread=0

## Evidence

Both runs measured main head `f69ab03335f84fb51c900e93c9343e209df6751e` (the head of origin/main after `git fetch`). Plan 05-14 quotes the run IDs in the arming comment and the ship step copies the section above into the phase pull request body.

| Command | Exit status |
|---|---|
| `gh workflow run ci.yml --ref main` (run 42752737395) | 0 |
| second `gh workflow run ci.yml --ref main`, sent once the first run was `in_progress` (run 42752770307) | 0 |
| `gh run watch 42752737395 --exit-status` | 0 |
| `gh run watch 42752770307 --exit-status` | 0 |
| `gh run view <id> --log` for each run, then grep for `perf control boot_to_play runs=N,N,N spread=0` | 0 |
| `gh api repos/szTheory/cogmaster/actions/runs/<id> --jq .id` for each run (event workflow_dispatch, branch main, conclusion success) | 0 |
| `cmake --workflow --preset ci` | 0 |
| `cmake --workflow --preset hygiene` | 0 |
| `ctest --test-dir build/ci -R '^docs\.' --output-on-failure` | 0 |

The two runs gave the same figure, so the measurement repeats across separate runners as well as within a run. The first run was in progress when the second was dispatched, so at most one run was pending in the concurrency group.

## What moved

- `docs/testing.md` (40,953 to 30,417 bytes) gave "Performance" and "Fuzzing" to `docs/perf-fuzz.md` and "Hosted RetroArch" and "Content rendering and input" to `docs/hosted.md`. It keeps one pointer paragraph in the CI section, an updated intro sentence, and the two job-table cells that said "(below)".
- `docs/embedding.md` (40,385 to 32,948 bytes) gave "Probing an image" to `docs/boards.md` and "Battery media" to `docs/saves.md`, and keeps a one-line pointer to each.
- The opening battery-test paragraph of `testing.md` moved to `docs/saves.md` under a "Testing saves" heading (the plan left this to judgement).
- Cross-links fixed: `audio.md` (hosted.md), `conformance.md` (perf-fuzz.md), the `fuzz` lane row in `testing.md`, and the probing link in `embedding.md`.
- `docs.api_names`: `DOC` is a list in `tests/cmake/repository.sh`; `tests/policy/tests.cmake` passes the three guides and the self test uses `guide_a.md` (names `COGMASTER_KEPT`) and `guide_b.md` (names `cogmaster_removed`) with its regex unchanged. No test was added, so no inventory changed.

## For plan 05-08

The two code comments that name moved sections are untouched here, as planned:

- `.github/workflows/nightly.yml` lines 2 and 3 cite `docs/testing.md` "Performance" and "Fuzzing"; both now live in `docs/perf-fuzz.md`.
- `src/battery.c` line 2 cites `docs/embedding.md` for battery media; it now lives in `docs/saves.md`.

`src/internal.h` line 60 and `runner/main.c` line 209 cite `docs/embedding.md` for fault kinds and names, which stay there.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Self-test command lost its list value**
- **Found during:** Task 2
- **Issue:** The self test packs its inner command into one list with `$<SEMICOLON>`, so a `DOC` value holding two paths would be split into separate arguments and the second path read as a command.
- **Fix:** The inner `-DDOC=` element writes its separator as a backslash before the semicolon generator expression, which CMake's list expansion keeps inside one element. Checked by running the inner command by hand with one guide (one message) and two (both messages).
- **Files modified:** `tests/policy/tests.cmake`
- **Commit:** fc56cee

**Total deviations:** 1 auto-fixed (1 bug in my first draft of the self test, not in existing code). **Impact:** none on scope.

## Known Stubs

None.

## Threat Flags

None. No network endpoint, auth path or trust boundary changed; the workflow dispatches ran the existing `perf` job unmodified.

## Self-Check: PASSED
