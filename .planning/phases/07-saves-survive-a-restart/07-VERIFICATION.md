---
phase: 07-saves-survive-a-restart
verified: 2026-11-10T00:00:00Z
status: passed
score: 5/5 must-haves verified
covered_files:
  - ".github/workflows/ci.yml"
  - ".planning/phases/07-saves-survive-a-restart/07-01-PLAN.md"
  - ".planning/phases/07-saves-survive-a-restart/07-01-SUMMARY.md"
  - ".planning/phases/07-saves-survive-a-restart/07-02-PLAN.md"
  - ".planning/phases/07-saves-survive-a-restart/07-02-SUMMARY.md"
  - ".planning/phases/07-saves-survive-a-restart/07-03-PLAN.md"
  - ".planning/phases/07-saves-survive-a-restart/07-03-SUMMARY.md"
  - ".planning/phases/07-saves-survive-a-restart/07-04-PLAN.md"
  - ".planning/phases/07-saves-survive-a-restart/07-04-SUMMARY.md"
  - ".planning/phases/07-saves-survive-a-restart/07-05-PLAN.md"
  - ".planning/phases/07-saves-survive-a-restart/07-05-SUMMARY.md"
  - ".planning/phases/07-saves-survive-a-restart/07-06-PLAN.md"
  - ".planning/phases/07-saves-survive-a-restart/07-06-SUMMARY.md"
  - ".planning/phases/07-saves-survive-a-restart/07-07-PLAN.md"
  - ".planning/phases/07-saves-survive-a-restart/07-07-SUMMARY.md"
  - ".planning/phases/07-saves-survive-a-restart/07-08-PLAN.md"
  - ".planning/phases/07-saves-survive-a-restart/07-08-SUMMARY.md"
  - ".planning/phases/07-saves-survive-a-restart/07-09-PLAN.md"
  - ".planning/phases/07-saves-survive-a-restart/07-09-SUMMARY.md"
  - ".planning/phases/07-saves-survive-a-restart/07-10-PLAN.md"
  - ".planning/phases/07-saves-survive-a-restart/07-10-SUMMARY.md"
  - "docs/saves.md"
  - "libretro/cogmaster_libretro.info.in"
  - "runner/args.c"
  - "runner/main.c"
  - "runner/save.c"
  - "src/battery.c"
  - "src/eeprom.c"
  - "src/mapper_ext.c"
  - "src/state.c"
  - "tests/libretro/host.c"
  - "tests/mapper/test_battery.c"
  - "tests/mapper/test_eeprom.c"
  - "tests/retroarch/run_retroarch.cmake"
  - "tests/runner/test_save.c"
  - "tests/runner/tests.cmake"
covered_digest: "v4:sha256:b227e9348bcbbedd804c58be4a254e4f13f62277b778aafbaca857dd20b66097"
behavior_unverified: 0
overrides_applied: 0
---

# Phase 7: Saves survive a restart Verification Report

**Phase Goal:** A Game Gear 93C46 game and a battery-backed game keep their progress across restarts in the runner and in RetroArch, saves survive a killed process, and `.srm` files cross between cogmaster and other released cores only in layouts that were measured.
**Verified:** 2026-11-10
**Status:** passed
**Re-verification:** Yes, after a one-sentence docs/saves.md change (the hosted restart claim is no longer deferred); no gaps existed, the report had gone stale
**Branch / HEAD:** `phase/07-saves-survive-a-restart` at fe618a4

Human verification items: none. Every check below is a test, a command I ran, or a hosted run. Nothing is
handed to the owner (AGENTS.md rule 6); no identity-bound consent or owner hardware is involved.

## Goal Achievement

### Observable Truths (ROADMAP success criteria)

