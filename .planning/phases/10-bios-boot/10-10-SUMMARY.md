---
phase: 10-bios-boot
plan: "10"
subsystem: libretro
tags:
  - bios
  - libretro
  - firmware
  - docs
status: complete

requires:
  - 10-05
  - 10-07
  - 10-08
  - 10-09
provides:
  - the libretro option cogmaster_boot (fast by default) that reads one of bios_U.sms, bios_E.sms or bios_J.sms from the system folder by resolved region and TV, with one warning and a fast boot on every refusal
  - three optional firmware rows in the core's information file, each mapped to libretro.host.bios by the claims check, and a self-test that the mapping fails without that test
  - libretro.host.bios, an in-process test whose BIOS frame equals the runner's for the same self-authored image
  - docs/bios.md and one README sentence
  - hosted dispatch D3, green on every lane, with the measured times for plan 11
affects:
  - 10-11

actuals:
  tokens: 5900
  tasks: 3
  commits: 3

plan_head_before: 4fcc52157296bb2b12fa2db6552cca4d4c4e4ae8
plan_head_after: a89107aeedf33f8aa6c61a95a20393a8f930416e
commits: 3

tech-stack:
  added: []
  patterns:
    - "A one-leg-per-call host mode driven by a cmake script: the script builds the system folders, asks the runner for every digest, and the host checks digest, folder calls, log lines and the word the line must hold"
    - "File names for the test folders are read from the built information file, never typed in the test, so a drifted name fails the test"
    - "A claim row family normalised in one awk statement: firmware[0-9]+_opt maps to one claim"

key-files:
  created:
    - tests/libretro/bios.h
    - docs/bios.md
  modified:
    - libretro/libretro.c
    - libretro/options.h
    - libretro/cogmaster_libretro.info.in
    - tests/cmake/repository.sh
    - tests/libretro/host.c
    - tests/libretro/tests.cmake
    - tests/inventory/ci.txt
    - README.md

key-decisions:
  - "The adapter names the file in its one warning and never the folder; a Game Gear game, an absent or empty or too long folder, an absent or wrong-sized file and a false answer from the frontend all give the same fast boot"
  - "With no content the option does nothing and logs nothing: there is no cartridge to start, so the test card loads as before"
  - "The region picks the file in the order Japan, PAL, export; PAL is the TV the option sets, so a European user with only bios_E.sms and the default TV gets fast boot plus a warning"
  - "The two libretro tests take the 20 s floor: the slowest hosted run of libretro.host.bios was 1.68 s (be32) and of the claim self-test 0.27 s"

requirements-completed:
  - BIOS-04

