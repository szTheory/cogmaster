---
phase: 11-pal-and-japanese-fm
plan: "04"
subsystem: compat
tags:
  - ledger
  - migration
  - ci-bridge
  - docs
status: complete

requires:
  - 11-03
provides:
  - the committed ledger in canonical format 2 with every status, evidence text and date unchanged
  - live-file assertions in compat.migration and a ratchet suite that speaks format 2 and guards tv and fm
  - one narrow ci.yml branch that lets exactly the format 1 to format 2 header pair through the base judge, and apt retries
  - the format note, the page intro and the README lines for PAL and FM
affects:
  - 11-05
  - 11-07
  - 11-09

actuals:
  tokens: 3500
  tasks: 3
  commits: 3

plan_head_before: dc7bbe941f03c92b42e5ce72e7cd2d50017392d6
plan_head_after: e66852d04d7eb4e63c4f2fdac1d331756455ed5d
commits: 3

tech-stack:
  added: []
  patterns:
    - "The migration is the normalize mode itself: one command, no migrate mode, byte for byte a fixed point"
    - "A workflow shell step is proved once by extracting its own lines from the committed YAML and running them against a base taken from origin/main"
    - "A bridge triggers on an exact header literal pair, so every other header takes the old path and fails closed"

key-files:
  created: []
  modified:
    - tests/compat/ledger.csv
    - tests/compat/migration.cmake
    - tests/compat/ratchet.cmake
    - tests/compat/update.cmake
    - scripts/compat-common.sh
    - docs/compatibility.md
    - docs/conformance.md
    - docs/testing.md
    - README.md
    - .github/workflows/ci.yml

key-decisions:
  - "The ledger was migrated with sh scripts/compat-common.sh normalize and nothing else; the diff is the header, the long cfg with fm=off, the tv column, quoting and row order"
  - "The live assertions in compat.migration are a function that returns a verdict, so the controls call it on a format 1 header copy and a copy with a field removed; the live file is never compared with the embedded literal"
  - "The bridge compares the first lines as strings through command substitution, so a CR-damaged or quoted header is not equal and goes to the base judge"
  - "The page intro and the audio predicate count were added with no new counted line by merging them into existing prints"

requirements-completed:
  - REG-01

coverage:
  - id: C1
    description: "The committed ledger is the canonical format 2 form: normalize of the released file equals it byte for byte, normalize of it reproduces it, the header and six lines are as specified, four rows pass and one is unknown as before"
    requirement: REG-01
    verification:
      - kind: integration
        ref: "git show origin/main:tests/compat/ledger.csv, then sh scripts/compat-common.sh normalize of that file piped to cmp against tests/compat/ledger.csv exit 0; normalize of tests/compat/ledger.csv piped to cmp against itself exit 0; grep -c '\"pass\"' prints 4 and grep -c '\"unknown\"' prints 1"
        status: pass
    human_judgment: false
  - id: C2
    description: "compat.migration also checks the live file: the format 2 header and normalize of it equals it in memory with CR tolerance; a copy with the format 1 header and a copy with a field removed each fail the same function"
    requirement: REG-01
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^compat\\.migration$' exit 0 (28 controls); unquoting one live status makes it exit 8 naming normalize, the format 1 header on the live file exits 8 naming the header, restored exit 0"
        status: pass
    human_judgment: false
  - id: C3
    description: "compat.ratchet runs on format 2 rows; the protected pairs gain export,ntsc to export,pal and fm=off to fm=on, each exit 2, with the older cases keeping their exit statuses and the format 1 base cases unchanged"
    requirement: REG-01
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^compat\\.ratchet' exit 0; with the resolved fm dropped from the stored config the fm pair exits 0 instead of 2 and compat.ratchet exits 8, restored exit 0"
        status: pass
    human_judgment: false
  - id: C4
    description: "A format 1 base against the migrated head: the unflipped pair exits 0 and the same pair with the first pass changed to fail exits 2"
    requirement: REG-01
    verification:
      - kind: integration
        ref: "sh scripts/compat-ratchet.sh V1 HEAD tests/compat/allow.txt 2026-11-11 16 16 exit 0; with FLIPPED in place of HEAD exit 2"
        status: pass
    human_judgment: false
  - id: C5
    description: "The ci.yml bridge prints the migration line and passes on the real files, fails closed with a flipped status, a dropped row or an impossible profile, takes the old path with equal headers, and takes no bridge with a quoted or CR-damaged head header; every apt-get call carries the retry option"
    requirement: REG-01
    verification:
      - kind: integration
        ref: "the step's own shell lines extracted from the committed ci.yml, bash -n exit 0, run against an origin/main base: real exit 0 with the line; flipped exit 2; equal headers exit 0 with no line; quoted exit 2 with no line; CR exit 2 with no line; bad row exit 2; dropped row exit 2; actionlint reports only unknown runner labels"
        status: pass
    human_judgment: false
  - id: C6
    description: "The page intro names the setting of each row, says TV is a setting of the run and audio is a pinned hash, and states the format change; the page is regenerated and fresh under BWK awk and mawk"
    requirement: REG-01
    verification:
      - kind: integration
        ref: "sh scripts/compat-common.sh docs tests/compat/ledger.csv piped to cmp against docs/compatibility.md exit 0, also inside ubuntu:26.04 (mawk) exit 0; ctest -R '^policy\\.compat' exit 0; grep -c 'Format 2' docs/compatibility.md prints 1"
        status: pass
    human_judgment: false
  - id: C7
    description: "The format note sits in docs/conformance.md and the README says in four lines how to ask for PAL and what happens with FM; the option labels are the ones in libretro/options.h; docs and README stay inside their budgets and the play-steps check is green"
    requirement: REG-01
    verification:
      - kind: integration
        ref: "ctest -R '^docs\\.readme_play' exit 0 (test and self_test); grep -F of the three option labels in libretro/options.h finds each; wc -c docs/conformance.md 30,557 (cap 31,400); cmake --workflow --preset ci exit 0 (628 tests); cmake --workflow --preset hygiene exit 0 (72 tests); cmake -DROOT=$PWD -P tests/cmake/budgets.cmake exit 0 (852 check lines)"
        status: pass
    human_judgment: false

