---
phase: 02-the-cpu-matches-the-public-vectors
plan: 10
subsystem: io
status: complete
tags:
  - io
  - ports
  - fault
requires:
  - "02-06: the first-fault record, its kinds and devices, and cogmaster_get_fault"
  - "02-08: the bus, its port decode for the VDP and the state visitor"
  - "02-09: the mapper's write switch and its default arms"
provides:
  - "src/io.c: every port the VDP does not answer; $3E, $3F, $DC, $DD, Game Gear $00-$06, open bus with the open_bus_reads counter"
  - "Port decode by A7, A6, A0 on the export console; full decode on the Japanese console and the Game Gear"
  - "$3E enables in the bus: cartridge and work RAM off give open bus and unmapped_read; the I/O chip off gives open bus with no fault"
  - "Region readback: $3F TH outputs echo on $DD b7-6 on the export console only; Game Gear $00 b6"
  - "Fast boot sets $3E $AB (SMS) or $A8 (GG), $3F $FF and the Game Gear's $01-$06 to $7F $FF $00 $FF $00 $FF"
  - "Every first-fault call site of phase 2: unmapped_read, unmapped_write, rom_write, mapper_range (ed_undefined was already in place)"
  - "The bus keeps the running instruction's address and first byte after DD/FD for the record; both are saved"
  - "Public API: COGMASTER_FAULT_KIND_* (1-5) and COGMASTER_FAULT_DEVICE_* (0-5)"
affects:
  - "Runner --trace first-fault: reads the record these call sites write"
  - "Input (later phase): port_dc and port_dd return released buttons until an input API exists"
  - "Phase 3: the SMS1 open-bus rule (last data-bus byte), the TH edge H-counter latch and vdp_fast_write append here"
actuals:
  tokens: 16200
  tasks: 3
  commits: 5
plan_head_before: 8142c6e27a5dcedb650d1c966df21b1eba3636a5
plan_head_after: a6b09779bbce7b48b526d05370d34a69cec50d0f
tech-stack:
  added: []
  patterns:
    - "Memory enables cost one test of $3E on the hot read path; everything else sits behind it in a cold function"
    - "Fault calls go through cogmaster__bus_fault, which stamps the beam and the running instruction, so a call site is one line in a cold arm"
key-files:
  created:
    - src/io.c
    - tests/io/tests.cmake
    - tests/io/test_io.c
  modified:
    - CMakeLists.txt
    - include/cogmaster.h
    - src/internal.h
    - src/bus.c
    - src/boot.c
    - src/mapper.c
    - src/state.c
    - src/z80.c
    - tests/api/v2.c
    - tests/bus/test_bus.c
    - tests/bus/tests.cmake
    - tests/bus/test_vdp.c
    - tests/core/test_core.c
    - tests/core/test_state.c
    - tests/tests.cmake
    - tests/inventory/ci.txt
    - tests/inventory/wasm32.txt
    - docs/embedding.md
decisions:
  - "Game Gear $00 bit 5 reads 0 on NTSC, so $00 is $C0 on an export unit and $80 on a Japanese one, as SMS-SYSTEM section 8's power-on image says; the plan's behaviour line said bit 5 = 1"
  - "The step stores the instruction's address (its first DD/FD prefix included) and its first byte after the prefixes in the bus struct; the record reads them, and the state saves them"
  - "Game Gear: $3E is not decoded (a write is unmapped_write), $3F latches, $04 ignores writes, $05 keeps no status flag, $DD reads $FF (no TH lines)"
  - "On the Japanese console an output TH reads 0 on $DD, MAME's choice in SMS-SYSTEM section 4"
  - "unmapped_write covers $00-$3F and $80-$BF on full-decode consoles; $40-$7F (sound chip) and $C0-$FF (I/O chip) writes are never faults"
  - "Reads of non-VDP ports in $40-$BF and of undocumented $C0-$FF ports on full-decode consoles are open bus, counted, not faults"
  - "With work RAM off, writes to $C000-$FFFF are dropped but $FFFC-$FFFF still reach the mapper"
  - "The Codemasters $4000 write takes its bank from bits 6-0; bit 7 is only the RAM enable"