coverage:
  - id: L1
    description: "The core option cogmaster_boot (Boot, reload content) has the values fast and bios, default fast, in all three option tables with the count raised from 11 to 12 and the static asserts kept; with the option fast no system folder is asked for and a hostile three-byte file in it is never read and logs nothing"
    requirement: BIOS-04
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^libretro\\.host\\.bios$' exit 0 (legs fast with the file present and fast with three-byte files: folder calls 0, log lines 0, the fast digest); cmake --build --preset ci exit 0 with the static asserts"
        status: pass
    human_judgment: false
  - id: L2
    description: "With the option bios and the right 8,192-byte file in the system folder, the delivered frame inside the BIOS phase equals the runner's --boot bios --bios frame for the same file and region and differs from the fast frame; the file is chosen by resolved region and TV (Japan bios_J.sms, PAL bios_E.sms, everything else bios_U.sms) and no name falls back to another"
    requirement: BIOS-04
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^libretro\\.host\\.bios$' exit 0, nine region by folder legs and the default leg; control: the PAL name changed to the export name made the pal leg exit non-zero and the restore made it exit 0; run 43074158512 passed it on 8 lanes"
        status: pass
    human_judgment: false
  - id: L3
    description: "Every refusal (no folder, a false answer, NULL, an empty string, a path no 512-byte buffer holds, an absent file, a file of 3 or 8,193 bytes, a Game Gear cartridge) gives the fast digest and exactly one log line that names the file and never the folder; a Game Gear cartridge makes no system-folder call; options are registered once"
    requirement: BIOS-04
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^libretro\\.host\\.bios$' exit 0; control: the adapter asking for the folder before the Game Gear refusal made the leg exit non-zero (folder calls 1, expected 0) and the restore made it exit 0"
        status: pass
    human_judgment: false
  - id: L4
    description: "The information file declares firmware_count 3 and three optional rows (path, description, opt) for bios_U.sms, bios_E.sms and bios_J.sms, a notes line with no digest, no bios.gg, and every earlier claim kept; the claims check maps firmware[0-9]+_opt to libretro.host.bios and a one-line fixture claiming firmware0_opt against an inventory without that test fails naming the claim"
    requirement: BIOS-04
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^libretro\\.info_claims' exit 0 (4 tests: the claims check, its two self-tests and the new firmware self-test); grep -cE '^firmware[0-2]_(desc|path|opt) = ' on the information file prints 9; the grep for md5 is empty"
        status: pass
    human_judgment: false
  - id: L5
    description: "docs/bios.md (7,132 bytes) carries the behaviour in plain words with no planning identifier and no digest: fast against BIOS boot, the dump and its size, the runner flags and exits, the libretro option and its three file names, what locks up and why, the Reset and 0066 note, the arbitration table with its model rows labelled, the limit of the CI image and how it is tested; the README gains one sentence after the guarded sentence (which still occurs once), one Documentation bullet and the exit-77 clause, no second command line and no new extension token"
    requirement: BIOS-05
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^(docs\\.|policy\\.)' exit 0 including docs.readme_play; cmake -DROOT=$PWD -P tests/cmake/budgets.cmake exit 0; the planning-identifier grep over docs/bios.md and README.md prints 0 for each; grep -c for the guarded sentence prints 1"
        status: pass
    human_judgment: false
  - id: L6
    description: "No hosted RetroArch BIOS session was added: tests/retroarch and tests/fixtures/play.h are unchanged; the hosted dispatch D3 is green on every job, including the retroarch job, the eight-lane hash-equality job (with the BIOS device row of the runner plan) and the build, asan, nofp, wasm32, be32 and perf jobs"
    requirement: BIOS-04
    verification:
      - kind: integration
        ref: "gh run view 43074158512 --json conclusion exit 0 printing success; the jq filter over the named jobs exit 0; git diff --quiet against the merge base over tests/retroarch and tests/fixtures/play.h exit 0; ctest -R '^policy\\.e2e_budget' exit 0 (3 tests)"
        status: pass
    human_judgment: false

duration: about 75 minutes
completed: 2026-11-11
---

# Phase 10 Plan 10: RetroArch boots from a system-folder BIOS on request Summary

The libretro core now starts through `bios_U.sms`, `bios_E.sms` or `bios_J.sms` from RetroArch's system folder only when the player sets the new Boot option to BIOS, gives the runner's frame for the same self-authored image, and falls back to a normal fast boot with one warning on every refusal; the information file declares the three files as optional firmware, each claim is backed by a test, and the user guide and one README sentence say how it works. Hosted dispatch D3 is green on every lane.

## Accomplishments

