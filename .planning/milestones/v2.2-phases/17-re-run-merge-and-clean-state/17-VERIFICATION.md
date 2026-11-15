---
phase: 17-re-run-merge-and-clean-state
verified: 2026-11-15T02:33:08Z
status: passed
score: 10/10 must-haves verified
covered_files:
  - .github/workflows/ci.yml
  - .planning/phases/17-re-run-merge-and-clean-state/17-01-PLAN.md
  - .planning/phases/17-re-run-merge-and-clean-state/17-01-SUMMARY.md
  - .planning/phases/17-re-run-merge-and-clean-state/17-02-PLAN.md
  - .planning/phases/17-re-run-merge-and-clean-state/17-02-SUMMARY.md
  - .planning/phases/17-re-run-merge-and-clean-state/17-03-PLAN.md
  - .planning/phases/17-re-run-merge-and-clean-state/17-03-SUMMARY.md
  - .planning/phases/17-re-run-merge-and-clean-state/17-04-PLAN.md
  - .planning/phases/17-re-run-merge-and-clean-state/17-04-SUMMARY.md
  - docs/compatibility.md
  - docs/conformance.md
  - docs/maintaining.md
  - docs/runner.md
  - runner/diag.c
  - runner/diag_class.h
  - scripts/library.sh
  - scripts/milestone-close.sh
  - tests/compat/ledger.csv
  - tests/hygiene/tests.cmake
  - tests/inventory/ci.txt
  - tests/inventory/wasm32.txt
  - tests/library/diagnose.cmake
  - tests/library/tests.cmake
  - tests/policy/repository-checks.txt
  - tests/policy/tooling_cases.cmake
  - tests/runner/diagnose.cmake
  - tests/runner/test_runner.c
covered_digest: "v4:sha256:f4f63f6e670663d667300ec05335ec862017cd0a938b779f82bb6491a7d9a331"
behavior_unverified: 0
overrides_applied: 0
---

# Phase 17: Re-run, merge and clean state Verification Report

**Phase Goal:** The rows the legacy renderer moved are in the ledger and on the compatibility page under a core stamp that is an ancestor of `main`, the rows that still fail are counted by cause, the diagnose vocabulary nothing raises any more is gone, the drifted job timeouts are re-derived from live runs, and a maintainer can check that the repository is in a clean state before a milestone-close pull request.
**Verified:** 2026-11-15T02:33:08Z
**Status:** passed
**Re-verification:** No, initial verification
**Branch and head:** `phase/17-re-run-merge-and-clean-state` at `89febfd`, phase base and `origin/main` both `0a97075` (re-fetched during this run). The only commits after `33bbfc2` touch `.planning/` (`git diff --quiet 33bbfc2 HEAD -- . ':(exclude).planning'` exits 0), so every test result below speaks for the shipped code.

## Goal Achievement

