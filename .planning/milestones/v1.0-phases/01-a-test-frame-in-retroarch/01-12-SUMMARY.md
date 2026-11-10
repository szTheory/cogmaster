---
phase: 01-a-test-frame-in-retroarch
plan: 12
subsystem: ci-close-out
status: complete
tags:
  - ci
  - timeouts
  - docs
  - release
requires:
  - "01-11: two green ci.yml runs (39335167905, 39336389305) and three nightly runs to measure from; draft probe PR #3"
provides:
  - "ci.yml and nightly.yml timeouts measured on this repository (run ID and sample count, provisional below ten samples)"
  - "be32 timeout scale 8 and wasm32 scale 5, measured; workflow.test_gate_timeout 4 s"
  - "docs/testing.md and docs/maintaining.md"
  - "Preparation docs consistent with what phase 1 built (size-tag cap, GG aspect, empty extensions, wasm32 toolchain)"
  - ".planning/todos/pending/post-merge-ci-follow-ups.md"
  - "Probe PR #3 closed; owner handed the ruleset command"
affects:
  - "/gsd-ship 1: opens the phase PR; its CI run is the first on 851a890"
  - "the first ci: pull request after the merge: the four follow-ups in the pending todo"
actuals:
  tokens: 14000
  tasks: 2
  commits: 2
plan_head_before: a43ecfa9456f1540d73c9fdef72af82fe3519d94
plan_head_after: 851a8903604cf3c98aa58a19df4dbbfb1720e7ee
tech-stack:
  added: []
  patterns:
    - "A timeout comment reads '<floor note;> 2 x <s> s, slowest of <n> runs, run <id>, provisional'"
    - "An emulated lane's timeout scale is the ceiling of the larger of its total and its slowest-test time ratio to the Linux x64 leg, read from the ctest per-test times in the job logs"
key-files:
  created:
    - docs/testing.md
    - docs/maintaining.md
    - .planning/todos/pending/post-merge-ci-follow-ups.md
  modified:
    - .github/workflows/ci.yml
    - .github/workflows/nightly.yml
    - CMakePresets.json
    - .planning/config.json
    - .planning/preparation/ARCHITECTURE.md
    - .planning/preparation/ENGINEERING.md
    - .planning/preparation/HOSTS-AND-INTEGRATION.md
    - .planning/STATE.md
decisions:
  - "The 5-minute floor also covers asan and perf (apt installs) and cleanup (GitHub API), as network-bound jobs; hygiene and nofp take the rule's 1 minute and build 3 minutes, all provisional on 2 samples"
  - "be32 timeout scale is 8 (shared tests 6.7x Linux x64 in total, the libretro host tests up to 7.3x); wasm32 is 5 (4.3x in total)"
  - "workflow.test_gate_timeout is 4 s: twice the slowest of four clean local runs of the ci workflow (1.61 s)"
  - "docs/testing.md gives no container one-liner: ubuntu:26.04's apt CMake (4.2.3) is older than the presets' 4.3 minimum, so the page says to take CMake from its own release there"
requirements-completed:
  - FRAME-01
  - FRAME-05
