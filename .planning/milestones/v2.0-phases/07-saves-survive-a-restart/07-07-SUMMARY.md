---
phase: 07-saves-survive-a-restart
plan: "07"
subsystem: saves
tags:
  - import
  - eeprom
  - libretro
  - interop
status: complete

requires:
  - phase: 07-saves-survive-a-restart
    provides: plan 07-03 EEPROM board and plan 07-05 owned eeprom.gg fixture
provides:
  - EEPROM import accepts only 0 or exactly 128 bytes, battery import unchanged, every refusal leaves the state image equal
  - a table-driven length matrix over three battery spans and the EEPROM board
  - libretro_saves = true in the .info, tied to a host span guard per fixture (battery 32768, EEPROM 128, plain cart 0)
  - a one-time recorded .srm measurement of two released cores
affects: [07-08, 07-09, 07-10]

actuals:
  tokens: 2400
  tasks: 3
  commits: 3

plan_head_before: 245dbbc706ae083b420b78330f76f43fccbd9765
plan_head_after: 044b292c8de03adebf2a6f4420d966cdd6250ccb
commits: 3

tech-stack:
  added: []
  patterns:
    - "A host mode compares the module's exported span with a second library instance loaded from the same bytes and with a length the registration states, plus a self-test through expect_fail that a wrong length fails"

key-files:
  created: []
  modified:
    - src/battery.c
    - tests/mapper/test_battery.c
    - tests/libretro/host.c
    - tests/libretro/tests.cmake
    - tests/inventory/ci.txt
    - libretro/cogmaster_libretro.info.in
    - tests/cmake/repository.sh
    - docs/saves.md

key-decisions:
  - "EEPROM import takes none or the whole span; the rule sits after the span check and before any source read, so a NULL source with a refused length still reports the size status"
  - "The host span guard uses the owned 32 KiB battery fixture; no 8 KiB battery fixture is owned, so 8 KiB and 16 KiB spans are covered in-process by the length matrix"
  - "libretro_saves maps to libretro.span.eeprom in the existing claims line (no counted line added)"
  - "docs claim only the two measured layouts: Gearsystem 128 bytes little-endian, Genesis Plus GX a 3-byte file for the battery fixture"

requirements-completed: [SAVE-03, SAVE-04]

coverage:
  - id: I1
    description: "Battery import is as shipped and EEPROM import accepts only 0 or 128 bytes; every refusal returns the save-size status with the state image unchanged"
    requirement: SAVE-03
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci -R '^battery\\.import$' (length matrix in tests/mapper/test_battery.c)"
        status: pass
    human_judgment: false
  - id: I2
    description: "The length matrix {0, 1, 127, 128, 129, 8192, 16384, 32768} over 8 KiB, 16 KiB, 32 KiB battery and EEPROM boards has the expected status for every cell"
    requirement: SAVE-03
    verification:
      - kind: unit
        ref: "tests/mapper/test_battery.c length_matrix via battery.import; negative control exit 8 without the rule"
        status: pass
    human_judgment: false
  - id: I3
    description: "The released Gearsystem and Genesis Plus GX save layouts are measured once and recorded, and docs claim only those"
    requirement: SAVE-03
    verification:
      - kind: other
        ref: "srm measurement section below; command and exit status recorded"
        status: pass
    human_judgment: false
  - id: I4
    description: "The libretro module's SAVE_RAM size equals the library battery length for the battery fixture, the EEPROM fixture and a plain cart, and the .info claims libretro_saves"
    requirement: SAVE-04
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^libretro\\.(span|info_claims)'"
        status: pass
    human_judgment: false
  - id: I5
    description: "The span guard and the claim mapping fail when the exported length is wrong or the claim maps to a missing test"
    requirement: SAVE-04
    verification:
      - kind: integration
        ref: "libretro.span.self_test and the two info_claims controls below"
        status: pass
    human_judgment: false
---

