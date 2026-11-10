---
phase: 02-the-cpu-matches-the-public-vectors
plan: 08
subsystem: vdp
status: complete
tags:
  - vdp
  - bus
  - interrupts
  - pause
  - state
requires:
  - "02-06: the bus with its frame origin and deadline"
  - "02-07: region and tv, cogmaster__frame_lines, the PAL frame"
provides:
  - "src/vdp.c: frame flag at tick 607 of line 192 (VDP_INT_TICK), /INT = flag AND R1 IE0, status read with bits 4-0 set"
  - "V counter (NTSC-192 and PAL-192 tables, step at tick 604) and live H counter (jump at tick 592)"
  - "Control port two-byte latch, codes 0-3, R0-R10; data port with auto-increment, read-ahead, VRAM 16 KiB and CRAM 32 bytes"
  - "Pause /NMI at tick 611 of line 260 (NTSC) or 311 (PAL) from m->vdp.pause; none on the Game Gear"
  - "VDP port decode per profile in bus.c: A7/A6/A0 mirrors on the export SMS, $7E/$7F/$BE/$BF only on the Japanese SMS and the Game Gear"
  - "m->bus.vdp_at, the tick of the last VDP port access"
  - "State: the VDP after the beam, with ranges and a /INT cross check; bus_vdp_at"
affects:
  - "Plan 10 (ports): the I/O port decode extends vdp_port's switch in bus.c; the Pause press becomes a public input that sets m->vdp.pause"
  - "Phase 3 (renderer): VDP_INT_TICK and the V counter tables move with the 224/240-line modes; the frame origin in bus.c does not"
actuals:
  tokens: 17400
  tasks: 2
  commits: 3
plan_head_before: dccc5cb1adf4e88d118f9e5dfc7f10764a13937e
plan_head_after: 946ecf25226a861c0990f48bb9a0cf7e5e87bcd2
tech-stack:
  added: []
  patterns:
    - "The bus brings the beam up to each access stamp in segments, firing each VDP event whose tick the beam reaches on the way; an event at the beam's own tick counts as passed"
    - "The V and H counters are pure functions of the beam (line, t) and the TV, so they need no saved state"
key-files:
  created:
    - src/vdp.c
    - tests/bus/test_vdp.c
  modified:
    - CMakeLists.txt
    - src/internal.h
    - src/bus.c
    - src/instance.c
    - src/state.c
    - tests/bus/test_bus.c
    - tests/bus/tests.cmake
    - tests/core/test_state.c
    - tests/inventory/ci.txt
    - tests/inventory/wasm32.txt
    - docs/embedding.md
    - .planning/preparation/ARCHITECTURE.md
    - .planning/preparation/SMS-SYSTEM.md
    - .planning/preparation/SMS-VDP.md
decisions:
  - "The V counter is computed from the beam at each read (the line's value before tick 604, the next line's from 604) instead of being stored by a 604 event: same reads, no extra state"
  - "The VDP access tick lives in the bus struct as vdp_at, so the VDP struct is all 8- and 16-bit fields with one size on every target"
  - "A first control byte sets the address's low byte at once; the second sets the high bits and the code"
  - "Data-port reads return the read-ahead byte and refill it from the current address before stepping, so code 0 then reads stream VRAM in order"
  - "The Game Gear profile raises no Pause NMI: in Game Gear mode Start is only a port bit"
  - "VDP reset follows the fast-boot table: R2 $0E, R10 $FF, others 0; soft reset keeps VRAM and CRAM, power and load clear them"
requirements-completed:
  - CPU-02
