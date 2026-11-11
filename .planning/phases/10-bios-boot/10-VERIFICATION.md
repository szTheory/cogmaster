---
phase: 10-bios-boot
verified: 2026-11-11T00:00:00Z
status: passed
score: 5/5 must-haves verified
covered_files:
  - ".env.example"
  - ".github/workflows/ci.yml"
  - ".planning/phases/10-bios-boot/10-01-PLAN.md"
  - ".planning/phases/10-bios-boot/10-01-SUMMARY.md"
  - ".planning/phases/10-bios-boot/10-02-PLAN.md"
  - ".planning/phases/10-bios-boot/10-02-SUMMARY.md"
  - ".planning/phases/10-bios-boot/10-03-PLAN.md"
  - ".planning/phases/10-bios-boot/10-03-SUMMARY.md"
  - ".planning/phases/10-bios-boot/10-04-PLAN.md"
  - ".planning/phases/10-bios-boot/10-04-SUMMARY.md"
  - ".planning/phases/10-bios-boot/10-05-PLAN.md"
  - ".planning/phases/10-bios-boot/10-05-SUMMARY.md"
  - ".planning/phases/10-bios-boot/10-06-PLAN.md"
  - ".planning/phases/10-bios-boot/10-06-SUMMARY.md"
  - ".planning/phases/10-bios-boot/10-07-PLAN.md"
  - ".planning/phases/10-bios-boot/10-07-SUMMARY.md"
  - ".planning/phases/10-bios-boot/10-08-PLAN.md"
  - ".planning/phases/10-bios-boot/10-08-SUMMARY.md"
  - ".planning/phases/10-bios-boot/10-09-PLAN.md"
  - ".planning/phases/10-bios-boot/10-09-SUMMARY.md"
  - ".planning/phases/10-bios-boot/10-10-PLAN.md"
  - ".planning/phases/10-bios-boot/10-10-SUMMARY.md"
  - ".planning/phases/10-bios-boot/10-11-PLAN.md"
  - ".planning/phases/10-bios-boot/10-11-SUMMARY.md"
  - "ASSET_POLICY.md"
  - "CMakeLists.txt"
  - "PROVENANCE.md"
  - "README.md"
  - "THIRD-PARTY-NOTICES.md"
  - "docs/bios.md"
  - "docs/embedding.md"
  - "docs/perf-fuzz.md"
  - "docs/runner.md"
  - "docs/testing.md"
  - "host/bios.c"
  - "host/bios.h"
  - "host/bios_table.inc"
  - "include/cogmaster.h"
  - "libretro/cogmaster_libretro.info.in"
  - "libretro/libretro.c"
  - "libretro/options.h"
  - "quality.json"
  - "runner/args.c"
  - "runner/args.h"
  - "runner/exit_status.h"
  - "runner/main.c"
  - "scripts/firmware-window.sh"
  - "scripts/hygiene.sh"
  - "src/boot.c"
  - "src/bus.c"
  - "src/instance.c"
  - "src/internal.h"
  - "src/io.c"
  - "src/state.c"
  - "tests/abi/tests.cmake"
  - "tests/api/v13.c"
  - "tests/api/v14.c"
  - "tests/api/v9.c"
  - "tests/bios/test_bios.c"
  - "tests/bios/tests.cmake"
  - "tests/built/expected.txt"
  - "tests/cmake/firmware_scan.cmake"
  - "tests/cmake/hashes.cmake"
  - "tests/cmake/repository.sh"
  - "tests/core/test_core.c"
  - "tests/core/test_state.c"
  - "tests/fixtures/tests.cmake"
  - "tests/fuzz/fuzz_bios.c"
  - "tests/fuzz/seeds.c"
  - "tests/fuzz/tests.cmake"
  - "tests/host/test_bios.c"
  - "tests/host/tests.cmake"
  - "tests/hygiene/tests.cmake"
  - "tests/inventory/ci.txt"
  - "tests/inventory/fuzz.txt"
  - "tests/inventory/hygiene.txt"
  - "tests/inventory/library.txt"
  - "tests/inventory/nofp.txt"
  - "tests/inventory/wasm32.txt"
  - "tests/libretro/bios.h"
  - "tests/libretro/host.c"
  - "tests/libretro/tests.cmake"
  - "tests/policy/firmware_release_cases.cmake"
  - "tests/policy/firmware_window_cases.cmake"
  - "tests/policy/repository-checks.txt"
  - "tests/policy/tests.cmake"
  - "tests/roms/manifest.txt"
  - "tests/roms/src/bios_shape.asm"
  - "tests/roms/src/bios_shape.link"
  - "tests/runner/tests.cmake"
  - "tests/tests.cmake"