| # | Truth | Status | Evidence |
|---|-------|--------|----------|
| 1 | EEPROM tests show the write-disabled power-up and the EWEN/EWDS/WRITE/ERASE/WRAL/ERAL commands, the 128-byte image on the battery span, the generation counter moving only on a completed change, a mid-command state round trip byte for byte, and the bit-banger in the fuzz target | VERIFIED | `src/eeprom.c` (device), `src/mapper_ext.c` board arm, `tests/mapper/test_eeprom.c`: closed-form cases for each command including power-up lock (lines 134-190), generation cases (WRAL reports once, same-data no bump, lines 284-315), `state_mid_command` round-trips a save state at every clock count 0..total for WRITE and READ and compares state image, chip, generation and RAM against an uninterrupted reference (lines 782-820). `tests/fuzz/fuzz_eeprom.c` is registered in `tests/fuzz/tests.cmake` (`fuzz.regress.eeprom`, `fuzz.libfuzzer.eeprom` in `tests/inventory/fuzz.txt`). All `eeprom.*` and `fuzz.regress.eeprom` tests pass. |
| 2 | `.srm` sizes and EEPROM word order of released cores measured once with command and exit status recorded; import refuses lengths over the span, EEPROM import accepts only 0 or 128, other cores' layouts claimed only where measured | VERIFIED | Measurement recorded in `07-07-SUMMARY.md` (commands, exit 0, zip SHA-256s, date, per-core result: Gearsystem 128 bytes little-endian, Genesis Plus GX 3 bytes for the battery fixture; nothing left `unmeasured`). Cross-check by me: cogmaster's own export for the same fixtures starts `34 12 01 00` (EEPROM) and `a5 5a 01 ff` (battery), consistent with the recorded foreign-core bytes. Import rules in `src/battery.c`; `battery.import` runs a 4-board by 8-length matrix with an unchanged-state check on every refusal and passes. I ran the runner with 127- and 129-byte EEPROM saves: exit 4 with a length message both times. |
| 3 | The libretro core reports saves enabled, and a guard test shows the exported span equals the battery span | VERIFIED | `libretro/cogmaster_libretro.info.in` line 20 `libretro_saves = "true"` (and `savestate = "false"` unchanged). `libretro.span.battery` (32768), `.eeprom` (128), `.plain` (NULL/0) compare the module's SAVE_RAM pointer/length with a second library instance and an expected length (`tests/libretro/host.c` `span_test`); `libretro.span.self_test` proves a wrong length fails; `libretro.info_claims` (+ self_test) ties every `true` claim to a test. All pass. |
| 4 | `cogmaster-run --save-interval N` writes after every N frames; a kill test between flushes leaves the last complete save readable; stale temporaries are swept; directory synced after rename on POSIX | VERIFIED | `runner/main.c:444-447` (`n % save_interval == 0` flush, failure exits at once), `runner/args.c:188,235` (requires `--save-dir`, whole number), `runner/save.c:88-125` (`save_sweep` of `<target>.tmp-0..1023` on load), `runner/save.c:247-282` (`save_sync_directory`: open dir, `fsync`, EINVAL/ENOTSUP tolerated, EACCES/EPERM on open tolerated after the review fix, fault `COGMASTER_SAVE_DIRSYNC`). Tests `runner.save.interval`, `.interval_fault`, `.kill`, `.sweep`, `.process.dirsync`, `runner.save.failures` pass. The kill test writes save A, kills (exit 93) at the first periodic flush, asserts no frame 2, one complete leftover temporary starting `a55a02`, target byte-equal to A, then a production run removes the temporary and continues. I also ran the behaviours by hand (table below). |
| 5 | A hosted RetroArch run writes a battery save and an EEPROM save, restarts and reads both back; `tests/retroarch` stays at or under 300 lines | VERIFIED | Hosted run https://github.com/szTheory/cogmaster/actions/runs/42945252988 (`workflow_dispatch` on this branch, head cad977a, conclusion success, all jobs success or skipped-by-design). Its `retroarch` job steps 15 "Battery save survives a RetroArch restart" and 16 "EEPROM save survives a RetroArch restart" are success (read with `gh run view --json jobs`). Each step runs RetroArch, compares its `.srm` to the `cogmaster-run --save-dir` oracle, restarts with `-DKEEP=1`, compares again, and asserts through `expect_fail.cmake` that the oracle's pass 1 and pass 2 differ (so equality after pass 2 proves the first save was read back). `policy.e2e_budget` passes (275 counted of 300; `wc -l tests/retroarch/*` is 261). |

**Score:** 5/5 truths verified, 0 present-but-behavior-unverified.

### Per-requirement table

