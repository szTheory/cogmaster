# Phase 15: Re-run, merge and timeouts - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md — this log preserves the alternatives considered.

**Date:** 2026-11-13
**Phase:** 15-re-run-merge-and-timeouts
**Areas discussed:** the library re-run and merge; the page and exit command; CI timeout re-derivation; sequencing, shipping and the owner fallback

The owner's fan-out boilerplate asks for one coherent set of recommendations that is followed, so no area was put to a vote. Three advisor agents researched one lens each (ledger and ratchet; CI economics; release engineering and hand-off); the orchestrator reconciled them. Runtime: Claude Code, model Sonnet 5.5.

---

## The library re-run and merge

| Option | Description | Selected |
|--------|-------------|----------|
| Owner runs the no-argument `library.sh` (rebuilds at HEAD) | The workflow rebuilds the runner, then `run` merges; evidence is the `merge:` line | ✓ |
| Owner runs `library.sh run` only | `run` does not rebuild; a stale binary would be stamped with the HEAD sha | |
| Owner runs `diagnose` first, then `run` | Extra run; `diagnose` covers only the first NTSC profile of each cart | |
| Cherry-pick flipped rows or filter PAL/FM | New tooling; the merge is all-or-nothing by design | |

**User's choice:** first option (standing boilerplate, no vote held).
**Notes:** allow-listing is impossible here because main is already at behaviour revision 18. A refused row is a core bug for a new branch. Exit status 0 does not prove a merge (`held (page over the docs cap)` exits 0).

## The page and the exit command

| Option | Description | Selected |
|--------|-------------|----------|
| Regenerate-and-compare plus `compat.exit`, recorded once | Existing CI tests already enforce it on the pull request | ✓ |
| New agreement gate | Nothing new would catch a fault the existing tests miss | |

**Notes:** `quality.json` floors stay; the page is the only file with counts.

## CI timeout re-derivation

| Option | Description | Selected |
|--------|-------------|----------|
| Re-read `ci-durations` just before the edit, twice the slowest, no dispatch | 37 to 40 runs behind each job already | ✓ |
| Dispatch nightly and ci.yml on the branch for fresh samples | Adds macOS overlap and cost; every edit loosens a limit | |
| Trim outliers or add a ceiling | Rule is settled; a floor already exists for network jobs | |

**Notes:** conflict between advisors on asan (7 versus 9 minutes) came from different slowest runs (194 s versus a later 243 s); the rule decides at execution time. be32 (21) and build (12) violate the rule more than the named five and ride in the same ci.yml edit, flagged as a note. Per-test ctest timeouts stay out.

## Sequencing, shipping and the owner fallback

| Option | Description | Selected |
|--------|-------------|----------|
| Owner run first, one ledger commit, one timeouts commit last, one push, PR opened once non-draft | Zero CI before the PR; one rehearsal | ✓ |
| Draft PR early (phase 11 pattern) | A draft runs CI on every push and cannot auto-merge | |
| Early push of nightly and digest edits, ci.yml last | Without a PR a push costs nothing, so there is no gain | |

**Notes:** conflict on the owner-absent fallback. One advisor proposed landing a `ci:` timeouts-only pull request; rejected because it releases nothing and cannot end the phase. Decision: prepare locally, do not push, record RUN-01 and RUN-02 pending. Title is `fix(compat): ...` (patch); `feat` would be a minor release and is the owner's override.

## Claude's Discretion

The awk transition-count placement, plan boundaries beyond two plans, the maintaining paragraph wording, rounding at the margin after a re-read.

## Deferred Ideas

Per-test timeouts, a cheaper be32 diagnose fixture, a PAL and non-pad pre-flight, the `held`-exit-0 and pre-merge `exit:` items, the bank-shift trace, the nightly freshness step.