- `libretro/options.h`: `cogmaster_boot`, "Boot (reload content)", values `fast` ("Fast boot (no BIOS)", the default) and `bios` ("BIOS from the system folder; PAL needs TV = PAL"); `LR_OPTION_COUNT` is 12 in the v2, v1 and v0 tables with their static asserts.
- `libretro/libretro.c`: `lr_bios` runs from `retro_load_game` after the machine, region, TV and FM options have settled `media`, only when content exists and the option is `bios`. A Game Gear machine reads nothing and logs one line. Otherwise the system folder (a false answer, NULL, an empty string or a path that would truncate a 512-byte `snprintf` is absent) is joined with exactly one name by region (Japan, then PAL, then the export file), read through `cogmaster__bios_read` into a static 8,193-byte buffer, and passed to the library only when it is exactly 8,192 bytes. Any other outcome logs one WARN line that names the file and never the folder and leaves the load on the fast boot. No digest, no recognition, no fallback between names, nothing from the content path or an option string, no other file access.
- `libretro/cogmaster_libretro.info.in`: `firmware_count = 3` and `firmware0..2_desc`, `_path`, `_opt = "true"`, plus a notes line (the firmware is optional, which region each file serves, a missing file means fast boot); no digest, no `bios.gg`, every earlier claim kept.
- `tests/cmake/repository.sh`: `info_claims` maps `firmware[0-9]+_opt` to `libretro.host.bios` by normalising the key in its awk and prints `firmware_opt needs libretro.host.bios, which is not in <inventory>`; no counted line added.
- `tests/libretro/host.c` and `tests/libretro/bios.h`: the fake frontend answers `GET_SYSTEM_DIRECTORY` from a settable folder (a call counter, and a mode that answers false while still filling the path in), carries the `cogmaster_boot` option (default `fast`) and keeps the last log line; the new `bios` mode is one leg per call: cartridge, frames, digest, boot, region, folder, expected folder calls, expected log lines, a word the log line must hold, and a check that the line never holds the folder.
- `tests/libretro/tests.cmake`: a script-mode branch builds the test folders at test time (one folder per file owner with the other two names wrong-sized, a three-byte folder, an 8,193-byte folder, an empty folder), reads the three names from the built information file, asks the runner for the BIOS and fast digests of each region and of the Game Gear cartridge, asserts that BIOS and fast differ, and runs 19 legs. `libretro.host.bios` and `libretro.info_claims.self_test.firmware` are registered; both entries are in `tests/inventory/ci.txt` only.
- `docs/bios.md` (7,132 bytes) and the README: see coverage L5.

## Task Commits

1. Task 1, the option, the wiring, the information rows, the claim mapping and `libretro.host.bios`: `24726ad`
2. Task 2, `docs/bios.md` and the README: `575739a`
3. Task 3, the two libretro timeouts settled from D3: `a89107a`

## Hosted dispatch D3

Dispatched on the head `575739a844d2a65d8ebfb10c5fcba70a7a3f2bf2` (the head after tasks 1 and 2); HEAD differs from it only by `tests/libretro/tests.cmake` (the settled timeouts and their comments), as `git diff --name-only 575739a844d2a65d8ebfb10c5fcba70a7a3f2bf2 HEAD -- . ':(exclude).planning'` shows.

- `git push origin phase/10-bios-boot` exit 0; `gh workflow run ci.yml --ref phase/10-bios-boot` exit 0; the run was found by head SHA with `gh run list ... --json databaseId,headSha,url` and `jq` (not `--limit 1`); `gh run watch 43074158512 --exit-status` exit 0. One attempt; no lane needed a re-dispatch, and no first-step DNS failure was seen.
- Run: https://github.com/szTheory/cogmaster/actions/runs/43074158512, conclusion success.
- Job conclusions, all success except the three that skip on a dispatch: changes, hygiene, perf, nofp (ubuntu-26.04-arm), nofp (ubuntu-26.04), asan, be32, wasm32, retroarch, hash-equality, `build (linux-arm64)`, `build (linux-x64)`, `build (macos-x64)`, `build (macos-arm64)`, `build (windows-x64)`, `build (windows-arm64)`, CI required; skipped: title, rehearsal, cleanup.
- `jq -e '[.jobs[] | select(.name | test("hash-equality|retroarch|asan|nofp|wasm32|be32|perf"))] | length >= 7 and all(.conclusion == "success")'` exit 0 (8 jobs); the same filter over `^build` exit 0 (6 jobs).
- The hosted retroarch job concluded success (the unchanged helper, 275 of 300 lines). The hash-equality job downloaded eight `hashes-*` artifacts of 3,403 bytes each (the device row of the runner plan is now compared across lanes) and concluded success.
- Perf (dispatch prints the control line only): `perf control boot_to_play runs=12288943,12288943,12288943 spread=0`. No delta against a fresh main dispatch was computed here; this plan's criterion is the perf job's success, and the adapter change does not touch the core's hot path.
- `libretro.host.bios` passed on 8 lanes in the log (`grep -E 'libretro\.host\.bios .*Passed'` counts 8 lines).

## Measured times for plan 11

Slowest hosted time per test and the number of runs read from the D3 job logs (`gh run view 43074158512 --log`); every run passed. All timeouts below are still provisional except the two libretro ones, which are settled here; plan 11 settles the rest as `max(20, ceil(2 x S))`.

