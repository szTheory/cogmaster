# Phase 5: Tune-up - Context

**Gathered:** 2026-11-10
**Status:** Ready for planning (adversarial pass applied: 7 blocking items fixed, 5 strikes made)

<domain>
## Phase Boundary

Merged changes that clear the v1.0 debt before the bus and I/O paths gain branches: measured CI timeouts, the perf gate armed, `quality.json`, docs and hosted-test headroom, the README ROM step, review dispositions, the weekly digest, the nightly issue path, Dependabot, and wasm32 promoted. No player-visible emulator feature. Requirements TUNE-01 to TUNE-10.

Discuss method: five role-lens advisors (DevOps/SRE, performance, build/docs, supply chain, QA/writing) plus an adversarial pass, per the owner's fan-out boilerplate (memory: discuss-fanout-and-ci-efficiency). Recommendations below are followed.
</domain>

<decisions>
## Implementation Decisions

### CI timeouts, efficiency, wasm32 (TUNE-01, TUNE-10, TUNE-03 CI part)
- Add `scripts/ci-durations.sh` (POSIX sh, `gh` and `jq`, read-only). It lists successful runs per workflow, strips matrix suffixes, and prints job, N, slowest seconds, run ID and the YAML comment text `2 x S s, slowest of N runs, run ID`. Matrix lanes fold into one timeout (worst lane named). The weekly digest reuses it for its slowest-job line.
- Amend TUNE-01 and ROADMAP Phase 5 criterion 1 wording to "slowest of the last 40 successful runs, or all of them if fewer, with N stated" (jobs such as rehearsal and release have fewer than 40).
- Apply 2x slowest with the minute counts taken from `ci-durations.sh` output, not from this document (several jobs have grown or were sized from a cancelled run). Keep a 5 min floor for network-bound jobs. A new workflow with no runs yet (`digest.yml`, `dependabot-automerge.yml`) uses a floor-only comment form, `5-minute floor; no runs yet`, accepted by the check when `minutes >= 5`; the first-run URL is recorded at ship.
- Tighten the `tests/cmake/repository.sh` timeouts check: reject any comment containing `provisional`; require `slowest of N runs, run <id>`; assert `minutes*60 >= 2*S` parsed from the comment. Add a `provisional` negative case to `hygiene.timeout_comments.self_test`. Delete the docs sentence "stays provisional until ten".
- Per-test ctest `provisional` Timeout comments in `tests/*/tests.cmake` are not CI jobs: seed them, do not widen TUNE-01.
- The agent confirms each cited run ID once with `gh api` and records command and exit status; CI itself never calls the network for this.
- wasm32 promotion (TUNE-10): the criterion is met (12 successful main push runs log hashes equal to Linux x64). Do the fold below as one commit together with the `ci_required` test change. Fold wasm32 into `hash-equality` (needs, COUNT 8, drop the `rm -rf hashes/hashes-wasm32` line), add it to `CI required` needs, remove `REPORT_ONLY=wasm32` from `tests/hygiene/tests.cmake` and the report-only wording in docs.
- Speed: try dropping the `g++-powerpc-linux-gnu` apt install and the CXX compiler from be32 (project is C only); keep only if configure passes and step time falls. No caching, no ccache, no concurrency changes.
- Dependabot interaction: a Dependabot Actions PR edits `ci.yml`, which sets `release_inputs=true`. The App key is already a Dependabot secret, but a Dependabot `pull_request` token is read-only, so the rehearsal cannot get its attestation and write permissions. Skip the rehearsal for the Dependabot actor AND add an actor clause to the `ci-required` jq (which today accepts a skipped rehearsal only when `release_inputs != true`, the event is not a pull request, or the run is nested), with a negative control in `ci_required.cmake` and the hygiene tests. Count those lines in the check budget.

### Perf gate (TUNE-02)
- Arming is a one-word change (`COGMASTER_PERF_MODE: gate`). Dispatch `gh workflow run ci.yml --ref main` twice on current main first; record the run URLs and the `perf control boot_to_play runs=a,b,c spread=0` lines in the PR body and SUMMARY. Fix the todo and docs wording that says `testcard` (the job prints `boot_to_play`) and the "report-only" line in `docs/maintaining.md`.
- Delete the `boot_to_play` allow line (357, 2027-01-07): it priced sound against a pre-sound base; every PR now bases on main with sound. `allow.txt` keeps comments only.
- The gate compares head against the PR's merge base measured twice (UNSTABLE guards repeatability); there is no stored baseline. Limit stays +1.00 %.
- Prove the real path once with a throwaway draft PR carrying a volatile busy loop: confirm `perf` and `CI required` fail with `verdict=FAIL`, record the run URL, close unmerged, delete the branch. Not recurring ceremony.
- Phases 6 to 9: a PR that adds more than 1 % on a hot path carries its own `allow.txt` line (percentage, expiry, reason naming the feature), deleted in the next phase's first commit.
- Order: take the two control URLs first, arm the gate in the LAST plan of the phase, then run the draft-PR proof against the armed gate (stack it on the phase branch if the arming commit is not yet on main). Perf job timeout comes from the TUNE-01 script.
- No absolute `ir_per_frame` ceiling in `quality.json` (the relative gate ratchets; seven phases at under 1 % is bounded). Revisit at the next tune-up.

