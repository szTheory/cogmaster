---
phase: 12-whole-list-in-the-ledger
plan: "05"
subsystem: compat-ledger
tags:
  - compat
  - ledger
  - seed
  - docs
status: complete

requires:
  - 12-04
provides:
  - scripts/compat-seed.sh, the one writer of the ledger (operation class, reads normalize and cases, never judges)
  - tests/compat/ledger.csv with one default-profile row for every Master System and Game Gear catalog identity (1,472 rows)
  - Five whole-list tests whose legs are in-process CMake functions of file paths
  - Dispatch D1 green on every lane's awk
affects:
  - 12-06
  - 12-07
  - 12-08
  - 12-10

actuals:
  tokens: 7800
  tasks: 3
  commits: 4

plan_head_before: 8a28d401028fa091f0d770de6ae5386ddb90d643
plan_head_after: 6a79d97c4ae2a71ccf8e7f01c10e88a7badc74a3
commits: 4

tech-stack:
  added: []
  patterns:
    - "The seed learns existing rows by zipping normalize and cases output by index, so the shared parser stays the only CSV reader"
    - "Whole-list legs are functions (error text out, file paths in, no -D read) so a script calls them in-process over stand-ins"
    - "A ledger line is read from its tail: the last six quoted fields hold no quote, so the id is field 2 and status and evidence are counted from the end whatever the title holds"

key-files:
  created:
    - scripts/compat-seed.sh
    - tests/compat/whole_list.cmake
    - tests/compat/whole_list_common.cmake
  modified:
    - tests/compat/ledger.csv
    - docs/compatibility.md
    - docs/conformance.md
    - tests/compat/tests.cmake
    - tests/policy/repository-checks.txt
    - tests/inventory/ci.txt
    - tests/inventory/wasm32.txt
    - tests/inventory/hygiene.txt
    - .github/workflows/ci.yml

key-decisions:
  - "The seed's idempotence test seeds with the date 1970-01-01, a date no ledger row holds, so a row the seed would add or re-date always differs from the committed bytes"
  - "The count and reason controls run on a small stand-in whole ledger written by the seed itself; the committed files are read once per leg"
  - "Only the build job timeout moved (6 to 8 minutes, the report's own comment text): the windows-arm64 build leg took 214 s of 360 s"

requirements-completed:
  - LEDG-01
  - LEDG-04

