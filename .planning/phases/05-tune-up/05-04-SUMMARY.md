---
phase: 05-tune-up
plan: "04"
subsystem: ci
tags: [ci, wasm32, hash-equality, ci-required, tests]
status: complete

requires:
  - phase: 05-tune-up
    provides: measured workflow timeouts (plan 05-03)
provides:
  - hash-equality over eight files with wasm32 among them
  - CI required needs wasm32
  - policy.ci_required with no report-only exemption
affects: [05-06, 05-14]

actuals:
  tokens: 2500
  tasks: 2
  commits: 1

plan_head_before: 7baab625c6f464e12ae0548451173daa093102f2
plan_head_after: 240988443f32648465d3136789caa7769dfb596c
commits: 1

tech-stack:
  added: []
  patterns:
    - "A lane is promoted by deleting its exemption, not by adding a second one"

key-files:
  created: []
  modified:
    - .github/workflows/ci.yml
    - tests/hygiene/tests.cmake
    - tests/cmake/ci_required.cmake
    - tests/cmake/repository.cmake
    - docs/testing.md
    - docs/maintaining.md

key-decisions:
  - "The wasm32 job keeps its own 5-minute floor comment unchanged; only its separate download-and-compare step went, since hash-equality now compares the uploaded hashes-wasm32"

requirements-completed: [TUNE-10]

coverage:
  - id: D1
    description: "wasm32 promotion precondition: ten or more of the last 20 successful main push runs logged that wasm32 hashes.txt equals Linux x64's"
    requirement: TUNE-10
    verification:
      - kind: integration
        ref: "the plan verify loop over gh run list and gh run view logs exit 0, printing wasm32 equal runs: 12"
        status: pass
    human_judgment: false
  - id: D2
    description: "hash-equality needs wasm32, drops the rm -rf hashes/hashes-wasm32 line and compares eight files; CI required needs wasm32"
    requirement: TUNE-10
    verification:
      - kind: integration
        ref: "grep -q -- -DCOUNT=8 .github/workflows/ci.yml exit 0; grep -q 'rm -rf hashes/hashes-wasm32' exit 1; the awk over CI required's needs exit 0; actionlint with the ubuntu-26.04 ignore exit 0"
        status: pass
    human_judgment: false
  - id: D3
    description: "policy.ci_required passes with no report-only exemption, and its self test still fails the skipped-build case as expected"
    requirement: TUNE-10
    verification:
      - kind: unit
        ref: "ctest --test-dir build/hygiene -R '^policy\\.ci_required' exit 0, 2 of 2 tests; grep -rq REPORT_ONLY tests .github exit 1"
        status: pass
    human_judgment: false
  - id: D4
    description: "No doc says wasm32 only reports, is outside CI required, or waits for promotion"
    requirement: TUNE-10
    verification:
      - kind: other
        ref: "grep -F over the five removed sentences in docs/testing.md exit 1; grep -F 'promoting wasm32' docs/maintaining.md exit 1"
        status: pass
    human_judgment: false
  - id: D5
    description: "The promotion is one commit that carries the ci.yml, hygiene test and ci_required check changes together"
    requirement: TUNE-10
    verification:
      - kind: other
        ref: "git show --stat HEAD lists ci.yml, tests/hygiene/tests.cmake and tests/cmake/ci_required.cmake in commit 2409884, exit 0"
        status: pass
    human_judgment: false
  - id: D6
    description: "The repository gates stay green after the change"
    requirement: TUNE-10
    verification:
      - kind: integration
        ref: "cmake --workflow --preset hygiene exit 0 (38 of 38); cmake --workflow --preset ci exit 0 (399 of 399)"
        status: pass
    human_judgment: false
---

# Phase 5 Plan 04: Promote wasm32 into hash-equality and CI required Summary

**wasm32's `hashes.txt` is now the eighth file `hash-equality` compares and `wasm32` is in `CI required`'s needs, in one commit with the `ci_required` test change that drops the report-only exemption.**

## Performance

- **Tasks:** 2 of 2, 1 commit (Task 1 commits nothing by design)
- **Files:** 6 modified

## Accomplishments