duration: about 40 minutes
completed: 2026-11-11
---

# Phase 11 Plan 04: Ledger migration and the ci.yml bridge Summary

The committed ledger is now the canonical format 2 file that `normalize` prints, with no status, evidence text or date moved; compat.migration, compat.ratchet and compat.public_update speak format 2; and one narrow branch in the protected comparison step lets the format 1 to format 2 change merge in a single pull request. REG-01 is complete.

## Accomplishments

- `tests/compat/ledger.csv` was migrated with `sh scripts/compat-common.sh normalize` only (the diff is the header, the explicit cfg with `fm=off`, the `tv` column, quoting and the order of the five rows). Real-file evidence below.
- `tests/compat/migration.cmake` gained a live block: a function returns a verdict for the format 2 header and `normalize(live) == live` (read with `file(READ)`, CRLF replaced in memory, never compared with the literal); two controls call it on a format 1 header copy and a copy with one field removed (28 controls now).
- `tests/compat/ratchet.cmake` moved to format 2 rows. The protected pairs are `sms,export,ntsc|gg,export,ntsc`, `export,ntsc|export,pal`, `fm=off|fm=on` (built on a row that spells its cfg, with a matching base), then the unchanged pairs. The cross-format cases now start from an embedded format 1 base text. `tests/compat/update.cmake` exchanges format 2 candidates and keeps the first candidate in format 1 with its pass assertion.
- `scripts/compat-common.sh` docs printer: the page names each row's setting, says TV is a setting of the run and not a fact about the cartridge, says `audio@F=HASH` pins a stream of changes and never how it sounds, and carries the one sentence on Format 2; an `audio predicates` count joined the list. No counted line was added (102 lines before and after).
- `.github/workflows/ci.yml`: one branch in the step "Protected compatibility comparison" and `-o Acquire::Retries=3` on all eight `apt-get` calls (four lines).
- `docs/testing.md` one sentence (343 bytes) beside the step; `docs/conformance.md` the format note (476 bytes); `README.md` four lines (398 bytes) on PAL and FM.

## Task Commits

1. Task 1, the migrated ledger, live assertions, format 2 ratchet and update tests, page intro: `acd7767`
2. Task 2, the bridge, apt retries and the testing.md sentence: `6d954d6`
3. Task 3, the format note and the README lines: `e66852d`

## The bridge and its trust statement

The step prints `compat: ledger format migration` and runs the head's `scripts/compat-ratchet.sh` (which reads both formats) over the base ledger and the head ledger only when all three hold: the trusted base ratchet exists, the first line of the trusted base ledger equals the format 1 header literal exactly, and the first line of `tests/compat/ledger.csv` equals the format 2 header literal exactly. The comparison is `[ "$(head -n 1 FILE)" = "$literal" ]`, so a quoted header, a header with a CR, a reordered or any other header is not equal and goes through the existing branches, where the base judge fails closed (its awk requires the old header and nine fields). Stage 3 (fresh fixture results) runs in the bridge as in the other branches. One comment above the branch names the fault it prevents and says it is dead once the base header is format 2.