covered_digest: "v4:sha256:752ebdb671c57122807ce314e4e5c063bee3c170fc3b1839c71fd9d6ea731adb"
behavior_unverified: 0
overrides_applied: 0
---

# Phase 10: BIOS boot Verification Report

**Phase Goal:** A user boots from their own BIOS dump in the runner or RetroArch, fast boot stays the default, BIOS and cartridge arbitration follows the documented hardware, and no BIOS byte ever enters the tree or a release.
**Verified:** 2026-11-11
**Status:** passed
**Re-verification:** No, initial verification
**Branch / HEAD:** `phase/10-bios-boot` at bd09bb4 (52 commits over `origin/main`)

Human verification items: none for phase completion. Every check below is a test, a command I ran, a negative control I
mutated, or a hosted run. One item is a documented owner hand-off and is not a gap (see "Carried hand-off").

The legal rule held for this verification: I read no BIOS dump, no owner library and no `.env.local` (the file does not
exist in this checkout, and `COGMASTER_BIOS_DIR` is unset). The window scan was exercised only against a random
scratch "dump" I generated in the scratchpad.

## Goal Achievement

### Observable Truths (ROADMAP success criteria, criteria 1, 2 and 4 as amended)

| # | Truth | Status | Evidence |
|---|-------|--------|----------|
| 1 | The release firmware scan runs in `ci`; the local firmware-window scan runs in the `library` preset and in the pre-commit and pre-push hooks; each has a negative control that runs in CI (release scan's in `ci`, window scan's in `hygiene`); both land before any BIOS code (BIOS-01) | VERIFIED | Release scan: `tests/cmake/firmware_scan.cmake` (digest of each file in the release zips and manifest files against `host/bios_table.inc` and `src/cartdb.inc`, copier-header sizes, vacuity guards) registered as `release.firmware_scan` plus eleven `policy.firmware_scan.*` controls in the `ci` inventory; `policy.bios_table*` and `policy.bios_dir_reads*` keep the table equal to ASSET_POLICY and ban directory reads in tests. Window scan: `scripts/firmware-window.sh` (32-byte windows, at least 8 distinct values, over tree, build outputs and zips; fails on an unusable directory, empty directory, unreadable target; only "no dump configured" exits 0 with `0 compared`); `local.firmware_window_scan` in `tests/tests.cmake` under `COGMASTER_LIBRARY` and in `tests/inventory/library.txt`; `scripts/hygiene.sh` `fw_scan` is called from `--staged` and the history mode and `.githooks/pre-commit` and `pre-push` exec `hygiene.sh`; 17 `policy.firmware_window.*` controls plus the byte-run tripwire controls live in `tests/hygiene/tests.cmake` and `tests/inventory/hygiene.txt`. I mutated `firmware_scan.cmake` (digest hit turned into a status message): `policy.firmware_scan.whole`, `.cartdb`, `.manifest` fail (3 of 11); restored. I ran the window scan by hand with a random 8 KiB scratch dump: a clean target exits 0 (`1 compared, 0 ignored`), a target containing 40 bytes of the dump exits 1 naming dump and target, a missing directory exits 2, an empty directory exits 2, no configuration exits 0 with `0 compared`. Order: `git merge-base --is-ancestor` of f69406e (release scan), c31c6c8 (window scan) and 947ac8e (hooks) against 0d8a598 (first header change, the first BIOS code) all exit 0; the first BIOS-bearing commit in the log is 0d8a598. |
| 2 | `cogmaster-run --boot bios --bios FILE` boots the image, the dump is named by SHA-1, fast boot stays the default with every existing hash unchanged, and the runner exits 77 when a BIOS is required and absent or not supported (BIOS-02) | VERIFIED | `runner/args.c` (`--boot fast\|bios`, `--bios FILE`, strict grammar, `--bios` without `--boot bios` is exit 2), `runner/main.c` `bios_load` (Game Gear cartridge, no `--bios`, missing file, wrong size all exit 77 with fixed text; unreadable file exit 1), `host/bios.c` and `host/bios_table.inc` (14 public digest rows, label only: a right-sized unknown dump still boots and prints `BIOS not recognised (sha1 ...)`). By hand with the owned images: default and `--boot fast` give the same frame hash; `--boot bios --bios bios_shape.bin --machine sms1` exits 0 and prints `BIOS not recognised (sha1 12a26ac0...)`, twice with the same hash; no `--bios` 77; nonexistent path 77; a 64 KiB cartridge file given as the BIOS 77; `--machine gg` 77; `--bios` alone 2. Existing hashes: `git diff --quiet origin/main -- tests/io tests/vdp tests/bus tests/audio tests/ports tests/testcard_pins.cmake tests/fuzz/fuzz_run.c tests/fuzz/fuzz_loader.c` exits 0; `git diff main...HEAD -- tests/built/expected.txt tests/diag/expected.csv tests/roms/manifest.txt tests/compat \| grep -c '^-[^-]'` prints 0 (additions only); the only removed test lines are the version and size pins. Mutation: absent-BIOS refusal changed from exit 77 to 1: `runner.bios.refusals` and `runner.bios.no_path_echo` fail; restored. |
| 3 | CI runs a self-authored BIOS-shaped image with no skip, and a check shows the fast-boot hand-off state equals the state that image reaches (BIOS-03) | VERIFIED | `tests/roms/src/bios_shape.asm`/`.link` (240 lines, MIT, built from source; digests of the image, probe cartridge and two bad variants pinned in `tests/built/expected.txt`; none committed as bytes, listed in the manifest comment). `bios.equivalence` compares the whole state record field by field, with every record name classified (an unclassified name fails: "equivalence: unclassified field"), plus controls `bios.equivalence.bad_ram` and `.bad_sp`. 26 tests match `bios`, none has a skip return code (grep of `tests/bios/tests.cmake`, `tests/fixtures/tests.cmake`: no skip); `ctest -R 'bios\|firmware'` 37 of 37 passed. Mutation: fast-boot hand-off SP changed from `0xDFF0` to `0xDFF1` in `src/boot.c`: `bios.equivalence`, `bios.equivalence.bad_ram` and `bios.load` fail (3 of 8); restored. Hostile images are bounded (`bios.hostile`, `fuzz.regress.bios`); nightly run 43076276567 passed `fuzz.libfuzzer.bios` (61.13 s). |
| 4 | The libretro `.info` declares the firmware as optional (three Master System file names; the Game Gear BIOS is a seed), and an in-process host test shows the adapter reads it from the system directory when the boot option asks for it (BIOS-04) | VERIFIED | `libretro/cogmaster_libretro.info.in`: `firmware_count = 3`, `bios_U.sms`, `bios_E.sms`, `bios_J.sms`, each `firmware*_opt = "true"`, no Game Gear row; `libretro/options.h` `cogmaster_boot` (default `fast`); `libretro/libretro.c` `lr_bios` reads only `GET_SYSTEM_DIRECTORY` plus a fixed name by resolved region and TV, refuses a Game Gear, one warning naming the file and never the folder, falls back to fast boot. `libretro.host.bios` (in-process, `tests/libretro/bios.h`) and `libretro.info_claims` (every firmware row needs that test, with `.self_test.firmware` control) pass. Mutation: `bios_E.sms` renamed in the adapter: `libretro.host.bios` fails; restored. |
| 5 | SMS-SYSTEM section 3 is checked against the code, and a test exercises each BIOS and cartridge enable combination under `$3E` with the documented result (BIOS-05) | VERIFIED | `.planning/preparation/SMS-SYSTEM.md` is amended (9 insertions, 6 deletions) and `docs/bios.md` "Who answers a read" states the per-machine table. `src/bus.c` `mem_slow_driven`: SMS1 cartridge wins, SMS2 ANDs, BIOS-only answers `$0000-$1FFF` only, neither is undriven; one decoder for read and `cogmaster_peek`; the hot-path flag `mem_slow` is recomputed by `cogmaster__bus_decode` from `io_out`, `io_reset`, `boot_power_on` and `state_load`. `bios.arbitration` runs all 16 combinations of b7, b6, b5, b3 (and a work-RAM-off leg) on both machines against a closed-form oracle table in the test (not captured from a run), with whole-table and single-entry-flip controls; the flag legs run 4,096 random `$3E` writes. Mutations: SMS2 AND changed to OR: `bios.arbitration` fails; cartridge/BIOS priority swapped: `bios.arbitration` fails; both restored. |