# Phase 7 Plan 07: Import rules and the libretro saves claim Summary

EEPROM import takes none or exactly 128 bytes beside the unchanged battery rule, the libretro `.info` now declares saves behind a per-fixture span guard, and the save layouts of two released cores are measured once and recorded.

## Accomplishments

- `cogmaster_import_battery` refuses, with the save-size status and before any source read, an EEPROM import of any length other than 0 and the span (a 93C46 word is two bytes). Battery behaviour is untouched.
- The `import` case of `battery.import` gained a 4 board by 8 length table (32 rows) with the expected status per cell and the full state image compared after every refusal; no new test name.
- `cogmaster-test-libretro-host` has a `span` mode; `libretro.span.battery` (32768), `.eeprom` (128), `.plain` (0, NULL) and `libretro.span.self_test` (a wrong length must exit 1 with `span mismatch`) are registered under the fixtures guard and inventoried with `libretro,portable`.
- `libretro_saves = "true"`; `claims["libretro_saves"]="libretro.span.eeprom"` was added inside the existing line, so `tests/cmake/repository.sh` has 178 lines before and after.
- `docs/saves.md`: EEPROM import paragraph, the stale "remains false" paragraph replaced, the interop sentence now says what was measured.

## Evidence

All run from the repository root. These are local runs; no hosted run belongs to this plan (the hosted round trip is plan 07-09).

| Command | Exit |
| --- | --- |
| `cmake --workflow --preset ci` (454 tests plus 4 release) | 0 |
| `cmake --workflow --preset hygiene` | 0 |
| `ctest --test-dir build/ci -R '^(battery\.|eeprom\.|runner\.save\.|libretro\.battery)'` | 0 |
| `ctest --test-dir build/ci -R '^(libretro\.\|policy\.\|repository\.)'` | 0 |
| `cmake -DROOT=$PWD -P tests/cmake/budgets.cmake` (723 check lines, 8270 src lines) | 0 |

Negative controls (each reverted afterwards):

| Control | Result |
| --- | --- |
| EEPROM rule disabled in `src/battery.c`, `ctest -R '^battery\.import$'` | exit 8; restored, same command exit 0 |
| length comparison disabled in the span mode, `ctest -R '^libretro\.span\.self_test$'` | exit 8; restored, same command exit 0 |
| rendered `.info` copy with an added `savestate = "true"`, `cmake -DINFO=<copy> -DINVENTORY=tests/inventory/ci.txt -DCHECK=info_claims -P tests/cmake/repository.cmake` | exit 1, prints `savestate = "true" maps to no test` |
| `claims["libretro_saves"]` pointed at a name absent from `ci.txt`, same command on the real `.info` | exit 1, prints `libretro_saves needs libretro.span.absent, which is not in .../ci.txt` |

Plan acceptance: `awk 'END {print NR}'` gives 178 for `tests/cmake/repository.sh` and for `git show origin/main:tests/cmake/repository.sh` (exit 0); `grep -c` of the four inventory rows prints 4; `git status --porcelain` shows no `.zip`, `.dylib`, `.srm`, `.gg` or `.sms` path.

## srm measurement

A check with no recurring value, run once by the agent (rule 6, rule 11); no harness, core, ROM or save is committed. Date: 2026-11-11 (UTC). RetroArch is the pinned disk image from `pins.txt` (`retroarch-dmg` 1.22.2, SHA-256 `81b79121ba26d539064ae13b4d0419a120c3d165afbe656cf5f5412b15fdb434`), fetched with `scripts/fetch-pin.sh retroarch-dmg <new dir>` (exit 0), attached read-only with `hdiutil attach` (exit 0) and run as `RetroArch.app/Contents/MacOS/RetroArch`, which reports `Version: 1.22.2 (Git 69a4f0ea) Nov 20 2025`. Settings: `tests/retroarch/test.cfg.in` with `@RA_DIR@` replaced by a fresh directory under the scratch area (null audio driver, no config save). The cores are the macOS arm64 nightly buildbot zips (rolling; the `stable/1.22.2` path answers 404 for both, so no stable zip exists to pin), each downloaded into its own new directory with `curl -fsSL` (exit 0), hashed with `shasum -a 256`, unzipped with `unzip -q` (exit 0), and run only as a core through RetroArch.

