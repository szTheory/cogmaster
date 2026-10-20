---
phase: 03-games-play-in-retroarch
reviewed: 2026-10-20T23:16:05Z
depth: standard
files_reviewed: 152
files_reviewed_list:
  - .env.example
  - .github/workflows/ci.yml
  - .github/workflows/nightly.yml
  - ASSET_POLICY.md
  - CMakeLists.txt
  - CMakePresets.json
  - PROVENANCE.md
  - README.md
  - THIRD-PARTY-NOTICES.md
  - cmake/CogmasterTests.cmake
  - docs/compatibility.md
  - docs/conformance.md
  - docs/embedding.md
  - docs/hardware/cartridges.md
  - docs/hardware/video.md
  - docs/maintaining.md
  - docs/runner.md
  - docs/testing.md
  - include/cogmaster.h
  - libretro/CMakeLists.txt
  - libretro/cogmaster_libretro.info.in
  - libretro/libretro.c
  - libretro/options.h
  - pins.txt
  - runner/args.c
  - runner/args.h
  - runner/exit_status.h
  - runner/expect.c
  - runner/expect.h
  - runner/input.c
  - runner/input.h
  - runner/main.c
  - runner/path.h
  - runner/probe.c
  - runner/save.c
  - runner/save.h
  - runner/sha1.c
  - runner/sha1.h
  - runner/stop.c
  - scripts/check-title.sh
  - scripts/compat-common.sh
  - scripts/compat-ratchet.sh
  - scripts/compat-update.sh
  - scripts/hygiene.sh
  - scripts/library.cmake
  - scripts/library.sh
  - src/battery.c
  - src/bus.c
  - src/cart.c
  - src/cartdb.c
  - src/cartdb.h
  - src/cartdb.inc
  - src/frame.c
  - src/instance.c
  - src/internal.h
  - src/io.c
  - src/mapper.c
  - src/sha1.c
  - src/sha1.h
  - src/state.c
  - src/vdp.c
  - src/vdp_render.c
  - tests/abi/tests.cmake
  - tests/api/v3.c
  - tests/api/v4.c
  - tests/api/v5.c
  - tests/api/v6.c
  - tests/api/v7.c
  - tests/api/v8.c
  - tests/built/expected.txt
  - tests/bus/test_vdp.c
  - tests/bus/tests.cmake
  - tests/cart/fixture.cmake
  - tests/cart/fixture_facts.txt
  - tests/cart/generator.cmake
  - tests/cart/test_cartdb.c
  - tests/cart/tests.cmake
  - tests/cmake/budgets.cmake
  - tests/cmake/ci_required.cmake
  - tests/cmake/expect_fail.cmake
  - tests/cmake/hashes.cmake
  - tests/cmake/repository.cmake
  - tests/cmake/repository.sh
  - tests/compat/allow.txt
  - tests/compat/ledger.csv
  - tests/compat/ratchet.cmake
  - tests/compat/tests.cmake
  - tests/compat/update.cmake
  - tests/core/test_core.c
  - tests/core/test_state.c
  - tests/diag/expected.csv
  - tests/fixtures/lines.c
  - tests/fixtures/play.c
  - tests/fixtures/play.h
  - tests/fixtures/tests.cmake
  - tests/fuzz/fuzz_run.c
  - tests/fuzz/fuzz_save.c
  - tests/fuzz/seeds.c
  - tests/fuzz/tests.cmake
  - tests/hygiene/fixtures/needs.txt
  - tests/hygiene/tests.cmake
  - tests/inventory/ci.txt
  - tests/inventory/fuzz.txt
  - tests/inventory/hygiene.txt
  - tests/inventory/library.txt
  - tests/inventory/nofp.txt
  - tests/inventory/wasm32.txt
  - tests/io/input_cases.h
  - tests/io/test_io.c
  - tests/io/tests.cmake
  - tests/library/tests.cmake
  - tests/libretro/battery.h
  - tests/libretro/controls.h
  - tests/libretro/host.c
  - tests/libretro/presentation.h
  - tests/libretro/tests.cmake
  - tests/mapper/test_battery.c
  - tests/mapper/tests.cmake
  - tests/perf/tests.cmake
  - tests/policy/budget_cases.cmake
  - tests/policy/e2e_cases.cmake
  - tests/policy/repository-checks.txt
  - tests/policy/tests.cmake
  - tests/retroarch/bmp.c
  - tests/retroarch/checkpoint.c
  - tests/retroarch/compare.c
  - tests/retroarch/flip.cmake
  - tests/retroarch/oracle.c
  - tests/retroarch/replay.c
  - tests/retroarch/run_retroarch.cmake
  - tests/retroarch/test.cfg.in
  - tests/roms/manifest.txt
  - tests/roms/src/battery.asm
  - tests/roms/src/battery.link
  - tests/roms/src/boot_to_play.asm
  - tests/roms/src/boot_to_play.link
  - tests/roms/src/boot_to_play_gg.asm
  - tests/roms/src/boot_to_play_gg.link
  - tests/roms/src/ports.asm
  - tests/roms/src/vdp_lines.asm
  - tests/roms/src/vdp_lines.link
  - tests/runner/expect.cmake
  - tests/runner/test_save.c
  - tests/runner/tests.cmake
  - tests/tests.cmake
  - tests/vdp/test_background.c
  - tests/vdp/test_geometry.c
  - tests/vdp/test_gg.c
  - tests/vdp/test_sprites.c
  - tests/vdp/test_timing.c
  - tests/vdp/tests.cmake
  - tools/cartdb.c