**Score:** 5/5 truths verified, 0 present-but-behavior-unverified.

### Per-requirement table

| Requirement | Code | Test | Command run by me | Exit |
|-------------|------|------|-------------------|------|
| BIOS-01 | `tests/cmake/firmware_scan.cmake`, `scripts/firmware-window.sh`, `scripts/hygiene.sh` (`fw_scan`), `.githooks/*`, `tests/tests.cmake`, `host/bios_table.inc`, `ASSET_POLICY.md`, `THIRD-PARTY-NOTICES.md` | `release.firmware_scan`, `policy.firmware_scan.*` (11), `policy.bios_table*`, `policy.bios_dir_reads*`, `policy.firmware_window.*` (17), `local.firmware_window_scan` | `cmake --workflow --preset ci`; `cmake --workflow --preset hygiene`; hand-run scan controls; mutation of the release scan; `git merge-base --is-ancestor` x3 | 0; 0; as listed; 3 controls fail as required; 0 |
| BIOS-02 | `runner/args.c`, `runner/main.c`, `runner/exit_status.h`, `host/bios.c`, `host/bios.h`, `docs/runner.md`, `docs/bios.md` | `runner.bios.grammar`, `.refusals`, `.no_path_echo`, `.boot`, `host.bios`, `bios.load`, existing hash tests unchanged | hand runs of `cogmaster-run` (8 invocations), mutation of the 77 path | 0, 77, 77, 77, 77, 2 as documented; control fails as required |
| BIOS-03 | `src/boot.c` (`boot_power_on`), `src/instance.c`, `src/state.c`, `src/bus.c`, `include/cogmaster.h` (media `bios`, `bios_size`, API 14, state 16), `tests/roms/src/bios_shape.asm` | `bios.equivalence` (+2 controls), `bios.load`, `bios.hostile`, `bios.state`, `fuzz.regress.bios`, `abi.api_v14`, hosted `fuzz.libfuzzer.bios` | `ctest -R 'bios\|firmware'`; mutation of the hand-off SP | 37 of 37 pass; control fails as required |
| BIOS-04 | `libretro/libretro.c`, `libretro/options.h`, `libretro/cogmaster_libretro.info.in` | `libretro.host.bios`, `libretro.info_claims` and its firmware self-test | `ctest -R 'libretro.host.bios\|libretro.info'`; mutation of a firmware name | pass; control fails as required |
| BIOS-05 | `src/bus.c`, `src/io.c`, `.planning/preparation/SMS-SYSTEM.md`, `docs/bios.md` | `bios.arbitration`, `bios.slow_flag`, the work-RAM-off leg inside `bios.arbitration` | `ctest -R '^bios\.'`; two mutations of the arbitration | 8 of 8 pass; both mutations fail the test |