requirements-completed: []
coverage:
  - id: D1
    description: "LD A,0; IN A,($00) on the export SMS records unmapped_read, device ctrl, addr $0000, pc $0002, opcode $DB, width 8, frame 0, line 192 and the dot of the I/O data T-state, read back through cogmaster_get_fault"
    requirement: CPU-05
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci --no-tests=error -R '^fault\\.unmapped_read$' --output-on-failure"
        status: pass
    human_judgment: false
  - id: D2
    description: "$3E b6 and b4 take the cartridge and work RAM off the bus: reads return $FF, count, and record unmapped_read on cart or ram; re-enabling restores the bytes; toggling alone records nothing"
    requirement: CPU-04
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci --no-tests=error -R '^io\\.port3e$' --output-on-failure"
        status: pass
    human_judgment: false
  - id: D3
    description: "$3F = $F5 then $55 reads $DD b7-6 as %11 then %00 on the export console and %00 on the Japanese one"
    requirement: CPU-04
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci --no-tests=error -R '^io\\.port3f_region$' --output-on-failure"
        status: pass
    human_judgment: false
  - id: D4
    description: "Idle $DC and $DD read $FF with $DD b4-5 set on export, Japanese, PAL and Game Gear profiles"
    requirement: CPU-04
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci --no-tests=error -R '^io\\.port_dc_dd$' --output-on-failure"
        status: pass
    human_judgment: false
  - id: D5
    description: "Game Gear $00 reads $C0 (export) and $80 (Japan); $01-$06 read their power-on values; link and stereo registers latch; $3E is not decoded"
    requirement: CPU-04
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci --no-tests=error -R '^io\\.gg_ports$' --output-on-failure"
        status: pass
    human_judgment: false
  - id: D6
    description: "Open bus reads $FF on the SMS2 and the Game Gear and counts one open_bus_reads per read"
    requirement: CPU-04
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci --no-tests=error -R '^io\\.open_bus$' --output-on-failure"
        status: pass
    human_judgment: false
  - id: D7
    description: "The export console mirrors $DC/$DD over $C0-$FF and $3E/$3F over $00-$3F; $3E b2 makes $C0-$FF open bus with no fault; the Japanese console and the Game Gear answer only documented addresses"
    requirement: CPU-04
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci --no-tests=error -R '^io\\.decode_mirrors$' --output-on-failure"
        status: pass
    human_judgment: false
  - id: D8
    description: "Fast boot leaves $3E $AB on the SMS and $A8 on the Game Gear and $3F $FF; the ports survive a state round trip and every port field one step out of range is refused"
    requirement: CPU-04
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci --no-tests=error -R '^core\\.(boot\\.handoff|state\\.round_trip|state\\.record_sweep)$' --output-on-failure"
        status: pass
    human_judgment: false
  - id: D9
    description: "OUT ($3E),A on the Game Gear and OUT ($00),A on the Japanese console record unmapped_write with device none; the same OUT ($00),A on the export console records nothing"
    requirement: CPU-05
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci --no-tests=error -R '^fault\\.unmapped_write$' --output-on-failure"
        status: pass
    human_judgment: false
  - id: D10
    description: "LD ($2000),A records rom_write on the cartridge on Sega and no-mapper images; LD ($FFFF),A and a cartridge RAM write record nothing"
    requirement: CPU-05
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci --no-tests=error -R '^fault\\.rom_write$' --output-on-failure"
        status: pass
    human_judgment: false
  - id: D11
    description: "$FFFF = 30 on a 384 KiB image and $FFFC with b7, b4 or b1-0 set record mapper_range on the mapper; bank 23 of 24, bank 35 of a 512 KiB image and $FFFC = $0C record nothing"
    requirement: CPU-05
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci --no-tests=error -R '^fault\\.mapper_range$' --output-on-failure"
        status: pass
    human_judgment: false
  - id: D12
    description: "One image per non-fault case runs a frame with no record: $C0-$FF with the I/O chip off, $F0-$F2, $DE/$DF, I/O chip writes, Game Gear link ports, bank wrap, $3E toggling, undocumented DD/FD/CB forms and SLL"
    requirement: CPU-05
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci --no-tests=error -R '^fault\\.not_faults$' --output-on-failure"
        status: pass
    human_judgment: false
  - id: D13
    description: "The five kind and six device constants are in the header, used in the API version 2 test and listed with their meanings in docs/embedding.md"
    requirement: CPU-05
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci --no-tests=error -R '^(docs\\.api_names|abi\\.api_version|abi\\.api_v2)$'"
        status: pass
    human_judgment: false
---

# Phase 2 Plan 10: I/O ports and the first-fault call sites Summary

The I/O ports now live in a new `src/io.c`. Port `$3E` takes the cartridge, work RAM and the I/O chip off the bus. Port `$3F` controls TH/TR, and the export console reads TH back on `$DD` for region detection. `$DC`/`$DD` read their idle values, and the Game Gear has its own `$00`–`$06`. Undriven reads return `$FF` and count as open bus. Ports decode by A7/A6/A0 on the export console and by full address on the Japanese console and the Game Gear. Every phase-2 first-fault kind is now recorded at its call site, and shipped-software probes never are. The header has public kind and device constants.

## Performance

- Started: 2026-10-20T03:42:05Z
- Tasks: 3 (tracer, then two TDD tasks with separate failing-test commits)
- Files: 3 created, 18 modified

