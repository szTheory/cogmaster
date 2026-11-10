---
phase: 01-a-test-frame-in-retroarch
plan: 10
subsystem: release-pipeline
status: complete
tags:
  - ci
  - release
  - release-please
  - attestations
  - bench
requires:
  - "01-08: scripts/publish.sh (prepare, upload, attested, verify, rehearsal-create, rehearsal-delete), classify-changes.sh release_inputs, the bench preset"
  - "01-09: ci.yml with workflow_call input ref, archives-<os>-<arch> artifacts holding the three zips and commit.txt, policy.ci_required"
provides:
  - ".github/workflows/ci.yml jobs rehearsal and cleanup, both in CI required's needs; CI required takes --arg release_inputs"
  - ".github/workflows/release.yml: release-please, release-ci (calls ci.yml at release-please's sha), publish, install-line-latest"
  - ".github/workflows/nightly.yml: bench x3, bench-wasm32, report (rolling nightly issue, force_failure input)"
  - "release-please-config.json and .release-please-manifest.json (0.0.0)"
  - "publish.sh verify: SIGNER_WORKFLOW honoured only for rehearsal-* tags"
  - "Hygiene CTest policy.publish_signer; six new policy.ci_required cases (18 in total)"
affects:
  - "plan 11: pushes the branch; the rehearsal must go green on the draft pull request (App token, merge-ref target_commitish, attest signer ci.yml)"
  - "plan 12: docs/testing.md describes the rehearsal and nightly, docs/maintaining.md the release path; measured timeouts replace the provisional ones"
actuals:
  tokens: 8100
  tasks: 2
  commits: 2
plan_head_before: d91e08e4f6684a426225bca240fb50fa1e17bda1
plan_head_after: 4bbc0544831bdceea17a2a63c391bcdaed149de4
tech-stack:
  added:
    - "actions/create-github-app-token 64ab729a6294000f5aa4a4bc0b73d1f32bc45510 (v3.3.0)"
    - "actions/attest 1e69f48acb82d1966a394da916b4c1698aa569d6 (v4.2.2)"
    - "googleapis/release-please-action 45996ed1f6d02564a971a2fa1b5860e934307cf7 (v5.0.0)"
  patterns:
    - "Archives artifacts are downloaded per leg and rearranged: zips side by side in dist, each commit.txt under dist/legs/N"
    - "A script's network path is tested end to end with gh and curl stubs on PATH that log their arguments"
key-files:
  created:
    - .github/workflows/release.yml
    - .github/workflows/nightly.yml
    - release-please-config.json
    - .release-please-manifest.json
  modified:
    - .github/workflows/ci.yml
    - scripts/publish.sh
    - tests/cmake/ci_required.cmake
    - tests/hygiene/tests.cmake
    - tests/inventory/hygiene.txt
decisions:
  - "The rehearsal and the release publish both use the App token as GH_TOKEN for every publish.sh step and the PATCH; actions/attest signs with the job's own OIDC token"
  - "REHEARSAL_TAG is a workflow-level env in ci.yml, so rehearsal and cleanup name the same rehearsal-<run_id>-<run_attempt> tag"
  - "A fork's pull request that changes a release input skips the rehearsal (no secrets) and so fails CI required; it must be rehearsed from a same-repository branch"
  - "release.yml keeps the seed's concurrency queue: max (green on muhtari main, run 38983378877) although actionlint 1.7.12 does not know the key"
  - "bench-wasm32 is kept from the seed; WASI has no RSS, so its max_rss_kib reads 0"
requirements-completed: []
coverage:
  - id: D1
    description: "ci.yml has rehearsal (App token, fail-fast message naming COGMASTER_APP_CLIENT_ID and COGMASTER_APP_PRIVATE_KEY, prepare, rehearsal-create, upload, attest, attested, PATCH prerelease=true make_latest=false, verify) and cleanup (if always() after a rehearsal that ran, rehearsal-delete); actionlint finds nothing but unknown runner labels"
    requirement: FRAME-06
    verification:
      - kind: other
        ref: "grep -Ec '^  (rehearsal|cleanup):' .github/workflows/ci.yml (prints 2); grep -c make_latest .github/workflows/ci.yml (prints 1); actionlint -ignore 'is unknown. available labels' .github/workflows/ci.yml (exit 0)"
        status: pass
      - kind: e2e
        ref: "hosted rehearsal and cleanup on the draft pull request, plan 11"
        status: unknown
    human_judgment: false
  - id: D2
    description: "CI required passes rehearsal and cleanup skipped only with release_inputs not true, outside a pull request, or in a nested call; a skipped rehearsal on a pull request that changes a release input fails, as do a failed rehearsal and a failed cleanup"
    requirement: FRAME-06
    verification:
      - kind: integration
        ref: "tests/hygiene/tests.cmake#policy.ci_required"
        status: pass
      - kind: integration
        ref: "tests/hygiene/tests.cmake#policy.ci_required.self_test"
        status: pass
    human_judgment: false
  - id: D3
    description: "publish.sh verify checks downloads against release.yml for a release tag whatever SIGNER_WORKFLOW says, and against SIGNER_WORKFLOW for a rehearsal- tag; with the override leaking to release tags the test fails naming 18 downloads verified otherwise"
    requirement: FRAME-06
    verification:
      - kind: integration
        ref: "tests/hygiene/tests.cmake#policy.publish_signer"
        status: pass
      - kind: other
        ref: "publish.sh edited to honour SIGNER_WORKFLOW for every tag, then ctest --test-dir build/hygiene -R policy.publish_signer (exit 8, 'fixture-release verified 0 downloads as release.yml and 18 otherwise'); file restored"
        status: pass
    human_judgment: false
  - id: D4
    description: "release.yml on push to main runs release-please with the App token and auto-merges its pull request, calls ci.yml at release-please's sha with contents write, pull-requests read, id-token write and attestations write, publishes the draft by ID with the same publish.sh steps, and runs the install line against releases/latest/download/"
    requirement: FRAME-06
    verification:
      - kind: other
        ref: "grep -c pull_request .github/workflows/release.yml (prints 0); grep -Ec 'uses: \\./\\.github/workflows/ci\\.yml' .github/workflows/release.yml (prints 1)"
        status: pass
      - kind: e2e
        ref: "release.yml on the merge to main, confirmed at ship"
        status: unknown
    human_judgment: false
  - id: D5
    description: "release-please-config.json holds the ENGINEERING section 5 values and the manifest and version.txt both read 0.0.0; release.single_version_source still passes"
    requirement: FRAME-06
    verification:
      - kind: other
        ref: "jq -e '.packages[\".\"][\"initial-version\"] == \"0.1.0\" and .packages[\".\"].draft == true' release-please-config.json (exit 0); jq -r '.[\".\"]' .release-please-manifest.json (prints 0.0.0)"
        status: pass
      - kind: integration
        ref: "tests/policy/tests.cmake#release.single_version_source"
        status: pass
    human_judgment: false
  - id: D6
    description: "nightly.yml runs the bench workflow preset and cogmaster-run --bench 300000 on three runners and under wasmtime, fails a report missing p50_us, p99.999_us, max_us or max_rss_kib, and decides the rolling issue from needs results with a force_failure input"
    requirement: FRAME-09
    verification:
      - kind: other
        ref: "cmake --workflow --preset bench, then build/bench/cogmaster-run --bench 3000 (exit 0, prints p50_us through max_us and max_rss_kib)"
        status: pass
      - kind: e2e
        ref: "the forced-failure workflow_dispatch run, after nightly.yml is on main"
        status: unknown
    human_judgment: false