### Runs I performed (all from the repository root)

| Command | Result | Exit |
|---------|--------|------|
| `cmake --workflow --preset ci` | 623 tests plus 5 `release` tests, all passed | 0 |
| `cmake --workflow --preset hygiene` | 70 of 70 passed (`hygiene.tree` 28 s, `hygiene.analyzer` 18 s; no timeout this run) | 0 |
| `sh scripts/hygiene.sh --tree`; `sh scripts/hygiene.sh --history origin/main..HEAD` | clean | 0; 0 |
| `cmake -DROOT=$PWD -P tests/cmake/budgets.cmake` | `repository: 845 check lines / 9402 src lines, strict one tenth` (limit 940) | 0 |
| `ctest --test-dir build/ci -N` against `tests/inventory/ci.txt` | 628 registered, 628 non-comment inventory lines, names equal (`diff` of the sorted lists prints nothing) | 0 |
| `grep -vc '^#' tests/inventory/wasm32.txt` and `nofp.txt` | 586 and 24; `quality.json` floors `tests.ci` 626 and `tests.wasm32` 584, each registered minus 2 | 0 |
| `wc -c docs/*.md` | largest `docs/testing.md` 36,918 bytes and `docs/embedding.md` 36,379, under the 38,912 cap in `quality.json`; `docs.budgets*` and `policy.quality*` pass inside the `ci` workflow | 0 |
| mutation `src/boot.c` SP `0xDFF0` to `0xDFF1`, rebuild, `ctest -R '^bios\.'` | `bios.equivalence`, `.bad_ram`, `bios.load` fail; `git checkout src/boot.c` | 3 failed; restored |
| mutation `src/bus.c` SMS2 AND to OR | `bios.arbitration` fails; restored | 1 failed; restored |
| mutation `src/bus.c` BIOS-before-cartridge order | `bios.arbitration` fails; restored | 1 failed; restored |
| mutation `libretro/libretro.c` `bios_E.sms` to another name | `libretro.host.bios` fails; restored | 1 failed; restored |
| mutation `runner/main.c` absent BIOS refusal 77 to 1 | `runner.bios.refusals`, `runner.bios.no_path_echo` fail; restored | 2 failed; restored |
| mutation `tests/cmake/firmware_scan.cmake` digest hit made non-fatal | `policy.firmware_scan.whole`, `.cartdb`, `.manifest` fail; restored | 3 failed; restored |
| `sh scripts/firmware-window.sh SCRATCH_DIR TARGET` with a random scratch dump (clean, leaking, missing dir, empty dir, no config) | `1 compared`; `matches ...leak.bin`; not usable; no usable dump; `0 compared` | 0; 1; 2; 2; 0 |
| `git status --short` after every restore and at the end (before this report) | empty | clean |
| `cogmaster-run` hand runs with `bios_probe.sms` and the owned `bios_shape.bin` (see truth 2) | as listed | 0, 77, 77, 77, 77, 2 |
| `git diff --quiet origin/main -- tests/io tests/vdp tests/bus tests/audio tests/ports ...` | no diff | 0 |
| `git diff --name-only --diff-filter=A main...HEAD` binary check; `git ls-files` for rom, bios, sms, gg, srm, sav | no new binary; only the manifest-listed `tests/vectors/z80/sst-sample.bin` | none new |
| grep of added non-planning lines for requirement IDs, `SEED-`, plan and phase numbers | no match (earlier hits were date strings and pre-existing IDs in `PROVENANCE.md` and `ASSET_POLICY.md`) | no planning IDs |
| grep for `TBD`, `FIXME`, `XXX` in changed non-planning files | only the `XXXXXX` mktemp template in `scripts/firmware-window.sh`, not a marker | none |
| `git log --format='%ae %ce' origin/main..HEAD` | 52 commits, all the GitHub noreply identity | n/a |

