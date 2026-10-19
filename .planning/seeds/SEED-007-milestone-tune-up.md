---
id: SEED-007
status: dormant
planted: 2026-10-18
planted_during: preparation, before milestone v1.0
trigger_when: every milestone after the first, as its first phase
scope: small
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