| Test | Slowest | Lane | Runs |
|---|---|---|---|
| bios.hostile | 0.43 s | be32 | 9 |
| bios.equivalence | 1.42 s | be32 | 9 |
| bios.equivalence.bad_ram | 0.21 s | be32 | 9 |
| bios.equivalence.bad_sp | 0.21 s | be32 | 9 |
| fixtures.bios_shape.pin.bios_probe | 0.05 s | build (macos-x64) | 9 |
| fixtures.bios_shape.pin.bios_shape | 0.05 s | build (windows-arm64) | 9 |
| fixtures.bios_shape.pin.bios_shape_bad_ram | 0.06 s | build (windows-arm64) | 9 |
| fixtures.bios_shape.pin.bios_shape_bad_sp | 0.06 s | build (macos-x64) | 9 |
| fuzz.regress.loader | 0.34 s | build (macos-arm64) | 9 |
| fuzz.regress.run | 0.89 s | be32 | 9 |
| fuzz.regress.save | 2.26 s | asan | 9 |
| fuzz.regress.eeprom | 1.80 s | build (macos-x64) | 9 |
| fuzz.regress.pad | 2.85 s | be32 | 9 |
| fuzz.regress.bios | 0.19 s | asan | 9 |
| host.bios | 0.07 s | wasm32 | 9 |
| runner.bios.grammar | 0.61 s | be32 | 9 |
| runner.bios.refusals | 0.50 s | be32 | 9 |
| runner.bios.no_path_echo | 0.24 s | be32 | 9 |
| runner.bios.boot | 0.27 s | be32 | 9 |
| libretro.host.bios | 1.68 s | be32 | 8 |
| libretro.info_claims.self_test.firmware | 0.27 s | build (windows-arm64) | 7 |

Also read, for completeness: bios.arbitration 0.27 s (wasm32), bios.load 0.41 s, bios.slow_flag 0.48 s and bios.state 0.41 s (all be32), 9 runs each.

Settled here: `libretro.host.bios` and `libretro.info_claims.self_test.firmware` take `TIMEOUT 20` (the 20 s floor; 2 x 1.68 s and 2 x 0.27 s are below it) with measured comments citing run 43074158512; no trim was needed on be32. Settled in plan 11, from the table above: the host, runner, fixtures, bios and fuzz timeouts, none edited here.

## Evidence

All commands from the repository root; exit statuses as observed.

- `cmake --preset ci` exit 0; `cmake --build --preset ci` exit 0; `ctest --test-dir build/ci -R '^libretro\.(host\.bios|info_claims)' --output-on-failure` exit 0 (4 tests: `Total Tests: 4` by `ctest -N`).
- Negative controls, each a python edit of `libretro/libretro.c` with a successful rebuild (exit 0) and a red test, then restored from a copy and re-run green: (1) the PAL file name replaced by the export name, `libretro.host.bios` exit non-zero at the pal leg; (2) `GET_SYSTEM_DIRECTORY` asked before the Game Gear refusal, `libretro.host.bios` exit non-zero with `parse(a[6]) failed: 1 != 0`; (3) the claim without a test behind it is the registered `libretro.info_claims.self_test.firmware`, which passes by failing the check on every configure. `git diff --stat HEAD` could not show each edit as new (the file was already modified in the working tree during task 1), so each is tied to its build exit 0 and its red result.
- `/opt/homebrew/opt/llvm@21/bin/clang-format --dry-run -Werror libretro/libretro.c libretro/options.h tests/libretro/host.c tests/libretro/bios.h` exit 0.
- `docker run --rm -v "$PWD":/w -w /w gcc:13 gcc -std=c17 -Wall -Wextra -Wpedantic -Werror -Wtype-limits -Wconversion -fsyntax-only -DCOGMASTER_LIBRETRO_VERSION='"0"' -I libretro -I host -I runner -I src -I include -I build/ci/generated libretro/libretro.c tests/libretro/host.c` exit 0 (the first attempt without the version define failed on the missing define alone, which the build supplies).
- `grep -c 'bios.gg' libretro/cogmaster_libretro.info.in` prints 0; `grep -cE '^firmware[0-2]_(desc|path|opt) = '` prints 9; the md5 grep output is empty (`test ! -s` exit 0).
- `git diff -U0 $(git merge-base origin/main HEAD) -- libretro/libretro.c | grep '^+[^+]' | grep -E 'game->path|getenv|opendir|fopen' | wc -l` prints 0.
- `ctest --test-dir build/ci -R '^(docs\.|policy\.)'` exit 0 after task 2 (55 policy tests and the docs tests); `wc -c` prints 7,132 for `docs/bios.md` and 4,095 for `README.md`; the largest `docs/*.md` is `docs/testing.md` at 36,714 of 38,912; the grep for a 40-hex token over `docs/bios.md` prints 0.
- `cmake --workflow --preset ci` exit 0 (623 tests and the 5 of the second stage) and `cmake --workflow --preset hygiene` exit 0, after tasks 1, 2 and 3 on the final head.
- Dispatch D3 (above): run 43074158512 success.
- No real firmware byte was read, copied, logged or compared; every BIOS-shaped file is the self-authored image or a file of owned filler.

