---
phase: 02-the-cpu-matches-the-public-vectors
plan: 01
subsystem: z80-vectors
status: complete
tags:
  - z80
  - singlesteptests
  - vectors
  - harness
requires:
  - "phase 01: src/internal.h instance, cogmaster_warnings, core compile options, scripts/fetch-pin.sh and pins.txt, scripts/hygiene.sh manifest hook"
provides:
  - "cogmaster-sst2bin: fixed-schema JSON converter to the access-form sample, refusing missing keys and non-rebuilding streams"
  - "tests/z80/sst_stream.c: the one stream builder, file-name rule and sample codec shared by converter and harness"
  - "cogmaster-test-sst: link-seam harness comparing 25 state fields, final RAM and every T-state; --sample/--group/--only/--expect-count"
  - "src/z80.h, src/z80_cycle.h, src/z80.c: Z80 state embedded in struct cogmaster, cycle primitives at 3 ticks per T-state, step with NOP, LD (BC),A and DD/FD prefix steps"
  - "tests/vectors/z80/sst-sample.bin (13,017,674 B) with its manifest row, notice and sst-z80 pins row"
affects:
  - "02-02: negative controls, converter refusal tests, allow list and vectors.* CTest names build on these executables"
  - "02-03 to 02-05: every later opcode plan grows src/z80.c against this harness"
  - "02-06: adds src/z80.c and the real bus seam to the library"
actuals:
  tokens: 15700
  tasks: 2
  commits: 2
plan_head_before: 68116afa15c26944e378984192dba2ab02f5a7d1
plan_head_after: 59dfdacca488a010ecdafbe0a6a566d234d0300c
tech-stack:
  added:
    - "SingleStepTests z80 v1 data at ebe1875d48f3 (MIT), fetched through scripts/fetch-pin.sh into build/pins/sst-z80"
  patterns:
    - "Every bus primitive calls the seam at its data T-state (T3 for M1 and memory, T4 for I/O); the harness subtracts a per-kind offset to get the cycle's start"
    - "Access-form records: each test stores bus cycles, not T-states; sst_stream_build expands both expected and actual"
    - "Final state stored as a 25-bit change mask over the initial state"
    - "The z80 sources build as an OBJECT library with the core's compile options and enter the harness through TARGET_OBJECTS, never through the cogmaster archive"
key-files:
  created:
    - src/z80.h
    - src/z80_cycle.h
    - src/z80.c
    - tests/z80/tests.cmake
    - tests/z80/sst_stream.h
    - tests/z80/sst_stream.c
    - tests/z80/sst2bin.c
    - tests/z80/sst_bus.h
    - tests/z80/sst_bus.c
    - tests/z80/sst_run.c
    - tests/vectors/z80/sst-sample.bin
    - tests/roms/manifest.txt
  modified:
    - src/internal.h
    - tests/tests.cmake
    - pins.txt
    - THIRD-PARTY-NOTICES.md
    - .gitignore
    - .gitattributes
decisions:
  - "Sample magic CGM-Z80V, version 1; per file a u16 index in closed-name order so a --files subset stays self-describing"
  - "Memory writes are stamped at T3 like reads (upstream shows the data at T2); the harness offset table absorbs it"
  - "The register file is reg[] and the refresh register r, since one struct cannot hold both names as r"
  - "An M1 and a memory read share their first two T-states; the converter tells them apart by the refresh address on T3, and when the refresh address equals the fetch address both forms rebuild the same stream"
requirements-completed: []
coverage:
  - id: D1
    description: "NOP passes all 100 sampled vectors on final state and every T-state through the committed sample"
    requirement: CPU-01
    verification:
      - kind: unit
        ref: "build/ci/tests/cogmaster-test-sst --sample tests/vectors/z80/sst-sample.bin --only 00 --expect-count 100"
        status: pass
    human_judgment: false
  - id: D2
    description: "LD (BC),A passes all 100 sampled vectors including WZ and the write cycle; with NOP 200 executed and none failed"
    requirement: CPU-01
    verification:
      - kind: unit
        ref: "build/ci/tests/cogmaster-test-sst --sample tests/vectors/z80/sst-sample.bin --only 00,02 --expect-count 200"
        status: pass
    human_judgment: false
  - id: D3
    description: "A DD or FD byte is its own step; the harness steps until no prefix is pending, and dd 00 and fd 00 pass 200 of 200"
    requirement: CPU-01
    verification:
      - kind: unit
        ref: "build/ci/tests/cogmaster-test-sst --sample tests/vectors/z80/sst-sample.bin --only dd 00,fd 00"
        status: pass
    human_judgment: false
  - id: D4
    description: "The converter rebuilds every upstream stream from its bus cycles: 160,400 sampled tests and all 1,604,000 tests at the pin"
    requirement: CPU-01
    verification:
      - kind: other
        ref: "build/ci/tests/cogmaster-sst2bin --dir build/pins/sst-z80/v1 --count 1000 --out <scratch>/full.bin"
        status: pass
    human_judgment: false
  - id: D5
    description: "The committed sample is 13,017,674 B, listed with its SHA-256 in tests/roms/manifest.txt and noticed; hygiene accepts the tree"
    requirement: CPU-01
    verification:
      - kind: other
        ref: "sh scripts/hygiene.sh --tree"
        status: pass
      - kind: integration
        ref: "ctest --preset hygiene (hygiene.tree, hygiene.format, hygiene.analyzer, policy.notices)"
        status: pass
    human_judgment: false
  - id: D6
    description: "The ci workflow still passes with the new sources and targets"
    requirement: CPU-01
    verification:
      - kind: integration
        ref: "cmake --workflow --preset ci"
        status: pass
    human_judgment: false