coverage:
  - id: C1
    description: "scripts/compat-seed.sh writes one default-profile row for each of the 1,463 Master System and Game Gear catalog identities and keeps the nine fixture rows and every pass or fail row byte for byte; its output over its own output is the same bytes"
    requirement: LEDG-01
    verification:
      - kind: integration
        ref: "compat.whole_list.fresh: the seed over the committed ledger and catalog equals the ledger (SHA-256), and each of the 21 compound ids equals string(SHA1) of mame/ plus the catalog slot"
        status: pass
    human_judgment: false
  - id: C2
    description: "A ledger that is not the seed's output is refused: a deleted catalog line, an unknown hand-flipped to unsupported, a flipped region and a board that arrived in the catalog each make the seed differ or exit non-zero"
    requirement: LEDG-01
    verification:
      - kind: unit
        ref: "compat.whole_list.fresh.self_test (expect_fail, exit 1, regex ledger is not whole (4 faults refused)); real-script control: the region dropped from the seed's default key turns compat.whole_list.fresh red"
        status: pass
    human_judgment: false
  - id: C3
    description: "The ledger's cartridge ids equal the catalog's Master System and Game Gear ids and the pin-count arithmetic 653 + 823 - 13 = 1463 = 1444 images + 19 compound, the catalog's SG-1000 lines equal 214, and rows stay below half the 8,192 cap"
    requirement: LEDG-01
    verification:
      - kind: unit
        ref: "compat.whole_list.count: rows=1472 cart_identities=1463 images=1444 compound=19 sg1000=214; controls on a stand-in (identity removed, pin counts one low, stray id) refused; real-script control: the ledger-to-catalog direction removed turns it red"
        status: pass
    human_judgment: false
  - id: C4
    description: "Every unsupported or skipped row's reason token equals its catalog token, a pass on a board-0 identity is refused, unsupported is exactly board 0, every non-compound catalog line equals its src/cartdb.inc fact in machine and board name, and the six mame_no lines equal the pin counts"
    requirement: LEDG-02
    verification:
      - kind: unit
        ref: "compat.whole_list.reason: 1463 catalog lines and 1444 table comparisons agree; seven controls refused (empty, free-text and unlisted reasons exit 2; unsupported on a supported board; pass on a board-0 id; catalog board edited away from the table; mame_no count off by one); real-script control: the table comparison disabled turns it red"
        status: pass
    human_judgment: false
  - id: C5
    description: "Every distinct default profile the ledger seeds (10 of machine, region, tv, fm, video and both ports) runs an owned fixture for one frame with exit 0, and a Game Gear with a paddle exits 77"
    requirement: LEDG-01
    verification:
      - kind: integration
        ref: "compat.whole_list.runnable: 10 profiles run, 1 control refused with 77"
        status: pass
    human_judgment: false
  - id: C6
    description: "The pull request is judged by the base branch's ratchet and the whole-list ledger flips nothing: the old judge from origin/main over the origin/main ledger and the head ledger passes, so does the new judge, and no old row changed or vanished"
    requirement: LEDG-03
    verification:
      - kind: integration
        ref: "git show origin/main: scripts run against tests/compat/ledger.csv with revisions 16 and 17 exits 0 with compat: protected comparison passed, repeated on the final ledger; git diff -U0 origin/main -- tests/compat/ledger.csv holds no removed data line"
        status: pass
    human_judgment: false
  - id: C7
    description: "docs/compatibility.md is the generator's output for the 1,472-row ledger (4,072 bytes, far below the 38,912 cap and the 36,864 page ceiling) and is identical under macOS awk, mawk and gawk"
    requirement: LEDG-04
    verification:
      - kind: integration
        ref: "policy.compat_doc_fresh and its self test; docs, normalize and the seed compared with cmp under mawk 1.3.4 and gawk 5.3.2 in ubuntu:26.04; the five legs' awk output equal under BWK, mawk and gawk"
        status: pass
    human_judgment: false
  - id: C8
    description: "The whole-list ledger and the hardened parser run green on gawk (Windows), mawk (Ubuntu) and BWK awk (macOS) in one hosted dispatch, with no release rehearsal and the Size ceilings step green"
    requirement: LEDG-01
    verification:
      - kind: e2e
        ref: "gh workflow run ci.yml on phase/12-whole-list-in-the-ledger: https://github.com/szTheory/cogmaster/actions/runs/43153144670 concluded success on head b2d6993"
        status: pass
    human_judgment: false
---

# Phase 12 Plan 05: The whole-list ledger Summary

`tests/compat/ledger.csv` now holds one honest default-profile row for each of the 1,463 Master System and Game Gear catalog identities plus the nine fixture rows (1,472 rows: 9 pass, 1,434 unknown, 5 unsupported, 24 skipped), written by one seed script from the catalog through the shared parser, accepted unchanged by the old base judge, and green on all three awks in a hosted dispatch.

## Performance

- **Tasks:** 3 of 3 (task 1 is the tracer: seed, ledger, page and the base-judge proof end to end)
- **Commits:** `8daaf47` (task 1), `47c90af` (task 2), `b2d6993` (task 3 docs), `6a79d97` (the build timeout)
- **Counted check code:** 879, unchanged (plan cap 894; the seed is `operation`, the two CMake files are `behavior`, the data files are data)

## Accomplishments