- Task 1 re-derived the promotion criterion from the API: 12 of the last 20 successful main push runs logged `wasm32 hashes.txt equals Linux x64's`. The other 8 listed runs do not exist because only 12 successful main push runs are on record.
- Task 2, in commit `2409884`: `hash-equality` needs `[changes, build, be32, wasm32]`, compares `-DCOUNT=8` and no longer removes `hashes-wasm32`; the `wasm32` job lost its report-only comment and its download-and-compare step; `CI required` needs `wasm32`; the `REPORT_ONLY` exemption left `tests/cmake/ci_required.cmake`, the key left `tests/cmake/repository.cmake`'s forwarded list, and both `-DREPORT_ONLY=wasm32` arguments plus the explaining sentence left `tests/hygiene/tests.cmake`; `docs/testing.md` and `docs/maintaining.md` no longer describe wasm32 as report-only or pending promotion.

## Task 1 evidence: the equality count

Commands, run from the repository root, with scratch files in the session scratchpad and no log committed:

| Command | Exit |
|---------|------|
| `gh run list --workflow ci.yml --branch main --event push --status success --limit 20 --json databaseId --jq '.[].databaseId'` | 0 |
| per run: `gh run view <id> --json jobs --jq '.jobs[] \| select(.name == "wasm32") \| .databaseId'`, then `gh run view <id> --job <job id> --log`, then `grep -q "wasm32 hashes.txt equals Linux x64's"` | 0 for all 12 |
| the plan's verify loop in one command, ending `[ "$n" -ge 10 ]` | 0, printed `wasm32 equal runs: 12` |

Run list (12 IDs returned): 42741490263 42728887374 42728289180 42725926350 42723573438 39517506314 39517457197 39517093391 39456530047 39455213405 39346780486 39346572559.

Per run, `<run id> <wasm32 job id> <match>`:

```
42741490263 129584605944 match
42728887374 129542547099 match
42728289180 129540478850 match
42725926350 129532475548 match
42723573438 129524379753 match
39517506314 118953809672 match
39517457197 118953755600 match
39517093391 118952479713 match
39456530047 118745463691 match
39455213405 118743308196 match
39346780486 118381537460 match
39346572559 118380857906 match
```

Count: 12 (the criterion needs at least 10). Example run URL: https://github.com/szTheory/cogmaster/actions/runs/42741490263.

## Task 2 evidence

| Command | Exit |
|---------|------|
| `cmake --preset hygiene` | 0 |
| `ctest --test-dir build/hygiene -R '^policy\.ci_required' --output-on-failure` | 0, 2 of 2 |
| `grep -q -- '-DCOUNT=8' .github/workflows/ci.yml` | 0 |
| `grep -q 'rm -rf hashes/hashes-wasm32' .github/workflows/ci.yml` | 1 |
| `awk '/name: CI required/ {f=1} f && /needs:/ {found = ($0 ~ /wasm32/); exit} END {exit !found}' .github/workflows/ci.yml` | 0 |
| `grep -rq 'REPORT_ONLY' tests .github` | 1 |
| `grep -F` over the five removed `docs/testing.md` sentences | 1 |
| `grep -F 'promoting wasm32' docs/maintaining.md` | 1 |
| `git show --stat HEAD` lists ci.yml, tests/hygiene/tests.cmake and tests/cmake/ci_required.cmake | 0 (3 of 3 files in 2409884) |
| `actionlint -ignore 'label "ubuntu-26.04' .github/workflows/ci.yml` | 0 |
| `cmake --workflow --preset hygiene` | 0, 38 of 38 |
| `cmake --workflow --preset ci` | 0, 399 of 399 |

No hosted run of the change exists yet; plan 05-06's phase-branch run is the first, and the merge-commit run on main is a ship-time confirmation for plan 05-14.

## Task Commits

1. **Task 1: count main runs whose wasm32 hashes equal Linux x64's** - no commit (evidence only, recorded above)
2. **Task 2: promote wasm32 into hash-equality and CI required** - `2409884` (ci)

## Deviations from Plan

None - plan executed exactly as written.

Observation, not a deviation: `.planning/todos/pending/post-merge-ci-follow-ups.md` and the STATE.md Pending Todos line still name wasm32 promotion as pending. They are planning records outside this plan's file list; plan 05-14 is the place to close that todo.

## Known Stubs

None.

## Threat Flags

None. No new endpoint, auth path or file access; a required check now covers one more lane.

## Self-Check: PASSED

- commit 2409884 is an ancestor of HEAD
- tests/cmake/ci_required.cmake, tests/hygiene/tests.cmake and .github/workflows/ci.yml found and changed in that commit