## Unchanged proof

- `git diff --quiet $(git merge-base origin/main HEAD) -- tests/io tests/vdp tests/bus tests/audio tests/ports tests/testcard_pins.cmake tests/fuzz/fuzz_run.c tests/fuzz/fuzz_loader.c tests/retroarch tests/fixtures/play.h` exit 0.
- `git diff --quiet origin/main -- tests/io tests/audio tests/testcard_pins.cmake` exit 0.

## Budgets

`cmake -DROOT=$PWD -P tests/cmake/budgets.cmake` exit 0: counted check code 833 (phase cap 847), src lines 9,399. The `info_claims` edit changes two lines and adds none. `docs/bios.md` is 7,132 bytes, `README.md` 4,095 of 12,288, and the largest doc 36,714 of 38,912.

## Deviations from Plan

### Interpretations (no behaviour lost)

- **No content, no log line.** The plan lists "no content" among refusals that give one warning and also says the adapter acts only when content exists. The adapter does nothing without content and logs nothing, since there is no cartridge to start and the test card loads as before.
- **One leg per host call.** The plan gives the host a list of arguments; the host takes one leg's setup and expectations per call and the script drives 19 legs, so the host needs no parser and the runner digests (BIOS and fast, per region and for the Game Gear cartridge) stay in the script.
- **Region by folder legs.** Every region against every owner folder is exercised (nine legs): the matching file gives the runner's BIOS frame with no log line, the other six give the fast frame of that region with a line naming the region's file.
- **A path that would truncate is tested as a refusal, not as a truncation.** The 600-character folder gives the fast digest and one line; a path whose truncated form names a real file cannot be built portably (MAX_PATH on Windows), so the truncation guard itself has no red control.
- **Both libretro timeouts at 20 s.** The plan formula `max(20, ceil(2 x S))` gives the floor for the claim self-test too, so its timeout is raised from the neighbours' 10 to 20.
- **Container compile needs the version define.** The build passes `COGMASTER_LIBRETRO_VERSION`; the standalone syntax check supplies a dummy.

**Total deviations:** 0 rule-driven fixes; 6 interpretations recorded above. **Impact:** none on the requirement.

## Known Stubs

None.

## Threat Flags

| Flag | File | Description |
|------|------|-------------|
| threat_flag: file_read | libretro/libretro.c | The adapter now opens one caller-influenced path read-only: the system folder the frontend names, joined with one of three fixed file names, read to at most 8,193 bytes into a static buffer; a path that would truncate, a false answer and a non-8,192 size are all refused with a fixed warning that never prints the folder |

## Phase notes

- docs/bios.md and the README sentence land here with the libretro behaviour; the closing plan verifies them and does not edit them. The sentence on the `.info` claim mapping is in `docs/bios.md`, not `docs/testing.md`.
- The two libretro timeouts are settled here from D3; the host, runner, fixtures, bios and fuzz timeouts are settled in plan 11 from the table above.
- Real-dump hand-off for the owner: nothing in this plan read a real BIOS, and a real BIOS boot under RetroArch (and the runner) is an owner run on their own machine,.

## Self-Check: PASSED

- This SUMMARY exists; `libretro/libretro.c`, `libretro/options.h`, `libretro/cogmaster_libretro.info.in`, `tests/libretro/bios.h` and `docs/bios.md` exist.
- Commits `24726ad`, `575739a` and `a89107a` are ancestors of HEAD (`git merge-base --is-ancestor` exit 0 for each).