### quality.json, lessons_unique, budgets (TUNE-04, TUNE-05)
- One flat `quality.json`: `ceilings{}`, `floors{}`, `reasons[{key,reason}]`, read by `tests/cmake/quality.cmake` with `string(JSON ...)`. Each invocation names its keys; a missing or unmeasured named key fails.
- Keys: `tests.ci` and `tests.wasm32` floors from `tests/inventory/*.txt` line counts; `size.zip.library|libretro|runner` ceilings at the max over all legs of the last main artifacts plus about 15 %; `size.wasm`; `docs.max_bytes` 38,912, read by `tests/cmake/budgets.cmake` in place of its hard-coded 40960 so there is one cap. Omit warnings, coverage and `size.cartdb` (Phase 6 adds it with its reason).
- Enforcement points: `tests.*` and `docs.max_bytes` run in the `ci` preset; zip sizes run in the per-leg `build` job after packaging (`ci.yml` build job); wasm size runs in the wasm32 lane. TUNE-04's success criterion is met by the `ci` preset covering the tests and docs keys plus these two job steps, each with its negative control; state this split in the plan and the phase summary rather than pretending the `ci` preset has zips.
- Ratchet is hand-tightened at tune-ups. A base-file compare (`-DBASE=`) reuses the trusted base copy in `ci.yml` (extend it to include `quality.json`); an absent base file passes (first PR), with a negative control for a present base; a ceiling rise or floor fall requires a `reasons` entry.
- Negative controls through `expect_fail.cmake`: ceiling minus 1, floor plus 1, missing key, malformed JSON, loosening with no reason; each exits 1 and names the key.
- `policy.lessons_unique`: an awk `CHECK` in `tests/cmake/repository.sh` over `^| XFER-N |` rows and `Lesson CM-/XFER-` headings; fails on a duplicate and on zero IDs found; fixture control with a duplicate.- Docs split: `docs/testing.md` loses "Hosted RetroArch" and "Content rendering and input" to `docs/hosted.md`, and "Performance" and "Fuzzing" to `docs/perf-fuzz.md`; `docs/embedding.md` loses "Battery media" to `docs/saves.md` and "Probing an image" to `docs/boards.md` (Phases 6 and 7 extend them). Fix `audio.md`, `conformance.md` and `nightly.yml` references by grep. Keep `docs.api_names` green by letting `DOC` take a list of files (2 lines plus self-test). No planning IDs in docs. Ratchet and digest prose goes in `docs/maintaining.md`.
- `tests/retroarch` (299 of 300): reclaim at least 25 lines in `bmp.c`, `play.h`, `compare.c` (merge helpers, `#pragma once`), verifying the BMP orientation first; `policy.e2e_budget` proves the count. Phase 7's round trip needs only another `run_retroarch.cmake` call and a `cmp`.
- Check-code budget: 528 of 744 now; this phase adds about 120 lines (quality 50, lessons 8, dependabot 10, ci_required actor clause and its tests, README check 40, digest formatter 60 to 80), leaving little. Classify `ci-durations.sh` and the digest formatter as `operation` in `tests/policy/repository-checks.txt` only because they never fail on repository content; `quality.cmake`, the lessons check and the README check are `check`. Do not reclassify to dodge the one-tenth rule.