1. **Seed and ledger (task 1).** `scripts/compat-seed.sh LEDGER CATALOG DATE` reads `normalize` and `cases` of the ledger, zips them by index, copies fixture rows and every pass, fail or infrastructure_error row verbatim, regenerates each unknown, unsupported or skipped row of the default key from its catalog line (keeping the date when the line is unchanged), drops one whose key no longer matches (a board arrived) and adds the default row, exits 1 on an id that is neither a fixture nor an sms or gg catalog line, and writes through `normalize`. The ledger and `docs/compatibility.md` were regenerated from it.
2. **Whole-list tests (task 2).** `compat.whole_list.{count,fresh,fresh.self_test,reason,runnable}`; the legs live in `whole_list_common.cmake` as functions of file paths, so the controls run in-process on a seven-line stand-in catalog and its seeded ledger. The live ledger is read once per test: one awk each for count and reason, one table awk over `src/cartdb.inc`, one seed run (three parses) for fresh, one `cases` parse for runnable.
3. **Docs and dispatch D1 (task 3).** A 1,796-byte `## Whole-list ledger` section in `docs/conformance.md` (file 34,116 bytes, cap 34,300); dispatch D1 on the pushed head `b2d6993`; the one job-timeout edit the measured report called for.

## Seed run

- `sh scripts/compat-seed.sh tests/compat/ledger.csv tests/compat/catalog.tsv "$(date -u +%F)" > ledger.csv` exit 0; standard error `compat-seed: identities=1463 rows=1472 unknown=1434 unsupported=5 skipped=24`.
- The seed over its own output `cmp`s equal (exit 0); `sh scripts/compat-seed.sh tests/compat/ledger.csv tests/compat/catalog.tsv 2026-11-12 | cmp - tests/compat/ledger.csv` exit 0 (the date is unused).
- `sh scripts/compat-common.sh docs tests/compat/ledger.csv > docs/compatibility.md` exit 0 (4,072 bytes); `wc -l tests/compat/ledger.csv` prints 1473; 29 lines hold `reason=` (5 unsupported + 24 skipped).
- Negative controls (recorded): a copy of the catalog with one unknown line made skipped gives a different ledger (summary `unknown=1433 skipped=25`); a copy of the ledger with a row for an unlisted id exits 1 naming the id; an invalid seed date (`x"y`, `not-a-date`) exits 2 through the parser; a catalog in which the Graphic Board line gained a board makes the seed re-key that row to unknown (`unknown=1435 unsupported=4`).

## Base-judge proof

- Old judge: `sh $t/scripts/compat-ratchet.sh $t/base.csv tests/compat/ledger.csv tests/compat/allow.txt <today> 16 17 build/ci/tests/compat-fixtures/results.csv`, with `compat-ratchet.sh`, `compat-common.sh` and the ledger taken by `git show origin/main:` into a temporary directory `t` and the revisions read from both headers (16 and 17): exit 0, `compat: protected comparison passed`. Run after task 1 and again on the final committed files after task 3: exit 0 both times.
- New judge with the same pair, absolute `COMPAT_CATALOG` and `COMPAT_REASONS`: exit 0, `compat: protected comparison passed` (both times).
- `git diff -U0 origin/main -- tests/compat/ledger.csv` exit 0 and `grep -c '^-"'` prints 0: no old row removed or changed.
- mawk and gawk proxy (`docker run ... ubuntu:26.04`, gawk 5.3.2 installed in the throwaway container only): `docs | cmp - docs/compatibility.md`, `normalize | cmp - tests/compat/ledger.csv` and the seed over the ledger `cmp` equal under both, exit 0. The `count`, `reason`, table, compound and tuple awk programs print identical output under macOS awk, mawk 1.3.4 and gawk 5.3.2 on the committed files.

## Hosted run D1

- Run: https://github.com/szTheory/cogmaster/actions/runs/43153144670 (`gh workflow run ci.yml --ref phase/12-whole-list-in-the-ledger`, found by head SHA `b2d69936ae80a78fcf73596d6bb8e3f287041851`, `gh run watch 43153144670 --exit-status` exit 0). No apt DNS failure, no rerun. `jq -e '.conclusion == "success"'` exit 0. The `rehearsal` job concluded `skipped` (no release input changed at that head); the `Size ceilings` step is green in all 6 lanes that run it.
- Job seconds: be32 293, build windows-arm64 214, build windows-x64 173, asan 140, macos-x64 136, macos-arm64 83, perf 67, linux-x64 61, wasm32 55, linux-arm64 52, hygiene 38, retroarch 41.
- Per-test slowest seconds, `sh scripts/ci-durations.sh tests 43153144670` exit 0 (lane from the job logs; only `portable` tests run on be32 and wasm32):