Verification was run against the tree, not the summaries. The library judge and `library.sh diagnose` were not re-run (instruction; they need the owner's library). Everything else that can be re-derived was.

### Observable Truths

| # | Truth | Status | Evidence |
|---|-------|--------|----------|
| 1 | Roadmap criterion 1: the ledger `core=` stamp is an ancestor of `origin/main` and the update goes through the unchanged ratchet with no `pass` row lost | VERIFIED | The branch ledger has exactly one distinct 40-hex `core=` value on 1116 lines, `0a97075f...`. `git merge-base --is-ancestor 0a97075... origin/main` exits 0; the Phase 15 pre-squash stamp `0b4ae67...` exits 1 (control). `gh api repos/szTheory/cogmaster/commits/0a97075...` exits 0. A row-for-row comparison of `origin/main:tests/compat/ledger.csv` with the branch file (Python `csv`, 1986 rows each, same keys, same order) gives one transition set, 87 `fail` to `pass`, and 0 `pass` rows lost. I reproduced the CI step: main's own `compat-ratchet.sh`, ledger and header from `git show origin/main:`, behaviour revisions 19 and 19, printed `compat: protected comparison passed`, exit 0. `git diff --quiet origin/main -- tests/compat/allow.txt quality.json src include` exits 0, so the ratchet and its allow list are untouched. The merge by pull request itself is the ship step (rule 9, rule 10); see "Carried to ship". |
| 2 | Roadmap criterion 2: the page equals its regeneration from the ledger, `compat.exit` exits 0, and `library.sh diagnose` counts the still-failing rows by cause | VERIFIED | `sh scripts/compat-common.sh docs tests/compat/ledger.csv \| cmp - docs/compatibility.md` exits 0 (10261 bytes). `ctest -R '^compat\.(exit\|whole_list\.fresh\|page_bound)$'` passes 3 of 3, and all 88 of `policy.*`, `docs.*` and `compat.*` pass. The page header reads pass=1116 fail=9, which matches the ledger count. Diagnose counts are recorded in `17-RUN-EVIDENCE.md` (header core `7da9ccc` equals the ledger commit, no dirty mark; leaves 2, 2, 1 and 0; groups_total 5 equals fail_total 5). Cross-check: the ledger holds 5 NTSC and 4 PAL `fail` rows, and diagnose covers the first NTSC profile only, so 5 equals 5. The run itself was not repeated here. |
| 3 | Roadmap criterion 3: the dead diagnose vocabulary is removed in one change, the line shape changes once, and the tests and `docs/runner.md` agree | VERIFIED | One commit, `8f4fa1e`, changes the printed line, the `library.sh` reducer and both diagnose tests together (`git log` for `runner/diag.c` and `scripts/library.sh` shows only it, `8750060` for the dead helpers and docs, and the unrelated `d2b8605` dotenv fix). `cogmaster-run --diagnose` on an owned fixture prints 23 fields, `diagnose 2` plus 21 words, ending `header=valid`. `docs/runner.md` carries a `diagnose 2` example of the same 23 fields, its `where` row has no `port`, and the `wclass` and `pclass` rows are gone. `git grep` for `wclass`, `pclass`, `reg-zm`, `where=port` outside `.planning` finds nothing; the remaining `library-diagnose 1` hits are the output header the plan keeps. `runner.diagnose`, `runner.diagnose.class`, `library.diagnose`, `.taxonomy`, `.hostile` and the five `.leak_*` all pass (13 of 13). The `cogmaster__diag_where` test now expects `unknown` for the controller and unnamed device. |
| 4 | Roadmap criterion 4: the be32, diagnose-test and hygiene tree-check timeouts are re-derived by twice-the-slowest from live runs, with run IDs, in one workflow commit, and `hygiene.timeout_comments` passes | VERIFIED | `git log --oneline origin/main..HEAD -- .github/workflows/ci.yml` lists `33bbfc2` only, and it is also the last non-planning commit. It touches `ci.yml`, `tests/hygiene/tests.cmake` and `tests/library/tests.cmake` only. I re-ran `sh scripts/ci-durations.sh ci.yml 40` and its 14 rows equal the committed values and comments exactly (be32 21 min, `2 x 609 s, slowest of 36 runs, run 43458609482`; rehearsal 14, `2 x 393 s`). I re-read the cited test runs: `library.diagnose` slowest is 144.43 s on macos-x64 in run 43460663238 (limit 289, twice is 288.86) and `hygiene.tree` is 37.32 s in run 43562888383 (limit 75, twice is 74.64); both lines carry a `# Timeout:` comment with run ID. Every `timeout-minutes` value is unchanged except rehearsal 13 to 14, so the change is raise-only. `nightly.yml`, `digest.yml` and `release.yml` are unchanged, and `ci.yml` parses with 14 jobs. `hygiene.timeout_comments` and its self test pass. See warning W3 on `runner.diagnose`. |
| 5 | Roadmap criterion 5: a milestone-close script reports the five facts, exits non-zero when any is off, a dirty-tree control proves it, `docs/maintaining.md` calls it a check not a gate, and `cmake --workflow --preset ci` exits 0 | VERIFIED | `scripts/milestone-close.sh` prints six rows (`open-prs`, `main-ci`, `release`, `release-pr`, `tree`, `unreleased`) and exits 0, 1 or 2. `policy.milestone_close` (one CTest name, eleven cases, fake `gh`) includes tracked-edit and untracked-file controls expecting exit 1 and `tree off`. I ran the driver against a scratch copy with the `tree` dirty test mutated out: cases `tracked` and `untracked` then fail (the driver exits 1), and the unmutated copy passes all 11. A live read-only run from the phase branch printed `open-prs ok`, `main-ci ok at 0a97075`, `release ok v0.15.0 ... CI success`, `release-pr ok`, `tree off  branch phase/17-...`, `unreleased ok`, exit 1: the real `gh` filters work, and the one off row is the correct answer for a non-`main` branch. `docs/maintaining.md` gains "Closing a milestone": run after the last release pull request merges, "a check to run, not a release gate", not run in CI. `git grep milestone-close -- .github` finds nothing. I ran `cmake --workflow --preset ci` on this head: 738 of 738, then 5 of 5, exit 0. |
| 6 | Plan 01: the vocabulary that is still raised stays (`vdpmode` with its `mode4` canary, `faddr`, the public `COGMASTER_FAULT_KIND_ROM_WRITE`, the `rom_write` and `ctrl` name-table entries) and TUNE-15's wording says why | VERIFIED | `runner/diag.c` still carries `[COGMASTER_FAULT_KIND_ROM_WRITE] = "rom_write"` and `[COGMASTER_FAULT_DEVICE_CTRL] = "ctrl"`; `runner/diag_class.h` still holds `mode4` (2 hits); `git diff --quiet origin/main -- src include tests/api/v2.c docs/embedding.md tests/bus` exits 0. Every `cogmaster__bus_fault` call site in `src/` uses the RAM, CART, MAPPER or VDP device, and none uses `DEVICE_CTRL`, `DEVICE_NONE` or `KIND_ROM_WRITE`, so the removed words were dead. `REQUIREMENTS.md` TUNE-15 now excludes `vdpmode` and cites `src/vdp_render.c` (the blocked-mode raise at line 214 is real). |
| 7 | Phase boundary: no `src/` or `include/` change, behaviour revision stays 19 and the state format 17 | VERIFIED | `git diff --quiet origin/main -- src include` exits 0. Both revisions read 19 and 19 in the ratchet reproduction above. |
| 8 | Sequencing: the stamp is the judged commit, and the shipped tree equals the judged tree where it matters | VERIFIED | The single branch stamp equals `git merge-base HEAD origin/main` (`0a97075`). `git diff --quiet 0a97075 HEAD -- src include CMakeLists.txt cmake` exits 0. The diagnose header core `7da9ccc` is the ledger commit, so the line shape counted is the final one. |
| 9 | A runner built before the prune, fed to the new reducer, is a malformed record and never a shifted field | VERIFIED | The reducer rejects any record unless `NF == 30` and `$9 == "2"` (`scripts/library.sh`); an old-shape line is token 1 with 32 fields, so both terms fail. The taxonomy case exercises the `NF` term with a 31-field line and passes with `malformed=3` unchanged. By construction, not by an old-shape canned line. |
| 10 | Prohibitions: no stamp-ancestry gate, no workflow or schedule runs the close check, no `jq` in its test, the script only reads | VERIFIED | `git diff --quiet origin/main -- tests/compat ':(exclude)tests/compat/ledger.csv'`, `scripts/compat-common.sh`, `scripts/compat-ratchet.sh` all exit 0; no `is-ancestor` or `merge-base` line is added under `.github`; `grep -c jq tests/policy/tooling_cases.cmake` is 0; a grep of the script for `git fetch`, `git push`, `gh pr merge`, write methods, `sleep` and retry hits only the comment "no retry". |

**Score:** 10/10 truths verified (0 present, behaviour-unverified)

The two behaviour-dependent truths (5 and 9) each have a passing behavioural test; truth 5's dirty-tree control was also shown to bite under mutation.

### Deferred Items

None. v2.2 has no phase after 17.

### Advisory (New Scope, Unevidenced)

Not applicable: initial verification.

### Required Artifacts

| Artifact | Expected | Status | Details |
|----------|----------|--------|---------|
| `runner/diag.c` | `diagnose 2` line, 21 words | VERIFIED | format string and arguments agree; live runner output has 23 fields |
| `runner/diag_class.h` | dead helpers removed, `mode4` kept | VERIFIED | `wclass` and `pclass` helpers gone, `cogmaster__diag_where` answers `unknown` for CTRL and NONE |
| `scripts/library.sh` | reducer for token 2, 30 fields | VERIFIED | `NF != 30`, `$9 != "2"`, `size` at `$29`, `header` at `$30`; sub-rows renumbered |
| `tests/library/diagnose.cmake`, `tests/runner/diagnose.cmake`, `tests/runner/test_runner.c` | new shape | VERIFIED | all pass |
| `docs/runner.md`, `docs/conformance.md` | new shape, worktree sentence | VERIFIED | 37491 and 38106 bytes, `docs.budgets` passes |
| `scripts/milestone-close.sh` | six rows, exit 0, 1 or 2 | VERIFIED | 103 lines (plan said 45 to 60), see W2 |
| `tests/policy/tooling_cases.cmake` | `policy.milestone_close` | VERIFIED | 11 cases, passes |
| `tests/policy/repository-checks.txt`, `tests/inventory/ci.txt`, `tests/inventory/wasm32.txt` | classified, inventoried | VERIFIED | `docs.budgets` passes (887 counted lines of 973 allowed) |
| `docs/maintaining.md` | "Closing a milestone" | VERIFIED | present, 21432 bytes |
| `tests/compat/ledger.csv`, `docs/compatibility.md` | carried, one stamp | VERIFIED | see truths 1 and 2 |
| `.github/workflows/ci.yml`, `tests/library/tests.cmake`, `tests/hygiene/tests.cmake` | one workflow commit | VERIFIED | `33bbfc2` |
| `17-RUN-EVIDENCE.md` | counts, commands, statuses only | VERIFIED | a grep for `/Users/`, `/home/`, `.sms`, `.gg`, `.zip` finds nothing in it or the four summaries |

### Key Link Verification

| From | To | Via | Status | Details |
|------|----|-----|--------|---------|
| `runner/diag.c` | `scripts/library.sh` | positional fields: token `$9`, `NF 30`, `$29`, `$30` | WIRED | `library.diagnose` runs the real runner through the reducer and passes |
| `scripts/library.sh` | `tests/library/diagnose.cmake` | canned lines in the same shape | WIRED | taxonomy, hostile and leak cases pass |
| `tests/policy/tooling_cases.cmake` | `scripts/milestone-close.sh` | a copy in a throwaway repo, `CMAKE_CONFIGURE_DEPENDS` | WIRED | mutating the copy changed the result |
| `tests/policy/repository-checks.txt` | `tests/cmake/budgets.cmake` | every script classified | WIRED | `docs.budgets` passes |
| worktree of the base | `tests/compat/ledger.csv` | `git diff` to file, `git apply` | WIRED | bytes equal to a regeneration; one stamp; two-file diff recorded |
| `scripts/ci-durations.sh` | `.github/workflows/ci.yml` | comment column | WIRED | re-run output equals the committed comments |
| `.github/workflows/ci.yml` | `tests/cmake/repository.sh` | `hygiene.timeout_comments` | WIRED | passes |

### Data-Flow Trace (Level 4)

| Artifact | Data Variable | Source | Produces Real Data | Status |
|----------|---------------|--------|--------------------|--------|
| `docs/compatibility.md` | counts and groups | `compat-common.sh docs` over the ledger | yes, byte-equal on regeneration | FLOWING |
| `milestone-close.sh` rows | `gh api` answers and `git` state | live `gh` and the work tree | yes, live run printed real shas and tags | FLOWING |
| `library.sh diagnose` groups | runner `diagnose 2` records | the built runner | yes, real runner output feeds `library.diagnose` | FLOWING |

### Behavioral Spot-Checks

| Behavior | Command | Result | Status |
|----------|---------|--------|--------|
| compat tests | `ctest --test-dir build/ci -R '^compat\.(exit\|whole_list\.fresh\|page_bound)$'` | 3 of 3 pass | PASS |
| diagnose, taxonomy, hostile, leak, runner class, milestone close, synthetic | `ctest --test-dir build/ci -R '^(runner\.diagnose\|runner\.diagnose\.class\|library\.diagnose.*\|policy\.milestone_close\|library\.synthetic)$'` | 13 of 13 pass | PASS |
| policy, docs and compat families | `ctest -R '^(policy\.\|docs\.\|compat\.)'` | 88 of 88 pass | PASS |
| hygiene timeouts and budgets | `ctest --test-dir build/hygiene -R '^(hygiene\.timeout_comments.*\|docs\.budgets.*)$'` | 10 of 10 pass | PASS |
| tree hygiene | `sh scripts/hygiene.sh --tree` | exit 0 | PASS |
| protected ratchet against main's judge | main's `compat-ratchet.sh` and ledger, revisions 19 and 19 | `compat: protected comparison passed`, exit 0 | PASS |
| runner line shape | `cogmaster-run --diagnose` on an owned fixture | `diagnose 2 ...`, 23 fields | PASS |
| dirty-tree mutation control | driver on a scratch copy with the tree row's dirty test removed | `tracked` and `untracked` fail, driver exit 1; unmutated exit 0 | PASS |
| close check live | `sh scripts/milestone-close.sh` | six rows, `tree` off on a phase branch, exit 1 | PASS |
| full ci workflow | `cmake --workflow --preset ci` | 738 of 738, then 5 of 5, exit 0 | PASS |

### Probe Execution

No probes declared by any plan; `scripts/*/tests/probe-*.sh` does not exist. Step 7c skipped.

### Requirements Coverage

| Requirement | Source Plan | Description | Status | Evidence |
|-------------|-------------|-------------|--------|----------|
| RUN-03 | 17-03 | Judge re-run on `main`, stamp an ancestor, rows merged through the unchanged ratchet, no `pass` lost | SATISFIED (merge at ship) | truths 1 and 8; the pull request is opened and merged by the ship step |
| RUN-04 | 17-03 | Page equals regeneration, `compat.exit` green, remaining fails counted by cause | SATISFIED | truth 2 |
| TUNE-15 | 17-01 | Dead diagnose vocabulary removed in one change, shape once, tests and docs agree, `vdpmode` stays | SATISFIED | truths 3, 6 and 9. `REQUIREMENTS.md` still shows the box unticked and the table row `Pending`, see W1 |
| TUNE-16 | 17-04 | be32, diagnose-test and hygiene tree-check timeouts re-derived with run IDs in one workflow commit | SATISFIED | truth 4 |
| TUNE-17 | 17-02 | Milestone-close script, non-zero when off, documented, dirty-tree test, a check not a gate | SATISFIED | truth 5 |

No orphaned requirements: `REQUIREMENTS.md` maps exactly RUN-03, RUN-04, TUNE-15, TUNE-16 and TUNE-17 to Phase 17, and each plan claims one set (01: TUNE-15, 02: TUNE-17, 03: RUN-03 and RUN-04, 04: TUNE-16).

### Anti-Patterns Found

| File | Line | Pattern | Severity | Impact |
|------|------|---------|----------|--------|
| (modified files) | - | `TBD`, `FIXME`, `XXX`, `TODO`, `HACK`, `PLACEHOLDER` | none | only hit is the `mktemp` template `diag.XXXXXX` in `scripts/library.sh:56`, not a marker and not new |
| (added lines outside `.planning`) | - | planning IDs in code, tests or user docs | none | grep for `TUNE-`, `RUN-`, `D-NN`, `Phase NN` finds none |
| (diff) | - | personal paths or addresses | none | grep for `/Users/` and `@gmail` finds none |

No blockers. The nine code-review findings (5 warnings, 4 info) are all about tooling or test code and are dispositioned as seeds (rule 7); none changes a truth above.

### Human Verification Required

None. Rule 6: the library run needed the owner's machine and was run by the agent with the owner's authorisation; nothing left depends on the owner's judgment or hardware. What a merge alone shows is confirmed at ship.

### Warnings (non-blocking)

- **W1 TUNE-15 bookkeeping.** `.planning/REQUIREMENTS.md` still shows `- [ ] **TUNE-15**` and `| TUNE-15 | Phase 17 | Pending |`, while the other four requirements were ticked and marked Complete. The delivery is verified (truths 3, 6, 9). The orchestrator should tick it at phase completion; I did not edit it.
- **W2 `milestone-close.sh` classification (review WR-05).** The script is 103 lines, not the 45 to 60 the plan and discuss step assumed. Counted as a `check` it would add 103 to 887 and give 990 against 973 allowed, so `docs.budgets` would fail. The `operation` classification is now the only thing keeping it green, and the context's premise ("no risk either way") no longer holds. This is an owner judgment (keep `operation` with an honest reason, or trim the script), already a seed, so it neither fails the phase nor needs a gap round.
- **W3 Timeout comments the commit left stale.** `runner.diagnose` was held at 633 s with the comment `2 x 316.11 s (be32), slowest of 1 run, run 43334476928`, which is not the 40-run basis. The hold is sound: CTest multiplies the limit by the lane scale (be32 x8 gives 5064 s against a 450.48 s raw reading), and the largest unscaled lane is 179.23 s (twice is 358.46 s, under 633). The reasoning is in `17-RUN-EVIDENCE.md` but not in the tree. Separately the `perf` line says `2 x 106 s` against a 6-minute value (review WR-01, a seed; the check only demands at least twice).
- **W4 Pushed more than once before the owner run.** The plan said the code of plans 01 and 02 would be pushed once; it was pushed three times (`e896d84`, `1b8d446`, `d2b8605`). The first dispatched run failed on both Windows legs because the manifest read in the new script was rewritten by the Windows shell; the second push fixed it, the third added a linked-worktree dotenv fallback with a test that fails without it. Both are real defects caught on the way and recorded in `17-RUN-EVIDENCE.md`. All three dispatched runs (`43657530824`, `43660745598`, `43664171483`) conclude success with no failed job.
- **W5 Diagnose scope.** `library.sh diagnose` covers the first NTSC profile only, so it counts 5 of the 9 failing ledger rows; the other 4 are PAL rows. This is the tool's existing design and the evidence says so; it is not a phase gap.
- **W6 Runtime log.** `STATE.md` has runtime and model rows for discuss, plan and execute 17-03 and 17-04, none named for 17-01 and 17-02 (rule 9). Info only.

### Carried to ship (commands, not owner items)

Recorded in `17-RUN-EVIDENCE.md` "Ship hand-off"; none of them ran here:

- pull request title `fix(compat): ...`, then `gh pr merge --auto --squash` in a shell, a CI run on the head commit with the rehearsal job running (the `ci.yml` edit is a release input), and the release pull request confirmed green with 19 assets;
- after the merge, `git merge-base --is-ancestor 0a97075f34e48e2b1d0f72aa231006cc9366a398 origin/main` (exits 0 now; a squash merge does not change it);
- the remote branch is still at `2cc8bb5` (the sampled head), so `33bbfc2` and the planning commits go up in that single push.

### Gaps Summary

No gaps. All five roadmap criteria and the plan-level truths hold in the tree. The two items worth a decision are W2 (owner judgment on the script's classification) and W1 (tick TUNE-15 at phase completion).

---

_Verified: 2026-11-15T02:33:08Z_
_Verifier: Claude (gsd-verifier)_
