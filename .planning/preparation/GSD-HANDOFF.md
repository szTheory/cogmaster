---
title: "GSD hand-off: kickoff, per-phase routine, settings and owner tasks"
summary: "How to start and run cogmaster with OpenGSD one step at a time: the kickoff text, the settings and why, the per-phase and milestone routines, the owner's tasks, and what was checked before hand-off."
read_when: "Before the kickoff, before each GSD step, and whenever a GSD step behaves unexpectedly."
updated: 2026-10-18
---

# GSD hand-off

| Question | Section |
|---|---|
| What do I type to start? | 2 |
| What do I do each phase? | 3 |
| What happens at a milestone boundary? | 4 |
| Why is each setting what it is? | 5 |
| What stops GSD from running on by itself? | 6 |
| What only the owner can do | 7 |
| What was verified before hand-off | 8 |

## 1. Why this file exists

cogmaster is built with OpenGSD (`@opengsd/gsd-core`). Preparation wrote the files GSD would otherwise collect by
asking questions (`PROJECT.md`, `REQUIREMENTS.md`, `config.json`, `research/SUMMARY.md` and `STACK.md`), so the
kickoff resumes at roadmap creation [GH.01]. GSD's own researcher still runs before every phase
(`workflow.research: true`), which is where it does its own research and sanity checks from here on.

## 2. Kickoff

**Before it** (about five minutes):
- The repository `szTheory/cogmaster` exists (public, created 2026-10-18) and `scripts/apply-repo-settings.sh
  --no-ruleset` has been applied; in any fresh clone run `git config core.hooksPath .githooks` first.
- `cat ~/.claude/gsd-core/VERSION` prints `1.17.0`, the version these files were checked against (section 8).
  If a stale cache reinstalled an older version, `npx -y @opengsd/gsd-core@1.17.0 --claude --global` restores it;
  1.10.0 has no resume path and errors "project already initialized" [GH.02]. Codex has GSD 1.16.0; to use Codex,
  reinstall 1.17.0 for Codex first and repeat the dry run (unverified there). Under Codex GSD writes its generated
  guide into `AGENTS.md` rather than `.claude/CLAUDE.md`; keep AGENTS.md marker-free.
- Do the owner tasks marked "kickoff" in section 7 (release App), so phase 1 never blocks halfway.
- Skim `.planning/PROJECT.md` and `.planning/REQUIREMENTS.md`; change anything that is off now, because the
  roadmap is built from them.

**Then**, in a new Claude Code session at the repository root, paste this as one message:

```
/gsd-new-project Resume the pre-seeded bootstrap. PROJECT.md, REQUIREMENTS.md, config.json and research/SUMMARY.md were written during preparation and are final for initialization: keep them as they are and continue from roadmap creation. Do not run questioning, workflow preferences, project research or requirements scoping. Structure mode is Horizontal Layers (standard template, no Mode lines). Use the "## v1.0 …" milestone heading from research/SUMMARY.md, copy each suggested phase's "Canonical refs" line into ROADMAP.md, and add "UI hint: no" to every phase. Stop after the roadmap is approved and committed.
```

Following `AGENTS.md`, the agent first creates the branch `gsd/roadmap` off `origin/main`. Expect two prompts:
**Structure** → "Horizontal Layers"; **Roadmap** → approve if the checklist holds, else "Adjust phases" and say
what to change. GSD then commits `ROADMAP.md`, `STATE.md` and the updated `REQUIREMENTS.md` and stops; the agent
pushes the branch and merges it by pull request (before phase 1 adds CI, `gh pr merge --squash` merges at once).

**Roadmap checklist:** heading `## v1.0 …`; four phases, each ending in an automated release; phase 1 contains
shipped C code, CI and the first release; no phase whose goal is a study, decision or experiment; every phase has
"Canonical refs" and "UI hint: no" and no `**Mode:**` line; every v1 requirement (26) is mapped exactly once.

