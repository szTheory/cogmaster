---
phase: 15-re-run-merge-and-timeouts
verified: 2026-11-13T21:34:54Z
status: passed
score: 12/12 must-haves verified
covered_files:
  - .github/workflows/ci.yml
  - .github/workflows/digest.yml
  - .github/workflows/nightly.yml
  - .planning/phases/15-re-run-merge-and-timeouts/15-01-PLAN.md
  - .planning/phases/15-re-run-merge-and-timeouts/15-01-SUMMARY.md
  - .planning/phases/15-re-run-merge-and-timeouts/15-02-PLAN.md
  - .planning/phases/15-re-run-merge-and-timeouts/15-02-SUMMARY.md
  - docs/compatibility.md
  - docs/conformance.md
  - docs/maintaining.md
  - docs/perf-fuzz.md
  - tests/compat/ledger.csv
covered_digest: "v4:sha256:a4539042373bafbd5caaa333c771dfe4b3c5b17e521afd5495c49259ae9926ae"
behavior_unverified: 0
overrides_applied: 0
---

# Phase 15: Re-run, merge and timeouts Verification Report

**Phase Goal:** The rows the fixes flipped to `pass` are in the ledger and the compatibility page, and the CI job timeouts that drifted from the twice-the-slowest rule are re-derived from live runs.
**Verified:** 2026-11-13T21:34:54Z
**Status:** passed
**Re-verification:** Refresh only. The first report (21:31:33Z, head e70c98c) had no open gaps; this refresh records that warnings W2 and W3 were fixed, and recomputes the fingerprint. See "Refresh" at the end.
**Branch head verified:** 3912004 (`phase/15-re-run-merge-and-timeouts`, base `origin/main` 621dd02), plus the uncommitted `15-RUN-EVIDENCE.md` Body edit that resolves W2

The SUMMARY and RUN-EVIDENCE claims were not trusted. Every number below was recomputed from the tree, the live
GitHub API or a fresh local run (the full run at e70c98c; the refresh re-checked only the two fixed items and the
fingerprint, because 3912004 changed one documentation line). Two ship-time facts (the CI run on the pushed head, and
the public reachability of the ledger stamp) cannot exist before the push and are listed under "Ship-time checks", not
as gaps.

## Goal Achievement

### Observable Truths

