# Phase 15: Re-run, merge and timeouts - Context

**Gathered:** 2026-11-13
**Status:** Ready for planning

<domain>
## Phase Boundary

The library judge is re-run over the owner's library at the Phase 14 head, and the rows that flipped to `pass` are
merged into `tests/compat/ledger.csv` and `docs/compatibility.md` through the existing ratchet, with no `pass` row
becoming `fail` (RUN-01). The page and its counts agree with the merged ledger and `compat.exit` stays green (RUN-02). The
CI job timeouts that drifted from the twice-the-slowest rule, and the stale fuzz and digest comments, are re-derived from
live runs, each with its run ID, in one edit per workflow file (TUNE-11). One pull request, one release.

Not this phase: any core, runner or test-code change (a regression the run exposes is a new fix on a new branch, D-04); a
numeric target for flipped rows; TMS9918 modes (SEED-006); the per-test ctest timeouts and the other SEED-015 `ci.yml` items
(D-13); raising `quality.json` floors; the Sega bank-shift trace (D-11).

</domain>

<decisions>
## Implementation Decisions

Method: the owner's standing fan-out boilerplate was applied. Three advisor agents (ledger, ratchet and page; CI timeout
economics; sequencing, hand-off and release engineering) each read the code, ran only read-only `gh` and `git` commands, applied
their role lenses and ran a self-adversarial pass. The orchestrator reconciled them (conflicts are named), re-checked the
two claims that decide the plan against the tree (the Phase 14 UAT record of the pad press, and that `library.sh run` does not
rebuild the runner) and ran a cross-cutting pass. No advisor saw a dump. All areas were taken without a per-area vote (the
boilerplate asks for one coherent set that is followed). Numbers below are from the discuss session; every one is re-read at
execution time.

### The library re-run (RUN-01)

