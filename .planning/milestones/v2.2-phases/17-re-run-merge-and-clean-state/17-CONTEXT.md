# Phase 17: Re-run, merge and clean state - Context

**Gathered:** 2026-11-14
**Status:** Ready for planning

<domain>
## Phase Boundary

The rows the Phase 16 legacy renderer moved reach the ledger and the compatibility page under a `core=` stamp that is an
ancestor of `origin/main`; the rows that still fail are counted by cause; the diagnose vocabulary nothing raises is gone
in one change of the line shape; the drifted timeouts are re-derived from live runs; and a maintainer can run one script
to check the repository is clean before a milestone-close pull request. Requirements: RUN-03, RUN-04, TUNE-15, TUNE-16,
TUNE-17. The release type is `fix(compat)` (a patch release, expected 0.15.1). No `src/` or `include/` change: the
core's behaviour revision stays 19 and the state format stays 17.

Starting point: Phase 16 is merged and released (v0.15.0, release run green, 19 assets). `origin/main` is 0a97075.

</domain>

<decisions>
## Implementation Decisions

The discuss step ran as a fan-out of five subagents (prune scope, timeouts, milestone-close script, re-run sequencing,
then an adversarial pass over the three code-level answers). The recommendations below are the synthesis; each carries
the evidence that decided it. Nothing here needs the owner except the run in D-04.

### Re-run sequencing (RUN-03, RUN-04)

- **D-01: One pull request, with the judge run on a detached worktree of `origin/main`.** The agent creates
  `git worktree add --detach <scratch>/main-wt origin/main` (0a97075, or later if `main` has moved and the tip is still a
  descendant of it), the owner runs the judge there, and the agent carries the machine-written ledger and page into the
  phase branch. The stamp is then a real ancestor of `main`. Rejected: judging on the phase branch (this is exactly the
  Phase 15 stamp `0b4ae67`, a pre-squash commit that `git merge-base --is-ancestor` refuses against `origin/main`; the v2.1
  audit flagged it); two pull requests in one phase (two ships and two CI cycles, and the timeouts would be derived before
  the final test mix exists).
- **D-02: The prune lands before the owner's session, so the diagnose counts come from the final line shape.** The judge
  verdicts cannot depend on the prune (the judge uses `--probe` and `--expect`, and `--diagnose` only in the diagnose
  branch; the prune touches only `runner/diag*`, the `library.sh` diagnose awk, two test files and `docs/runner.md`), so
  the stamp describing main at 0a97075 is honest. The agent records one command at ship to prove the shipped tree matches
  what was judged where it matters: `git diff --quiet 0a97075 HEAD -- src include CMakeLists.txt cmake` exits 0.
- **D-03: Order of work inside the phase:** (1) prune, docs and the milestone-close script, pushed once so CI gives a
  clean sample; (2) one owner session; (3) the ledger and page commit; (4) the last commit holds every timeout edit (D-11);
  (5) the final push, the pull request titled `fix(compat): ...`, `gh pr merge --auto --squash`, a CI run on the head,
  the release pull request confirmed green. If the owner is present earlier, the judge half of D-04 may run first for an
  early signal on a lost `pass` row; the diagnose half must wait for the prune.
- **D-04: One owner session, two commands, counts only (rule 6: only the owner holds the library).** The owner exports
  the library directory in the shell where the agent runs the commands (the Phase 15 pattern), and runs `unset
  COGMASTER_LIBRARY_OUT` first (a value pointing into the main checkout is refused with exit 2).
  (a) In the worktree, the no-argument form `sh scripts/library.sh`: it configures, builds the runner at that commit and
  runs `library.local` (about 11 minutes in Phase 15). `run` alone does not rebuild, so a stale binary could be stamped.
  The agent reads `build/library/results/summary.txt` only and accepts it when it carries `merge: rows_added=N ratchet=ok
  page_bytes=B` and `git -C <worktree> diff --name-only` lists exactly `tests/compat/ledger.csv` and
  `docs/compatibility.md`.
  (b) On the phase head with the carried ledger, `sh scripts/library.sh diagnose`: it stamps nothing; the agent reads the
  counts by cause from standard output and `build/library/diag/diagnose.txt` and records counts in VERIFICATION.
  If the owner cannot run it, prepare locally, open no pull request, stop, and leave RUN-03 and RUN-04 pending (the same
  hand-off as Phase 15; fallback is the owner running in their own terminal and pasting counts). Never read the library
  directory; a hook blocks commands that name the owner's secret file and the agent never routes around it.
