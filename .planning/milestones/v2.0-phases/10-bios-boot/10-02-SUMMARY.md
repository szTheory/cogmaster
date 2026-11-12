---
phase: 10-bios-boot
plan: "02"
subsystem: firmware-legality
tags:
  - bios
  - window-scan
  - hygiene
  - hooks
  - hosted-dispatch
status: complete

requires:
  - 10-01
provides:
  - scripts/firmware-window.sh, the local 32-byte window scan of the owner's dump against the tree, build outputs and release zips
  - local.firmware_window_scan, the library-preset registration that passes and says 0 compared when no dump is configured
  - ten hygiene-lane controls (hit, array, w32, w31, low_entropy, end, clean, unset, staged, history) proving what fails and what passes
  - hygiene.sh byte_run (256 consecutive two-digit byte literals fail "retyped bytes") with three controls
  - the staged and history hook branches of hygiene.sh, both controlled
  - the first hosted dispatch of the scans and the settled timeouts of plans 01 and 02
affects:
  - 10-03

actuals:
  tokens: 36000
  tasks: 3
  commits: 3

plan_head_before: 16645bced111641babe1a4e30d22a102565e9305
plan_head_after: 45def49b4d3fdcb0c970baec1c6958c29a2d0067
commits: 3

tech-stack:
  added: []
  patterns:
    - "A byte scan feeds awk one line of hex bytes at a time and rolls a 32-byte window through it, so no string grows with the target (BWK awk substr is linear in the string length)"
    - "A window control that must pass is only a control if the scan would match without the property under test, so low_entropy and end use their own dump"
    - "A hook branch is controlled by running hygiene.sh in a throwaway repository with the directory variable passed through env, its name assembled from two pieces"

key-files:
  created:
    - scripts/firmware-window.sh
    - tests/policy/firmware_window_cases.cmake
  modified:
    - scripts/hygiene.sh
    - tests/policy/repository-checks.txt
    - tests/policy/firmware_release_cases.cmake
    - tests/hygiene/tests.cmake
    - tests/tests.cmake
    - tests/inventory/hygiene.txt
    - tests/inventory/library.txt
    - .github/workflows/ci.yml

key-decisions:
  - "The window scan rolls a 32-byte window through od output line by line (stride 1, one hash lookup per byte) instead of sliding substr over one long hex string: BWK awk on the agent's machine took 56 s for 1 MB that way and 1.1 s with the rolling window"
  - "A text target is reduced to its byte literals with tr and grep (every even-length run of hex digits in order), which makes the 0x, dollar and bare forms one rule and needs no sed"
  - "The end and low_entropy controls use their own dumps because the plan's dump repeats every 256 bytes and every window of it has 32 distinct values, so the plan's dump alone could not make either control fail on a broken scan"
  - "The hook is one function fed rev:path names, so both branches fit in 6 counted lines and byte_run stays (4 lines, 10 in all)"
  - "The hygiene job timeout rises from 2 to 3 minutes: its slowest measured duration at this head is 63 s, and twice that is 126 s"

requirements-completed: []