**If GSD asks its full set of questions instead:** "What do you want to build?" → "Use .planning/PROJECT.md as
written"; saved defaults found → "Configure fresh" (never "Use as-is": the owner's global defaults are yolo mode
with web checks); mode/granularity/execution/git → Interactive, Coarse, Sequential, Yes; research/plan check/
verifier → Yes; compact content, AI models, PR body sections → No, Inherit, none; research the domain first →
Skip; feature scoping → "Use the existing REQUIREMENTS.md unchanged"; structure → Horizontal Layers. GSD's config
writer keeps an existing `config.json`, so these answers matter only if that file is missing [GH.01].

**After the kickoff**, these print the values shown:
```
node ~/.claude/gsd-core/bin/gsd-tools.cjs query config-get mode --raw                    # interactive
node ~/.claude/gsd-core/bin/gsd-tools.cjs query config-get workflow.auto_advance --raw   # false
node ~/.claude/gsd-core/bin/gsd-tools.cjs query config-get workflow.test_command --raw   # cmake --workflow --preset ci
```

## 3. Each phase: one session per step, so the model can change between steps

1. The agent branches `phase/NN-slug` off `origin/main` (AGENTS rule 10; the pre-commit hook refuses commits on
   `main` once a remote exists) and records the runtime and model in STATE.md.
2. `/gsd-discuss-phase N`. The agent first re-reads `.planning/LESSONS.md` (AGENTS preamble). The owner's global
   GSD profile turns on advisor mode, so discuss also spawns advisor researchers; each advisor gets the owner's
   fan-out brief (`.planning/preparation/BRIEF.md`, "Fan-out method"), ending in one coherent recommendation per
   point that the step then follows without asking again.
3. `/gsd-plan-phase N`. Plans that do not share a test inventory are placed in parallel waves (LESSONS XFER-066).
4. `/gsd-execute-phase N`. Code review, the test gate (`workflow.test_command`), the verifier and the phase close
   all run inside this step [GH.01]; code review runs before verification (AGENTS rule 7).
5. Read `.planning/phases/NN-*/NN-VERIFICATION.md`:
   - `passed` → step 6;
   - `human_needed` → `/gsd-verify-work N`, which closes every item with code, configuration, a test, CI, hosted
     evidence or the agent's own reading, viewing and running, and hands the owner only what AGENTS rule 6 leaves
     to them;
   - `gaps_found` → run one fresh verifier first (LESSONS XFER-053), then one gap round (`/gsd-plan-phase N` for
     the gaps, then execute); a second only with new evidence.
6. `/gsd-ship N` opens the pull request titled "Phase N: name". The agent retitles it as a Conventional Commit
   (`gh pr edit --title "feat: …"`; `fix:`, or `ci:` for CI-only work so no release is cut), runs
   `gh pr merge --auto --squash` in a shell (never inside GSD command text, where GSD would read the option as
   its own auto flag), re-runs the title check if it ran before the retitle (`gh run rerun --failed`), and
   confirms that a CI run exists on the head commit (ship pushes a CI-skip commit; the repository's squash
   settings keep that token off `main`).
7. CI goes green → the pull request merges → release-please opens its release pull request with the release App's
   token → it merges when CI passes → the release publishes itself with attested archives.
8. The agent checks that the release pull request is green (a red release PR silently stops all releases) and,
   after phase 1, opens the one budgeted `ci:` follow-up PR for the post-merge tail (arming the perf gate,
   promoting the wasm32 lane) [LESSONS XFER-065].
9. Optional, about two minutes, outside the agent's sandbox: run the README's install line and play one game in
   RetroArch. This is never recorded as a verification item.

**Verification default (settled 2026-10-18; never re-proposed):** automation first. Deterministic behaviour goes
in local tests; integration, end-to-end, smoke and seam checks go in CI when they catch recurring regressions; a
one-time hosted outcome is collected by a read-only evidence command. Passing results are recorded as command,
exit status and run URL. The owner is asked only for identity-bound consent or a run on their own hardware, with
the reason stated. Judging docs, images and RetroArch's menus is the agent's job, and a truth only a merge can show
is confirmed at ship (step 8), never in UAT.