- **D-05: Carry, never hand-edit.** `git -C <worktree> diff -- tests/compat/ledger.csv docs/compatibility.md | git apply`
  on the phase branch. Then check, recording command and exit status (rule 11): one distinct 40-hex `core=` value;
  `git merge-base --is-ancestor <stamp> origin/main` exits 0; `gh api repos/<owner>/<repo>/commits/<stamp>` exits 0; no
  `pass` row lost (the ratchet already ran inside `run`). Afterwards `git worktree remove --force` the worktree: its
  `build/library/results/<n>.path` file holds the owner's library path.
- **D-06: Failure branches, with no invented exception path (rule 8).** A ratchet refusal means a Phase 16 renderer bug
  flipped a `pass` row to `fail`: `allow.txt` cannot help (it needs a behaviour revision above base) and none may be
  added. Treat it as a core bug: fix on a new branch, merge to `main`, then restart from the worktree step (the
  two-pull-request shape, forced by evidence). `library.sh diagnose` reports `run_fail_not_in_ledger`, a cheap pre-flight.
  A page over the docs byte cap exits 0 with `merge: held`: triage the cap, never edit the page by hand. If `main`
  moves before the merge the stamp stays an ancestor and nothing re-runs.
- **D-07: Ancestry stays unenforced by code.** Nothing in CI or tests checks the stamp (only a 40-hex shape check in
  `compat-common.sh` and the `core=` regex in `tests/compat/update.cmake`). Do not add a gate for it: rule 7 asks every
  gate to name a fault it prevents, and the Phase 15 audit fault is caught by the recorded `is-ancestor` command at ship.
  `docs/conformance.md` line 129 (the pull-request-head workaround) gets one sentence saying to judge a commit already on
  `main` through a worktree.

### Diagnose prune (TUNE-15)

- **D-08: Remove what is verified dead, change the line shape once.** Every raise site in `src/` uses the device RAM,
  CART, MAPPER, VDP or CPU; nothing raises the device CTRL or NONE nor the kind ROM_WRITE, and a loaded state cannot
  reach `--diagnose` (`runner/args.c` forbids other run options with it). So `wclass`, `pclass`, `where=port`, `dev=ctrl`
  (same cause, not on the roadmap list) and `fault=rom_write` go. The line drops from 32 to 30 fields and the runner
  token becomes `diagnose 2`, so a stale runner against the new `library.sh` is a malformed record, not a silent
  mismatch. The `library-diagnose 1` output header stays (its sub-row set only shrinks and it is human output).
- **D-09: Keep what is still raised or pinned.** `vdpmode` stays (text, multicolour, the mixed and invalid classes and the
  native Game Gear still raise `unsupported_video_mode`; its `mode4` word is an unreachable canary by design, so leave it)
  and `faddr` stays (`mapper_range` still raises). The public `COGMASTER_FAULT_KIND_ROM_WRITE` constant, the name-table
  entries for `rom_write` and `ctrl` (the tables use designated initialisers, so dropping an entry leaves a NULL that
  `printf` crashes on in `runner/main.c`), `tests/api/v2.c` and `docs/embedding.md` stay. The bus test `fault.rom_write`
  asserts no fault and stays. `cogmaster__diag_where` maps the CTRL and NONE devices to `unknown`.
- **D-10: Change list (net about -100 lines).** `runner/diag_class.h`, `runner/diag.c`, `scripts/library.sh` (closed
  lists lose `rom_write`, `ctrl`, `port`; `$9 != "2"`; `NF != 30`; size and header move to `$29` and `$30`; the label
  array and `subs` rows renumber), `tests/runner/test_runner.c` (the class tables and the CTRL/NONE checks), `tests/runner/diagnose.cmake`,
  `tests/library/diagnose.cmake` (about 19 canned lines, the shape regex, the leaf names, `tail_port`/`tail_rom`; the two
  canned rows that used the dead values re-point to `unmapped_read dev=cart where=rom` and `unmapped_write dev=cart`; row
  counts stay), `docs/runner.md` (the example line, the `where` row, delete the two class rows; it sits at 37,973 of a
  38,912-byte cap, so shrinking is safe) and `docs/conformance.md` line 35. No always-`none` field is kept for
  compatibility: the line is local-only and nothing committed reads it. Run `runner.diagnose`, `runner.diagnose.class`,
  `library.diagnose` with its taxonomy, hostile and leak cases, then the full preset.