Gearsystem: zip `https://buildbot.libretro.com/nightly/apple/osx/arm64/latest/gearsystem_libretro.dylib.zip`, SHA-256 `0d8961815749088616ebc93c0d39bd44bd97015cfec8480200c918b73a7d3ed9`, core version string in its own log `3.9.24-12-g1be9a12`. Core options file contained `gearsystem_mapper = "EEPROM 93C46"` (its log shows `Forcing Mapper: EEPROM 93C46`). Command: `RetroArch --verbose --config <cfg> -L <core dylib> --max-frames=120 build/ci/fixtures/eeprom.gg`, exit 0. Result: `<saves>/Gearsystem/eeprom.srm` is 128 bytes (`wc -c`), first bytes `34 12 01 00` (`od -An -tx1 -N4`): the signature word `$1234` low byte first, then the run counter 1 low byte first. Verdict: 128 bytes, little-endian word order, identical to the layout cogmaster exports. Repeated with `--max-frames=600`: exit 0, same size and bytes.

Genesis Plus GX: zip `https://buildbot.libretro.com/nightly/apple/osx/arm64/latest/genesis_plus_gx_libretro.dylib.zip`, SHA-256 `384f7cfd3fc145a4d030e30969cd33058b6bf6561cd9e304121290f2eb9abcac`. Command: `RetroArch --verbose --config <cfg> -L <core dylib> --max-frames=120 build/ci/fixtures/battery.sms`, exit 0. Result: `<saves>/Genesis Plus GX/battery.srm` is 3 bytes, first bytes `a5 5a 01`. Verdict: the first three bytes equal what cogmaster holds for the fixture, but the file is 3 bytes, not the 32768-byte span cogmaster exports; the bytes beyond the third and whether the core would load a 32768-byte file are not measured. Repeated with `--max-frames=600`: exit 0, same size and bytes. Cogmaster would import this file by its short-length rule (copy, pad with FF); that padding is cogmaster's, not a measured property of the other core.

RetroArch wrote each file under a per-core subdirectory of the save directory. Nothing was `unmeasured`.

## Deviations from Plan

**1. [Rule 3 - Blocking] Host span mode reads the image into a 262144-byte buffer**
- **Found during:** Task 2
- **Issue:** the owned fixtures are exactly 65536 bytes, so a 65536-byte buffer never reaches EOF and the `feof` check failed.
- **Fix:** the buffer matches `frame_test`'s 262144 bytes.
- **Files modified:** tests/libretro/host.c
- **Commit:** 62088be

**2. [Plan wording] No 8 KiB battery fixture exists**
- The plan anticipated this: the host guard uses the owned 32 KiB battery fixture, and the 8 KiB and 16 KiB spans are covered in-process by the length matrix.

**3. [Plan wording] Core zips are nightly, not stable**
- The stable buildbot path returns 404 for both cores; the measured zips are the rolling nightly ones, recorded by SHA-256 and date as CONTEXT prescribes.

**Total deviations:** 1 auto-fixed (Rule 3), 2 wording notes. **Impact:** none on scope.

## Known Stubs

None.

## Threat Flags

None. No new endpoint, auth path or file access; the host mode reads only a fixture named on its command line.

## Next

Ready for 07-08 (the runner message for an EEPROM length refusal).

## Self-Check: PASSED

- Files modified exist; commits 72ae9be, 62088be, 044b292 are ancestors of HEAD (`git merge-base --is-ancestor`).
- `cmake --workflow --preset ci` and `--preset hygiene` exit 0.
