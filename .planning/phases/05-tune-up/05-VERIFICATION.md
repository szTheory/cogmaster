---
phase: 05-tune-up
verified: 2026-11-10T09:00:00Z
status: passed
score: 5/5 must-haves verified
covered_files:
  - .github/dependabot.yml
  - .github/workflows/ci.yml
  - .github/workflows/dependabot-automerge.yml
  - .github/workflows/digest.yml
  - .github/workflows/nightly.yml
  - .planning/phases/05-tune-up/05-01-PLAN.md
  - .planning/phases/05-tune-up/05-01-SUMMARY.md
  - .planning/phases/05-tune-up/05-02-PLAN.md
  - .planning/phases/05-tune-up/05-02-SUMMARY.md
  - .planning/phases/05-tune-up/05-03-PLAN.md
  - .planning/phases/05-tune-up/05-03-SUMMARY.md
  - .planning/phases/05-tune-up/05-04-PLAN.md
  - .planning/phases/05-tune-up/05-04-SUMMARY.md
  - .planning/phases/05-tune-up/05-05-PLAN.md
  - .planning/phases/05-tune-up/05-05-SUMMARY.md
  - .planning/phases/05-tune-up/05-06-PLAN.md
  - .planning/phases/05-tune-up/05-06-SUMMARY.md
  - .planning/phases/05-tune-up/05-07-PLAN.md
  - .planning/phases/05-tune-up/05-07-SUMMARY.md
  - .planning/phases/05-tune-up/05-08-PLAN.md
  - .planning/phases/05-tune-up/05-08-SUMMARY.md
  - .planning/phases/05-tune-up/05-09-PLAN.md
  - .planning/phases/05-tune-up/05-09-SUMMARY.md
  - .planning/phases/05-tune-up/05-10-PLAN.md
  - .planning/phases/05-tune-up/05-10-SUMMARY.md
  - .planning/phases/05-tune-up/05-11-PLAN.md
  - .planning/phases/05-tune-up/05-11-SUMMARY.md
  - .planning/phases/05-tune-up/05-12-PLAN.md
  - .planning/phases/05-tune-up/05-12-SUMMARY.md
  - .planning/phases/05-tune-up/05-13-PLAN.md
  - .planning/phases/05-tune-up/05-13-SUMMARY.md
  - .planning/phases/05-tune-up/05-14-PLAN.md
  - .planning/phases/05-tune-up/05-14-SUMMARY.md
  - README.md
  - quality.json
  - runner/main.c
  - scripts/ci-durations.sh
  - scripts/digest.sh
  - tests/cmake/quality.cmake
  - tests/cmake/readme.cmake
  - tests/cmake/repository.sh
  - tests/perf/allow.txt
covered_digest: "v4:sha256:5a82e962c6f345aae9aaf615f620c8355f53e896b7fc4b2e13d6cfd59e718c26"
behavior_unverified: 0
overrides_applied: 0
---

# Phase 5: Tune-up Verification Report

**Phase Goal:** The v1.0 debt is merged: CI timeouts, the perf gate and size ceilings are measured and armed against current `main`, the docs and hosted-test budgets have room for this milestone, CI reports on itself weekly, and the README tells a player how to load a ROM.
**Verified:** 2026-11-10
**Status:** passed
**Re-verification:** No, initial verification
**Branch/head:** `phase/05-tune-up` at d0af219

## Commands re-run by the verifier (not taken from SUMMARY)

| Command | Exit | Result |
|---|---|---|
| `cmake --workflow --preset ci` | 0 | 418 of 418 tests passed (macOS), plus the 4 release-labelled tests |
| `cmake --workflow --preset hygiene` | 0 | all hygiene tests passed, includes `hygiene.timeout_comments`, `digest.*`, `policy.digest*` |
| `cmake -DROOT=$PWD -P tests/cmake/budgets.cmake` | 0 | 729 check lines of 7455 `src/` lines, strict one tenth |
| `ctest -R 'policy\.(quality\|lessons\|dependabot)\|docs\.budgets\|docs\.readme\|policy\.e2e'` | 0 | 28 of 28 |
| `sh scripts/ci-durations.sh ci.yml` (live) | 0 | counts and run IDs agree with `ci.yml` comments, see note on drift below |
| per-run `gh run view --log` grep over the last 20 successful `main` push runs | 0 | 12 runs logged `wasm32 hashes.txt equals Linux x64's` |
| `gh run view` on runs 42752737395, 42752770307, 42764095086, 42764989102, 42762171858, 42762969470, 42741490263, 42758855565, 42759595082 | 0 | conclusions as cited below |
| `gh api repos/szTheory/cogmaster/issues/17` | 0 | CLOSED, label `nightly`, only nightly issue in the repository |