coverage:
  - id: D1
    description: "firmware-window.sh loads every 32-byte stride-1 window of each regular file of at most 256 KiB in a flat symlink-free directory that has at least 8 distinct values, matches the targets' raw bytes and their byte literals, prints one line per hit naming the dump by ordinal and size and the target, never a window, offset or dump path, and says 0 compared with exit 0 when no dump is configured"
    requirement: BIOS-01
    verification:
      - kind: unit
        ref: "ctest --test-dir build/hygiene -R '^policy\\.firmware_window\\.' exit 0 (10 tests); hit, array, w32 and end fail on the whole-line regex, w31, low_entropy, clean and unset pass; one-shot default-target scan with a self-authored dump exit 0 in 46 s; mutations of the entropy filter, the last-window loop, the window width and the literal reduction each made the matching control go red"
        status: pass
    human_judgment: false
  - id: D2
    description: "local.firmware_window_scan is registered in the library preset only with the local label and no skip property, runs the script with no argument and passes saying no dump configured"
    requirement: BIOS-01
    verification:
      - kind: integration
        ref: "ctest --test-dir build/library -R '^local\\.firmware_window_scan$' -V exit 0 printing 'no dump configured, 0 compared'; grep -Eq 'SKIP_RETURN_CODE|SKIP_REGULAR_EXPRESSION' tests/tests.cmake exit 1; grep -rn COGMASTER_BIOS_DIR tests prints nothing; ctest policy.bios_dir_reads exit 0"
        status: pass
    human_judgment: false
  - id: D3
    description: "hygiene.sh fails 256 or more consecutive two-digit byte literals as retyped bytes, passes 255 and 255 split by one prose line, and the real tree passes"
    requirement: BIOS-01
    verification:
      - kind: unit
        ref: "ctest --test-dir build/hygiene -R '^hygiene\\.byte_run\\.' exit 0 (3 tests); sh scripts/hygiene.sh --tree exit 0; threshold lowered to 5 made --tree exit 1 naming tests/libretro/controls.h, restored exit 0"
        status: pass
    human_judgment: false
  - id: D4
    description: "The staged and history branches of hygiene.sh run the window scan over staged blobs and the blobs of the commits about to be pushed when a dump directory is set, and each is controlled by a fixture that fails on the window line"
    requirement: BIOS-01
    verification:
      - kind: integration
        ref: "ctest --test-dir build/hygiene -R '^policy\\.firmware_window\\.(staged|history)$' exit 0; deleting each branch's call from a throwaway copy made hygiene.sh exit 0 where the control expects 1"
        status: pass
    human_judgment: false
  - id: D5
    description: "The first hosted dispatch sees the exact inventories, the new hygiene tests, release.firmware_scan on the real zips of every build leg and the hygiene job inside its timeout; be32, wasm32 and perf could not start because the runners could not resolve the Ubuntu package archives"
    requirement: BIOS-01
    verification:
      - kind: other
        ref: "runs 43047222253, 43049044281 and 43051607563 on head 947ac8e: hygiene, six build legs, asan, two nofp and retroarch success on all three, release.firmware_scan Passed 18 of 18, be32 and wasm32 cancelled and perf failed at their apt-get step in all three (main run 43053345486 fails perf at the same step); the dispatch is not green, so the plan's all-legs criterion is not met"
        status: unknown
    human_judgment: false
---

# Phase 10 Plan 02: Window scan, retyped-bytes tripwire and hooks Summary

The second firmware legality scan exists: `scripts/firmware-window.sh` rolls a 32-byte window of the owner's own dump through the tree, the build outputs and the release zips (raw bytes and byte literals) and prints only an ordinal, a size and a target name; `hygiene.sh` gains a retyped-bytes tripwire and runs the scan from the commit and push hooks; each has controls that fail and pass in the Linux hygiene lane, and the first hosted dispatch proved the scans on every leg that could start.

## Performance

- **Duration:** about 1 h 10 min, most of it three hosted dispatches waiting on jobs that could not reach the package archives
- **Completed:** 2026-11-11
- **Tasks:** 3
- **Files:** 2 created, 8 modified

## Accomplishments