**Closeout policy:** a fix merged after a phase's verification makes that verification "stale". Accept an
override closeout at milestone end when CI passes on the merged head and the milestone audit covers the phase;
do not spend sessions refreshing digests.

**Cross-project operating lessons** (LESSONS XFER-016, 017, 055): prove a playability claim in one integrated host
session; diagnose a stalled guest at its first fault with the runner's trace; close verification gaps in one
current-revision batch; keep acceptance blockers separate from engineering dependencies.

## 4. Milestone boundaries

On a `milestone/vX.Y-slug` branch, one session each: `/gsd-audit-milestone` → `/gsd-complete-milestone` →
`/gsd-new-milestone`. Within those steps the agent also:
- refreshes `.planning/PRODUCT-ROADMAP.md` (reconcile shipped work, keep unresolved debt, re-rank
  Immediate/Near/Mid/Long, record why) and snapshots it as `milestones/vX.Y-PRODUCT-ROADMAP.md`;
- refreshes `.planning/preparation/PROMPT-COVERAGE.md` (every owner ask still has a home and a status);
- surfaces the seeds whose triggers fired; the tune-up seed is the first phase of every new milestone;
- offers a lesson export ("export lessons", `.planning/LESSONS.md`).

## 5. Settings and why

`config.json` was written by GSD's own writer (`query config-new-project`) with `GSD_HOME` pointing at an empty
directory, so none of the owner's global defaults leaked in, and read back with `config-get` (section 8).

| Setting | Value | Why |
|---|---|---|
| `mode` | interactive | With yolo, the end of one step can start the next [GH.01] |
| `workflow.auto_advance`, `workflow._auto_chain_active` | false | Stops discuss, plan and execute from chaining |
| `granularity` | coarse | About four phases per milestone |
| `parallelization`, `workflow.use_worktrees` | false | A small, coupled codebase; no worktree paths in tracked files |
| `model_profile`, `resolve_model_ids` | inherit, omit | Every GSD subagent uses the session's model; no model IDs in a public file |
| `workflow.research`, `plan_check`, `verifier`, `code_review` | true | GSD's own research and checks before and after every phase |
| `workflow.ui_phase`, `ui_safety_gate`, `ui_review`, `ai_integration_phase`, `api_coverage_gate`, `schema_push_detection`, `schema_drift_gate` | false | Written for web projects; they react to words like "screen" |
| `workflow.security_enforcement` | false | A web threat-model checklist; fuzz targets and sanitizer builds are requirements instead |
| `workflow.nyquist_validation`, `pattern_mapper`, `tdd_mode` | false | GSD's own choice at coarse granularity; no code to map before phase 1 |
| `workflow.test_command` | `cmake --workflow --preset ci` | Without it the test gate runs `true`. It fails until phase 1 creates the preset |
| `workflow.test_gate_timeout` | 900 | Phase 1 replaces it with twice the measured gate time |
| `git.branching_strategy`, `git.create_tag` | none, false | The agent branches before discuss, so planning and code share one pull request; release-please owns tags |
| `git.base_branch`, `git.protected_branches` | main, [main] | Ship targets `main`; execute and ship warn if run on it |
| `hooks.community` | true | Turns on GSD's Conventional Commit check |

Subagent reasoning effort comes from the owner's global GSD agent install, not from this repository.

## 6. What keeps GSD from running on by itself

- GSD commands get only a phase number. An argument containing `--auto` or `--chain` makes one step start the
  next, and GSD tests for the substring [GH.01].
- `/gsd-next`, `/gsd-autonomous`, `/gsd-manager` and `/gsd-progress --next` or `--do` start the following step.
- In interactive mode the phase transition only moves STATE's pointer; it does not start the next phase [GH.01].
- `scripts/hygiene.sh` refuses a commit if `.planning/config.json` leaves interactive mode or turns a chain flag on.