Trust: a pull request run executes the merge commit's ci.yml, so the candidate already controls this step and the existing first-introduction branch; the bridge adds no capability beyond a ci.yml edit, which already forces the release rehearsal and the owner's review, and it is reachable only while the base header is format 1. In the bridge the head's judge is the one that reads the head ledger, so an invalid head ledger (an impossible profile, a dropped row, a changed status) still exits 2 unless the judge itself is replaced, which is the same ci.yml-level capability.

## Evidence

All commands from the repository root on this Mac; scratch files lived under a session scratch directory (never committed, no log in the tree). No hosted run belongs to this plan.

Real-file migration evidence (exit statuses):

- `git show origin/main:tests/compat/ledger.csv > V1` exit 0 (origin/main fetched at the start; the file equalled the pre-migration HEAD file, `git diff --quiet origin/main -- tests/compat/ledger.csv` exit 0 before the edit).
- `sh scripts/compat-common.sh normalize V1 | cmp - tests/compat/ledger.csv` exit 0.
- `sh scripts/compat-common.sh normalize tests/compat/ledger.csv | cmp - tests/compat/ledger.csv` exit 0.
- `test "$(head -n 1 tests/compat/ledger.csv)" = 'id,title,machine,region,tv,mapper,predicate,status,evidence,date'` exit 0; `grep -c '' tests/compat/ledger.csv` prints 6.
- Unchanged facts: v1 has 4 `pass` and 1 `unknown` (`grep -c` of the unquoted words); v2 `grep -c '"pass"'` prints 4 and `grep -c '"unknown"'` prints 1; the diff stat is 6 insertions and 6 deletions.

Cross-format negative control: `sh scripts/compat-ratchet.sh V1 FLIPPED tests/compat/allow.txt 2026-11-11 16 16` exit 2 (message `compat: invalid ledger record 12`), FLIPPED being the migrated ledger with its first `pass` changed to `fail`; the unflipped migrated ledger in its place exit 0 (`compat: protected comparison passed`).

Bridge simulation. Setup: `git archive origin/main scripts/compat-ratchet.sh scripts/compat-common.sh tests/compat/ledger.csv include/cogmaster.h | tar -x -C BASE` exit 0, the four files copied into a `compat-base` directory with a sha file the way the "Preserve baseline" step does; `git archive HEAD | tar -x -C HEAD_COPY` exit 0 (made a throwaway git repository so `git rev-parse HEAD` works), with `build/ci/tests/compat-fixtures/results.csv` from the last ci workflow copied in. The step's script was extracted from the committed ci.yml with python (lines after the step's `run: |` and before the next `- name:`, de-indented by 10 spaces) and `bash -n` exit 0. Each run: `cmake -E chdir HEAD_COPY env RUNNER_TEMP=... GITHUB_SERVER_URL=http://example.invalid GITHUB_REPOSITORY=sim GITHUB_RUN_ID=1 bash -eo pipefail step.sh`, output saved to a file and judged with `grep -q`:

| Case | Exit | Migration line | Last line printed |
|---|---|---|---|
| real files: v1 base, real v2 head | 0 | yes | `compat: protected comparison passed` |
| head with the first status changed pass to fail | 2 | yes | `compat: invalid ledger record 17` |
| base replaced by copies of the head's own judge files and ledger (equal headers) | 0 | no | `compat: protected comparison passed` |
| head header quoted | 2 | no | `compat: invalid ledger record 7` (base judge, fails closed) |
| head header with a CR appended | 2 | no | `compat: invalid ledger record 7` (base judge, fails closed) |
| extra: v2 header, a Game Gear row claiming PAL | 2 | yes | `compat: invalid ledger record 8` |
| extra: v2 header, one row dropped | 2 | yes | `compat: invalid ledger record 16` |

All mutations were made in copies; `git status --short` in the repository was empty after the runs.