- `scripts/firmware-window.sh` (35 counted lines): `DUMPDIR` from the argument, the environment or the one line of `.env.local` (read with `sed -n`); every regular, non-symlink file of at most 256 KiB in a flat directory is a dump (others are counted as ignored); windows with fewer than 8 distinct values never load; each target is read as raw bytes and, when text, as the even-length hex runs it holds in order; output is `firmware window: dump N (B bytes) matches <target>` per hit, `firmware window: K compared, I ignored` when clean, `firmware window: no dump configured, 0 compared` when none is set. Default targets are the tree (`git ls-files --cached --others --exclude-standard` minus `bios/ roms/ private/` and the dump directory), `build/*/libcogmaster.a`, `cogmaster-run*`, `cogmaster_libretro.*` and each `cogmaster-*.zip` extracted into a trap-cleaned scratch directory.
- `tests/policy/firmware_window_cases.cmake` (73 lines, fixtures and registrations only, hygiene lane only): a self-authored 8 KiB dump (`byte i = (i * 37 + 11) mod 256`, generated with the shell), planted slices, and ten controls. The four that fail match the whole output `^firmware window: dump 1 (8192 bytes) matches [^ ]+\n$`.
- `scripts/hygiene.sh` (85 to 95 lines, 10 counted): `byte_run` in `check_text` (4 lines) and the hook (6 lines: one function fed `rev:path` names, one call per branch).
- `local.firmware_window_scan` in the library preset (label `local`, 300 s limit, no skip property).
- Three `hygiene.byte_run.*` controls: 300 literals fail, 255 pass, 255 split by one prose line pass.
- Timeouts of the 12 tests of plan 01 and the 13 of this plan settled at the 20 s floor with their measured times; the hygiene job's timeout raised to 3 minutes.

## Task Commits

1. Task 1, the script, the eight direct controls and the library registration: `c31c6c8`
2. Task 2, byte_run, the staged and history hook branches and their five controls: `947ac8e`
3. Task 3, the settled timeouts and the hygiene job's `timeout-minutes`: `45def49`

## Hosted dispatch D1

Commands from the repository root. Head dispatched: `947ac8e180d232a46628d3ea7f5c7fecefc88693` (after `git push origin phase/10-bios-boot`, exit 0). The dispatched head differs from the final head only in the four files of the settle commit `45def49` (the three registration files' comments and `.github/workflows/ci.yml`'s hygiene timeout line); `git diff --stat 947ac8e HEAD -- . ':(exclude).planning'` lists exactly those.

| Attempt | Dispatch and run | Conclusion |
| ------- | ---------------- | ---------- |
| 1 | `gh workflow run ci.yml --ref phase/10-bios-boot` exit 0, https://github.com/szTheory/cogmaster/actions/runs/43047222253 | failure |
| 2 | same command, https://github.com/szTheory/cogmaster/actions/runs/43049044281 | failure |
| 3 | same command, https://github.com/szTheory/cogmaster/actions/runs/43051607563 | failure |

Each run was found by head SHA (`gh run list --workflow ci.yml --branch phase/10-bios-boot --event workflow_dispatch --json databaseId,headSha`, filtered with `jq`), never by `--limit 1`.

Per-job conclusions, identical in all three runs: `changes`, `hygiene`, `build (linux-x64)`, `build (linux-arm64)`, `build (macos-x64)`, `build (macos-arm64)`, `build (windows-x64)`, `build (windows-arm64)`, `asan`, `nofp (ubuntu-26.04)`, `nofp (ubuntu-26.04-arm)` and `retroarch` succeeded; `title`, `cleanup`, `rehearsal` and `hash-equality` were skipped; `perf` failed, `be32` and `wasm32` were cancelled by their `timeout-minutes`, and `CI required` failed because of those three.