| Requirement | Code | Test | Command run by me | Exit |
|-------------|------|------|-------------------|------|
| SAVE-01 | `src/eeprom.c`, `src/mapper_ext.c`, `src/cartdb.inc`, `tools/cartdb.c`, `src/battery.c` | `eeprom.commands`, `.lock`, `.read`, `.changes`, `.differential`, `.board.*`; `fixtures` eeprom fixture rows; `runner.save.continuation.eeprom` | `ctest --test-dir build/ci -R '^(eeprom\.\|battery\.\|runner\.save\|libretro\.\|fuzz\.regress\.eeprom\|abi\.\|api\.\|cart\.\|fixtures\.)' -j8` (155 tests) | 0 |
| SAVE-02 | `src/state.c` (`eeprom_io` bounds and cross-checks), `tests/fuzz/fuzz_eeprom.c` | `eeprom.state.window_open`, `.mid_command` (every count), `.bounds` (six refusals), `fuzz.regress.eeprom` | same ctest run | 0 |
| SAVE-03 | `src/battery.c` import rule; amended wording in `.planning/REQUIREMENTS.md` and `.planning/ROADMAP.md`; `docs/saves.md` | `battery.import` length matrix; measurement in `07-07-SUMMARY.md` | same ctest run; `grep -c 'EEPROM import accepts only 0 or 128 bytes' .planning/REQUIREMENTS.md .planning/ROADMAP.md`; runner with 127/129-byte EEPROM saves | 0; prints 1 and 1; runner exit 4 (twice) |
| SAVE-04 | `libretro/cogmaster_libretro.info.in` (`libretro_saves = "true"`), `libretro/` adapter, `tests/libretro/host.c` `span_test` | `libretro.span.battery/.eeprom/.plain/.self_test`, `libretro.info_claims(.self_test)`, `libretro.battery.sms2/.gg` | same ctest run | 0 |
| SAVE-05 | `runner/main.c`, `runner/args.c`, `runner/save.c` | `runner.save.interval`, `.interval_fault`, `.kill`, `.sweep`, `.process.dirsync`, `.failures` | same ctest run; manual two-process and sweep runs (below) | 0 |
| SAVE-06 | `.github/workflows/ci.yml` retroarch job steps, `tests/retroarch/run_retroarch.cmake` (`KEEP`) | hosted steps 15 and 16; `policy.e2e_budget` | `gh run view 42945252988 --json conclusion,jobs`; `cmake -DROOT=$PWD -P tests/cmake/budgets.cmake` | success; 0 |

### Runs I performed (all from the repository root)

| Command | Result | Exit |
|---------|--------|------|
| `cmake --workflow --preset ci` (full: configure, build, all tests, release tests) on fe618a4 | passed | 0 |
| `cmake -DROOT=$PWD -P tests/cmake/budgets.cmake` | `repository: 723 check lines / 8279 src lines, strict one tenth` | 0 |
| `build/ci/cogmaster-run --frames 30 --save-dir D build/ci/fixtures/eeprom.gg` twice, fresh temp dir `D` | `eeprom.srm` 128 bytes; bytes `34 12 01 00 ff ff` then `34 12 02 00 ff ff`; second differs from first | 0 / 0 |
| same for `battery.sms` | `battery.srm` 32768 bytes; `a5 5a 01 ff` then `a5 5a 02 ff` | 0 / 0 |
| `--frames 30 --save-interval 5 --save-dir D eeprom.gg` | runs, save present | 0 |
| `--save-interval 5` without `--save-dir` | `--save-interval requires --save-dir` | 2 |
| `--save-interval 0` (off) / `--save-interval 4294967296` | accepted / rejected | 0 / 2 |
| pre-create `eeprom.srm.tmp-0` and `eeprom.srm.tmp-1023`, run again | both temporaries removed, save kept | 0 |
| EEPROM save of 127 bytes, then 129 bytes | `file length 127 does not match the 128-byte save`; `oversized file` | 4 / 4 |
| `git ls-files '*.srm' '*.gg' '*.sms'` (re-run on fe618a4) | empty | 0 |
| `git ls-files` for `.bin/.rom/.sav` | only `tests/vectors/z80/sst-sample.bin`, which is listed in `tests/roms/manifest.txt` | n/a |
| `git diff --name-only origin/main...HEAD` filtered for `srm/gg/sms/rom/bin` | empty | n/a |
| grep for `TBD`, `FIXME`, `XXX`, `TODO`, `HACK` in changed non-planning files | no match | 1 (no match) |
| author check, `git log --format='%an <%ae>' origin/main..HEAD` | 45 commits, all `szTheory <28652+szTheory@users.noreply.github.com>` | n/a |

### Specific checks requested