### Hosted evidence (read with `gh run view`)

| Run | Workflow | Head | Conclusion | Notes |
|-----|----------|------|------------|-------|
| https://github.com/szTheory/cogmaster/actions/runs/43062734904 | CI dispatch D2 | 076f13b | success | core load, boot path, state binding, perf delta -11 bp |
| https://github.com/szTheory/cogmaster/actions/runs/43074158512 | CI dispatch D3 | 575739a | success | libretro BIOS test on all lanes; source of the settled timeouts |
| https://github.com/szTheory/cogmaster/actions/runs/43076225840 | CI dispatch D4 | fb40676 | success | final functional head before the review fixes |
| https://github.com/szTheory/cogmaster/actions/runs/43076276567 | Nightly dispatch | fb40676 | success | `fuzz` job green; log shows `fuzz.libfuzzer.bios Passed 61.13 sec` |
| https://github.com/szTheory/cogmaster/actions/runs/43076272120 | CI on main (perf base) | 202f107 | success | `perf control boot_to_play runs=12303282 x3 spread=0` |
| https://github.com/szTheory/cogmaster/actions/runs/43082065462 | CI dispatch after the review fixes | fb2106a | success | all 19 jobs success (hygiene, perf, asan, be32, nofp x2, build x6, retroarch, wasm32, hash-equality, `CI required`); title, rehearsal and cleanup skipped as for any dispatch |

