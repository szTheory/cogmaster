---
created: 2026-10-19
title: Post-merge ci follow-ups after phase 1 lands on main
area: ci
severity: minor
files:
  - .github/workflows/ci.yml
  - .github/workflows/nightly.yml
  - .github/dependabot.yml
  - .github/workflows/dependabot-automerge.yml
  - scripts/apply-repo-settings.sh
  - tests/hygiene/tests.cmake
---

## Problem

Phase 1 built the CI, release and nightly pipelines on a branch. Four follow-ups need runs that only exist once
the phase is merged to `main` (ENGINEERING §4 budgets one `ci:` pull request for this tail), and they would be
lost if they lived only in the phase 1 SUMMARY files:

1. **Perf gate is in report mode.** `ci.yml` sets `COGMASTER_PERF_MODE: report`. Arming it needs two
   Cachegrind control runs, `workflow_dispatch` of `ci.yml` on `main`, each printing
   `perf control testcard runs=<a>,<b>,<c> spread=0` (FRAME-09).
2. **The nightly report has never written an issue.** `nightly.yml`'s `report` job is skipped on pull requests.
   It needs one forced-failure dispatch (`force_failure: true`) on `main` that opens the `nightly` issue, then a
   normal run that closes it (ENGINEERING §7).
3. **No Dependabot yet.** `.github/dependabot.yml` (github-actions only, weekly, minor and patch grouped, majors
   apart, `ci` prefix, 7-day cooldown) and `dependabot-automerge.yml` (App token, auto-merge patch and minor only,
   only when Dependabot triggered the run, with a `policy.dependabot` test) are not written (RESEARCH Open
   Question 5). `apply-repo-settings.sh --check` already expects the Dependabot copy of the App key.
4. **wasm32 is report-only.** It is outside `CI required`'s needs and outside `hash-equality`'s count of seven.
   Promote it after ten green runs (DEC.53).

## Solution

One `ci:` pull request after the phase 1 merge and its release are green:

1. Dispatch `ci.yml` on `main` twice: `gh workflow run ci.yml --ref main`. If both control runs show `spread=0`,
   set `COGMASTER_PERF_MODE: gate` in `ci.yml` and cite both run URLs in the pull request body.
2. Dispatch `gh workflow run nightly.yml --ref main -f force_failure=true`; confirm the `nightly` issue opened,
   then let the next scheduled night (or a plain dispatch) close it. Record both run URLs.
3. Add `.github/dependabot.yml` and `.github/workflows/dependabot-automerge.yml` with the `policy.dependabot`
   hygiene test and its self test, in the same pull request.
4. Once `gh run list --workflow ci.yml --branch main --status success` shows ten runs with
   `wasm32 hashes.txt equals Linux x64's`, add `wasm32` to `CI required`'s needs, drop `REPORT_ONLY=wasm32` from
   `policy.ci_required`, and raise `hash-equality`'s `COUNT` to 8.

Also re-derive the `ci.yml` timeouts once ten successful runs exist, replacing `provisional`.