ci.yml diff: `git diff -U0 origin/main -- .github/workflows/ci.yml` exit 0; five hunks, 15 added lines. Hunk 1 (`@@ -198 +198,11`) is the bridge: three comment lines, the two literals, the new `if` with its condition, the migration echo, `base_rev`, the head judge call over two lines, and the old `if` turned `elif`; hunks 2 to 5 (`-267 +277`, `-307 +317`, `-352 +362`, `-579 +589`) are the four apt lines, each belonging to the apt option. `grep 'apt-get' .github/workflows/ci.yml | grep -vc 'Acquire::Retries=3'` prints 0 and both `grep -c apt-get` and `grep -c 'Acquire::Retries=3'` print 8 (invocations, two per line). `actionlint .github/workflows/ci.yml` reports 15 findings, all "label ubuntu-26.04 ... is unknown" (the installed actionlint predates that runner label; the file's other checks, including the shell, are clean: `actionlint -oneline | grep -vc 'is unknown'` prints 0).

Negative controls (python edit, ctest red, restore, ctest green):

- Dropping the resolved fm from the stored config (key without fm): `compat.ratchet` exit 8 with "protected fm=off|fm=on: expected exit 2, actual 0"; restored exit 0. (The earlier plan's tv control covers `export,ntsc|export,pal`.)
- Live ledger with one status unquoted: `compat.migration` exit 8 ("normalize does not reproduce it"); live header replaced by the format 1 header: exit 8 ("header is not the format 2 header"); restored exit 0.

Verify commands: `ctest --test-dir build/ci -R '^(compat\.|policy\.compat|library\.|docs\.)'` exit 0; the same compat and policy subset on `build/hygiene` exit 0 (9 and 2 tests); `cmake --workflow --preset ci` exit 0 (628 tests, 5 release tests) after tasks 1 and 3; `cmake --workflow --preset hygiene` exit 0 (72 tests) after each task; `git diff --quiet origin/main -- include src` exit 0.

mawk proxy: `docker run --rm -v "$PWD:/src" -w /src ubuntu:26.04 sh -c 'sh scripts/compat-common.sh docs tests/compat/ledger.csv | cmp - docs/compatibility.md && sh scripts/compat-common.sh normalize tests/compat/ledger.csv | cmp - tests/compat/ledger.csv'` exit 0.

Budgets: `cmake -DROOT="$PWD" -P tests/cmake/budgets.cmake` exit 0, 852 check lines and 9,402 src lines (unchanged from plan 03; this plan added 0 counted lines against its cap of +4; ceiling 888). Docs bytes: conformance.md 30,081 to 30,557 (+476, cap 31,400, note cap 600), testing.md 31,339 to 31,682 (+343, sentence cap 350, file cap 33,900), README.md 4,095 to 4,493 (+398, cap 500; four added lines by `grep -c '^+[^+]'`, five by `--numstat` counting the separating blank line, no deletions), compatibility.md 2,316 on origin/main to 2,758 (+442, intro cap 900). Planning-identifier grep over added lines in docs, README and ci.yml prints 0.

## Deviations from Plan

### Interpretations (no behaviour lost)

- **ratchet.cmake loop shape.** The `fm=off|fm=on` pair is built inside the loop on a row that spells its cfg, with a base of the same spelled row; the loop sets and unsets `base_text` for that pair only. The cross-format block keeps plan 03's cases, with an embedded format 1 base (`header1`, `row1`) because the global header and row are now format 2.
- **Extra simulation cases.** Two cases beyond the five required (an impossible profile and a dropped row under the exact header pair) were run because the plan's threat is an invalid ledger passing through the bridge; both exit 2.
- **actionlint.** It exits 1 on this file only for runner labels it does not know (`ubuntu-26.04`, `ubuntu-26.04-arm`); recorded as the finding list above, not a workflow defect.
- **Page growth.** The page intro measured against origin/main is +442 bytes in total (it includes the profile cells plan 03 already changed); the intro text itself is 327 bytes with its blank line and the `audio predicates` count line 22.
- **No `state advance-plan`.** The Plan line was set by hand as in plans 11-01 to 11-03.

**Total deviations:** 0 rule-driven fixes; 5 interpretations above. **Impact:** none on the plan's outcome.

## Known Stubs

None.

## Threat Flags

None beyond the register. T-11-13 (the bridge needs the exact header pair; seven simulations) and T-11-14 (byte-equal normalize, live and literal assertions, the flipped cross-format ratchet) are covered above; T-11-15 (apt retries) is accepted.

## Phase notes

- The bridge's first hosted run is the draft pull request opened in plan 07; the ship run is the second. A workflow_dispatch compares the head with itself, so only a pull request run exercises it.
- The real-file `cmp` (`normalize` of the released ledger against the committed one) is repeated on the final ledger in plan 09; plan 05 adds rows, so that command will then compare the released five rows' facts only through compat.migration, as the plan for it states.
- Plan 05 adds the five `audio@300` rows to the ledger; `compat.migration` stays valid because it checks the live file by header and fixed point, never against the literal.

## Self-Check: PASSED

- Commits `acd7767`, `6d954d6` and `e66852d` are ancestors of HEAD (`git rev-list --count dc7bbe9..HEAD` prints 3) and each message ends with the attribution trailer.
- `tests/compat/ledger.csv`, `tests/compat/migration.cmake`, `.github/workflows/ci.yml` and the other modified files exist; `grep -c 'ledger format migration' .github/workflows/ci.yml` prints 1.
