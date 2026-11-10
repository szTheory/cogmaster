---
id: SEED-007
status: dormant
planted: 2026-10-18
planted_during: preparation, before milestone v1.0
trigger_when: every milestone after the first, as its first phase
scope: small
audit_acknowledged:
  milestone: v1.0
  at: 2026-11-09
  status: dormant
---

# SEED-007: Each milestone starts with a faster, more trustworthy pipeline and docs that match the release

## Why This Matters

CI time, flakiness, documentation and quality drift a little with every phase. One short phase at the start of
each milestone removes what built up and raises the bar the rest of the milestone must hold. This is where the
owner's recurring CI-efficiency audit and "weakest quality dimension" review happen, without being asked.

## When to Surface

**Trigger:** every milestone after the first. Keep this seed after it is used; it applies again next time.

## Scope Estimate

**Small** — one phase whose results are merged changes:
- CI efficiency, from the report: slowest job faster; timeouts re-derived (twice the slowest of the last
  40 runs, run ID beside it); any flaky test fixed or removed with the stronger signal named; cores used; caching
  decided by measurement; lowest-value tests removed only when a stronger test covers them.
- Weakest dimension: score performance, CI/CD, ease of use, architecture, docs/README and upgrade path 1–5 with
  links; the lowest one gets this phase's main change.
- Ratchet: `quality.json` ceilings tightened to measured values; floors raised.
- Freshness: docs whose code changed, size budgets, the README install line, `scripts/apply-repo-settings.sh
  --check`, Dependabot majors, runner-image changes, pins of fetched suites.
- Direction: refresh `.planning/PRODUCT-ROADMAP.md` and `.planning/preparation/PROMPT-COVERAGE.md`; export lessons.
- v2.0's tune-up first builds the weekly triage digest, the CI-efficiency report, `quality.json` and the
  `policy.lessons_unique` check.

## Breadcrumbs

- `.planning/preparation/ENGINEERING.md` sections 4, 7, 8, 11 and 12: CI design, triage, the ratchet, docs budgets,
  recurring loops.
- `.planning/preparation/DECISIONS.md`: the open questions at its end.

## Notes

The seeds are this project's rolling roadmap: the current milestone is `.planning/ROADMAP.md`; direction beyond it
is `.planning/PRODUCT-ROADMAP.md`; SEED-001 to SEED-006 and SEED-008 hold each horizon's narrative.

Deferred from the v2.0 tune-up (phase 5), for the next one:

- P1 WR-02, a tooling finding deferred under AGENTS rule 7 (review findings about test or tooling code become
  seeds), not fixed in Phase 5: `scripts/classify-changes.sh` treats the `(listing failed)` sentinel line as a known
  path, so a PR whose file listing cannot be fetched yields `release_inputs=false` and a release-pipeline change may
  merge without its rehearsal. Fix: let the sentinel set `unknown`, and add a hygiene test for it.
- P1 IN-04, a tooling finding deferred under AGENTS rule 7, not fixed in Phase 5: `scripts/publish.sh prepare` matches
  the expected asset names as regular expressions, so the `.` in `cogmaster-*.zip` is a wildcard and an oddly named
  file is not reported as unexpected. Fix: `grep -qxF --`, which compares them literally.
- P1 WR-04, accepted risk: the release App key is a plain repository secret that any same-repository branch
  workflow can read. A reviewer-gated environment would break AGENTS rules 6 and 8 (the owner verifies nothing and no
  second person is ever needed), so it stays accepted; revisit only if the App gains permissions beyond contents,
  pull-requests and issues write, or a second maintainer joins.
- Third-party SN76489 and YM2413 sound ROM conformance stays deferred with no lane (carried from v1.0; it is not a
  tune-up requirement). A lane needs an admitted, permissively licensed ROM set first.
- Per-test ctest `TIMEOUT` comments are still unmeasured. `hygiene.tree` exceeded its 60 s limit under heavy machine
  load (observed in plans 05-03 and 05-07; it passes alone in 37 to 57 s), so a measured per-test timeout with the
  run ID beside it would replace the guess.
- An absolute `ir_per_frame` ceiling in `quality.json`, and a coverage lane next to it, so the ratchet also bounds
  the interpreter's work per frame and not only its ratio to the base branch.