coverage:
  - id: D1
    description: "EI; HALT with R1 IE0 set and a $0038 handler that reads $BF and increments $C100 gives exactly +1 per frame over 10 frames"
    requirement: CPU-02
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci --no-tests=error -R '^bus\\.halt_bound\\.ie_positive$' --output-on-failure"
        status: pass
    human_judgment: false
  - id: D2
    description: "EI; HALT with IE0 clear runs 10 frames on NTSC and PAL, each FRAME with overshoot at most 69 ticks, exact totals, CPU still halted, frame flag pending and /INT high"
    requirement: CPU-02
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci --no-tests=error -R '^bus\\.halt_bound\\.ei_masked$' --output-on-failure"
        status: pass
    human_judgment: false
  - id: D3
    description: "A JP loop of OUT ($BF),A and OUT ($BE),A returns every frame on NTSC and PAL with overshoot at most 69 ticks and exact totals"
    requirement: CPU-02
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci --no-tests=error -R '^bus\\.hostile_vdp_loop$' --output-on-failure"
        status: pass
    human_judgment: false
  - id: D4
    description: "For OUT (n),A, OUT (C),r, OUTI and IN A,(n) the VDP access tick is the start tick plus 3 x T-state 10, 11, 15 and 10; LD (nn),A and LD A,(nn) end 39 ticks after they start"
    requirement: CPU-02
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci --no-tests=error -R '^bus\\.stamps$' --output-on-failure"
        status: pass
    human_judgment: false
  - id: D5
    description: "The frame flag rises at tick 607 of line 192 only; a status read returns it with bits 4-0 set, clears it and releases /INT; /INT follows the flag AND IE0"
    requirement: CPU-02
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci --no-tests=error -R '^vdp\\.status$' --output-on-failure"
        status: pass
    human_judgment: false
  - id: D6
    description: "$7E matches the NTSC-192 and PAL-192 read sequences on every line at ticks 603 and 604; $7F matches the pixel-counter formula for t 0 to 683"
    requirement: CPU-04
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci --no-tests=error -R '^vdp\\.(vcounter\\.ntsc|vcounter\\.pal|hcounter)$' --output-on-failure"
        status: pass
    human_judgment: false
  - id: D7
    description: "Control port latch, codes 0-3, R0-R10 stored and R11-R15 ignored, latch reset by status reads and data accesses; data port auto-increment with 14-bit wrap, read-ahead, VRAM and CRAM stores"
    requirement: CPU-04
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci --no-tests=error -R '^vdp\\.(control_port|data_port)$' --output-on-failure"
        status: pass
    human_judgment: false
  - id: D8
    description: "On the export SMS every port in $40-$BF reaches the VDP by A6/A0; on the Japanese SMS and the Game Gear only $7E, $7F, $BE and $BF do, and every other port reads $FF and leaves the VDP unchanged"
    requirement: CPU-04
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci --no-tests=error -R '^vdp\\.port_decode$' --output-on-failure"
        status: pass
    human_judgment: false
  - id: D9
    description: "Pause latched during line 100 raises the NMI in the step that crosses tick 611 of line 260 (NTSC) or 311 (PAL); the $0066 handler's IN A,($7E) reads $FF, one line later reads $00; one press gives one NMI; the Game Gear gives none"
    requirement: CPU-04
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci --no-tests=error -R '^vdp\\.pause_nmi$' --output-on-failure"
        status: pass
    human_judgment: false
  - id: D10
    description: "The VDP fields and bus_vdp_at are saved, swept out of range and refused; a /INT that disagrees with the VDP flag and IE0 is refused"
    requirement: CPU-02
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci --no-tests=error -R '^core\\.state\\.' --output-on-failure"
        status: pass
    human_judgment: false
  - id: D11
    description: "The ci workflow passes and every inventory entry ran once"
    requirement: CPU-02
    verification:
      - kind: integration
        ref: "cmake --workflow --preset ci; cmake -DJUNIT=build/ci/junit.xml -DINVENTORY=tests/inventory/ci.txt -DEXCLUDE=release -P tests/cmake/check_junit.cmake"
        status: pass
    human_judgment: false
metrics:
  duration: 9min
  completed: 2026-10-19
---

# Phase 2 Plan 8: VDP timing half, the Pause tick and the run bounds Summary