findings:
  critical: 1
  warning: 1
  info: 0
  total: 2
status: issues_found
---

# Phase 3: Code Review Report

**Reviewed:** 2026-10-20T23:16:05Z
**Depth:** standard
**Files Reviewed:** 152
**Status:** issues_found

## Narrative Findings (AI reviewer)

## Summary

The complete supplied scope was reviewed: core video, timing, input, cartridge metadata and persistence, public interfaces, runner, libretro adapter, fixtures, tests, build/CI and supporting documentation. Two product defects were confirmed: Windows PNG failure cleanup can address and delete a different file, and the runner loses the new VDP fault's symbolic identity.

Scope resolution was degraded with `no-task-commit-rows`. Review used all 152 existing files in the supplied union of SUMMARY paths and phase-range changes, with range base `d4159066396c0c28899b4c4559f142afc882d021`. The 15 `missingOnDisk` entries are deleted CMake helpers consolidated during this phase; they are not additional unreviewed files. No in-scope file was ignored. Generated metadata and inventory rows were inspected through their generator/consumer contracts and full-table checks as well as source review.

The preparation index, settled decisions, relevant video/system/cartridge/host references and phase 02 review disposition informed the review. Settled hardware models and previously deferred phase 02 items are not reopened. No external reviewer lane or Fallow pass was used.

## Critical Issues

### CR-01: Windows PNG failure cleanup can delete a different Unicode filename

**Classification:** BLOCKER
**File:** `runner/main.c:174-182`
**Related:** `runner/path.h:8-23`, `runner/main.c:513-529`

**Trigger:** Run the Windows runner with a non-UTF-8 active code page, a non-ASCII PNG filename, and a write or close failure after opening that output. If a file exists at the name produced by interpreting the UTF-8 bytes in the active code page, cleanup deletes that other file.

**Issue:** Windows arguments are now converted to UTF-8, and `cogmaster__path_open` explicitly converts UTF-8 back to UTF-16 before `_wfopen`. The error branch still calls narrow `remove(path)`. That call does not share the explicit UTF-8 conversion. With code page 1252, an output named `café.png` is opened correctly, but its UTF-8 bytes identify `cafÃ©.png` to the narrow removal API. A short write or failed close therefore leaves the partial intended output and can delete an unrelated existing file.

This mismatch is introduced by the phase's replacement of the PNG `fopen` with the wide-path helper while retaining narrow cleanup. The runner does not set a UTF-8 CRT locale or carry an active-code-page manifest. Microsoft documents that the CRT retains the active code page unless UTF-8 mode is enabled, and provides `_wremove` for wide paths. The wrong-file deletion is a source/API-contract inference, supported by a local encoding assertion; it was not exercised on a native Windows host. Sources: [CRT UTF-8 and active-code-page behavior](https://learn.microsoft.com/en-us/cpp/c-runtime-library/reference/setlocale-wsetlocale?view=msvc-170#utf-8-support), [remove and _wremove](https://learn.microsoft.com/en-us/cpp/c-runtime-library/reference/remove-wremove?view=msvc-170).

**Fix:** Add a removal helper beside `cogmaster__path_open` that uses the same `cogmaster__path_wide` conversion and `_wremove` or `DeleteFileW` on Windows, with `remove` on POSIX. Use it in this branch. The existing `save_remove` in `runner/save.c` already follows the correct conversion pattern. Verify the failure path with distinct intended and code-page-decoded filenames, forcing a short write or close failure and checking that only the intended partial PNG is removed.

## Warnings

### WR-01: The runner prints the new VDP fault as two unknown fields

**Classification:** WARNING
**File:** `runner/main.c:208-217`
**Related:** `include/cogmaster.h:79`, `include/cogmaster.h:87`, `src/vdp_render.c:179-182`, `docs/runner.md:166-167`

**Trigger:** Enable a legacy/unsupported video mode with display enabled and run with `--trace first-fault`.