Perf: head `perf control boot_to_play runs=12288943,12288943,12288943 spread=0` (run 43076225840) against main
12303282; `(12288943 - 12303282) * 10000 / 12303282 = -11` basis points (limit +100), so the derived `mem_slow` flag
did not slow the fast path.

`fb2106a` is an ancestor of HEAD; the one commit after it (bd09bb4) changes the hygiene job `timeout-minutes` and a
timeout in `tests/policy/firmware_window_cases.cmake`. It is covered by my local `hygiene` workflow (70 of 70) and its
hosted confirmation is the pull-request run at ship, a merge-time check by design (rule 6).

### Required artifacts and key links

| Artifact / link | Status | Details |
|-----------------|--------|---------|
| `src/bus.c` `mem_slow_driven`, `mem_read_slow`, `bus_decode` | VERIFIED | substantive; `mem_read` and `cogmaster_peek` call them; `bus_decode` called from `io_out` (`$3E`), `io_reset`, `boot_power_on`, `state_load` |
| `src/instance.c` load and reset BIOS paths | VERIFIED | `tagged_bios`, size-tag guard, exactly 8,192 bytes, both-or-neither, no content or Game Gear refused, instance-owned copy, a load without BIOS clears the last one; reset kinds call `boot_power_on` when `bios.size != 0` |
| `src/state.c` header | VERIFIED | boot mode (offset 53) and BIOS CRC-32 (offset 54) tied into the header; BIOS bytes never visited; format 16, API 14 |
| `runner/main.c` `bios_load` -> `cogmaster_load` media | WIRED | `run(opt, image, n, bios)` sets `media.bios`; hand runs show the BIOS path boots |
| `host/bios.c` -> runner and libretro | WIRED | runner labels through `cogmaster__bios_identify`; libretro reads through `cogmaster__bios_read` |
| `libretro.c` `lr_bios` -> media | WIRED | called only when `cogmaster_boot` is `bios`; `libretro.host.bios` covers it |
| `scripts/firmware-window.sh` -> hooks, library preset, hygiene controls | WIRED | `hygiene.sh fw_scan`, `tests/tests.cmake`, `tests/hygiene/tests.cmake` |
| `release.firmware_scan` -> `ci` preset | WIRED | in the `ci` inventory and run by `cmake --workflow --preset ci` |
| `tests/inventory/*.txt`, `quality.json` floors | WIRED | 628 registered equals 628 inventoried; floors 626 and 584 |