### Digest, nightly, Dependabot (TUNE-03, TUNE-08, TUNE-09)
- `digest.yml`: Monday cron plus `workflow_dispatch` with `dry_run`. Workflow-level `contents: read`; the job adds `actions: read`, `pull-requests: read`, `issues: write`; no App token. It rewrites one open `digest` issue body and also writes `$GITHUB_STEP_SUMMARY`; a comment is added only for attention items (open nightly issue, red or idle PR, job near its timeout margin). Green weeks stay silent. Flaky means `run_attempt > 1` that succeeded, naming the failed job. Write `gh api` output to a file before `jq` (SIGPIPE trap). Keep it out of `CI required`.
- Testability: the formatting script reads JSON fixtures on stdin; a CTest asserts the slowest job and the flaky job; `policy.digest` checks permissions and cron; the agent does one `gh workflow run digest.yml -f dry_run=true` and records the URL.
- Nightly: dispatch `nightly.yml --ref main -f force_failure=true`, confirm the `nightly` issue opens, wait for the run to finish, then a plain dispatch and confirm it closes. Sequential (no concurrency group). "Sound code" is met by existing green scheduled runs (cite one URL).
- Dependabot: `dependabot.yml` github-actions only, weekly, one group minor+patch, majors separate PRs, `commit-message.prefix: ci`, `cooldown.default-days: 7`. `dependabot-automerge.yml`: `on: pull_request` (never `pull_request_target`), no checkout, job guard `user.login == 'dependabot[bot]'`, App token with only `pull-requests: write` and `contents: write`, no new third-party action: decide major versus minor/patch by parsing `gh pr view --json title` (the Dependabot title carries `from X to Y`), `gh pr merge --auto --squash` only when not semver-major. Add `workflows: write` only if the first merge needs it. Owner action: none (secrets and variable already exist).
- `policy.dependabot` modelled on `policy.action_pins`: asserts ecosystem, weekly, group types, prefix, cooldown, no major in group, the actor guard, and no `pull_request_target`, `workflow_run` or checkout in the automerge file; negative control fixtures fail with named messages.

### README and review triage (TUNE-06, TUNE-07)
- README: replace "Quick start: RetroArch" with "Play a game", leading with "cogmaster ships no BIOS and no game; bring a `.sms` or `.gg` file you own, no BIOS is needed." RetroArch steps Load Core then Load Content; Start Core alone shows the test card. Runner: `bin/cogmaster-run --frames 600 --hash game.sms`, then `--png 600:shot.png`; exit 77 means unsupported (link to the runner doc). A `-P` check (about 40 lines, like `install_line.cmake`) runs the README's first runner command against a manifest fixture, compares to the README text block, requires the README ROM extensions to be a subset of `supported_extensions`, requires the no-BIOS sentence once, with a negative control.
- Dispositions (edit the three `NN-REVIEW-DISPOSITION.md` files in place; verify with one grep that none is `open`, every `deferred` names a SEED, every `skipped` has a reason; record command and exit status in the phase summary):
  - Fix now: P1 WR-02 (listing-failed sentinel in `classify-changes.sh`, flip the hygiene test), P1 IN-01 (clock figure 315/88 MHz x 3), IN-02, IN-03, IN-04 (`grep -qxF --` in `publish.sh`), P2 IN-03 (refuse a body over 1 MiB in `src/cart.c` with a test), P4 IN-01 (comment: PSG steps every 48 ticks).
  - Closed with reason: P1 WR-01, WR-03, IN-05, IN-06, IN-08; P2 IN-04; P4 IN-02, IN-03.
  - Seeds: P1 WR-04 (accepted risk; a reviewer-gated environment would break rules 6 and 8) into SEED-007; P1 IN-07, P2 IN-02, P2 IN-06, P4 IN-04 into SEED-002; P2 IN-01 into SEED-004; P2 IN-05 rides the Phase 6 plan (touches `probe.c`).
  - Do not reopen P2 IN-01, IN-04 or P4 IN-04 here. The audit's "IN-01..04" wording means each phase's own IDs.

### Claude's Discretion
Exact script and file names beyond those listed, test names, ordering of plans, and wording of docs, within the budgets above.
</decisions>

<code_context>
## Existing Code Insights

### Reusable Assets
- `tests/cmake/repository.cmake`, `expect_fail.cmake`, `policy.action_pins` (pattern for new policy checks); `tests/cmake/perf_compare.cmake` and the `perf` job; `scripts/apply-repo-settings.sh`; `install_line.cmake`; `tests/inventory/*.txt`.

### Established Patterns
- Every check ships a negative control that fails with a named message; POSIX sh or CMake `-P` for repository checks; timeouts carry a YAML comment with the run ID; evidence is a command, exit status and run URL.

### Integration Points
- `.github/workflows/ci.yml`, `nightly.yml`, new `digest.yml` and `dependabot-automerge.yml`, `.github/dependabot.yml`, `tests/hygiene/tests.cmake`, `tests/cmake/budgets.cmake`, `docs/*.md`, `README.md`, `.info.in`.
</code_context>

<specifics>
## Specific Ideas

Owner standing preferences: copy-paste over new dependencies; flat dependency tree; look for CI speed wins whenever a run happens; automate verification, no human UAT.
</specifics>

<deferred>
## Deferred Ideas

- Per-test ctest `provisional` Timeout comments (seed).
- Absolute `ir_per_frame` ceiling in `quality.json` and a coverage lane (next tune-up).
- ccache or build caching (no measured payoff).
- P1 IN-07, P2 IN-01, IN-02, IN-05, IN-06, P4 IN-04 per the seed list above.
</deferred>