**Issue:** The core now emits `COGMASTER_FAULT_KIND_UNSUPPORTED_VIDEO_MODE` and `COGMASTER_FAULT_DEVICE_VDP`, both value 6. The runner's name arrays stop at index 5, so a recognized core fault is reported as `dev=unknown kind=unknown`. The new mode fault is indistinguishable from an unrecognized record in the runner's diagnostic output. The runner's documented field list also omits the new names.

A 1 KiB synthetic ROM that writes display-enable to R1 while leaving Mode 4 off produced:

```text
fault frame=0 line=0 dot=256 pc=$0009 op=$76 addr=$0000 width=8 dev=unknown kind=unknown
```

**Fix:** Add `"unsupported_video_mode"` and `"vdp"` at their public enum indexes, preferably using designated indexes tied to the public constants. Update the runner field documentation and cover the actual symbolic output with a focused CLI case.

## Nonblocking test/tooling seeds

No additional test/tooling defect was confirmed. Under AGENTS.md rule 7, test or tooling observations would be nonblocking seeds rather than product requirements; neither finding above is a test/tooling finding.

## Review coverage and focused evidence

All 152 files listed in the frontmatter received standard-depth review. Important callers were followed across rendering/bus timing, completed-frame and LCD state, input and lag accounting, metadata/mapper/battery state, runner file handling, and frontend geometry and persistence. The generated cartridge table contains 1,657 records; full-table schema/range/geometry checks and sorted unique SHA-1 checks found no additional issue. All six scoped inventories were checked for sorted unique names (346 CI, 3 fuzz, 38 hygiene, 1 library, 18 no-float, 326 WASI).

These are local review checks, not phase verification or a full CI result. No native Windows runtime reproduction, hardware measurement or hosted RetroArch run is claimed. Run URL is not applicable to these local commands. No source was changed and no logs were committed.

| Command | Exit status | Observation |
|---|---:|---|
| `cmake --build build/ci --target cogmaster-run` | 0 | Current runner target; Ninja reported no work. |
| Synthetic-ROM command below | 0 | Runner exit 0; assertion confirms both VDP labels print as unknown. |
| Encoding/source-path command below | 0 | Intended UTF-16 path differs from CP1252 interpretation; source uses wide open and narrow remove. |
| Scope/table/inventory command below | 0 | 152 existing scoped files; 1,657 sorted unique metadata keys; all six inventories sorted and unique. |

Commands ran from the repository root. The synthetic ROM is temporary and is automatically removed.

```sh
python3 -c 'import pathlib,subprocess,tempfile; code=bytes([0xf3,0x3e,0x40,0xd3,0xbf,0x3e,0x81,0xd3,0xbf,0x76]); f=tempfile.NamedTemporaryFile(suffix=".sms"); f.write(code+bytes(1024-len(code))); f.flush(); p=subprocess.run(["build/ci/cogmaster-run",f.name,"--frames","2","--trace","first-fault"],text=True,capture_output=True); print(p.stdout,end=""); print("runner_exit="+str(p.returncode)); assert p.returncode==0 and "dev=unknown kind=unknown" in p.stdout'
```

```sh
python3 -c 'from pathlib import Path; p="caf\u00e9.png"; raw=p.encode("utf-8"); wrong=raw.decode("cp1252"); assert p!=wrong; print("UTF8 wide-open target:",ascii(p)); print("CP1252 narrow-remove target:",ascii(wrong)); source=Path("runner/main.c").read_text(); assert "cogmaster__path_open(path, \"wb\")" in source and "(void)remove(path);" in source; print("encoding and source-path assertions: OK")'
```

```sh
python3 -c 'from pathlib import Path; import json,re; s=json.loads(Path("/tmp/cogmaster-phase03-review-scope.json").read_text()); assert len(s["files"])==152 and all(Path(p).is_file() for p in s["files"]); rows=[x for x in Path("src/cartdb.inc").read_text().splitlines() if x.startswith("{")]; keys=[re.match(r"\{\"([0-9a-f]{40})\"",x).group(1) for x in rows]; assert len(rows)==1657 and keys==sorted(set(keys)); print("scope=152; cartdb=1657 sorted unique SHA-1 records"); inv=[Path(p) for p in s["files"] if p.startswith("tests/inventory/")]; counts={}; [(counts.update({p.name:len(a)}), (_ for _ in ()).throw(AssertionError(str(p))) if a!=sorted(set(a)) else None) for p in inv for a in [[x.split("\t")[0] for x in p.read_text().splitlines() if x and not x.startswith("#")]]]; print("inventory sorted/unique:",counts)'
```

---

_Reviewed: 2026-10-20T23:16:05Z_
_Reviewer: Codex (gsd-code-reviewer)_
_Depth: standard_