| #  | Truth | Status | Evidence |
| -- | ----- | ------ | -------- |
| 1 | SC1 / RUN-01: the library judge was re-run at the fix head and its rows were merged through the existing ratchet, with no `pass` row becoming `fail` | VERIFIED | Independent CSV parse (python3 -I) of `origin/main:tests/compat/ledger.csv` against HEAD: 1986 rows in both, 0 duplicate keys, 0 rows added or dropped, header identical, no change to title/machine/region/tv/mapper. Only status changes are fail to pass: gg/ntsc/fm-off 18, sms/ntsc/fm-off 126, sms/ntsc/fm-on 16, sms/pal/fm-off 125 = 285. Base pass 744 to head pass 1029, fail 381 to 96, pass_lost = 0. `build/library/results/summary.txt` holds `merge: rows_added=0 ratchet=ok page_bytes=13813` (grep exit 0). The OLD base judge, rebuilt from `git show origin/main:` copies of `scripts/compat-ratchet.sh` and `scripts/compat-common.sh`, run against the new ledger at revisions 18 and 18 with the fixture results: prints `compat: protected comparison passed`, exit 0. |
| 2 | SC2 / RUN-02: the page and its counts agree with the merged ledger, and `compat.exit` stays green | VERIFIED | `sh scripts/compat-common.sh docs tests/compat/ledger.csv \| cmp - docs/compatibility.md` exit 0. Page reads `pass=1029 fail=96`, `unknown=832 unsupported=5 skipped=24`, `Rows: 1986. Evaluated coverage: 1125`, `pal rows judged: 512; fm=on rows judged: 53`; these equal the ledger counts and the `exit:` lines of summary.txt. `wc -c docs/compatibility.md` is 13813, equal to the `page_bytes` the merge printed. `ctest --test-dir build/ci -R '^(compat\|hygiene\|docs\|policy)\.'` 78 of 78 passed, exit 0, including `compat.exit`, `compat.exit.self_test`, `compat.whole_list.fresh`, `compat.page_bound`. Judged-cart floors from summary.txt: `cart_pass=554/20 cart_pal_judged=511/1 cart_fm_judged=51/1 judged_cart_floors=met flag=absent`. A grep of README.md and docs/*.md (scratch sh script) for 744, 381, 1029 and 96 beside pass or fail finds 0 matches outside `docs/compatibility.md`. |
| 3 | SC3 / TUNE-11: asan, nightly bench and bench-wasm32 timeouts and the fuzz and digest comments are re-derived by the twice-the-slowest rule from live runs, each with its run ID, in one edit per workflow file, and `hygiene.timeout_comments` passes | VERIFIED | `sh scripts/ci-durations.sh ci.yml 40`, `nightly.yml 40`, `digest.yml 40` re-run live (each exit 0): be32 609 s/run 43458609482/21 min, build 332 s/43460663238/12, asan 243 s/43462502049/9, bench 368 s/42764095033/13, bench-wasm32 456 s/42768407261/16, fuzz 144 s/43020467034, digest 55 s/42769805382 (1 run). Every edited `timeout-minutes` line in the diff carries exactly these minutes, seconds and run IDs. All seven cited runs exist, conclude `success` and are on the stated workflow (`gh api .../actions/runs/<id>` exit 0 each). Every other timeout line in the three files (14 in CI, 5 in Nightly, 1 in Digest) also satisfies `M*60 >= 2*S` against the live read (narrowest: rehearsal 780 s against 770 s, wasm32 300 s against 282 s). `cmake -DDIR=$PWD/.github/workflows -DCHECK=timeouts -P tests/cmake/repository.cmake` exit 0 and `ctest --test-dir build/hygiene -R timeout_comments` passed. Edit shape: the timeout lines were edited once, in `afd413e` (touches ci.yml, nightly.yml, digest.yml together); `git diff -U0 afd413e HEAD -- .github/workflows` contains no `timeout-minutes` line; `git log origin/main..HEAD -- .github/workflows/ci.yml` and `digest.yml` each list exactly one commit. See the note on the three later nightly.yml review-fix commits under "Notes and warnings" (W1). |
| 4 | SC4 / local half: `cmake --workflow --preset ci` passes at this head | VERIFIED | Re-run at e70c98c in this verification: exit 0 (`WORKFLOW_EXIT=0`), 676 of 676 tests and 5 of 5 release tests passed; the working tree stayed clean. `cmake --workflow --preset hygiene` exit 0, 84 of 84 passed. `sh scripts/hygiene.sh --tree` exit 0 (re-run at the refresh as well); `sh scripts/hygiene.sh --history origin/main..HEAD` exit 0. Commit 3912004 changed one sentence in `docs/conformance.md`, not code. The hosted half (a CI run on the pushed head) is a ship-time check below. |
| 5 | D-01 / RUN-01: the owner ran the no-argument `sh scripts/library.sh` once at the phase head, and nothing outside `.planning/` moved between the hand-off and the merge | VERIFIED | Circumstantial but consistent: judged commit 0b4ae67 authored 12:46:26; `build/library/cogmaster-run` was rebuilt at 12:51:08 (after it, so the no-argument workflow's rebuild step ran); `summary.txt` written 13:01:52 (about 10.7 minutes later, matching the recorded 644 s). The ledger carries one stamp, `core=0b4ae67b7f14c3d997c24ca719fb18f1d3cd29a4`, on 1116 rows. Stamp check at the ledger commit: single full-length stamp, `git merge-base --is-ancestor` exit 0, `git diff --quiet 0b4ae67 8964eef^ -- . ':(exclude).planning'` exit 0. `git diff --quiet origin/main 0b4ae67 -- . ':(exclude).planning'` exit 0, so the judged core is byte for byte the core on `main`. `git diff --quiet 0b4ae67 HEAD` over src, include, runner, libretro, scripts, CMake files is clean (the only non-planning differences since are the ledger, page, docs and workflows; re-checked at 3912004). |
| 6 | D-03: the run is accepted on the merge line plus a two-file change, never on its exit status | VERIFIED | Merge line present (above). Commit `8964eef` holds exactly `docs/compatibility.md`, `tests/compat/ledger.csv` and the evidence note. `scripts/library.sh` lines 252-264 write the ledger and page only after update, ratchet and the page cap all pass, which is the plan's interrupted-run argument. |
| 7 | D-04: rows merged wholesale; allow-list empty, `quality.json` floors untouched, no core/runner/test/script change in the plan | VERIFIED | `git diff --quiet origin/main -- tests/compat/allow.txt tests/compat/quality.json include/cogmaster.h` exit 0 (allow.txt holds only its two comment lines; behaviour revision 18 on both sides). `git diff --quiet origin/main...HEAD -- . ':(exclude).planning' ':(exclude)docs' ':(exclude).github/workflows' ':(exclude)tests/compat/ledger.csv'` exit 0: the branch changes nothing in src, include, runner, libretro, scripts, tests or CMake. 0 added or removed `TIMEOUT` lines under tests/ or CMakeLists.txt. |
| 8 | D-02 / D-15 / rule 3: counts only, nothing from the dumps in the tree, commits or hand-off | VERIFIED | The plan's note-leak grep over `15-RUN-EVIDENCE.md` exit 0 (re-run at the refresh, after the Body edit); the `### Body` region grep (no file extension, home path, skip token, `title=`, forbidden phrase) exit 0 (re-run at the refresh); `grep -Eq '^\| *transition count *\|.*\| *0 *\| *$'` exit 0. `sh scripts/hygiene.sh --tree` exit 0 and `--history origin/main..HEAD` exit 0. The page and ledger contain only public catalog facts. What the agent opened during the run cannot be audited from the tree; the committed artifacts are clean. |
| 9 | D-10: `build` and `be32` are corrected in the same edit | VERIFIED | Diff: build 9 to 12 (`2 x 332 s`, run 43460663238), be32 15 to 21 (`2 x 609 s`, run 43458609482). Before: 540 s against 664 s and 900 s against 1218 s, so both were below the rule. Live read confirms both figures. |
| 10 | D-13 / D-18: SEED-016 corrected (pad press taken at the Phase 14 UAT; bank-shift trace stays out), SEED-015 records the timeout findings | VERIFIED | `14-UAT.md` test 1 records the press taken (`frozen_candidates=1`, `button1_changed_picture=0`, `button2_changed_picture=0`) and the bank-shift trace not taken, which is what SEED-016 now says; the stale "Not taken, for Phase 15" bullet is gone. SEED-015 gains the Phase 15 findings bullet and the "Done in Phase 15" tail on the drifted-timeouts bullet; the per-test TIMEOUT values it names (160, 142, 104, 45 s) stay open by design. Both seeds cite `15-RUN-EVIDENCE.md`. |
| 11 | D-13: the CI-speed paragraph landed in `docs/maintaining.md` without planning labels, under the docs cap | VERIFIED | Paragraph present after the macOS sentence (open once; rehearsal cost; dispatch compares the ledger with itself). Grep of the added doc and workflow lines for planning IDs and `Phase N`: none. `docs/maintaining.md` is 20537 bytes against the 38912 cap; the docs tests passed inside the 676. |
| 12 | D-15: the counts-only ship title and body are ready, flipped figures kept apart from newly judged | VERIFIED | `### Title` is `fix(compat): publish the post-fix library results and re-derive job timeouts` (patch release). `### Body` has `rows flipped (fail to pass): 285`, `newly judged: 0`, `other transitions: none`; each number matches my independent transition count. Totals, 512 PAL and 53 FM judged, 50 ntsc fail rows (48 fm-off, 2 fm-on) and 46 pal fail rows, and 1116 re-judged rows all match the ledger. The timeout table matches the workflow diff. The Body also carries the "Nightly changes" bullet that resolves W2 (below). |

**Score:** 12/12 truths verified (0 present, behavior-unverified)

### Notes and warnings (none blocks the goal)

**W1 - Plan 02's "timeouts commit is the last non-planning commit, only timeout lines change" no longer holds literally; the roadmap contract does.**
Code review (run before verification, rule 7) added three fix commits to `nightly.yml` after the plans: `34b464a`
(drop `continue-on-error` from the fuzz job), `01d5632` (a `concurrency` block that cancels a superseded
pull-request run), `2eecf46` (`-o Acquire::Retries=3` on both apt sites), plus doc lines in `docs/perf-fuzz.md`,
`docs/maintaining.md` and `docs/conformance.md`. Judgment: ROADMAP SC3 and TUNE-11 ("re-derived ... in one edit per
workflow file") concern the timeout re-derivation, and that was one edit: no `timeout-minutes` line or comment changes
after `afd413e`, `ci.yml` (the file whose edit triggers the rehearsal) and `digest.yml` have exactly one branch commit each,
and the roadmap note ("ci.yml is edited once in this phase") holds. The 15-02 must-have wording is plan-level and was
written before the review step existed. Checks on the review-fix lines: `actionlint` reports only the unknown
`ubuntu-26.04` runner label that already appears in untouched `ci.yml` lines (no expression or syntax error);
`report` lists `fuzz` in `needs`, so removing `continue-on-error` does make a fuzz finding open the nightly issue as the
new `docs/perf-fuzz.md` sentence says; no new `timeout-minutes` is involved. This is recorded as an accepted
deviation by the verifier's judgment, not as an owner-signed override. (Unchanged by the refresh.)

**W2 - RESOLVED. The ship hand-off body was stale against the final diff.**
Refresh check: `### Body` of `15-RUN-EVIDENCE.md` now holds the bullet "Nightly changes: the fuzz job is no longer
report-only, a superseded Nightly run on a pull request is cancelled, and the Nightly package installs retry". Each
clause matches the diff (`34b464a` removes `continue-on-error`, `01d5632` adds `cancel-in-progress` for `pull_request`
events, `2eecf46` adds `-o Acquire::Retries=3`). It names workflows by display name and no file. The `### Body` greps
(flipped and newly judged lines present; no file extension, home path, skip token, `title=` or forbidden phrase) exit 0,
the note-leak grep exits 0 and `sh scripts/hygiene.sh --tree` exits 0. That edit is still uncommitted in the working
tree, and the coordinator commits it. Original finding, kept for the record: the Body said nothing about the
review-fix changes the pull request carries, and the squash text understated what the release changes. The note's
`## Final gates` rows ("the timeouts commit is the newest commit touching anything outside .planning") were true when
recorded at `de463f2` and are historical now.

**W3 - RESOLVED. The new sentence in `docs/conformance.md` was inaccurate for this very pull request.**
Refresh check at `3912004`: line 129 now reads "its sources equal the merge commit's, and only the ledger, the page, the
docs and the workflows differ." Against the tree: `git diff --quiet 0b4ae67 HEAD -- src include runner libretro scripts
CMakeLists.txt CMakePresets.json tests/CMakeLists.txt` exits 0; the same diff over everything outside `.planning`,
excluding `docs`, `.github` and the ledger, exits 0; and `git diff --name-only 0b4ae67 HEAD -- . ':(exclude).planning'`
lists exactly the three workflows, `docs/compatibility.md`, `docs/conformance.md`, `docs/maintaining.md`,
`docs/perf-fuzz.md` and `tests/compat/ledger.csv`. The sentence is true. Original finding, kept for the record: the
earlier wording said the stamped commit's tree equals the merge commit's outside `.planning/`, which was false because
the ledger, page, workflows and docs differ.

**Open review items (by design, not gaps):** WR-02 (dead ledger-migration shims in the protected comparison, an owner scope
decision), IN-01 (stale cited runs on wasm32, retroarch, hash-equality, perf; D-10 leaves them while the rule holds, and
my live read confirms every such job still satisfies `M*60 >= 2*S`), IN-02 (`gh pr list` default limit in the digest) and
IN-03 (a cut HTML entity in two page rows, present in the base page too; fixing it needs a generator change and a
regeneration, not this phase). WR-05 is a ship-time fact (below).

### Ship-time checks (cannot exist before the push; confirm at `/gsd-ship`, rules 9, 10, 12)

| Check | Command | Now |
| ----- | ------- | --- |
| The ledger stamp is a public commit (D-06, rule 12) | `gh api repos/szTheory/cogmaster/commits/0b4ae67b7f14c3d997c24ca719fb18f1d3cd29a4` | HTTP 422 "No commit found" at the first run: the branch is unpushed (`git ls-remote --heads origin 'phase/15*'` prints nothing, `git branch -r --contains 0b4ae67` prints nothing). Must answer 200 after the push; a pull request keeps its commits reachable after the squash. |
| The pull request has a CI run on its head commit (SC4, hosted half) | `gh run list --commit <head sha>` after `gh pr create` | No pull request exists. `gh api repos/.../actions/runs?branch=phase/15-re-run-merge-and-timeouts` total_count 0. |
| Nightly's own pull-request run exercises the new `concurrency` expression and apt retries | PR checks on the pushed head | Not before the push. |
| Release pull request green; no `autorelease: pending` left | `gh pr list --label 'autorelease: pending'` after the merge | Post-merge. |

### Required Artifacts

| Artifact | Expected | Status | Details |
| -------- | -------- | ------ | ------- |
| `tests/compat/ledger.csv` | merged post-fix rows, stamped `core=` | VERIFIED | 1986 rows, 1116 stamped with one 40-hex commit, 285 fail-to-pass, written by `scripts/library.sh` only |
| `docs/compatibility.md` | regenerated page, new totals | VERIFIED | byte-equal to `compat-common.sh docs` output; contains "Generated from `tests/compat/ledger.csv`" |
| `15-RUN-EVIDENCE.md` | counts-only note with `## Merge checks` | VERIFIED | leak greps exit 0 (re-run after the Body edit); library lines the owner did not paste are `not reported`, as the plan allows |
| `.github/workflows/{ci,nightly,digest}.yml` | re-derived `timeout-minutes` and comments | VERIFIED | seven lines edited in `afd413e`, all matching the live read |
| `docs/maintaining.md` | three-rule CI-speed paragraph | VERIFIED | present, no planning IDs, within cap |
| SEED-015 / SEED-016 | breadcrumbs | VERIFIED | see truth 10 |

### Key Link Verification

| From | To | Via | Status | Details |
| ---- | -- | --- | ------ | ------- |
| `tests/compat/ledger.csv` | `docs/compatibility.md` | `compat-common.sh docs` | WIRED | `cmp` exit 0; `compat.whole_list.fresh` passed |
| `tests/compat/ledger.csv` | `tests/compat/exit.cmake` | `compat.exit` reads ledger and `.info` flag | WIRED | `compat.exit` and `compat.exit.self_test` passed |
| workflow `timeout-minutes` lines | `tests/cmake/repository.sh` | `hygiene.timeout_comments` (timeouts mode) | WIRED | check exit 0 on all 20 lines |
| timeouts table | `scripts/ci-durations.sh` | run IDs and seconds | WIRED | live re-run reproduces every row |

### Data-Flow Trace (Level 4)

Not applicable in the UI sense. The one data chain is judge result to ledger to page to exit verdict: summary.txt counts
(`pass=1029 fail=96`, `rows=1986`) equal the ledger parse, the page header and `compat.exit`, so the page is not a stub.

### Behavioral Spot-Checks

| Behavior | Command | Result | Status |
| -------- | ------- | ------ | ------ |
| Page equals its regeneration | `sh scripts/compat-common.sh docs tests/compat/ledger.csv \| cmp - docs/compatibility.md` | exit 0 | PASS |
| Old base ratchet accepts new ledger | `sh <origin/main copy>/compat-ratchet.sh base.csv tests/compat/ledger.csv tests/compat/allow.txt <date> 18 18 build/ci/tests/compat-fixtures/results.csv` | `compat: protected comparison passed`, exit 0 | PASS |
| Compat, hygiene, docs, policy tests | `ctest --test-dir build/ci -R '^(compat\|hygiene\|docs\|policy)\.' --no-tests=error` | 78 of 78, exit 0 | PASS |
| Timeout arithmetic and comment form | `cmake -DDIR=$PWD/.github/workflows -DCHECK=timeouts -P tests/cmake/repository.cmake` | exit 0 | PASS |
| Action pins unchanged in form | `cmake ... -DCHECK=action_pins -P tests/cmake/repository.cmake` | exit 0 | PASS |
| Full ci workflow | `cmake --workflow --preset ci` | exit 0, 676 and 5 tests | PASS |
| Hygiene workflow | `cmake --workflow --preset hygiene` | exit 0, 84 tests | PASS |
| Live durations reproduce the table | `sh scripts/ci-durations.sh <wf> 40` x3 | exit 0, identical seconds and run IDs | PASS |
| Workflow syntax | `actionlint` on the three workflows | only the repository-wide unknown `ubuntu-26.04` label | PASS (no expression error) |
| Refresh: tree hygiene after the two fixes | `sh scripts/hygiene.sh --tree` | exit 0 | PASS |

### Probe Execution

SKIPPED: the phase declares no `probe-*.sh` scripts and is not a migration or tooling phase.

### Requirements Coverage

| Requirement | Source Plan | Description | Status | Evidence |
| ----------- | ----------- | ----------- | ------ | -------- |
| RUN-01 | 15-01 | Library judge re-run after the fixes; flipped `pass` rows merged through the ratchet, no `pass` row becomes `fail` | SATISFIED | truths 1, 5, 6, 7; 285 fail-to-pass, pass_lost 0, old base judge exit 0. The "by pull request" half completes at ship. |
| RUN-02 | 15-01 | Page and counts agree with the merged ledger; `compat.exit` stays green | SATISFIED | truth 2 |
| TUNE-11 | 15-02 | asan, bench, bench-wasm32 timeouts and fuzz and digest comments re-derived with run IDs, one edit per workflow file | SATISFIED | truths 3, 9, 11; see W1 for the later nightly.yml review fixes |

All three IDs declared in the plan frontmatter (15-01: RUN-01, RUN-02; 15-02: TUNE-11) appear in REQUIREMENTS.md and map to
Phase 15; `grep "Phase 15" .planning/REQUIREMENTS.md` returns exactly those three, so there are no orphaned requirements.

### Anti-Patterns Found

| File | Line | Pattern | Severity | Impact |
| ---- | ---- | ------- | -------- | ------ |
| (added lines of workflows and docs) | - | TBD / FIXME / XXX / TODO / HACK / PLACEHOLDER | none found | - |
| `docs/conformance.md` | 129 | claim not true for this PR (W3) | Warning, resolved at 3912004 | sentence now matches the tree |
| `15-RUN-EVIDENCE.md` `### Body` | - | omitted the Nightly review-fix changes (W2) | Warning, resolved (edit uncommitted at refresh) | body now names them |

### Prohibitions (plan frontmatter `must_haves.prohibitions`)

| Prohibition | Disposition | Evidence |
| ----------- | ----------- | -------- |
| Ledger and page never edited by hand; no allow-list | resolved | allow.txt and `quality.json` unchanged (exit 0); page byte-equals regeneration; merge line `ratchet=ok` |
| Agent reads only summary.txt; no dump data in tree or text | resolved on committed artifacts | leak greps and `hygiene.sh --tree/--history` exit 0; agent-side file opens are not auditable from the tree, and the verifier read only the counts lines of summary.txt |
| No core, runner, script, test or behaviour-revision change | resolved | branch diff outside `.planning`, docs, workflows and ledger is empty; revision 18 both sides |
| Nothing pushed, no pull request in the plans | resolved | `git ls-remote --heads origin 'phase/15*'` prints nothing; 0 runs on the branch; 0 `workflow_dispatch` runs of CI or Nightly since 17:00Z |
| No outlier trimmed, no ceiling, no job outside the named seven edited for timeouts, no per-test TIMEOUT touched | resolved | timeouts diff is the seven lines; 0 TIMEOUT lines changed |
| Workflow files edited in one commit only for timeouts | resolved with W1 | `afd413e` is the only commit that touches a `timeout-minutes` line |
| No planning IDs in `docs/maintaining.md`; body names no file | resolved | greps exit 0 (Body re-checked at the refresh) |
| Flipped figure counts only fail-to-pass | resolved | body 285 fail-to-pass, newly judged 0, equals my count |

None is flagged `unverified-prohibition`; no judgment-tier item remains for the owner (rule 6).

### Human Verification Required

None. Everything above was checked by command, by reading docs against code or by parsing the data. The remaining items
are ship-time facts that the ship step confirms by command.

### Gaps Summary

No gaps. The ledger and page carry 285 fail-to-pass rows through the unchanged ratchet with no `pass` row lost, the page is
the regeneration of the ledger and `compat.exit` is green, and every timeout line the roadmap names (and the two it did not)
now equals twice the slowest successful run in a live 40-run window with its run ID, confirmed against the API.
Of the three non-blocking warnings, W2 (hand-off body) and W3 (the `docs/conformance.md` sentence) are resolved and
re-checked; W1 stays a recorded judgment about the review-fix commits.

## Refresh

Requested after W2 and W3 were fixed, because the first digest went stale (`verification.status` read `stale`). Done:
the W3 sentence re-checked against the tree (true); the W2 Body bullet re-checked against the three review-fix commits and
the body greps (clean); `sh scripts/hygiene.sh --tree` re-run (exit 0); `verification.fingerprint` recomputed over the same
covered set, with the new `covered_digest` copied verbatim into the frontmatter (the change comes from
`docs/conformance.md`). No truth changed status; the score stays 12/12, and the ship-time checks are unchanged. The
`15-RUN-EVIDENCE.md` Body edit is uncommitted and this report is untracked; the coordinator commits both.

---

_Verified: 2026-11-13T21:34:54Z_
_Verifier: Claude (gsd-verifier)_
