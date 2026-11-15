# Phase 17: Re-run, merge and clean state - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md — this log preserves the alternatives considered.

**Date:** 2026-11-14
**Phase:** 17-re-run-merge-and-clean-state
**Areas discussed:** Re-run sequencing, Diagnose prune, Timeouts, Milestone-close check

The owner's standing instruction for discuss steps (fan out subagents over every decision, all role lenses, primary
sources, an adversarial pass, then one coherent set of one-shot recommendations that is followed) replaced the
per-area selection questions. Four research agents ran in parallel and a fifth reviewed three of the answers
adversarially. No question went to the owner.

---

## Re-run sequencing (RUN-03, RUN-04)

| Option | Description | Selected |
|--------|-------------|----------|
| A. Two pull requests | Prune, timeouts and CI first as a code pull request; merge; judge on the new `main` in a ledger pull request | Fallback only (used if the ratchet refuses) |
| B. Judge first, prune after | Ledger pull request on current `main`, then a prune pull request | |
| C. One pull request, judge in a worktree | Judge a detached worktree of `origin/main` (stamp is an ancestor), carry the diff to the phase branch, prune on the branch, re-run only `diagnose` on the phase head | ✓ |
| D. Judge on the phase branch | Stamp is a pre-squash commit, the Phase 15 audit finding | |

**Choice:** C. **Adversarial changes taken:** one owner session for both commands after the prune lands (not two);
`unset COGMASTER_LIBRARY_OUT`; remove the worktree afterwards (it holds the owner's library path); record
`git diff --quiet 0a97075 HEAD -- src include CMakeLists.txt cmake`; the judge never calls the diagnose code, so verdicts
cannot depend on the prune. Ancestry stays unenforced by code (the recorded `is-ancestor` command is the gate).

---

## Diagnose prune (TUNE-15)

| Option | Description | Selected |
|--------|-------------|----------|
| Remove `wclass`, `pclass`, `where=port`, `dev=ctrl`, `fault=rom_write`; keep `vdpmode`, `faddr`; bump token to `diagnose 2` | Verified dead at every raise site; 32 to 30 fields | ✓ |
| Keep always-`none` fields for compatibility | No committed consumer exists; dead weight | |
| Also remove the `mode4` word and the public ROM_WRITE constant | A canary and a public API change; out of scope | |

**Choice:** first row. The name tables keep their `rom_write` and `ctrl` entries (a NULL would crash `printf`).

---

## Timeouts (TUNE-16)

| Option | Description | Selected |
|--------|-------------|----------|
| One last commit with `ci.yml` and CTest values, raise-only | be32 job holds at 21 minutes; `library.diagnose` 206 to 289 s; `hygiene.tree` 60 to 75 s; rehearsal 13 to 14 minutes; comments refreshed | ✓ |
| Also lower perf from 6 to 4 minutes | Mechanical result of the rule, saves nothing | |
| Sample from the phase pull request's own runs | Circular; a run is not a sample of its own limits | |

**Choice:** first row; sample the green run on the previous head.

---

## Milestone-close check (TUNE-17)

| Option | Description | Selected |
|--------|-------------|----------|
| About 45 lines of POSIX sh, class `operation`, six rows, fake-`gh` test with a small case loop | Exit 0, 1, 2; not run in CI | ✓ |
| Thirteen separately registered ctest names | Doubles inventory lines and runs on every lane | |
| Run in CI or on a schedule | A flaky live-state gate (rule 8) | |

**Choice:** first row, with the exact `CI required` filter, the manifest read from `origin/main`, and a row for releasable
commits since the latest tag.

---

## Claude's Discretion

Helper names and awk rewrites; whether the script reuses a `compat-common.sh` function; the plan and wave split; whether
the script counts release assets.

## Deferred Ideas

Removing `mode4`; dropping the public ROM_WRITE constant; running the check in CI; re-deriving `library.synthetic` and
`compat.ratchet` limits and trimming `runner.diagnose` under qemu (SEED-015 notes).