## Success Criteria (ROADMAP Phase 5)

| # | Criterion | Status | Evidence |
|---|---|---|---|
| 1 | Every CI job timeout carries slowest-run ID with N, none `provisional`; wasm32 required by `CI required` after ten equal main runs | VERIFIED | Every `timeout-minutes` line in `ci.yml`, `nightly.yml` and `release.yml` has the form `2 x <s> s, slowest of <N> runs, run <id>` (or a stated 5-minute floor plus the same). `hygiene.timeout_comments` and its self test pass. No `provisional` in any workflow. `CI required` needs `wasm32` (`ci.yml:749`); `hash-equality` needs `wasm32` and compares eight files. Verifier recount: 12 successful main push runs logged wasm32 hashes equal to Linux x64's (needs 10). |
| 2 | Control change raising instructions per frame fails the armed gate; armed after two zero-spread control runs; no stale `boot_to_play` allow line | VERIFIED | `ci.yml:29` `COGMASTER_PERF_MODE: gate`, armed by commit 950ff2c that touches `ci.yml` alone. Control runs 42752737395 and 42752770307 (main, `workflow_dispatch`, success) both logged `perf control boot_to_play runs=12302839,12302839,12302839 spread=0`. Proof run 42764095086 (pull_request): `perf boot_to_play head=12902843 base=12302839 delta_bp=487 verdict=FAIL`; the only non-success jobs were `perf` and `CI required`. `tests/perf/allow.txt` has comments only. |
| 3 | `quality.json` ceilings and floors enforced where measurable, each with a negative control; `lessons_unique` and `dependabot` in `ci`; Dependabot weekly, minor and patch grouped | VERIFIED | `quality.json` holds `docs.max_bytes`, three zip ceilings, `size.wasm`, floors `tests.ci` 422 and `tests.wasm32` 397, each ceiling with a reason naming its measuring run. `policy.quality` (real file against both inventories) plus `.ceiling`, `.floor`, `.missing_key`, `.malformed` and the five ratchet controls pass. `docs.budgets` plus `.doc_over` covers the docs cap. Each `build` leg runs a "Size ceilings" step after packaging with an inline `expect_fail` control; the `wasm32` job runs "Size ceiling" with its own control. `policy.lessons_unique` (+ self test, empty) and `policy.dependabot` (+ self test) are in the ci preset and pass. `.github/dependabot.yml`: `github-actions`, `weekly`, group `minor-and-patch` with `minor` and `patch`. Hosted proof: run 42764989102 (all 20 jobs success). |
| 4 | Scheduled weekly digest reports duration, flaky tests, open nightly issues, names slowest job; nightly lanes ran on sound code; forced failure opens one issue, next pass closes it | VERIFIED | `digest.yml` runs on `cron "23 6 * * 1"`; `scripts/digest.sh` prints `Slowest job: ...`, flaky and nightly-issue sections; `digest.format`, `digest.timeouts`, `policy.digest` and its self test pass. Nightly: forced run 42762171858 (main, failure) opened issue 17; plain run 42762969470 (main, `f69ab03`, all 7 jobs success) closed it with a "Passed" comment; 8faf841 (sound merge) is an ancestor of f69ab03 (recorded in 05-13, ancestry check re-derivable with `git merge-base`). |
| 5 | Every `docs/*.md` more than 2 KiB under cap and `tests/retroarch` at least 20 lines under 300; README load-ROM steps plus the one-sentence no-BIOS statement; all phase 1, 2 and 4 review findings dispositioned | VERIFIED | Largest doc `docs/embedding.md` is 33,101 bytes against a 38,912 cap (5,811 spare); `policy.e2e_budget` prints 273 lines (27 spare). README "Play a game" gives RetroArch and runner steps and the sentence "cogmaster ships no BIOS and no game; bring a `.sms` or `.gg` file you own, no BIOS is needed."; `docs.readme_play` (+ self test) runs the README commands against a built fixture and counts the sentence exactly once. Dispositions: the v1.0 phase 1 to 4 `*-REVIEW-DISPOSITION.md` files all state `open: 0`; every phase 1 and 4 warning and info row is fixed, skipped with a reason or deferred to SEED-002 or SEED-007, which hold WR-02, WR-04 and IN-04. Phase 4 WR-02 is fixed per `04-REVIEW-FIX.md`. |

## Requirements Coverage

Every ID appears in at least one PLAN `requirements:` field and in REQUIREMENTS.md (checked, traceability rows 112 to 121 read Complete). No orphaned Phase 5 requirement.

