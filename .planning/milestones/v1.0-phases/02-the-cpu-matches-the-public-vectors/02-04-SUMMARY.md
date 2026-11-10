---
phase: 02-the-cpu-matches-the-public-vectors
plan: 04
subsystem: z80
status: complete
tags:
  - z80
  - cpu
  - singlesteptests
  - vectors
requires:
  - "02-03: the main page, the step, and the flag helpers in z80_cycle.h"
provides:
  - "src/z80_cb.c: cogmaster__z80_cb, the CB page by x/y/z fields and DD CB / FD CB"
  - "The DD/FD prefix model in src/z80.c: each prefix byte its own step, hx/r8/mem_hl remapping"
  - "ctest vectors.cb, vectors.dd, vectors.fd, vectors.ddcb, vectors.fdcb (portable;vectors), empty allow list"
affects:
  - "02-05: the ED page must ignore idx (DD ED runs as the ED instruction); rp() in z80.c now remaps p = 2 under a prefix, so ED code should build pairs with cogmaster__z80_pair directly"
  - "Interrupt acceptance (later plan): must not accept while z->idx is set"
actuals:
  tokens: 4970
  tasks: 3
  commits: 3
plan_head_before: 30ef738c32cf77b3609705b0f1e67be02ee2f85f
plan_head_after: 9e63ce2566fed02541f05d721aaf434e1d0f7297
tech-stack:
  added: []
  patterns:
    - "z->idx lives across steps: the prefix step sets it and returns; every other step runs exec() and clears it at the end"
    - "hx(z) gives the high-byte index of HL, IX or IY; r8() remaps register fields 4 and 5; mem_hl(m, n) returns HL or reads d, idles n T-states and sets WZ"
key-files:
  created:
    - src/z80_cb.c
  modified:
    - src/z80.c
    - src/z80.h
    - tests/z80/tests.cmake
    - tests/inventory/ci.txt
    - tests/inventory/wasm32.txt
    - docs/conformance.md
decisions:
  - "The prefix is consumed by the step after it (no loop): step() returns right after a DD/FD fetch with idx set, and clears idx after any other opcode"
  - "Index remapping is centralised in hx/r8/mem_hl/rp_hi rather than branching on idx in each helper; EX DE,HL and EXX use the true HL pair explicitly"
  - "The five new shards share one foreach registration in tests/z80/tests.cmake"
  - "requirements-completed stays empty: CPU-01 also needs the ED group (plan 05)"
requirements-completed: []
coverage:
  - id: D1
    description: "The CB page decoded by x/y/z matches all 25600 sampled tests on final state, RAM and every T-state"
    requirement: CPU-01
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^vectors\\.cb$' -V"
        status: pass
    human_judgment: false
  - id: D2
    description: "DD and FD prefixes are steps of their own and the indexed pages match 25200 sampled tests each"
    requirement: CPU-01
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci --no-tests=error -R '^vectors\\.(dd|fd)$' --output-on-failure"
        status: pass
    human_judgment: false
  - id: D3
    description: "No loop over prefix bytes in the step"
    requirement: CPU-01
    verification:
      - kind: other
        ref: "grep -Ec '(while|for) *\\(.*(0xDD|0xdd|0xFD|0xfd)' src/z80.c prints 0"
        status: pass
    human_judgment: false
  - id: D4
    description: "DD CB and FD CB read d and op as memory reads, copy the result to register z when z is not 6, and match 25600 sampled tests each"
    requirement: CPU-01
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^vectors\\.(ddcb|fdcb)$' -V"
        status: pass
    human_judgment: false
  - id: D5
    description: "Twelve vectors-labelled tests pass, every inventory entry ran once, and the ci workflow passes"
    requirement: CPU-01
    verification:
      - kind: integration
        ref: "cmake --workflow --preset ci; cmake -DJUNIT=build/ci/junit.xml -DINVENTORY=tests/inventory/ci.txt -DEXCLUDE=release -P tests/cmake/check_junit.cmake"
        status: pass
    human_judgment: false
metrics:
  duration: 10min
  completed: 2026-10-20
---

# Phase 2 Plan 4: The CB page, DD/FD prefixes and DDCB/FDCB Summary

The Z80 CB page (`src/z80_cb.c`, 108 lines, all x/y/z) and the DD/FD prefix model in `src/z80.c` (652 lines).
Each prefix byte is its own 4 T-state step that leaves `idx` set for the next step. Under `idx`, H/L remap to the
index halves and `(HL)` becomes `(IX+d)` with WZ set. DD CB and FD CB read d and op as plain memory reads. Five
new shards pass with an empty allow list: cb 25,600, dd 25,200, fd 25,200, ddcb 25,600 and fdcb 25,600.

## What was built

- **Task 1 (tracer):** `cogmaster__z80_cb` handles rotates and shifts (including SLL), BIT (X/Y from WZ's high
  byte on `(HL)`), RES and SET, plus the 4+3+1+3 `(HL)` layout. All of `vectors.cb` passed on the first run, so the
  tracer gate re-ran green before any expansion.
- **Task 2:** `step()` keeps `idx` across steps: a DD/FD fetch sets it and returns, and every other opcode runs
  `exec()` and clears it. `hx`, `r8`, `mem_hl` and `rp_hi` hold all the remapping. `LD (IX+d),n` reads d and n,
  then idles 2 T-states. EX DE,HL and EXX keep the true HL. Both shards passed on the first build.
- **Task 3:** registered `vectors.ddcb` and `vectors.fdcb`, merged all five new registrations into one `foreach`,
  and extended the "The shards" paragraph in `docs/conformance.md`.

## Verification

- `cmake --workflow --preset ci`: exit 0.
- `ctest --test-dir build/ci --no-tests=error -L '^vectors$'`: exit 0; the JSON listing has exactly 12 tests.
- The `-V` output shows `executed 25600` for cb, ddcb and fdcb and `executed 25200` for dd and fd, each with
  `failed 0`.
- `check_junit.cmake` against `tests/inventory/ci.txt`: exit 0 (92 tests ran once each).
- Prefix-loop grep prints 0. Line counts: `src/z80_cb.c` 108, `src/z80.c` 652, both under 800.
- Allow list: 0 non-comment lines.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] The step cleared idx at its start**
- **Found during:** Task 2
- **Issue:** plan 03's step reset `idx` before decoding, so a prefix could never reach the next step.
- **Fix:** split the switch into `exec()`. `step()` returns right after a DD/FD fetch and clears `idx` only after a
  non-prefix opcode.
- **Files modified:** src/z80.c
- **Commit:** bfa1d50

**2. [Ordering] The DD CB / FD CB code landed with Task 1**
- **Found during:** Task 1
- **Issue:** the CB entry was written once with its indexed branch (`indexed()`), so Task 3 only had to register
  the shards and update the docs.
- **Files modified:** src/z80_cb.c
- **Commit:** 80d9f24

**3. [Rule 2 - Docs] Shard list in docs/conformance.md**
- **Found during:** Task 3
- **Fix:** named the five new shards and the prefix-step comparison rule.
- **Commit:** 9e63ce2

## Known Stubs

None. The ED page still falls to the default arm by design; plan 05 fills it.

## Commits

- `80d9f24` feat(02-04): z80 cb page by x/y/z fields and the vectors.cb shard
- `bfa1d50` feat(02-04): dd and fd prefixes as steps of their own, vectors.dd and vectors.fd
- `9e63ce2` test(02-04): vectors.ddcb and vectors.fdcb shards

## Self-Check: PASSED
