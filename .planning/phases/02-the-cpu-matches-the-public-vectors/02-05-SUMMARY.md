---
phase: 02-the-cpu-matches-the-public-vectors
plan: 05
subsystem: z80
status: complete
tags:
  - z80
  - cpu
  - interrupts
  - singlesteptests
  - vectors
requires:
  - "02-04: the DD/FD prefix model, idx across steps, z80_cb.c"
provides:
  - "src/z80_ed.c: cogmaster__z80_ed, the ED page with the 16 block operations and the 176 holes"
  - "Interrupt and NMI acceptance in cogmaster__z80_step, each its own step (NMI 11 T, IM 0/1 13 T, IM 2 19 T)"
  - "ctest vectors.ed (portable;vectors) and 13 z80.* unit tests (portable;z80) from cogmaster-test-z80int"
affects:
  - "Later bus work: the library's bus must define cogmaster__bus_ed_undefined (the fault seam) and drive z80.int_line / z80.nmi_pending"
  - "IM 0 assumes $FF on the data bus; a profile with a device driving the bus during acknowledge would need a seam"
actuals:
  tokens: 8600
  tasks: 2
  commits: 3
plan_head_before: 088d7c62ea94c2c9f39d01356f509d6d599ea37f
plan_head_after: d3ddf4f67b6ea11434b4e8443515cbebc8a0acd1
tech-stack:
  added: []
  patterns:
    - "Block instructions are one iteration per step; a repeat idles 5 T, moves PC back two, sets WZ = PC + 1 and takes Y/X from PC's high byte"
    - "The step saves q, p and ei of the previous step before clearing them; acceptance reads p_prev and ei_prev"
    - "ED code builds pairs with cogmaster__z80_pair directly so a pending DD/FD never remaps HL"
key-files:
  created:
    - src/z80_ed.c
    - tests/z80/int_tests.c
  modified:
    - src/z80.c
    - src/z80.h
    - tests/z80/tests.cmake
    - tests/inventory/ci.txt
    - tests/inventory/wasm32.txt
    - docs/conformance.md
decisions:
  - "An NMI is not delayed by EI; only /INT waits one instruction after EI (UM0080; Sean Young 5.5; hoglet67 NMI-during-EI notes). Both wait for a pending prefix"
  - "ED holes are every ED opcode outside 40-7F and the 16 block ops (176 in all); 77 and 7F are plain 8-T NOPs that do not reach the seam, because the vectors cover them"
  - "The interrupt acknowledge has no bus seam: it costs 6 T, counts in R and reads $FF"
  - "cogmaster-test-z80int reuses the cogmaster-z80-sst object library rather than compiling the z80 sources a second time"
  - "requirements-completed stays empty: CPU-01 also needs the nightly full-set lane (plan 17)"
requirements-completed: []
coverage:
  - id: D1
    description: "The ED page, block operations included, matches all 8000 sampled ED tests on final state and every T-state with an empty allow list"
    requirement: CPU-01
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^vectors\\.ed$' -V"
        status: pass
    human_judgment: false
  - id: D2
    description: "All seven shards pass and the vectors label selects 13 tests"
    requirement: CPU-01
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci --no-tests=error -L '^vectors$' --output-on-failure"
        status: pass
    human_judgment: false
  - id: D3
    description: "IM 0, IM 1 and IM 2 acceptance, EI delay, DI, NMI, RETN, LD A,I P/V clear, HALT exit and acceptance between LDIR iterations"
    requirement: CPU-01
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci --no-tests=error -R '^z80\\.int\\.' --output-on-failure"
        status: pass
    human_judgment: false
  - id: D4
    description: "A 65536-byte DD run advances in 4 T steps with no acceptance, and R counts one per opcode fetch with bit 7 kept"
    requirement: CPU-01
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci -R '^z80\\.int\\.(prefix_run|r_prefixes)$'"
        status: pass
    human_judgment: false
  - id: D5
    description: "Every ED hole is an 8 T no-operation that adds 2 to R, changes nothing else and reaches the ed_undefined seam once"
    requirement: CPU-01
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci -R '^z80\\.ed\\.holes$'"
        status: pass
    human_judgment: false
  - id: D6
    description: "The ci workflow passes and every inventory entry ran once"
    requirement: CPU-01
    verification:
      - kind: integration
        ref: "cmake --workflow --preset ci; cmake -DJUNIT=build/ci/junit.xml -DINVENTORY=tests/inventory/ci.txt -DEXCLUDE=release -P tests/cmake/check_junit.cmake"
        status: pass
    human_judgment: false