| Requirement | Plans | Status | Evidence |
|---|---|---|---|
| TUNE-01 | 05-03 | SATISFIED | Measured timeouts with run ID and N on every `timeout-minutes` line; no `provisional`; `hygiene.timeout_comments` passes; live `ci-durations.sh` agrees. |
| TUNE-02 | 05-01, 05-14 | SATISFIED | Gate armed; two zero-spread controls; FAIL proof run 42764095086; green final-head run 42764989102; allow file comment-only. |
| TUNE-03 | 05-03, 05-13 | SATISFIED | `scripts/ci-durations.sh` names the slowest job; `digest.yml` weekly schedule with `digest.sh` formatter and tests. |
| TUNE-04 | 05-05, 05-06 | SATISFIED | `quality.json` ceilings (docs, three zips, wasm) and floors enforced in `ci` preset and in build and wasm32 jobs; `policy.lessons_unique` in `ci`. |
| TUNE-05 | 05-01, 05-02 | SATISFIED | Docs split into topic files (largest 33,101 of 38,912 bytes); `tests/retroarch` at 273 of 300 lines. |
| TUNE-06 | 05-10 | SATISFIED | README "Play a game" plus the no-BIOS sentence, guarded by `docs.readme_play`. |
| TUNE-07 | 05-07, 05-08, 05-09 | SATISFIED | Cartridge body over 1 MiB refused with `runner.refuse.size` and `cart.probe.sizes` passing; comment corrections; dispositions with `open: 0`; tooling findings in SEED-007. |
| TUNE-08 | 05-13 | SATISFIED | Runs 42762171858 (forced failure) and 42762969470 (pass), issue 17 opened then closed; sound code ancestry. |
| TUNE-09 | 05-11, 05-12 | SATISFIED | `dependabot.yml` weekly with `minor-and-patch`; `policy.dependabot` plus self test; `dependabot-automerge.yml` skips release inputs. |
| TUNE-10 | 05-04 | SATISFIED | `CI required` needs `wasm32`; 12 equal main runs re-counted; `policy.ci_required` has no report-only exemption. |

## Anti-Patterns and Review Dispositions

- No `TODO`, `FIXME`, `TBD` or `XXX` in the new workflows, scripts or check files (grep over `digest.yml`, `dependabot-automerge.yml`, `digest.sh`, `ci-durations.sh`, `readme.cmake`, `quality.cmake`).
- Code review (`05-REVIEW.md`, 12 findings): WR-01, WR-03 and WR-04 fixed (commits 50eb5bd, a5af131, 673d9be, 71ffdc9); WR-02, WR-05 and IN-01 to IN-07 deferred to SEED-007 as tooling findings under AGENTS rule 7. Not gaps.

## Observations (non-blocking)

1. **wasm32 timeout comment is one run behind.** `ci.yml:338` cites 67 s, run 42752737395. A live `ci-durations.sh ci.yml` now shows a 108 s wasm32 job in phase-branch run 42759595082, which came after the timeouts were measured. The 5-minute floor still exceeds twice 108 s (216 s), and the arithmetic check passes, so nothing is at risk. The next tune-up re-measures. The same drift exists for `rehearsal` (4 vs 5 runs) and `perf` (26 vs 27 runs).
2. **Review-fix commits landed after the last hosted full-run on the phase head.** The final-head run 42764989102 is at 73547df; commits 50eb5bd to 673d9be later changed `dependabot-automerge.yml`, `runner/main.c`, `scripts/digest.sh`, `tests/cmake/repository.sh` and the digest fixture. Verifier re-ran `cmake --workflow --preset ci` and `hygiene` on d0af219 locally (both exit 0). The phase pull request run is the hosted confirmation.
3. **Run 42758855565 (phase branch dispatch) is red by design.** It failed at the new "Size ceiling" step because `size.wasm` had no ceiling yet; it supplied the measurement (901,343 bytes), and run 42759595082 on 59aaf53 passed with the ceiling in place.

## Ship-time confirmations (not gaps, per AGENTS rule 6)

Recorded in `05-14-SUMMARY.md` "Ship-time confirmations": the digest dry run, the first Dependabot pull request, first-run timeouts of `digest.yml` and `dependabot-automerge.yml`, the wasm32 lane on the merge commit, the next scheduled nightly, and the rule 10 steps (title, `gh pr merge --auto --squash`, CI run on the head, release pull request green). These need the merge to main and do not affect the status.

## Human Verification Required

None. Every criterion is met by an automated check, a command with an exit status or a cited run URL.

## Gaps Summary

None. All five ROADMAP success criteria and all ten TUNE requirements are met in the codebase and on the cited hosted runs.

---

_Verified: 2026-11-10_
_Verifier: Claude (gsd-verifier)_