| Test | windows-arm64 | windows-x64 | be32 | wasm32 | TIMEOUT |
|---|---|---|---|---|---|
| compat.whole_list.fresh.self_test | 8.79 | 3.54 | not run | not run | 90 |
| compat.whole_list.fresh | 6.83 | 3.29 | not run | not run | 90 |
| compat.whole_list.reason | 3.69 | 1.45 | not run | not run | 60 |
| compat.whole_list.count | 2.24 | 0.85 | not run | not run | 60 |
| compat.whole_list.runnable | 2.19 | 1.17 | not run | not run | 60 |
| compat.ratchet | 68.16 | 25.26 | not run | not run | 150 |
| library.synthetic | 51.07 | 36.01 | not run | not run | 120 |
| compat.public_update | 47.73 | 23.23 | not run | not run | 91 |
| compat.fixtures | 4.68 | 6.61 | 44.66 | 3.94 | 89 |
| compat.hardening | 16.10 | 7.80 | not run | not run | 90 |
| compat.migration | 14.71 | 4.87 | not run | not run | 60 |
| policy.compat_doc_fresh | 1.65 | 0.85 | not run | not run | 30 |
| compat.page_bound / .self_test | 1.49 / 1.64 | 0.82 / 0.93 | not run | not run | 60 |
| cart.generator / .catalog | 0.48 / 0.38 | 0.28 / 0.20 | 0.76 / not run | 0.20 / not run | 30 |
| cart.database | 0.02 | 0.01 | 0.04 | 0.08 | |

- The five new tests are all under 15 percent of their timeouts on the slowest lane; none exceeds 9 s. Nothing grows with the row count on be32 or wasm32 (none of the new tests runs there).
- Near half of the timeout (all existing tests; the timeouts are twice an earlier measurement by the rule, so about half is where they sit): `compat.public_update` on windows-arm64 47.73 s of 91 s (52 percent, slightly above the 45.10 s the limit was sized from), `compat.fixtures` on be32 44.66 s of 89 s (50 percent), `compat.ratchet` on windows-arm64 68.16 s of 150 s (45 percent), `library.synthetic` on windows-arm64 51.07 s of 120 s (43 percent). None failed; plan 10 settles timeouts.
- The live-ledger parse on gawk (Windows): `policy.compat_doc_fresh`, one `docs` run over the 1,472-row ledger plus a CMake start and a compare, takes 1.65 s on windows-arm64 and 0.85 s on windows-x64 (upper bounds of one parse); macOS awk takes about 0.25 s native. `compat.whole_list.fresh`, three parses and a seed, takes 6.83 s on windows-arm64.
- Job-timeout branch 2: `sh scripts/ci-durations.sh ci.yml 40` exit 0 reported 8 minutes for `build` (2 x 214 s, the windows-arm64 leg of this run) against `timeout-minutes: 6` in `ci.yml`; every other job's report minutes is at or below its line. The line was set to `timeout-minutes: 8  # 2 x 214 s, slowest of 40 runs, run 43153144670` (the report's text); `ctest --test-dir build/hygiene -R '^hygiene\.timeout_comments'` exit 0; `git diff -U0 origin/main -- .github/workflows/ci.yml` holds that one line only. `ci.yml` is a release input (`scripts/classify-changes.sh`), so the single pull request will also run the release rehearsal (about 6 extra minutes); the widening is verified by that pull request run, not by a second dispatch.

## Negative controls (each: python edit, rerun, red, restore, green)

| Control | Mutation | Result |
|---|---|---|
| Seed key | region dropped from the seed's default key | `compat.whole_list.fresh` red; restored green |
| Count | ledger-to-catalog direction of the identity leg removed | `compat.whole_list.count` red (the stray-id control refused for another reason); restored green |
| Reason | table comparison made a no-op | `compat.whole_list.reason` red (the board-edited control accepted); restored green |
| Self test | four faults over a stand-in whole ledger | each refused; the case ends `ledger is not whole (4 faults refused)` |

## Deviations from Plan

### Plan text and acceptance wording