---

# Phase 1 Plan 10: Rehearsed Release Path and Nightly Bench Summary

**The pull request now rehearses every publish step with the release App's token inside `CI required`. Merging to
main publishes the release through release-please and the same `publish.sh` steps. A nightly bench records the
runner's own percentiles and `max_rss_kib`.**

## What was built

- **Rehearsal in `ci.yml`.** On a same-repository pull request that changes a release input, the `rehearsal` job
  first checks that the App is configured and fails with a message naming the missing variable or secret. It then
  mints the App token and gathers the six legs' archives into `dist/` with each `commit.txt` under `dist/legs/N`.
  It runs `prepare`, `rehearsal-create rehearsal-<run_id>-<run_attempt>`, `upload`, `actions/attest`, `attested`,
  a PATCH to a prerelease with `make_latest=false`, and `verify`, with `SIGNER_WORKFLOW=ci.yml` and
  `SOURCE_REF=refs/pull/<n>/merge`. `cleanup` deletes the release and its tag whatever the outcome.
- **Roll-up.** `CI required` needs both jobs and takes `--arg release_inputs`. `policy.ci_required` now has 18
  synthetic cases.
- **Signer.** `publish.sh verify` uses `SIGNER_WORKFLOW` only for `rehearsal-*` tags, so a release always verifies
  as `release.yml` from `refs/heads/main`. `policy.publish_signer` runs `verify` end to end against logging `gh`
  and `curl` stubs.
- **`release.yml`.** It runs only on pushes to main. Its jobs are release-please (App token, auto-merge of the
  release PR), `release-ci` (calls `ci.yml` at release-please's sha with the permission union), `publish` (draft by
  ID, App token) and `install-line-latest`.
- **release-please files.** `release-please-config.json` holds the ENGINEERING §5 values; the manifest reads
  `0.0.0`.
- **`nightly.yml`.** It runs the bench on three runners plus wasm32 and checks that the report holds the
  percentile and RSS lines. A rolling issue is decided from `needs` results, and a `force_failure` dispatch input
  lets the failure path be tested on purpose.

## Verification run

- `cmake --workflow --preset hygiene`: exit 0, 23 tests. `check_junit.cmake` on the hygiene inventory: exit 0.
- `cmake --workflow --preset ci`: exit 0, 80 + 4 tests. Both `check_junit.cmake` calls: exit 0.
- `actionlint -ignore 'is unknown. available labels' .github/workflows/*.yml`: one finding,
  `release.yml` `concurrency.queue`, kept from the seed (see Deviations).

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Unquoted optional release ID in cleanup**
- **Found during:** Task 1 (actionlint SC2086)
- **Fix:** `rehearsal-delete "$REHEARSAL_TAG" ${ID:+"$ID"}` passes the ID only when the rehearsal produced one.
- **Files modified:** .github/workflows/ci.yml
- **Commit:** 48bf9f2

### Notes

- The plan listed `ci.yml` under Task 2's files, but Task 2 needed no change there. The permission union lives on
  `release.yml`'s `release-ci`.
- The seed's `concurrency.queue: max` is kept in `release.yml`. actionlint 1.7.12 rejects the key, but the seed's
  release.yml ran green on muhtari main with it (run 38983378877). If the hosted run refuses it, plan 11 drops it.
- No CTest was added for the workflow YAML beyond `policy.action_pins`, `hygiene.timeout_comments` and
  `policy.ci_required`, which already glob every workflow file.

## Known Stubs

None.

## Threat Flags

| Flag | File | Description |
|------|------|-------------|
| threat_flag: credential-use | .github/workflows/ci.yml | The release App's private key now reaches pull_request runs from same-repository branches (rehearsal, cleanup); forks never receive it. The token is minted with contents: write only |

## Self-Check: PASSED