### Timeouts (TUNE-16)

- **D-11: All timeout edits go in one last commit, after a clean sample.** The "diagnose test" and "hygiene tree check"
  are CTest `TIMEOUT` values in `tests/*.cmake` (multiplied by the lane scale: asan 3, be32 8, wasm32 5), not
  `timeout-minutes` lines; only the `timeout-minutes` lines are machine-checked by `hygiene.timeout_comments`. The last
  non-planning commit holds the `ci.yml` `timeout-minutes` lines and the CTest `TIMEOUT` values together, so
  `git log --oneline origin/main..HEAD -- .github/workflows/ci.yml` lists exactly one commit. Before it, wait for CI to be
  green on the previous head, whose code equals the shipped code with the old limits, then
  `sh scripts/ci-durations.sh ci.yml 40` (its `comment` column is the text for each `timeout-minutes` line). A pull
  request's own run is not a sample of its own limits, and every change is a raise except the optional perf one, so the
  pull request cannot trip a new limit. Do not re-derive after that push; a `ci.yml` edit also starts the rehearsal job
  (about 6 minutes), so push it once. Do not touch `nightly.yml` or `digest.yml` (an edit starts the macOS nightly bench).
- **D-12: Discuss-time measurement (re-measure at execute; the last 40 successful `ci` runs, 43129813707 to
  43610370595).** The be32 job limit holds at 21 minutes (slowest 609 s, run 43458609482; since Phase 16 at most 562 s).
  The test limits are what drifted: `library.diagnose` (`tests/library/tests.cmake`) 206 s to 289 s (slowest 144.43 s,
  macos-x64, run 43460663238); `hygiene.tree` (`tests/hygiene/tests.cmake`) 60 s to 75 s (slowest 37.32 s, run 43562888383);
  the rehearsal job 13 to 14 minutes (2 x 393 s = 786 s exceeds 780 s; run 43513035368); `runner.diagnose` holds at 633 s
  (still at least twice every lane's slowest; the be32 job cap is its real guard). The hygiene job stays at its 5-minute
  floor with a refreshed comment (the cited run left the window). Every other `timeout-minutes` line needs a comment
  refresh only. The perf job (6 minutes against a 106 s slowest) stays at 6: lowering is allowed by the rule but saves
  nothing, so keep this change raise-only.
- **D-13: Comment rules that the check enforces.** `(^|; )2 x <int> s[^,]*, slowest of N run(s), run <9+ digits>`, the
  value at least twice the slowest, two spaces before `#`, the word `provisional` rejected, no planning IDs in code
  comments, jobs with a floor keep the `5-minute floor (API|download); ` prefix, edits matched on display name (`CI
  required`, matrix suffixes removed), and counts are runs, not lanes (be32 shows 36 runs, not 40). Add a comment above
  `hygiene.tree` (it has none). Form for CTest values: `# Timeout: 289 s; 2 x 144.43 s (macos-x64), slowest of 40 runs,
  run 43460663238.`

### Milestone-close check (TUNE-17)

- **D-14: A new `scripts/milestone-close.sh`, about 45 lines of POSIX sh, classified `operation`** in
  `tests/policy/repository-checks.txt` (like `apply-repo-settings.sh` and `ci-durations.sh`; an unclassified new script
  fails `tests/cmake/budgets.cmake`). Word the reason without "never fails" (it exits 1 on tree state). The budget
  carries no risk either way (887 counted check lines against a limit of 973; as a `check` it would be about 932).
- **D-15: Rows, one per line, `name  ok|off|unknown  short reason`, counts and 7-character shas only, no URLs.**
  (1) `open-prs`: `gh api "repos/$repo/pulls?state=open&per_page=100" --jq length`; any open pull request, release-please
  and dependabot included, is `off` (rule 10: the release pull request is never held), and the docs say to run the script
  before the close pull request is opened. (2) `main-ci`: the `CI required` check run on the head of `main`, read with the
  exact filter `check-runs?check_name=CI%20required` (the unfiltered list pages at 30 runs; the commit `status` endpoint
  says `pending` here and is never used); `success` is ok, `failure`, `cancelled` and `timed_out` are `off`, `queued`,
  `in_progress` and an empty result are `unknown`. (3) `release`: ok only when `releases/latest` equals `v<manifest
  version>` (the manifest read with `git show origin/main:.release-please-manifest.json`, not the working tree), no draft
  exists (`gh api --paginate .../releases` with `select(.draft)`; drafts are missing from `gh release list`), and `CI
  required` is green on the tag. (4) `release-pr`: a merged pull request still labelled `autorelease: pending` means the
  tag step failed (searched with the search endpoint, since the issues endpoint also returns closed-unmerged ones).
  (5) `tree`: `off` if the branch is not `main`, if `git status --porcelain` is non-empty (untracked files included; do
  not copy the `.planning` exemption from `library.sh`), or if local `HEAD` differs from the remote `main` sha.
  (6) the stale-ok gap: `off` if any commit between the latest tag and `main` starts `feat`, `fix` or `perf` (a releasable
  squash merge before release-please opens its pull request leaves `latest == manifest` and zero open pull requests).
- **D-16: Exit codes and shell hygiene.** 0 when all ok, 1 when any row is `off`, 2 when none are off but some are
  `unknown` (network failure, rate limit, CI still running); last line `close-check: all ok` or `close-check: N off, M
  unknown`. Every `gh` call is `v=$(gh ...) || v=unknown`, never a pipe (a piped `$?` under `set -e` without `pipefail`
  stayed green while reporting failures in a sibling), `set -eu`, no retries, read-only.
- **D-17: Test in `tests/policy/tooling_cases.cmake`, small.** One or two ctest names driving a case loop (about six
  cases), not 13 registrations (each name adds two inventory lines and runs on every lane, including Windows and wasm32).
  A throwaway repo from `git init -b main`, a copy of the script, a fake `gh` first on `PATH` (the `policy.pins` fake-`curl`
  pattern; `policy.library_dirty.*` is the dirty-tree pattern to reuse), the shim returns already-filtered text because
  `--jq` runs inside `gh` (the filters are therefore untested offline: keep them trivial and run the script once live on
  clean `main`, recording the command and exit status per rule 11). Cases: baseline all-ok exit 0; a dirty-tree control
  (tracked edit and an untracked file) exit 1 with a `tree` row `off`; one negative control per remaining row family
  (open pull request, red CI, tag differs from manifest or a draft, pending release pull request); `unknown` exit 2
  (an in-progress run or a failing `gh`). No `jq` dependency: the test runs on every lane.
- **D-18: Docs and CI.** `docs/maintaining.md` "Closing a phase" gets a short "Closing a milestone" paragraph: run
  `sh scripts/milestone-close.sh` from `main` after the close merges and `git pull`; it is a check to run, not a release
  gate; an `off` or `unknown` row is something to look at and re-run (for instance while a release pull request is in
  flight); the existing open-`autorelease: pending` and 19-asset checks in that section are referenced, not
  duplicated. It is not run in CI (it reads live remote state and would turn a scheduled job into a flaky gate; rule 8).

### Claude's Discretion

- Exact helper names, awk rewrites and the numbering of renumbered fields in `library.sh`.
- Whether the milestone-close script reuses a function from `compat-common.sh` or stays self-contained (prefer self-contained
  at about 45 lines: another copy beats another dependency).
- Plan and wave split, within one pull request and with `ci.yml` last.
- Whether to count the asset total in the script or only point at the existing doc check.

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Project rules and state
- `AGENTS.md` (loaded as `CLAUDE.md`) — rules 3, 6, 7, 8, 9, 10, 11, 12 and the Style section
- `.planning/ROADMAP.md` "Phase 17" — goal, five success criteria, the note on the prune and the owner run
- `.planning/REQUIREMENTS.md` — RUN-03, RUN-04, TUNE-15, TUNE-16, TUNE-17
- `.planning/STATE.md`, `.planning/PROJECT.md` — current state and key decisions
- `.planning/LESSONS.md` — XFER-012 (measured timeouts), XFER-028 (piped `$?`), XFER-039 (drafts by id with `gh api`), XFER-054 (evidence from the command, from the repository root)
- `.planning/preparation/DECISIONS.md`, `.planning/preparation/ENGINEERING.md` §4 (timeouts) — settled choices
- `.planning/research/NEXT-MILESTONE.md` — the fan-out that scoped v2.2

### Phase 16 and Phase 15 precedent
- `.planning/phases/16-graphics-i-and-ii-draw/16-CONTEXT.md`, `16-VERIFICATION.md` — what still faults (text, multicolour, mixed, invalid, native Game Gear)
- `.planning/milestones/v2.1-MILESTONE-AUDIT.md` and the Phase 15 plans, summaries and verification under `.planning/milestones/` — the RUN-01 pattern and the pre-squash stamp finding

### Code and tests touched
- `scripts/library.sh`, `scripts/library.cmake`, `scripts/compat-common.sh`, `scripts/compat-update.sh`, `scripts/compat-ratchet.sh` — judge, stamp, ratchet
- `runner/diag.c`, `runner/diag_class.h`, `runner/args.c`, `runner/main.c` — the diagnose line and fault names
- `tests/runner/diagnose.cmake`, `tests/runner/test_runner.c`, `tests/library/diagnose.cmake`, `tests/library/tests.cmake`, `tests/hygiene/tests.cmake` — tests to update or re-time
- `tests/policy/tooling_cases.cmake`, `tests/policy/repository-checks.txt`, `tests/cmake/budgets.cmake`, `tests/inventory/ci.txt`, `tests/inventory/wasm32.txt` — where the new script's classification and test go
- `.github/workflows/ci.yml` (never `nightly.yml` or `digest.yml`), `scripts/ci-durations.sh`, `tests/cmake/repository.cmake` (`-DCHECK=timeouts`) — timeout derivation and check
- `include/cogmaster.h`, `src/internal.h`, `tests/api/v2.c`, `docs/embedding.md` — the fault constants that stay

### Docs
- `docs/runner.md` (diagnose line, near its byte cap), `docs/conformance.md` (stamp workaround, evidence classes), `docs/compatibility.md` (generated page, never hand-edited), `docs/maintaining.md` ("Closing a phase")

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `scripts/apply-repo-settings.sh --check`, `scripts/ci-durations.sh`: house style for a `gh`-based `operation` script (`set -eu`, one line per problem, exit 1 on drift).
- `tests/policy/tooling_cases.cmake`: throwaway git repos, a fake tool first on `PATH`, `expect_fail.cmake`; the `policy.library_dirty.*` cases are the dirty-tree control to reuse.
- `scripts/library.sh` dirty-tree guard (Phase 16): refuses a `run` against the tracked ledger from a tree with tracked changes outside `.planning`, which a detached clean worktree satisfies.

### Established Patterns
- Evidence is a command, an exit status and a run URL, never a committed log; machine-written ledger and page are carried, never edited.
- The owner's library is read only through counts-only scripts the owner runs or exports for; no agent reads it.
- Timeout comments are machine-checked; values are twice the slowest recent run with run IDs.

### Integration Points
- `library.sh` awk positional fields ($9 token, NF 32, $24 to $32) are the one tightly coupled consumer of the diagnose line.
- `tests/inventory/ci.txt` and `wasm32.txt` list every ctest name; a new test name adds lines to both.

</code_context>

<specifics>
## Specific Ideas

- The owner asked (standing instruction) for the fan-out, adversarial pass and one coherent recommendation set, preferring
  another copy and paste to another dependency and noting CI speed wins. CI notes for later seeds, not this phase:
  `runner.diagnose` under qemu is 313 to 450 s of the be32 job's 422 to 609 s and also runs on eight other lanes
  (check whether the be32 hashes depend on it); `library.synthetic` (97 s of 160 s) and `compat.ratchet` (78 s of 142 s)
  sit near their limits; `build` has exactly 2.0x and `wasm32` a thin margin.
- Release run 43610370563 finished `success` (all jobs including `publish` and `install-line-latest`); v0.15.0 is not a draft and has 19 assets.

</specifics>

<deferred>
## Deferred Ideas

- Remove the `mode4` word from the `vdpmode` vocabulary: it is a documented canary, not dead by accident; leave it.
- Drop `COGMASTER_FAULT_KIND_ROM_WRITE` from the public header: a public API change, out of scope; the constant stays pinned by `tests/api/v2.c`.
- Run `milestone-close.sh` in CI or on a schedule: it would make a live-state check a flaky gate (rule 8).
- Re-derive `library.synthetic` and `compat.ratchet` limits, and look at running `runner.diagnose` under qemu only where its hashes matter: note in SEED-015.

</deferred>

---

*Phase: 17-Re-run, merge and clean state*
*Context gathered: 2026-11-14*