**1. [Planner reading] The identity controls run on a stand-in**
- The count and reason controls edit a seven-line stand-in catalog and the ledger the seed writes over it, not copies of the live files; messages carry the stand-in's numbers (4 against 5) where the plan names the live ones (1462 against 1463). The live files are read once by each leg.

**2. [Planner reading] The "one direction" control is the ledger-to-catalog direction**
- With equal cardinalities one direction implies the other, so removing either alone is only visible through the stray-id control's message; the mutation removes the ledger-to-catalog direction and that control then fails its message check.

**3. [Planner reading] The idempotence date is 1970-01-01**
- The plan names a seed date argument; `wl_fresh` passes `1970-01-01` so a re-added or re-dated row differs from the committed bytes (the committed rows share the seed date, and a mutation of the key would otherwise be invisible).

**4. [Planner reading] Four faults, not three, in the self test**
- A fourth fault (a board that arrived in the catalog while the ledger stayed) joins the three of the plan; the case message is `ledger is not whole (4 faults refused)`.

**5. [Plan text] An invalid seed date**
- The parser checks a date's shape, not its calendar range, so `2026-13-45` is accepted and `not-a-date` exits 2; the plan's "invalid seed date exits 2" holds for a malformed date only. The parser is unchanged.

**6. [Rule 1 - doc] A stale row cap in docs/conformance.md**
- The parser paragraph still said rows to 4096; it now says 8192 (the cap since the hardening plan). One word in a file this plan edits anyway.

**7. [Plan text] Branch 2 of the job-timeout task was taken**
- The report asked for 8 minutes on `build`; the plan's branch 2 sets the line and its comment from the report. Recorded above with its commands.

**Total deviations:** 7 (1 auto-fixed doc word, 6 plan text or acceptance wording). **Impact:** none on scope; the ledger, tests and base-judge property are as the plan states.

## Known Stubs

None.

## Threat Flags

None. The seed reads and writes only files named on its command line and a temporary directory it removes; no new endpoint, authentication path or trust boundary.

## Counted check code and docs bytes

- Counted check code: **879** (plan 04 printed 879; this plan adds no counted line; ceiling 905, hard limit 940); `src` 9,403 lines.
- Docs bytes: `docs/compatibility.md` 4,072 (cap 38,912; generated, about 8 KB expected); `docs/conformance.md` 32,320 to 34,116 (section 1,796 of at most 1,800; file at most 34,300).
- Inventories: `ci.txt` and `wasm32.txt` +5 each (10 above origin/main, adding plan 01 and 02 and 04 names), `hygiene.txt` +4 (8 above origin/main); 662 registered tests in the `ci` preset.

## Evidence (commands and exit statuses; no log committed)

- `cmake --workflow --preset ci`: exit 0 (662 tests, 5 release tests) after task 2, after task 3 and after the timeout edit; `cmake --workflow --preset hygiene`: exit 0 (82 tests) each time.
- `ctest --test-dir build/ci -R '^compat\.whole_list' --output-on-failure`: exit 0, slowest of 4 local runs 0.09 s (count), 0.63 s (fresh), 0.26 s (fresh.self_test), 0.14 s (reason), 0.23 s (runnable); `ctest --test-dir build/ci -N -R '^compat\.whole_list'` prints `Total Tests: 5`.
- `cmake -DROOT="$PWD" -P tests/cmake/budgets.cmake`: exit 0 after each task, `879 check lines / 9403 src lines`.
- `git diff --numstat origin/main -- tests/inventory/ci.txt tests/inventory/wasm32.txt tests/inventory/hygiene.txt`: 10/0, 10/0, 8/0.
- `git diff --name-only origin/main | grep -E 'publish\.sh|release-please-config'`: no match (exit 1); the only `ci.yml` line is the build `timeout-minutes` line.
- No C file touched, so no gcc:13 container check was due.

## Self-Check: PASSED

- `scripts/compat-seed.sh`, `tests/compat/whole_list.cmake`, `tests/compat/whole_list_common.cmake`, `tests/compat/ledger.csv` and `docs/compatibility.md` exist and are committed; commits `8daaf47`, `47c90af`, `b2d6993` and `6a79d97` are ancestors of HEAD; run 43153144670 concluded success.