### Anti-patterns

None blocking. No debt markers, no planning identifiers in code, test names or user docs, no provisional timeout added
(the provisional counts in every changed registration file equal their counts on `main`; the two remaining
occurrences are the existing `timeout_comments` fixtures in `tests/hygiene/tests.cmake`). The review rounds are
recorded in `10-REVIEW-CORE.md`, `10-REVIEW-HOST.md` and `10-REVIEW-FIX.md`; the tooling findings are seeds per rule 7.

Residual notes (no gap): the window scan documents what it does not cover (base64, decimal lists, XOR, compression,
hexdump layouts, a dump kept as a zip) in its own header and in `docs/testing.md`; two arbitration rows are labelled
model choices, not measurements, in `docs/bios.md` and the code comments (a BIOS-only read at `$2000-$BFFF`, and the
empty-slot answer); both are stated openly rather than claimed as hardware facts.

## Carried hand-off (not a gap; recorded as the batched owner hand-off)

The checks that need the owner's own real BIOS dump are not run, status `not_run`, and cannot be run here without
reading the owner's firmware, which the legal rule forbids:

1. **Real-BIOS boot and the local window scan.** In SEED-010 ("The BIOS local judge (hand-off, not a requirement)"):
   per dump, the runner's recognised label or SHA-1 fact on stderr, exit 0 for a runnable dump and 77 with a Game Gear
   cartridge, and `K compared` with exit 0 from the window scan; plus the residue a real BIOS leaves that fast boot
   lacks (logo tiles, VDP registers, work RAM above `$C000`, Z80 registers, PSG attenuation, beam phase, paging after a
   large checksum). Outcomes are predicates only, batched with the phase 11 and 12 library runs. The CI image proves
   the mechanism, not that residue, which `docs/bios.md` says plainly.
2. **Seeds for what was left out** are in SEED-011 (Game Gear 1 KiB BIOS, built-in-game BIOSes, BIOS boot without a
   cartridge, region hint, Mark III profile, hosted RetroArch BIOS session, CI history scan, publish-job scan), each
   with its trigger. None changes a hash today.

`human_needed: false`: the owner verifies nothing by default (rule 6); these are identity-bound library runs on the
owner's hardware, correctly outside CI.

## Advisory notes (no gap)

1. **Hosted run after the last commit.** bd09bb4 (one timeout number and one test timeout) postdates the last hosted
   dispatch; local `cmake --workflow --preset hygiene` passes; the pull-request run at ship confirms it.
2. **ASan, be32 and wasm32 not run locally.** The hosted runs 43076225840 and 43082065462 passed `asan`, `be32`,
   `wasm32`, `nofp` and `hash-equality` on heads that contain every functional change.
3. **`hygiene.tree` timeout.** The review fixer saw it exceed 60 s under load; it passed in 28 s here and 51 s alone in
   their note. Per rule 7 it is a tooling concern, not a gap; no change was made.
4. **STATE.md runtime row.** The verify step's runtime and model row is for the orchestrator to add.

## Gaps Summary

No gaps. All five roadmap criteria and all five BIOS requirements are backed by code that exists, is wired and is
exercised by tests that I ran (ci workflow 628 of 628, hygiene workflow 70 of 70) and whose negative controls I
broke on purpose and watched fail (hand-off constants, arbitration order and AND/OR, adapter file name, runner exit
status, release scan). The hosted evidence covers every lane on the last functional head and after the review fixes,
the nightly ran the BIOS fuzz target, and perf moved by -11 basis points. The tree is clean, no firmware byte is
tracked or packaged, and the counted check code is 845 of 940. The only carried item is the owner-dump local judge,
`not_run`, recorded in SEED-010 and SEED-011.

---

_Verified: 2026-11-11_
_Verifier: Claude (gsd-verifier)_