- **D-01:** The owner runs the library judge once, at the start of the execute session, as the no-argument
  `sh scripts/library.sh` (the `library` workflow: it rebuilds the runner at HEAD, then its `library.local` test runs `run`).
  Not `sh scripts/library.sh run` alone: `run` does not rebuild, and a stale binary would stamp `core=<HEAD sha>` on rows it
  did not judge. The agent states why in the ask (rule 6: the dumps and the secrets guard are the owner's; this is a run on the
  owner's own library). The earlier Phase 13/14 prohibition on this command was for their phases, which had to leave the ledger
  alone; here the merge is the goal. The agent makes no tracked edit until the owner reports the `merge:` line, so the recorded
  tree is clean (changes under `.planning/` are excluded by the judge's dirty check, and they are the only ones allowed).
  — **Reversibility:** reversible — the merge writes two tracked files and nothing else.
- **D-02:** What the agent may read afterwards is `build/library/results/summary.txt` only (counts: the `library:`, `pal:`, `fm:`,
  `merge:` and `exit:` lines) and the tracked ledger and page diff (public catalog facts). It never reads another file in that
  directory (`.path`, `.probe` and probe files stay unread). The owner may instead paste those blocks.
- **D-03:** The run's exit status is not the evidence. Only `merge: held (ratchet refused)` and `(update refused)` exit 1;
  `held (page over the docs cap)` exits 0, and when the merge is held the `exit:` block still reads the pre-merge ledger. The
  agent accepts the run only on the line `merge: rows_added=N ratchet=ok page_bytes=B` plus `git diff --numstat` showing exactly
  two changed tracked files, `tests/compat/ledger.csv` and `docs/compatibility.md`.
- **D-04:** Merge everything the run produces, wholesale: default, PAL and Japanese-FM rows alike. There is no row filter and none
  is added. Audio and fixture facts are dropped by the updater by design, so they never reach the ledger. `allow.txt` stays empty
  and `quality.json` is untouched (its floors of 20 cart passes, one PAL row and one FM row are already met; raising them is a
  reviewed edit that nothing here needs). Allow-listing is structurally impossible in this pull request (main is already at
  behaviour revision 18, and an allow line needs the head revision to exceed the base). If the ratchet refuses a row, the whole
  merge is held: stop, report the printed counts, triage as a core bug and fix it on a new branch, then re-run. Never edit
  the ledger by hand, never allow-list. Phase 14's `run_fail_not_in_ledger=0` covers only the first NTSC profile of each cart,
  so a PAL or non-pad-port pass row that the `$FFFC` change now fails would first appear as this refusal; that is the intended
  detector, and no pre-flight tool is added (rules 7 and 8). — **Reversibility:** reversible.
- **D-05:** Expect large churn and say so in the pull request body: every re-judged row's `evidence` (`core=<HEAD sha>`) and
  `date` change, passes included, about 1,100 judged rows. To keep real flips visible, run one awk transition count from the
  public ledger against `origin/main` (counts by machine, tv, fm, old status, new status), record the command and its exit
  status, and commit nothing. Success looks like zero pass-to-fail transitions and a default-profile fail count near the 49
  recorded in `14-FIX-OUTCOMES.md`; a material difference is a question for the owner's optional counts-only `diagnose`, not a
  failure by itself (the ratchet is the gate).
- **D-06:** Rule 12 for the evidence sha: the `core=` value is the branch HEAD at run time, which may be a local planning
  commit. It must answer 200 from `gh api repos/<o>/<r>/commits/<sha>` before ship, which holds once the branch is pushed
  (a pull request keeps its commits reachable after a squash). The verifier checks it after the push and records the command and
  its status.

### The page and the exit command (RUN-02)

- **D-07:** The page is machine output and is never hand-edited. Prove agreement with two commands the agent runs once and
  records (command, exit status): `sh scripts/compat-common.sh docs tests/compat/ledger.csv | cmp - docs/compatibility.md`, and
  `ctest --preset ci -R compat.exit -V`. In CI, `compat.whole_list.fresh`, `compat.exit`, `compat.page_bound` and the protected
  ratchet already enforce it on the pull request; nothing new is written. The only place with pass and fail counts is the page
  (no other file quotes them), so no other file needs an edit; the verifier greps once to confirm.

### CI timeouts (TUNE-11)

- **D-08:** Method: `sh scripts/ci-durations.sh <workflow> 40` for `ci.yml`, `nightly.yml` and `digest.yml`, read again
  immediately before the edit (the maximum moves: a main run finished during the discuss and raised asan from 194 s to 243 s).
  Every timeout is twice the slowest successful run in that window, rounded up to whole minutes, never below the 5-minute floor
  where the repository already applies one, and each comment names that run's ID. No outlier is trimmed and no ceiling is added:
  pull-request runs on old branches are real successful runs of the same job. No live dispatch is needed or made: the window
  already holds 37 to 40 runs per job, which satisfies "two runs, never one", and every edit here loosens a limit. Do not dispatch
  Nightly (the macOS overlap rule); the nightly schedule adds samples by itself.
- **D-09:** The five named items and their discuss-time values (re-read at execution): `ci.yml` asan 5 to 9 minutes (2 x 243 s,
  run 43462502049; 7 if the 194 s maximum still stands); `nightly.yml` bench 10 to 13 (2 x 368 s, run 42764095033, a
  perf-proof pull request run; scheduled main runs top out at 301 s, kept anyway) and bench-wasm32 15 to 16 (2 x 456 s, run
  42768407261, a bimodal runner effect, kept); the fuzz comment (the 5-minute floor stands; 144 s, run 43020467034 replaces the
  stale 90 s); the digest comment (`no runs yet` is stale: one dispatch from `main` exists, 55 s, run 42769805382, minutes
  stay 5). Cite exact run IDs from the final read.
- **D-10:** Two jobs the roadmap does not name violate the same rule more than any of the five, and are corrected in the same
  single `ci.yml` edit at no extra edit cost: be32 (15 minutes against 609 s, run 43458609482, so 21) and build (9 against 332 s,
  run 43460663238, so 12). The cause is test growth, not the runner: `runner.diagnose` (311 s on the be32 lane) and
  `library.diagnose` (142 s) arrived in Phases 13 and 14. The pull request body names this as a note, not a requirement.
  Other jobs whose cited run ID is stale but whose minutes still satisfy twice the slowest (wasm32, retroarch, perf, hygiene,
  nofp) are left alone and listed as a note: refreshing a comment that no rule requires is invented work.
- **D-11:** `hygiene.timeout_comments` checks only that each comment is internally of the form `2 x S s, slowest of N runs, run
  <id>` or a floor line, so it cannot see drift; do not treat a pass as proof. The reviewer reads each edited line against the
  fresh script output. The digest comment may read `5-minute floor; 2 x 55 s, slowest of 1 run, run 42769805382` (the form is
  already used at `ci.yml:390`).
- **D-12:** One edit per workflow file means one commit that touches `ci.yml`, `nightly.yml` and `digest.yml` together,
  and it is the last commit pushed. `git log origin/main..HEAD -- .github/workflows/ci.yml` shows exactly one commit at verify.
  Editing `ci.yml` triggers the release rehearsal once (about six extra minutes); `nightly.yml` is path-filtered to run on its own
  pull request, so it overlaps CI once on macOS; the precedent (Phase 10, both green) accepts that, and no hand dispatch is made.
- **D-13:** Out of scope, left as notes in SEED-015: the per-test ctest timeouts (`library.synthetic` 160 s, `compat.ratchet`
  142 s, `compat.public_update` 104 s, `compat.csv` 45 s) because `hygiene.timeout_comments` scans `timeout-minutes` only and the
  roadmap names jobs, not tests; the CI-speed observation that `runner.diagnose` roughly doubled the be32 lane (a possible test-lane
  change is a seed, because making a job faster inside the same pull request would put its timeout out of step with the runs it
  cites); the three-rule "CI speed" text of SEED-015 goes into `docs/maintaining.md` only if the planner can write it in
  a short paragraph (open the pull request once at the end, a release-input edit doubles the run, a dispatch compares the ledger
  with itself).

### Sequencing, shipping and the fallback

- **D-14:** Order. (1) Owner run (D-01). (2) Agent verifies by counts (D-02 to D-05), then one commit with the ledger, the page
  and the `.planning` notes. (3) Local gates, run once and recorded: `cmake --workflow --preset ci`; the old base judge from
  `git show origin/main:` copies of `scripts/compat-ratchet.sh` and `scripts/compat-common.sh` against the new ledger (the
  method `docs/conformance.md` gives and #37 used), exit 0. (4) The single timeouts commit (D-12). (5) One push, then
  `gh pr create` non-draft with the final title and body. A pushed branch without a pull request costs no CI (`ci.yml` runs on a
  pull request and on pushes to `main` only), and every edit of a pull request's title or body reruns the matrix, so write both once.
  No draft pull request early: a draft runs the same CI on every push and cannot auto-merge. (6) `gh pr merge --auto --squash`, then
  confirm a CI run exists on the head commit (`gh run list --commit <sha>`), then, after the merge, that the release pull request
  is green and no `autorelease: pending` pull request is left (`docs/maintaining.md`). Plans end where each GSD step ends;
  ship, review and verify are separate steps (rule 9).
- **D-15:** Title: `fix(compat): publish the post-fix library results and re-derive job timeouts`, a patch release (0.14.2 under
  the repository's release-please config, `bump-minor-pre-major`). `feat` would give 0.15.0 (the precedent of #33 and #37); the
  owner can override, but never `docs:` or `ci:`, which release nothing and would break "every phase ends in a release". Body:
  counts only (rows flipped, pass and fail totals, PAL and FM counts, `rows_added`, `ratchet=ok`, `page_bytes`, the judged-cart
  floors verdict, the timeout table with run IDs), no catalog titles, no file names, no phrase "owner library" (say compatibility
  results), no CI skip token (the body becomes the squash message). — **Reversibility:** reversible.
- **D-16:** Shape: two plans. Plan 1 (wave 1): the owner hand-off as a blocking checkpoint, the counts verification, the ledger and
  page commit, the local gates. Plan 2 (wave 2): the timeouts and comments in one commit, the optional maintaining paragraph, and the
  seed breadcrumbs (D-18). The planner may merge or split if its own analysis says so.
- **D-17:** If the owner is not present for the run, the agent still prepares plan 2 locally but does not push or open a pull
  request, records RUN-01 and RUN-02 as pending with `stopped_at` "owner run pending", names the exact command and why only the
  owner can run it, and stops. A timeouts-only `ci:` pull request would release nothing and cannot end the phase; a
  `fix` pull request with no results would claim a fix it does not carry. The phase stays open until the run happens.
- **D-18:** Breadcrumbs. `SEED-016` still says the pad press on the `static_no_fault` row was "not taken"; the Phase 14 UAT
  records it taken (one frozen candidate, no picture change under either button, rank 10 closed), so correct the line. The Sega
  bank-shift trace (D-06 of Phase 14) stays out: HEAD no longer raises that fault and the runner prints no write values, so the
  trace needs a baseline build and a manual read, and its residual risk is covered better by this run, where any pass row the
  change displaced is a refused ratchet row. It reopens only if the run refuses a row. Update `SEED-015` with the timeout
  findings (D-13). Evidence rows in the phase notes are a command, an exit status and a run URL, never a pasted log.
- **D-19:** Review runs before verification, over what changed. Little code changes here (two tracked data files, three workflow
  files, docs, seeds), so review is short; findings about tooling become seeds, with at most two fix rounds and one gap round
  (rule 7). Phase 14 skipped review as an advisory step; this phase records review as run or explicitly not applicable, never silent.

### Claude's Discretion

- Whether the awk transition count is a one-off command in the notes or a few lines in the verification record.
- The plan boundaries beyond D-16, and the wording of the optional maintaining paragraph.
- The exact rounding at the margin when a re-read changes the maximum between discuss and execute.

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Phase inputs
- `.planning/ROADMAP.md` (Phase 15) and `.planning/REQUIREMENTS.md` (RUN-01, RUN-02, TUNE-11, Out of Scope) - goal, criteria and fences.
- `.planning/phases/14-fix-the-causes/14-FIX-OUTCOMES.md`, `14-CONTEXT.md`, `14-VERIFICATION.md`, `14-UAT.md` - what changed in the core, the 205 to 49 library result, the pad press record.
- `.planning/seeds/SEED-016-failing-carts.md` (breadcrumb to correct), `.planning/seeds/SEED-015-whole-list-follow-ups.md` (CI items, "settle the sample-based timeouts again", review leftovers about the `exit:` block), `.planning/seeds/SEED-006-sg1000-mode.md`.
- `.planning/LESSONS.md` and `CLAUDE.md` (root) - rules 3, 6, 7, 8, 9, 10, 11, 12.

### Ledger, ratchet and page
- `scripts/library.sh` (the `run` branch, merge and `merge:` line), `scripts/library.cmake`, `scripts/compat-ratchet.sh`, `scripts/compat-common.sh` (`cases`, `docs`, `normalize`, ratchet rules around lines 72 to 104), `scripts/compat-update.sh`, `tests/compat/exit.cmake`, `tests/compat/quality.json`, `tests/compat/tests.cmake`.
- `docs/conformance.md` (library, whole-list ledger, ratchet and exit sections), `docs/compatibility.md`, `docs/testing.md`, `docs/maintaining.md`.

### CI timeouts and release
- `scripts/ci-durations.sh`, `tests/cmake/repository.sh` (`timeouts` mode), `tests/hygiene/tests.cmake` (`hygiene.timeout_comments`), `scripts/classify-changes.sh` (which edits trigger the rehearsal), `release-please-config.json`.
- `.github/workflows/ci.yml` (asan, be32, build jobs), `.github/workflows/nightly.yml` (bench, bench-wasm32, fuzz), `.github/workflows/digest.yml`.

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `sh scripts/library.sh` (no argument) and its `merge:` summary: the only merge path; it refuses a ratchet failure.
- `scripts/ci-durations.sh`: prints job, runs, slowest seconds, run ID, minutes and the exact comment text to paste.
- `compat-common.sh docs`: regenerates the page from the ledger, the only way the page is written.

### Established Patterns
- Ledger and page change only through `library.sh` or `compat-update.sh`; the committed page must equal regenerated output.
- The old-base judge is proved locally from `git show origin/main:` copies, because a dispatch compares the ledger with itself.
- Timeout comments read `2 x S s, slowest of N runs, run <id>`; a floor line names its reason.

### Integration Points
- Behaviour revision on `main` is 18 (`include/cogmaster.h`), so the ratchet's revision exception cannot apply to this pull request.
- `ci.yml` and `release.yml` edits trigger the release rehearsal; `nightly.yml` is path-filtered onto its own pull request run.

</code_context>

<specifics>
## Specific Ideas

- Judge the re-run by zero pass-to-fail transitions and the default-profile fail count near 49, not by the number of rows flipped.
- Name the runner and timing figures "slowest successful run in the window", never "measured on" a single run.

</specifics>

<deferred>
## Deferred Ideas

- Per-test ctest timeouts (`library.synthetic`, `compat.ratchet`, `compat.public_update`, `compat.csv`): SEED-015.
- Making the be32 lane cheaper (the `runner.diagnose` fixture), to be judged against the be32 coverage it should keep: a seed.
- Pre-flight check of PAL and non-pad-port profiles before the merge (new tooling, rules 7 and 8): only if a refusal ever needs it.
- The `exit:` block reading the pre-merge ledger when the merge is held, and exit status 0 on `held (page over the docs cap)`: SEED-015 already lists the first; add the second if either bites.
- The Sega bank-shift trace and the Start-press campaign: reopen only on a refused row.
- The CI nightly freshness step and the other SEED-015 `ci.yml` items: still wait for a phase that edits `ci.yml` for its own reasons.

</deferred>

---

*Phase: 15-Re-run, merge and timeouts*
*Context gathered: 2026-11-13*