The three failures are the same fault and are not this branch's: perf, be32 and wasm32 are the jobs whose first step is `sudo apt-get update && sudo apt-get install` of valgrind, the powerpc cross compiler and the WASI sysroot, and in every attempt the log shows `Temporary failure resolving 'azure.archive.ubuntu.com'`, `'archive.ubuntu.com'` and `'security.ubuntu.com'` (packages.microsoft.com resolved in the same step; the same three hosts answered 200 from the agent's machine at the time). A dispatch of `main` at `202f107` (run https://github.com/szTheory/cogmaster/actions/runs/43053345486, started 15:56 UTC) failed its perf job at the same `Cachegrind` step. The last green runs of this workflow (main push run 43021845129 at 12:36 UTC) predate it. No code or workflow line of this plan is on that path, so there is nothing to fix on the branch; the three jobs have not run on this branch.

Evidence the dispatch did give, all from the exact inventories (602 tests in the ci lane, 63 in the hygiene lane):

- `release.firmware_scan` Passed on all six build legs in all three runs, 18 of 18 (`grep -E 'release\.firmware_scan .*Passed'` over each run's log), the slowest 0.79 s on `build (macos-x64)`. The 12 tests of plan 01 passed on all seven ci-lane executions per run (six build legs and asan).
- The 13 new hygiene-lane tests passed on the hygiene job in all three runs.
- Hygiene job durations (`completedAt` minus `startedAt`): 42 s (run 43047222253), 63 s (run 43049044281), 48 s (run 43051607563); the same job took 42 s and 55 s on the last two main and phase 9 runs, so the spread is the runner's. Twice the slowest, 126 s, is above the old 2-minute limit, so `.github/workflows/ci.yml` changed from `timeout-minutes: 2  # 2 x 52 s, slowest of 40 runs, run 39515950108` to `timeout-minutes: 3  # 2 x 63 s, slowest of 3 runs, run 43049044281`; `ctest -R '^hygiene\.timeout_comments'` exits 0 on the new line. `hygiene.tree` itself took 26 s, 35 s and 33 s in the three runs (26.31, 35.27 and 32.74 s).
- Settled timeouts (all at the 20 s floor, N counted in test executions across lanes and runs, be32 and wasm32 unmeasured because those jobs never reached their tests; the policy and release tests are CMake and shell scripts that do not run under qemu): the failing and self-test controls of the digest tables and the release scan 0.29 s of 231; `policy.bios_table` 0.27 s of 21; `policy.bios_dir_reads` 1.39 s of 21; the passing scan controls 0.10 s of 63; `release.firmware_scan` 0.79 s of 18; `policy.firmware_window.*` 0.16 s of 30; `hygiene.byte_run.*` 0.10 s of 9. The word provisional is gone from the three registration files (`grep -c provisional` is 0, 0 and the 2 pre-existing lines of the timeout-comment self-test fixture).

The plan's D1 criterion (every leg green, `conclusion` success) is therefore not met, and the exact criteria that need the be32, wasm32 and perf legs (the exact inventories on be32 and wasm32, the perf rows) are untested hosted for these commits. Hand-off: re-dispatch `ci.yml` on `phase/10-bios-boot` once the runners can resolve the Ubuntu archives (D2 after plan 05 is the next dispatch and covers those legs anyway); nothing in this plan changes for them.

## Evidence

All commands from the repository root; exit statuses as observed.

- `cmake --workflow --preset hygiene`: exit 0 after tasks 1 and 2 and the settle commit (58, 63 and 63 tests); `cmake --workflow --preset ci`: exit 0 after each task (602 tests plus 5 release tests).
- `ctest --test-dir build/hygiene -N -R '^policy\.firmware_window\.'` prints `Total Tests: 8` after task 1 and `-R '^(hygiene\.byte_run\.|policy\.firmware_window\.)'` prints `Total Tests: 13` after task 2.
- `grep -rl firmware_window tests/inventory` prints `tests/inventory/hygiene.txt` and `tests/inventory/library.txt` (the second holds only `local.firmware_window_scan`, whose name contains the string; the ten controls are in hygiene.txt only); `grep -c '^local.firmware_window_scan	local$' tests/inventory/library.txt` prints 1.
- Library preset: `cmake --preset library` and the build exit 0 with `COGMASTER_ROM_DIR` pointed at an empty temporary directory (the preset demands an existing directory; no owner path was used); `ctest --test-dir build/library -R '^local\.firmware_window_scan$' -V` exits 0 and prints `firmware window: no dump configured, 0 compared`; `grep -Eq 'SKIP_RETURN_CODE|SKIP_REGULAR_EXPRESSION' tests/tests.cmake` exit 1; `grep -rn COGMASTER_BIOS_DIR tests` prints nothing (exit of the negated test 0); `ctest --test-dir build/ci -R '^policy\.bios_dir_reads$'` exit 0.
- One-shot, self-authored dump only (never a real one): `env COGMASTER_BIOS_DIR=<dump dir> sh scripts/firmware-window.sh "" <file holding a 64-byte slice>` exit 1 with exactly one output line (`wc -l` prints 1) and `grep -c <dump dir>` over it prints 0; the clean file exits 0 with `1 compared, 0 ignored` and 0 path matches. With the dump directory set and no arguments the whole default target set (725 tree files, the 13 MB vector sample, every build directory's library and runner, the three zips) scans clean, exit 0 in 46 s.
- Portability: the script's four outputs (hit, clean, unset, 1 MB target) are identical on macOS (BSD od, BWK awk) and in a `gcc:13` container (GNU od, mawk, dash); the generated dumps have identical `cksum` on both (1424017787 for the 8 KiB dump); the `byte_run` 300-literal and 255-literal cases and `hygiene.sh --tree` give exit 1, 0 and 0 in the same container.
- Mutation controls on the script (each confirmed to change the file, run through the ten fixtures, the original untouched): removing the 8-distinct-values test made the 40-byte run in the low dump match (`low_entropy` would go red); stopping the load loop one byte early made the end window go unmatched (`end` would go red); narrowing the window to 31 bytes in the load and the scan made the 31-byte slice match (`w31` would go red); reducing text with `cat` instead of the literal filter kept the array detected only because `tr` already splits on `x` and the dollar sign, so the sed reduction was dropped as redundant.
- byte_run threshold control: `python3` edit of `run >= 256` to `run >= 5`, `git diff --stat` shows 1 file changed, `sh scripts/hygiene.sh --tree` exit 1 naming `tests/libretro/controls.h`; the saved real file restored and `--tree` exit 0 (see Deviations for why the threshold is 5 and not 150).
- Hook branch controls: in throwaway repositories under `build/hygiene/policy/fwin`, `hygiene.sh --staged` exit 1 and `hygiene.sh --history HEAD` exit 1 with the self-authored dump set; after deleting each branch's call line from a copy of the script they exit 0 and 0.
- Order: `test -z "$(git diff --name-only origin/main...HEAD -- include src runner libretro 'host/bios.[ch]')"` exit 0 after each task and at the end; `git diff --quiet origin/main -- tests/io tests/vdp tests/bus tests/audio tests/ports tests/testcard_pins.cmake` exit 0.
- `ctest --test-dir build/ci -R '^(policy\.|docs\.|release\.)'` exit 0 (65 tests).

## Order evidence so far

```
test -z "$(git diff --name-only origin/main...HEAD -- include src runner libretro 'host/bios.[ch]')"   exit 0
git merge-base --is-ancestor c31c6c8 HEAD   exit 0   (the window scan and its controls)
git merge-base --is-ancestor 947ac8e HEAD   exit 0   (byte_run and the hook)
git merge-base --is-ancestor 45def49 HEAD   exit 0
```

Plan 01's release scan (`f69406e`) and digest table (`c7ef931`) precede these; no file under include, src, runner, libretro or host/bios has changed on the branch, so both BIOS-01 scans and their negative controls exist before any BIOS code. Plan 03 records the final ordering.

## Budgets

`cmake -DROOT=$PWD -P tests/cmake/budgets.cmake`: exit 0, **833 counted check lines** / 9237 src lines (baseline 788 after plan 01: script +35, hygiene.sh +10). The plan 02 cap is 845 and the phase end is 847, so 14 lines of the phase allowance remain, 2 of them for plan 10. `tests/policy/firmware_window_cases.cmake` is 73 lines (cap 120) and `tests/policy/firmware_release_cases.cmake` is still exactly 120 (comments only changed).

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] The planned scan shape was quadratic on the agent's awk**
- **Found during:** Task 1, first default-target run of the script (it did not finish in 4 minutes on the 13 MB vector sample)
- **Issue:** sliding `substr` over one long hex string is linear per call in BWK awk, so 1 MB of target took 56 s; the whole tree would have taken hours. The hosted hygiene job would not notice (mawk and gawk are constant time).
- **Fix:** the shell prints `od` lines and `N`, `T` and `I` marker lines; awk rolls a 32-byte window through every byte (stride 1, one hash lookup per byte), counts distinct values in a dump window incrementally, and never holds a string longer than 64 characters. Same detection (every window of a dump against every window of a target), 1 MB in 1.1 s, the default target set in 46 s.
- **Files modified:** scripts/firmware-window.sh
- **Commit:** c31c6c8

**2. [Rule 1 - Bug] The plan's dump could not make two controls fail on a broken scan**
- **Found during:** Task 1, writing `low_entropy` and `end`
- **Issue:** the dump repeats every 256 bytes and every window of it has 32 distinct values, so a 40-byte run of one byte could not match even without the entropy filter, and the last 32 bytes also occur 31 other times, so a scan that dropped its last window would still pass `end`.
- **Fix:** `low_entropy` scans against its own 64-byte single-value dump (it matches without the filter) and `end` against a dump that switches to step 91 at byte 8160 (the last window occurs once); both mutations were run and make the control go red.
- **Files modified:** tests/policy/firmware_window_cases.cmake
- **Commit:** c31c6c8

**3. [Rule 1 - Bug] The plan's byte_run negative control names numbers the stated rule cannot produce**
- **Found during:** Task 2, the threshold control
- **Issue:** the plan expects `run >= 150` to fail on `src/opll_tables.inc` (152 literals), but that file's rows are decimal; under the rule as written (only two-digit `0x??` and `$??` tokens) the longest run in the tree is 9, in `tests/libretro/controls.h`, so 150 changes nothing.
- **Fix:** the one-shot control lowers the threshold to 5 and sees exit 1 naming `tests/libretro/controls.h`; the rule itself is as the plan states and the real tree passes at 256 with room.
- **Files modified:** none (the control is a one-shot)
- **Commit:** none

### Interpretations of the plan (no behaviour lost)

- The text-literal reduction is `tr -cs '0-9a-fA-F' '\n' | grep -E '^([0-9a-fA-F]{2})+$'` (every even-length run of hex digits, in order, joined), not a `sed` rewrite of `0x` and `$`: the non-hex characters already separate those prefixes, so the sed removed nothing. Odd-length runs (offsets, decimal words) are dropped without breaking the run, so `od` and `xxd` listings reduce to their bytes.
- The unset control runs in a throwaway repository with the variable removed, because the script also reads the working repository's `.env.local` and a developer's own setting would change a control that expects 0 compared.
- Both hook branches write each blob to a name made from its `rev:path` inside the trap-cleaned directory, so a hit prints that temporary path (a sanitised `rev:path`) and not the repository path; the control matches the window line only.
- The plan's acceptance line `grep -rl 'firmware_window' tests/inventory` showing only hygiene.txt cannot hold, since `local.firmware_window_scan` is in library.txt by the plan's own registration; the ten controls are in hygiene.txt only, as the plan requires.
- Hosted D1: not green, for the reason in its section; the settled timeouts and the hygiene timeout were taken from the legs that ran, the be32 and wasm32 times do not exist.

**Total deviations:** 3 auto-fixed bugs, 0 architectural. **Impact:** none on the plan's outputs except the open hosted criterion.

## Phase notes

- testing.md rows (and the statement that base64, decimal lists, XOR and compression are not covered by the window scan, nor retyped decimal arrays by byte_run) land in plan 06; PROVENANCE and the env example in plan 08; embedding.md edits in plans 04 and 05; the final timeouts of the host, runner and libretro tests in plan 11.
- Carried to plan 03 and the next dispatch: the be32, wasm32 and perf legs have not run on this branch; the runners' package-archive DNS failure must clear (or the dispatch be repeated) before D2 can be read.
- The window scan's default target set is about 46 s on the agent's machine with this tree and its build directories; `local.firmware_window_scan` has a 300 s limit.

## Known Stubs

None.

## Threat Flags

None.

## Self-Check: PASSED

- scripts/firmware-window.sh and tests/policy/firmware_window_cases.cmake exist.
- Commits c31c6c8, 947ac8e and 45def49 are ancestors of HEAD (`git merge-base --is-ancestor` exit 0 for each).