- **SAVE-03 wording consistency.** `.planning/REQUIREMENTS.md` line 38 and `.planning/ROADMAP.md` criterion 2 carry the same sentence ("import refuses lengths over the span, EEPROM import accepts only 0 or 128 bytes, and the layouts of other cores are claimed only where measured"). `docs/saves.md` says the same in other words: EEPROM import takes none or exactly 128 bytes (prefix refused, no padding, trimming or conversion), battery import keeps the API 7 rule (lengths 0 through the span, oversize refused), and interop is claimed only for layouts that cogmaster wrote or that were measured (Gearsystem 128 bytes little-endian; Genesis Plus GX a 3-byte file for the battery fixture; "Nothing else is claimed"). The three agree. The docs do not repeat the exact phrase (grep count 0 in `docs/saves.md`); the content is equivalent, so this is not a finding.
- **SAVE-04.** `libretro_saves = "true"` plus the span guard: confirmed above. The span guard is registered under the fixtures guard and, with `libretro.info_claims`, fails if the claim loses its test.
- **SAVE-05.** Interval, sweep and directory sync: confirmed above, in code and by running.
- **Clean tree.** No ROM or save file is tracked other than the manifest-listed vector. Fixtures are built from source (`tests/roms/src/eeprom.asm`, `battery.asm`), listed in `tests/roms/manifest.txt` comments and `tests/built/expected.txt`.
- **Rule 1 and rule 3 for the measurement notes.** The notes name only released binaries (nightly buildbot zips, run as cores through the pinned RetroArch), record zip SHA-256 and date, and contain no source of any GPL or LGPL emulator. The only byte values written down are the two or three signature bytes of the phase's own self-authored fixtures (`34 12`, `a5 5a 01`), which describe a layout rather than being a save file. No personal path, e-mail or name appears in `.planning/phases/07-*` or in `docs/saves.md`, `docs/runner.md`, `docs/hosted.md` (grep for `/Users/`, e-mail patterns and the owner's first name is empty).

### Required artifacts and key links

| Artifact / link | Status | Details |
|-----------------|--------|---------|
| `src/eeprom.c` device, `src/mapper_ext.c` arm, `src/state.c` visitor | VERIFIED | exist, substantive, wired through the `COGMASTER_BOARD_SEGA_EEPROM` arm; 7 EEPROM rows plus the owned fixture row carry persistence 2, geometry 1, 128 bytes in `src/cartdb.inc` (review confirmed; `cart.*` tests pass) |
| `include/cogmaster.h` versions | VERIFIED | `COGMASTER_API_VERSION 11`, `COGMASTER_STATE_FORMAT_VERSION 13`, `COGMASTER_BOARD_SEGA_EEPROM 10u`; `abi.*` and `api.*` tests pass |
| `tests/fuzz/fuzz_eeprom.c` to fuzz registration | WIRED | `tests/fuzz/tests.cmake` loops `loader run save eeprom` for regress and libFuzzer targets |
| `runner/main.c` to `cogmaster__save_flush` | WIRED | periodic flush at line 447; data flows from `cogmaster_get_battery` to the temp file and rename |
| `.github/workflows/ci.yml` retroarch steps to `run_retroarch.cmake` `KEEP` | WIRED | both steps pass `-DKEEP=1` on the second pass; hosted steps succeeded |
| `libretro_saves` claim to `libretro.span.eeprom` | WIRED | `tests/cmake/repository.sh` claims map; `libretro.info_claims.self_test` proves the control |

### Anti-patterns

None blocking. No debt markers in any file changed by the phase.

## Advisory notes (no gap, nothing for the owner)

1. **Hosted run predates the review-fix commits.** The hosted run's head is cad977a. Commits a67a3d9, c66aaba and d9625cb (the two review fixes and a format fix: `src/state.c`, `runner/save.c`, `docs/runner.md`, two tests) came after it and are not on the remote yet (branch is 5 commits ahead of origin). I ran the full `cmake --workflow --preset ci` on HEAD locally (exit 0), which includes the new cases in `eeprom.state.bounds` and `runner.save.failures`. The pull request run at ship (plan 07-10 ship list items 0 to 3) is the hosted confirmation for those commits; it is a merge-time check by design (rule 6), not a verification item.
2. **Stale word in `docs/saves.md` (resolved).** The Testing saves section said "the deferred hosted save-file claim"; commit fe618a4 changed it to "They do not establish the hosted save-file claim; the hosted RetroArch restart steps do." I re-read the whole file: it now agrees with the SAVE-03 import rules (0 or exactly 128 bytes for the EEPROM board, battery span rule, measured-only interop) and with the SAVE-06 hosted restart steps, with no remaining "deferred" wording (`grep -n deferred docs/saves.md` is empty).
3. **Foreign-core measurement is a local, one-time run.** As the context decision prescribes, it has no run URL and rests on the recorded command, exit status, zip SHA-256 and date. The rolling nightly zips cannot be pinned (rule 12 cannot apply, and the stable path answers 404). I confirmed internal consistency (same first bytes as cogmaster's own export) but did not re-fetch the foreign cores; re-measurement is not required by the requirement ("measured once").

## Gaps Summary

No gaps. All five roadmap criteria and all six SAVE requirements are backed by code that exists, is wired and is exercised by tests that I ran and that pass, by manual two-process runs, and by a hosted RetroArch run whose save-round-trip steps succeeded.

---

_Verified: 2026-11-10 (re-verified on fe618a4)_
_Verifier: Claude (gsd-verifier)_