## Task Commits

1. Task 1, tracer: `IN A,($00)` through to `cogmaster_get_fault`, `3c26bc7`
2. Task 2, ports per profile: RED `1cba32b`, GREEN `f87420d`
3. Task 3, remaining faults and public codes: RED `229f212`, GREEN `a6b0977`

## Tracer feedback gate

The tracer was verified end to end. `cmake --build --preset ci && ctest --test-dir build/ci --no-tests=error -R '^fault\.unmapped_read$'` exited 0, and the whole ci test set passed (143/143) before the plan went further.

## TDD Gate Compliance

- Task 2 RED (`1cba32b`): `io.port3e`, `io.port3f_region`, `io.gg_ports` and `io.decode_mirrors` failed on their assertions (`mem_ctrl == 0xAB`, the `$DD` echo, `$00 == 0xC0`, the mirrors). `io.port_dc_dd` and `io.open_bus` already passed, because open bus was already `$FF`.
- Task 3 RED (`229f212`): `fault.unmapped_write`, `fault.rom_write` and `fault.mapper_range` failed on `f.present == 1`. `fault.not_faults` is a guard and passed, as it should.
- GREEN (`f87420d`, `a6b0977`): `cmake --workflow --preset ci` exited 0 with 157/157, and `cmake --workflow --preset hygiene` exited 0 with 23/23.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Game Gear `$00` bit 5 follows the hardware reference, not the plan's behaviour line**
- Found during: Task 2
- Issue: The plan said "bit 5 is 1 (NTSC)". SMS-SYSTEM section 8 gives the power-on image as `$C0`, which means bit 5 is 0 on NTSC.
- Fix: On NTSC, `$00` reads `$C0` on an export unit and `$80` on a Japanese one. The test asserts those values.
- Files modified: src/io.c, tests/io/test_io.c
- Commit: f87420d

**2. [Rule 3 - Blocking] The fault record needed the instruction's address and opcode**
- Found during: Task 1
- Issue: The Z80 tracked neither the instruction start nor its opcode, and memory faults cannot work them out afterwards.
- Fix: The step stores both in `struct cogmaster__bus` (`op_pc`, `op`). The bus struct tripwire goes from 32 to 40 bytes, and both fields are saved in the state. The vector harness does not read them.
- Files modified: src/z80.c, src/internal.h, src/bus.c, src/state.c
- Commit: 3c26bc7

**3. [Rule 1 - Bug] A test assumption that went stale**
- Found during: Task 2
- Issue: `vdp.port_decode` assumed every non-VDP port reads `$FF`. Now the I/O chip answers at `$C0`–`$FF` and the Game Gear at `$00`–`$06`.
- Fix: The `$FF` check is now limited to `$40`–`$BF`. The check that nothing reaches the VDP still covers every port.
- Files modified: tests/bus/test_vdp.c
- Commit: f87420d

**4. [Rule 1 - Bug] The test's own value for `$04` was wrong**
- Found during: Task 2
- Issue: The RED test expected Game Gear `$04` to read `$00` after a write. Its power-on value is `$FF`.
- Fix: The test now expects `$FF`.
- Commit: f87420d

**5. [Rule 1 - Bug] The Codemasters bank was taken from bits 6–0**
- Found during: Task 3
- Issue: The `$4000` write passed bit 7, the RAM enable, into the bank number. On an image whose page count is not a power of two that picks the wrong bank, and it would raise mapper_range every time RAM was enabled.
- Fix: The bank now comes from `v & $7F`. Bit 7 still sets the RAM enable.
- Files modified: src/mapper.c
- Commit: a6b0977

**6. [Scope] Fault arms for the slot area and work RAM landed in Task 2**
- The `$3E` open-bus path and its unmapped_read record share one cold function in `bus.c`, so they went in together in Task 2 rather than in Task 3.

## Requirements

CPU-04 and CPU-05 are partly delivered and stay open. CPU-04 still needs the `ports` fixture reporting through the
RAM mailbox. CPU-05 still needs `--probe`, `--trace first-fault` and the fuzz lanes, which later plans in this
phase deliver. So this plan marks neither complete.

## Known Stubs

- `src/io.c` `port_dc`/`port_dd`: every button reads released, because no input API exists yet (phase 2 defers `set_input`, D-26). This is intentional, and it is the idle value the plan asks for.

## Threat Flags

None. The change adds no new trust boundary: the ports and the fault record are in-instance state, and every new state field is range-checked by VALIDATE.

## Self-Check: PASSED

- FOUND: src/io.c, tests/io/test_io.c, tests/io/tests.cmake
- FOUND commits: 3c26bc7, 1cba32b, f87420d, 229f212, a6b0977
