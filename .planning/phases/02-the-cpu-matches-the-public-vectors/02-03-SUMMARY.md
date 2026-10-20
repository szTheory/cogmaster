---
phase: 02-the-cpu-matches-the-public-vectors
plan: 03
subsystem: z80
status: complete
tags:
  - z80
  - cpu
  - singlesteptests
  - vectors
requires:
  - "02-01: z80_cycle.h bus primitives, the harness and the committed sample"
  - "02-02: --allow and tests/vectors/z80/allow.txt"
provides:
  - "The complete Z80 main (unprefixed) page in src/z80.c: x/y/z decode of 0x40-0xBF, one-line cases elsewhere"
  - "Shared helpers in src/z80_cycle.h: flag bits COGMASTER__Z80_CF..SF, sz53, sz53p, alu, inc8, dec8, pair, set_pair"
  - "ctest vectors.base (portable;vectors), 25200 sampled tests, empty allow list"
affects:
  - "02-04 and 02-05: the DD/FD and CB/ED pages reuse the step, the static helpers in z80.c and the inline helpers in z80_cycle.h; DD and FD still only set idx, CB and ED still fall to the default arm"
actuals:
  tokens: 6850
  tasks: 2
  commits: 3
plan_head_before: f87214f837047fe13842af87f78e79de1368bb77
plan_head_after: 6d332d01101cf5ade262b6df37f86e2f2d6d4c23
tech-stack:
  added: []
  patterns:
    - "Main-page helpers take the opcode's field as a constant argument (register index, pair p, condition y, ALU op y); no function-pointer table, no flag table"
    - "Every flag write goes through cogmaster__z80_setf so Q follows F; EX AF,AF' and POP AF write F directly so Q stays 0"
    - "The step saves Q before clearing it, so SCF/CCF read the previous instruction's Q"
key-files:
  created: []
  modified:
    - src/z80.c
    - src/z80_cycle.h
    - tests/z80/tests.cmake
    - tests/inventory/ci.txt
    - tests/inventory/wasm32.txt
    - docs/conformance.md
decisions:
  - "Accumulator, INC/DEC and flag-bit helpers live as static inline functions in z80_cycle.h so the CB and ED pages reuse them; pair-, condition- and stack-level helpers stay static in z80.c until a second file needs them"
  - "A halted step runs one M1 at PC and steps PC back, so R counts and the step stays bounded"
  - "Idle T-states pass the address the bus would hold (IR after an opcode fetch, the last operand address otherwise); the harness compares only their position, since the stream builder repeats the previous address"
  - "requirements-completed stays empty: CPU-01 needs all seven groups, and this plan completes only base"
requirements-completed: []
coverage:
  - id: D1
    description: "The 128 x/y/z-decoded opcodes 0x40-0xBF (LD r,r', ALU A,r, the (HL) forms and HALT) match every sampled test on final state, RAM and every T-state"
    requirement: CPU-01
    verification:
      - kind: integration
        ref: "build/ci/tests/cogmaster-test-sst --sample tests/vectors/z80/sst-sample.bin --only 40,41,46,70,77,80,86,8e,96,9e,a6,ae,b6,be,bf --expect-count 1500"
        status: pass
    human_judgment: false
  - id: D2
    description: "The whole main page (252 files, first 100 tests each) matches the sample with an empty allow list, run in ci as vectors.base"
    requirement: CPU-01
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^vectors\\.base$' -V"
        status: pass
    human_judgment: false
  - id: D3
    description: "No function-pointer table and no flag lookup table in the Z80 sources"
    requirement: CPU-01
    verification:
      - kind: other
        ref: "grep -Eq '(\\(\\*|\\* *const).*\\[' src/z80.c and grep -Eq 'static const uint8_t [a-z_]*(flag|sz|parity)[a-z_]*\\[' src/z80.c src/z80_cycle.h both exit 1"
        status: pass
    human_judgment: false
  - id: D4
    description: "vectors.base is registered once and listed in the ci and wasm32 inventories; the ci workflow passes"
    requirement: CPU-01
    verification:
      - kind: integration
        ref: "cmake --workflow --preset ci"
        status: pass
    human_judgment: false
metrics:
  duration: 5min
  completed: 2026-10-20
---

# Phase 2 Plan 3: The Z80 main page Summary

The whole unprefixed Z80 page in `src/z80.c` (587 lines): `0x40-0xBF` decoded by x/y/z fields, every other opcode
a one-line case calling a helper with constant arguments, flags computed inline, Q, WZ, `ei` and HALT as the
vectors expect. `vectors.base` passes all 25,200 sampled tests on final state and every T-state with an empty allow
list.

## What was built

- **Task 1 (tracer):** `block()` decodes `0x40-0xBF` (LD r,r', ALU A,r, `(HL)` forms, HALT). The flag helpers
  (`sz53`, `sz53p` with XOR-fold parity, `alu`, `inc8`, `dec8`) and the flag bit names went into `z80_cycle.h`.
  A halted step runs one M1 at PC and steps PC back. All 128 block files passed on the first run (12,800 tests).
- **Task 2 (TDD):** RED commit registered `vectors.base` (12,175 failures at that point). GREEN commit added the
  rest of the page: 16-bit loads and INC/DEC, ADD HL,rr, rotates on A, DAA, CPL, SCF/CCF reading the previous
  step's Q, the exchanges, JR/DJNZ/JP/CALL/RET/RST with their internal T-states, PUSH/POP, `IN A,(n)` and
  `OUT (n),A`, DI/EI. The full group passed on the first build that compiled. No refactor commit was needed.
- `docs/conformance.md` gained a short "The shards" section.

## Verification

- `cmake --workflow --preset ci`: exit 0.
- `ctest --test-dir build/ci --no-tests=error -R '^vectors\.base$' --output-on-failure`: exit 0; the JSON listing
  holds exactly one test; `-V` output contains `executed 25200`.
- Allow list: 0 non-comment lines. `wc -l < src/z80.c`: 587, under 800.
- Both no-table greps exit 1.

## TDD Gate Compliance

- RED: `242a723` test(02-03), with `vectors.base` failing 12,175 of 25,200.
- GREEN: `6d332d0` feat(02-03), with `vectors.base` passing.
- REFACTOR: not needed.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 2 - Docs] A shard paragraph in docs/conformance.md**
- **Found during:** Task 2
- **Issue:** project rule 5 wants docs to land with the behaviour; the conformance guide did not name the shard.
- **Fix:** added "The shards" section naming `vectors.base` and its count.
- **Files modified:** docs/conformance.md
- **Commit:** 6d332d0

Two `-Wsign-conversion` errors were fixed before commit (casts only), so they are not listed as deviations.

## Known Stubs

None. CB, ED, and the DD/FD continuations fall to the default arm by design; plans 04 and 05 fill them.

## Commits

- `ae8d3e5` feat(02-03): x/y/z decode of the z80 load and accumulator block
- `242a723` test(02-03): vectors.base shard over the sampled main page
- `6d332d0` feat(02-03): the rest of the z80 main page

## Self-Check: PASSED