coverage:
  - id: D1
    description: "Every ci.yml timeout-minutes is twice the slowest of the two successful runs, rounded up to minutes, with run ID, sample count and provisional; the 5-minute floor applies to the network-bound and roll-up jobs; nightly bench and bench-wasm32 likewise from three runs; release.yml stays provisional with 0 samples"
    requirement: FRAME-05
    verification:
      - kind: integration
        ref: "tests/hygiene/tests.cmake#hygiene.timeout_comments (cmake --workflow --preset hygiene, exit 0)"
        status: pass
      - kind: other
        ref: "grep -Ec 'timeout-minutes: [0-9]+ +# .*(run [0-9]+|provisional)' .github/workflows/ci.yml and grep -c 'timeout-minutes:' both print 14; every cited run ID answers gh run view -R szTheory/cogmaster (exit 0)"
        status: pass
    human_judgment: false
  - id: D2
    description: "be32 and wasm32 timeout scales re-derived from this repository's runs, with the runs cited in CMakePresets.json"
    requirement: FRAME-05
    verification:
      - kind: other
        ref: "gh run view <run> --log --job <id> per-test times for be32, wasm32 and build (linux-x64) on runs 39335167905 and 39336389305; jq . CMakePresets.json (exit 0)"
        status: pass
    human_judgment: false
  - id: D3
    description: "workflow.test_gate_timeout in .planning/config.json is twice the measured local clean ci workflow time"
    requirement: FRAME-05
    verification:
      - kind: other
        ref: "rm -rf build/ci then /usr/bin/time -p cmake --workflow --preset ci, four runs, exit 0, slowest real 1.61; jq -e '.workflow.test_gate_timeout | type == \"number\" and . > 0' .planning/config.json (exit 0)"
        status: pass
    human_judgment: false
  - id: D4
    description: "With the measured timeouts pushed, the pull request's CI run for 79b6f19 is green, every job and CI required included"
    requirement: FRAME-01
    verification:
      - kind: e2e
        ref: "https://github.com/szTheory/cogmaster/actions/runs/39337625991 (plan verify command 3 against 79b6f19, exit 0)"
        status: pass
    human_judgment: false
  - id: D5
    description: "docs/testing.md and docs/maintaining.md exist under the 40 KB budget, carry no planning IDs and no project version, and pass docs.budgets, docs.api_names and release.single_version_source"
    requirement: FRAME-05
    verification:
      - kind: integration
        ref: "tests/policy/tests.cmake#docs.budgets (testing.md 22735 bytes, maintaining.md 10278 bytes)"
        status: pass
      - kind: integration
        ref: "tests/policy/tests.cmake#release.single_version_source"
        status: pass
      - kind: other
        ref: "cmake --workflow --preset ci (exit 0); sh scripts/hygiene.sh --tree (exit 0)"
        status: pass
    human_judgment: false
  - id: D6
    description: "Preparation docs agree with what phase 1 built: the size-tag row's 4096 cap, the GG aspect row (PAR 6:5 reported, 4:3 in the adapter), the empty supported_extensions, and the distribution wasm32 toolchain"
    requirement: FRAME-01
    verification:
      - kind: other
        ref: "grep '^| Size tag' .planning/preparation/ARCHITECTURE.md | grep -q 4096 (exit 0)"
        status: pass
    human_judgment: false
  - id: D7
    description: "The draft probe PR #3 is closed with the branch kept, and the post-merge ci follow-ups are one pending todo"
    requirement: FRAME-05
    verification:
      - kind: other
        ref: "gh pr list --head phase/01-a-test-frame-in-retroarch --state open --json number --jq length | grep -qx 0 (exit 0); the todo-file grep for Cachegrind, Dependabot, wasm32 and forced-failure (exit 0)"
        status: pass
    human_judgment: false
duration: 20min
completed: 2026-10-19
---

# Phase 1 Plan 12: Measured Timeouts, Maintainer Docs and Phase Close-Out Summary

**Every CI timeout now comes from this repository's own runs. The be32 and wasm32 scales are measured at 8 and 5.
`docs/testing.md` and `docs/maintaining.md` describe the lanes and the release path, the preparation docs match
what was built, and probe PR #3 is closed. CI on the pushed timeouts is green (run 39337625991).**

## Owner step (identity-bound: repository admin rights)

`CI required` has now reported on this repository (runs 39335167905, 39336389305, 39337625991), so the branch
ruleset can go on. Applying it changes repository settings, which needs the owner's admin rights; the agent does
not change settings (AGENTS rule 6). From the repository root:

```sh
scripts/apply-repo-settings.sh
scripts/apply-repo-settings.sh --check
```

The first command applies the settings and the committed ruleset `.github/rulesets/main.json`: no deletion or
force push on `main`, linear history, squash-only pull requests, and the required check `CI required` from GitHub
Actions with no bypass. The second only reads. It should print `no drift on szTheory/cogmaster` and exit 0. Run
both before `/gsd-ship 1` merges, so the phase PR is the first merge the ruleset gates.

## Measured timeouts (seconds of the slowest successful run, then minutes set)

| Job | Slowest | Run | Set |
|---|---|---|---|
| changes | 6 | 39336389305 | 5 (floor) |
| hygiene | 26 | 39336389305 | 1 |
| title | 5 | 39335167905 | 5 (floor) |
| build ×6 | 74 (windows-arm64) | 39335167905 | 3 |
| asan | 22 | 39335167905 | 5 (floor, apt) |
| nofp ×2 | 8 | 39336389305 | 1 |
| be32 | 34 | 39336389305 | 5 (floor) |
| hash-equality | 6 | 39335167905 | 5 (floor) |
| wasm32 | 21 | 39335167905 | 5 (floor) |
| retroarch | 21 | 39336389305 | 5 (floor) |
| perf | 19 | 39335167905 | 5 (floor, apt) |
| rehearsal | 365 | 39335167905 | 13 |
| cleanup | 12 | 39335167905 | 5 (floor, API) |
| CI required | 3 | 39336389305 | 5 (floor) |
| nightly bench ×3 | 62 (macos-15) | 39335167849 | 3 |
| nightly bench-wasm32 | 82 | 39334762549 | 3 |

All are provisional: ci.yml has 2 samples and nightly.yml 3. The run on the new timeouts, 39337625991, stayed
inside all of them. Its slowest jobs were rehearsal at 361 s, macos-x64 at 58 s and windows-arm64 at 56 s.

**Scales.** These come from per-test ctest times in the job logs, over the tests each lane shares with the Linux
x64 leg:

| Lane | Tests | Total ratio | Slowest tests | Scale |
|---|---|---|---|---|
| be32 | 43 | 6.60 (run 39335167905), 6.72 (run 39336389305) | libretro host tests, up to 7.3 | 11 → 8 |
| wasm32 | 40 | 3.51 (run 39335167905), 4.32 (run 39336389305) | | 3 → 5 |

**Gate timeout.** Four clean local runs of `cmake --workflow --preset ci` took 1.55 to 1.61 s, all exit 0. Twice
the slowest, rounded up, gives `workflow.test_gate_timeout` = 4.

## Verification (command, exit status)

- `cmake --workflow --preset hygiene`: exit 0, including `hygiene.timeout_comments`.
- `jq -e '.workflow.test_gate_timeout | type == "number" and . > 0' .planning/config.json`: exit 0.
- Plan verify 3 against `79b6f19`, the pushed HEAD after the timeout change: exit 0. The run is
  https://github.com/szTheory/cogmaster/actions/runs/39337625991 and all 20 jobs succeeded.
- `cmake --workflow --preset ci`: exit 0, 80 and 4 tests.
- `sh scripts/hygiene.sh --tree`: exit 0.
- `gh pr list --head phase/01-a-test-frame-in-retroarch --state open --json number --jq 'length' | grep -qx 0`:
  exit 0.
- The todo-file grep for Cachegrind, Dependabot, wasm32 and forced-failure: exit 0.
- Acceptance greps:
  - The timeout comment count equals the `timeout-minutes:` count; both are 14.
  - Every cited run ID resolves with `gh run view <id> -R szTheory/cogmaster`.
  - The size-tag row contains 4096.
  - Both docs are under 40960 bytes.
- `gh release list` printed nothing and `git ls-remote --tags origin 'rehearsal-*'` printed nothing, so the
  rehearsal in run 39337625991 cleaned up after itself.

## Post-merge follow-ups

These are in `.planning/todos/pending/post-merge-ci-follow-ups.md`, to be done in one `ci:` pull request after
the merge:

- arm the perf gate after two zero-spread Cachegrind control runs;
- dispatch the nightly forced failure once;
- add Dependabot and its auto-merge workflow;
- promote wasm32 after ten green runs.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 2 - Missing critical] Wider 5-minute floor**
- **Found during:** Task 1.
- **Issue:** The plan's floor list leaves out asan, perf and cleanup, which also depend on the network. asan and
  perf run `apt-get`, and cleanup calls the GitHub API. Without the floor they would get 1-minute timeouts, which a
  slow mirror would break.
- **Fix:** Applied the ENGINEERING §4 floor for network-bound jobs to all three. Each comment says why
  (`(apt)`, `(API)`).
- **Files modified:** .github/workflows/ci.yml
- **Commit:** 79b6f19

**2. [Rule 1 - Bug] The nightly timeouts were seed values**
- **Found during:** Task 1.
- **Issue:** `nightly.yml` is in the plan's file list, and its bench and bench-wasm32 still had seed timeouts of
  20 and 30 minutes. Three successful runs existed to measure from.
- **Fix:** Measured both from those three runs (3 minutes each, provisional). `report` stays provisional with 0
  samples, because it is skipped on pull requests.
- **Files modified:** .github/workflows/nightly.yml
- **Commit:** 79b6f19

**3. [Rule 1 - Bug] The seed's container recipe does not work here**
- **Found during:** Task 2.
- **Issue:** The seed's Docker one-liner for the Linux lanes either fetches a pinned CMake, which this
  repository's `pins.txt` does not have, or uses apt's. `docker run ubuntu:26.04 apt-cache policy cmake` shows
  4.2.3, below the presets' `cmakeMinimumRequired` 4.3.
- **Fix:** `docs/testing.md` lists each lane's packages and the container mount, and says to take CMake 4.3 from
  its own release. It gives no command that would fail.
- **Files modified:** docs/testing.md
- **Commit:** 851a890

### Notes

- After the probe PR was closed, the final push (`851a890`, docs and planning only) has no CI run, because
  `ci.yml` runs on pull requests and on `main`. The last code-bearing commit, `79b6f19`, is green. `/gsd-ship 1`
  opens the phase PR, and that PR runs CI on `851a890`.

**Total deviations:** 3 auto-fixed (1 missing critical, 2 bugs). **Impact:** timeouts are safer for network
jobs, and the docs give no command that would fail.

## Known Stubs

None.

## Next

Phase complete, ready for `/gsd-ship 1` after the owner step above.

## Self-Check: PASSED

- FOUND: docs/testing.md
- FOUND: docs/maintaining.md
- FOUND: .planning/todos/pending/post-merge-ci-follow-ups.md
- FOUND: 79b6f19 (ancestor of HEAD)
- FOUND: 851a890 (ancestor of HEAD)
- FOUND: run 39337625991 concluded success for 79b6f19