metrics:
  duration: 25min
  completed: 2026-10-19
---

# Phase 2 Plan 5: The ED page, interrupt and NMI acceptance Summary

`src/z80_ed.c` (335 lines) holds the ED page as one x/y/z switch. Block instructions run one iteration per
step. A repeat idles 5 T-states, moves PC back two, sets WZ = PC + 1 and takes Y/X from PC's high byte; INxR and
OTxR also adjust P/V and H. Holes are 8 T-state NOPs that call `cogmaster__bus_ed_undefined`. `vectors.ed`
passed all 8,000 sampled tests on its first build. The step in `src/z80.c` (723 lines) now accepts an NMI or
/INT as a step of its own. 13 scripted unit tests pin the behaviour the vectors leave out.

## What was built

- **Task 1 (tracer):** the ED page: IN/OUT (C) with `OUT (C),0` writing 0, ADC/SBC HL, LD (nn),rr, NEG,
  RETN/RETI (IFF1 = IFF2), IM, LD I/R,A, LD A,I/R (`p` = 1), RRD/RLD and the 16 block operations. `vectors.ed`
  is registered in both inventories, and the vectors label now selects 13 tests. The tracer gate re-ran green
  (13/13) before Task 2.
- **Task 2 (TDD):** RED commit `3c36885`: 9 of 13 cases failed (every acceptance case). The prefix-run, R and
  hole cases already passed against Task 1's code. GREEN commit `d3ddf4f`: `nmi()` and `irq()` run at an
  instruction boundary with no prefix pending. /INT also waits on the previous step's EI mark. An accepted
  interrupt clears `halted` and, right after LD A,I/R, clears P/V. `docs/conformance.md` gains the ED shard and
  a "What the vectors do not cover" section that names each test.

## Verification

- `cmake --workflow --preset ci`: exit 0.
- `ctest -L '^vectors$'`: 13 tests, all pass. `vectors.ed -V` prints `executed 8000` and `failed 0`.
- `ctest -R '^z80\.'`: 13 tests, all pass.
- `check_junit.cmake` against `tests/inventory/ci.txt`: exit 0 (106 tests ran once each).
- Line counts: `src/z80_ed.c` 335, `src/z80.c` 723, both under 800. The planning-ID grep over src, tests, docs
  and include finds nothing. Allow list: 0 non-comment lines.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Correctness] NMI is not blocked by EI**
- **Found during:** Task 2
- **Issue:** the plan's action gates both NMI and /INT on "ei clear". The EI delay applies to the maskable
  interrupt only (UM0080 "EI"; the hoglet67 NMI-during-EI notes describe an NMI accepted right after EI).
- **Fix:** NMI waits only for a pending prefix. `z80.int.nmi` checks that an NMI right after EI is accepted and
  keeps IFF2 = 1.
- **Files modified:** src/z80.c, tests/z80/int_tests.c
- **Commit:** d3ddf4f

**2. [Rule 2 - Style] Planning IDs in hardware comments**
- **Found during:** Task 2 acceptance check
- **Issue:** the first draft cited the preparation pack's fact IDs. The acceptance grep forbids them in src.
- **Fix:** the comments now cite the primary documents: Sean Young's sections, UM0080, the stardot thread, the
  msx.org thread, the MEMPTR document and MacDonald's smstech.
- **Commit:** d3ddf4f

**3. [Scope] Holes include A4-A7, AC-AF, B4-B7 and BC-BF**
- The plan lists holes as "00-3F, 80-9F except the block ops, C0-FF". The other x = 2 opcodes outside the block
  ops are holes too and are not in the vectors, so the hole test covers all 176.

## Known Stubs

None.

## TDD Gate Compliance

- RED: `3c36885` test(02-05), with 9 failing cases.
- GREEN: `d3ddf4f` feat(02-05), with all 13 passing.
- REFACTOR: none needed.

## Commits

- `fcca064` feat(02-05): z80 ed page with block operations and holes, vectors.ed
- `3c36885` test(02-05): failing interrupt, nmi, ei delay and halt exit cases
- `d3ddf4f` feat(02-05): interrupt and nmi acceptance as steps of their own

## Self-Check: PASSED