## 7. Owner tasks

| When | Task |
|---|---|
| Done 2026-10-18 | The public repository `szTheory/cogmaster` exists; `scripts/apply-repo-settings.sh --no-ruleset` applied merge, security, Actions and label settings; the required-check ruleset is applied in phase 1 once CI reports (`scripts/apply-repo-settings.sh` without the flag) |
| **Kickoff** | GitHub App `cogmaster-release` (private, webhook off; repository permissions: Contents, Pull requests, Issues and Workflows read/write, Metadata read), installed on this repository only. muhtari's manifest-flow helper registers it in one command and pipes the private key straight into `gh secret set COGMASTER_APP_PRIVATE_KEY`, once for Actions and once with `--app dependabot`; the key is never shown or saved. `COGMASTER_APP_CLIENT_ID` is an Actions variable. Agents cannot read key files (GSD's secret-read guard), so these commands are yours. Details: ENGINEERING 13 |
| If the key must change | On the App's settings page generate a new private key, run `gh secret set COGMASTER_APP_PRIVATE_KEY < <key file>` and again with `--app dependabot`, then delete the old key there and the file |
| Done (present) | RetroArch 1.22.2 is installed in `/Applications`; the `retroarch-metal` cask is the Apple Silicon build the hosted job pins (HOSTS 3). The App Store build cannot load outside cores |
| Any time | Optional `.env.local` (copy `.env.example`): `COGMASTER_ROM_DIR` for the local-only `library` preset with your own cart dumps; `COGMASTER_BIOS_DIR` for `--boot bios` with your own BIOS dumps. Results stay on your machine |
| Each phase close | Optional two-minute smoke (section 3 step 9) |
| v3.0 or sooner | In Playstead, adding the `sms` and `gg` system IDs (they must equal the file extensions) and the RetroArch-with-core launch path are Playstead-side decisions (HOSTS 4) |
| Never in this repository | `/gsd-profile-user`: it writes a developer profile into the tracked `.claude/CLAUDE.md` |
| Before going public with docs | Read `PROVENANCE.md`, `ASSET_POLICY.md` and the README once; ad-hoc code signing only (a Developer ID certificate puts a legal name in artifacts) |

## 8. What was checked

All on 2026-10-18, against `@opengsd/gsd-core` 1.17.0. (Filled in by the preparation session's final checks; see
the "Verification" section of the session's plan.)
- **Config.** Written by GSD's own writer with `GSD_HOME` pointing at an empty directory; read back with
  `config-get` (mode interactive, coarse, inherit/omit, both chain flags false, the test command, branching none,
  create_tag false, community hooks on). No `preferences`, `effort` or model IDs reached the file. Diff against
  muhtari's config: only `test_gate_timeout` (900 vs its measured 3) and the absent `code_review_depth_overrides`.
- **Resume state, roadmapper dry runs, hygiene self-test, fact-check, fresh-eyes review, public pins:** recorded
  below when run.

## Sources

| ID | Tier | Source | Pin or access date | Supports |
|---|---|---|---|---|
| GH.01 | T1 | `@opengsd/gsd-core` installed package: `workflows/new-project.md` (resume path at L95), `execute-phase.md`, `ship.md`, `verify-work.md`, `transition.md`; `bin/lib/init.cjs` L1308, `config.cjs` L280–312 (`GSD_HOME` defaults merge), `roadmap-parser.cjs`, `profile-output.cjs` | 1.17.0, read 2026-10-18 | Resume path, chaining, branching, config, ship behaviour, milestone headings, instruction file |
| GH.02 | T1 | nesturbator `.planning/preparation/GSD-HANDOFF.md` (version-pin warning, ship-step detail, signing note) | `9f9386c661f6` | Routine details muhtari dropped |
| GH.03 | T1 | muhtari `.planning/preparation/GSD-HANDOFF.md`, `.planning/LESSONS.md` | `952fce970117` | Kickoff text, routine, settings table, owner tasks |