duration: 10min
completed: 2026-10-19
---

# Phase 2 Plan 01: Z80 vector pipeline Summary

**SingleStepTests z80 vectors run end to end. A C converter writes a 13 MB access-form sample, and a link-seam harness rebuilds every T-state through the same stream builder. The new Z80 core passes all 100 sampled vectors each for NOP and LD (BC),A, and all 200 for the DD/FD 00 prefix steps.**

## Performance

- **Duration:** about 10 min
- **Started:** 2026-10-20T02:06:17Z
- **Tasks:** 2 of 2
- **Files:** 18 created or modified

## Accomplishments

- `cogmaster-sst2bin` reads the 1,604 upstream files by the closed name rule, which has no directory listing and handles names that contain spaces. It parses the fixed schema and fails naming a missing key. It skips unknown keys. It exits 1 naming the file and test when a stream does not rebuild. A run converts all 160,400 sampled tests in under a second. A run with `--count 1000` shows that every one of the 1,604,000 tests at the pin rebuilds.
- `sst_stream_build` holds the D-12 templates in one place. Upstream's memory read template is `[a,-,----][a,-,r-m-][a,d,----]`, measured from `7e`.
- `cogmaster-test-sst` reads the sample with one `fread` and restores only the touched RAM. It steps until no prefix is pending, then compares the 25 fields, the final RAM, the total length and each T-state. It prints `mismatch "<file>" test <n> <field>: expected …, actual …`, then `failed N` and `executed N`.
- The Z80 core lives in `src/z80.h`, `src/z80_cycle.h` and `src/z80.c`. M1 forms the refresh address from R before the increment and keeps bit 7. Each step clears q, p, ei and idx. Opcodes 00, 02, DD and FD are implemented.

## Sample

- Path: `tests/vectors/z80/sst-sample.bin`
- Size: 13,017,674 B, between the 8,000,000 B floor and the 16 MiB cap.
- SHA-256: `189a33d4396f13b2cb93acbcc5da42fb7d5da0176f2558aaf09ae10d23f26bda`
- Generated by `build/ci/tests/cogmaster-sst2bin --dir build/pins/sst-z80/v1 --count 100 --out tests/vectors/z80/sst-sample.bin`. The archive was fetched with `sh scripts/fetch-pin.sh sst-z80 build/pins/sst-z80`, which checked its digest.

## Task Commits

1. **Task 1 (tracer): converter, sample, harness and core with NOP:** `3d77906`. The tracer gate re-ran the `<verify>` commands: the `--only 00 --expect-count 100` harness run exited 0, and so did `hygiene.sh --tree`.
2. **Task 2 (tdd): LD (BC),A with WZ, and the DD/FD prefix steps:** `59dfdac`

## TDD Gate Compliance

- RED: before Task 2, `cogmaster-test-sst --only 00,02 --expect-count 200` exited 1 with `failed 100`. The first line was `mismatch "02" test 0 wz: expected 41503, actual 41029`. The test is the committed harness and sample, so RED needed no new test commit.
- GREEN: after `59dfdac` the same command exits 0 and prints `failed 0` and `executed 200`.
- REFACTOR: none needed.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] `.gitignore` ignored the sample**
- **Found during:** Task 1
- **Issue:** `*.bin` is ignored everywhere except `/tests/roms/**`, so `tests/vectors/z80/sst-sample.bin` could not be committed.
- **Fix:** added the exception `!/tests/vectors/z80/sst-sample.bin`. Also added `/tests/vectors/** -text` to `.gitattributes`, so git never rewrites the hashed bytes. This is the same rule `/tests/roms/**` has.
- **Commit:** `3d77906`

**2. [Rule 3 - Blocking] One extra header, `tests/z80/sst_bus.h`**
- **Found during:** Task 1
- **Issue:** `sst_bus.c` and `sst_run.c` share the bus context and the access log, and the plan lists no header for them.
- **Fix:** a 40-line header declaring `struct sst_bus` and `sst_bus_load`.
- **Commit:** `3d77906`

**3. [Naming] Register file `reg[]`, primitives prefixed `cogmaster__z80_*`**
- The plan names both the register file and the refresh register `r`, which one struct cannot hold. The file is `reg[]` and refresh is `r`.
- The primitives are named `cogmaster__z80_m1`, `cogmaster__z80_mread` and so on, following the module-prefix style rule. Behaviour is as planned.

**4. [Build] z80 sources as an OBJECT library**
- MSVC's `/Zl` in the core's options would strip the CRT from a whole executable. So only `src/z80.c` gets the core options, through `cogmaster-z80-sst` (OBJECT) and `$<TARGET_OBJECTS:…>`. The harness still never links the `cogmaster` archive, and both acceptance greps hold.

## Known Stubs

- `src/z80.c`'s default arm executes any opcode other than 00, 02, DD and FD as a 4 T-state M1 only. This is intentional. Plans 02-03 to 02-05 fill the main, CB, DD/FD and ED pages against this harness. The `vectors.*` CTest names in plan 02-02 make the gaps visible.

## Threat Flags

None. The converter and harness are test-only host tools. `sst_decode` bounds-checks every field read from the sample.

## Requirements

CPU-01 progressed but is not complete: only two opcodes and the prefix step match so far. Plans 02-02 to 02-05 and 02-17 complete it, so `requirements-completed` is empty and REQUIREMENTS.md is left unchanged.

## Self-Check: PASSED

- All 12 created files exist on disk.
- Commits `3d77906` and `59dfdac` are ancestors of HEAD.