`src/vdp.c` adds the VDP's timing half. The frame flag rises at tick 607 of line 192 (`VDP_INT_TICK`), and /INT
is that flag AND R1 IE0. The plan also adds the 192-line V counter tables with their step at tick 604, the live H
counter, the control and data ports with VRAM, CRAM and R0-R10, and the Pause /NMI at tick 611 of line 260 on NTSC
and line 311 on PAL. The bus fires these events as the beam reaches them and decodes the VDP ports for each profile.
With this, `run_frame` is proven bounded for DI;HALT, EI;HALT and a hostile port loop, and every access is proven to
be stamped on the tick lattice. That is the last of CPU-02's evidence.

## Tasks

| Task | Name | Commit | Files |
|---|---|---|---|
| 1 | Tracer: frame interrupt end to end | edd7aed | src/vdp.c, src/bus.c, src/internal.h, src/instance.c, CMakeLists.txt, tests/bus/* , inventories |
| 2 (RED) | Failing counter, port, status, Pause and stamp cases | 3fc652f | tests/bus/test_vdp.c, tests/bus/test_bus.c, tests/core/test_state.c, src/internal.h, src/state.c |
| 2 (GREEN) | Counters, data port, Pause NMI tick, VDP state, documents | 946ecf2 | src/vdp.c, src/bus.c, src/state.c, docs/embedding.md, preparation documents |

The tracer gate passed: its automated verify (`bus.halt_bound.*`, 4 tests) was re-run end to end before Task 2 was
expanded.

## TDD Gate Compliance

- RED: `3fc652f test(02-08)`. The command was `ctest --test-dir build/ci -R '^(vdp\.|bus\.(stamps|hostile_vdp_loop)$|core\.state\.)' --output-junit ../red/red-02-08.xml`
  and it exited 8. The classifier returned `RED_EVIDENCE_OK` (target `vdp.pause_nmi`, 11 of 14 failing).
  Semantic assessment: every failure is on a planned assertion. No NMI was raised (`nmi_pending` stayed 0), `vdp_at`
  stayed 0 where the stamps were expected, `$7E`/`$7F` read `$FF`, the latch did not clear on a data access, the
  state named no VDP fields, and a mismatched `z80_int_line` was accepted. There were no build, load or crash
  failures. `vdp.status` already passed because the Task 1 tracer had built it. To make the tests compile, the RED
  commit added the `vdp_at` field and its state visit but never wrote to the field.
- GREEN: `946ecf2 feat(02-08)`. The full ci workflow passes.
- REFACTOR: none needed.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Data-port read stepped before refilling**
- **Found during:** Task 2 GREEN
- **Issue:** The first GREEN draft stepped the address before refilling the read-ahead byte, so reads after code 0
  skipped a byte. `vdp.data_port` caught it.
- **Fix:** Refill from the current address, then step it. Two test expectations were also wrong (code 0 steps past
  its read-ahead byte, and code 2 leaves the address at `$0A00`) and were corrected.
- **Files modified:** src/vdp.c, tests/bus/test_vdp.c
- **Commit:** 946ecf2

**2. [Design] The 604 V-counter event is a comparison, not a stored step**
- The V counter is computed from the beam at each read, so no saved field or dispatcher event is needed. Reads are
  the same as with a stored step, as `vdp.vcounter.*` shows at ticks 603 and 604.

**3. [Design] The access tick lives in the bus**
- The plan put "last access tick" in the VDP struct. It is `m->bus.vdp_at` instead, which keeps the VDP struct free
  of 64-bit fields and gives its state tripwire one size on every target.

**4. [Rule 1 - Docs] SMS-VDP section 4.6 had the same stale "line 261" Pause wording**
- It was corrected with ARCHITECTURE and SMS-SYSTEM, although SMS-VDP.md was not in the plan's file list.

## Known Stubs

None. Writes to `$40-$7F` (the sound chip) are decoded and dropped until the PSG lands. The Pause request has no
public input yet; the plan defers that to the phase-3 `set_input`.

## Self-Check: PASSED
