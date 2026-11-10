---
phase: 02-the-cpu-matches-the-public-vectors
plan: 09
subsystem: cart
status: complete
tags:
  - cart
  - mapper
  - probe
  - state
requires:
  - "02-06: the image loader, its size rules, copier-header strip and padding"
  - "02-08: the bus with its memory decode and state format 2"
provides:
  - "src/mapper.c: no-mapper, Sega and Codemasters boards behind one switch on the board (read, write, reset)"
  - "src/cart.c: the internal probe (copier header, ROM header at $7FF0/$3FF0/$1FF0, Codemasters header, 8 KiB Game Gear refusal, 48 KiB rule, LD (nn),A scan) and the public cogmaster_probe"
  - "Cartridge RAM in the ROM's allocation: 32 KiB on the Sega board, 8 KiB on the Codemasters board, zeroed at load and kept by resets"
  - "The Sega register writes at $FFFC-$FFFF also land in work RAM, so $DFFC-$DFFF read them back"
  - "Fast boot pages the Sega board 0/0/1/2; the Codemasters board resets to 0/1/0"
  - "The state appends the board (fixed to the image's), the mapper control, three banks ranged below the page count, and the cartridge RAM bytes"
  - "Public API: cogmaster_probe, cogmaster_cart_info (first size 52), COGMASTER_BOARD_ROM_ONLY/SEGA/CODEMASTERS, COGMASTER_STATUS_ERR_UNSUPPORTED_BOARD (8)"
affects:
  - "Plan 10 (faults): rom_write and mapper_range record calls go in the default arms of cogmaster__mapper_write's switch"
  - "Runner --probe: reads cogmaster_probe and prints the fields; digests stay in the runner"
  - "Fuzz: the loader target feeds cogmaster_probe and load"
actuals:
  tokens: 24400
  tasks: 3
  commits: 5
plan_head_before: af64ae3f371f623ae213075fc85907f47b2353a1
plan_head_after: b7da844adf566a3a5b3894878546023486b43d24
tech-stack:
  added: []
  patterns:
    - "A bank number is reduced modulo the page count when it is written, so a read is one shift and one OR with no bounds test"
    - "Cartridge RAM sits after the padded ROM in the same allocation and is freed with the ROM, so a load has one allocation and one failure path"
    - "The internal probe result is the public cogmaster_cart_info; load and cogmaster_probe run the same code"
key-files:
  created:
    - src/mapper.c
    - tests/mapper/tests.cmake
    - tests/mapper/test_mapper.c
    - tests/cart/tests.cmake
    - tests/cart/test_cart.c
  modified:
    - CMakeLists.txt
    - include/cogmaster.h
    - src/internal.h
    - src/cart.c
    - src/bus.c
    - src/boot.c
    - src/instance.c
    - src/state.c
    - tests/api/v2.c
    - tests/core/test_core.c
    - tests/core/test_state.c
    - tests/core/tests.cmake
    - tests/tests.cmake
    - tests/inventory/ci.txt
    - tests/inventory/wasm32.txt
    - docs/embedding.md
decisions:
  - "The opcode scan refuses an image only when the stores to another board's registers ($0002-$0004, $3FFE, $6000, $A000) outnumber the stores to the Sega registers ($FFFC-$FFFF), so one stray data match cannot refuse a Sega game; stores to the Codemasters $8000 family count for neither"
  - "The $FFFC bank shift is added when a bank register is written, then the sum is reduced modulo the page count. Banks written earlier keep their value"
  - "machine_guess and region_guess are 0 (AUTO) when no header names them, so the runner can fall back to the file extension"
  - "checksum is the header's stored word; checksum_ok compares it with the sum over the range the size code names, skipping the header's own 16 bytes, clamped to the body; a code with no range gives checksum_ok 0"
  - "Load also refuses any 8 KiB body loaded as a Game Gear, header or not, with ERR_UNSUPPORTED_BOARD"
  - "The Codemasters RAM bit is kept in the control byte as $80; the state allows only 0 or $80 there"
  - "The state keeps format version 2: phase 2 has not released a format, and a state of the old layout is refused by its size"
requirements-completed: []
coverage:
  - id: D1
    description: "A 512 KiB Sega image is probed, loaded and paged 0/1/2. Each of banks 0-31 reads its stamp through slots 1 and 2. After $FFFD = 5, reads below $0400 see bank 0 and reads from $0400 see bank 5"
    requirement: CPU-03
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci --no-tests=error -R '^mapper\\.sega\\.banks$' --output-on-failure"
        status: pass
    human_judgment: false
  - id: D2
    description: "Sega wrap: on 512 KiB, 35 selects bank 3 and $FF selects bank 31; on 384 KiB, bank = value mod 24"
    requirement: CPU-03
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci --no-tests=error -R '^mapper\\.sega\\.wrap$' --output-on-failure"
        status: pass
    human_judgment: false
  - id: D3
    description: "$FFFC b3 maps cartridge RAM into slot 2 (pattern, then ROM, then the pattern again), and b2 picks a second 16 KiB bank. Both banks keep their bytes across a soft reset"
    requirement: CPU-03
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci --no-tests=error -R '^mapper\\.sega\\.ram$' --output-on-failure"
        status: pass
    human_judgment: false
  - id: D4
    description: "Writes to $FFFC-$FFFF read back at $DFFC-$DFFF; a write to the $DFFF mirror pages nothing"
    requirement: CPU-03
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci --no-tests=error -R '^mapper\\.sega\\.mirror$' --output-on-failure"
        status: pass
    human_judgment: false
  - id: D5
    description: "$FFFC b1-0 add $00, $18, $10 or $08 to later bank writes, and banks written earlier keep their value"
    requirement: CPU-03
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci --no-tests=error -R '^mapper\\.sega\\.shift$' --output-on-failure"
        status: pass
    human_judgment: false
  - id: D6
    description: "Codemasters: banks reset to 0, 1, 0; writes to $4000 and $8000 page slots 1 and 2; code running in RAM pages slot 0 through $0000, the first 1 KiB included; only the first byte of each slot is a register"
    requirement: CPU-03
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci --no-tests=error -R '^mapper\\.codemasters\\.banks$' --output-on-failure"
        status: pass
    human_judgment: false
  - id: D7
    description: "Codemasters $4000 b7 maps 8 KiB of RAM at $A000-$BFFF; with the bit clear, ROM is back and writes there do nothing"
    requirement: CPU-03
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci --no-tests=error -R '^mapper\\.codemasters\\.(ram|wrap)$' --output-on-failure"
        status: pass
    human_judgment: false
  - id: D8
    description: "No mapper: 32 KiB and 48 KiB images read linearly up to $BFFF, with $FF past the image; ROM and Sega register writes page nothing; work RAM mirrors at $E000"
    requirement: CPU-03
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci --no-tests=error -R '^mapper\\.none\\.layout$' --output-on-failure"
        status: pass
    human_judgment: false
  - id: D9
    description: "A 40 KiB image pads to 48 KiB with $FF. A 136 KiB Sega image pads to 9 pages, the tail of bank 8 reads $FF, and banks 9 and 10 wrap to banks 0 and 1"
    requirement: CPU-03
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci --no-tests=error -R '^mapper\\.pad_wrap$' --output-on-failure"
        status: pass
    human_judgment: false
  - id: D10
    description: "The board, mapper registers and cartridge RAM survive a save and load on Sega and Codemasters images; every field one step out of range is refused, and the refusal changes nothing"
    requirement: CPU-03
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci --no-tests=error -R '^core\\.state\\.' --output-on-failure"
        status: pass
    human_judgment: false
  - id: D11
    description: "Each branch of the board guess gives its board: copier header, header at $7FF0, $3FF0 and $1FF0, good and bad checksum, Codemasters header, 48 KiB headerless, $A000 stores refused, $FFFF stores Sega, 8 KiB Game Gear refused"
    requirement: CPU-05
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci --no-tests=error -R '^cart\\.probe\\.branches$' --output-on-failure"
        status: pass
    human_judgment: false
  - id: D12
    description: "500 seeded images in heap buffers of exactly their size probe to the same output twice and stay unchanged; a NULL image is refused without a read; size tags are honoured"
    requirement: CPU-05
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci --no-tests=error -R '^cart\\.probe\\.(pure|sizes)$' --output-on-failure"
        status: pass
    human_judgment: false
  - id: D13
    description: "Load refuses an unsupported board, an 8 KiB Game Gear header and an 8 KiB image loaded as a Game Gear with ERR_UNSUPPORTED_BOARD, leaving the instance, its image and the allocator untouched"
    requirement: CPU-05
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci --no-tests=error -R '^core\\.(load\\.board|api\\.statuses)$' --output-on-failure"
        status: pass
    human_judgment: false
  - id: D14
    description: "The ci workflow passes, every inventory entry ran once, the header magic appears only as hex bytes, and no planning ID appears in src, tests, docs or include"
    requirement: CPU-05
    verification:
      - kind: integration
        ref: "cmake --workflow --preset ci; cmake -DJUNIT=build/ci/junit.xml -DINVENTORY=tests/inventory/ci.txt -DEXCLUDE=release -P tests/cmake/check_junit.cmake"
        status: pass
    human_judgment: false
metrics:
  duration: 40min
  completed: 2026-10-20
---

# Phase 2 Plan 9: Cartridge boards and the probe Summary

This plan adds the three v1.0 cartridge boards in `src/mapper.c`, all dispatched by one `switch`. The Sega board
has bank wrap, the fixed first 1 KiB, 32 KiB of cartridge RAM, the bank shift and the `$DFFC` mirror. The
Codemasters board has its `$0000`/`$4000`/`$8000` registers and 8 KiB of RAM at `$A000`. The no-mapper board
decodes linearly. A table-free probe in `src/cart.c` picks the board at load. The same probe is public as
`cogmaster_probe`, which fills a `cogmaster_cart_info` and refuses unsupported boards with
`COGMASTER_STATUS_ERR_UNSUPPORTED_BOARD`.

## Tasks

| Task | Name | Commit | Files |
|---|---|---|---|
| 1 (tracer) | 512 KiB Sega image probed, loaded and bank-switched | b342878 | src/mapper.c, src/cart.c, src/bus.c, src/boot.c, src/instance.c, src/internal.h, include/cogmaster.h, CMakeLists.txt, tests/mapper/*, tests/api/v2.c, docs/embedding.md, inventories |
| 2 (RED) | Failing board register and mapper state cases | 4300e62 | tests/mapper/*, tests/core/test_state.c, tests/core/tests.cmake, inventories |
| 2 (GREEN) | Board, mapper registers and cartridge RAM in the state | c0d283e | src/state.c |
| 3 (RED) | Failing probe branch, purity, size and board-refusal cases | 5d98536 | tests/cart/*, tests/core/test_core.c, tests/core/tests.cmake, tests/api/v2.c, include/cogmaster.h, src/cart.c (stub), tests/tests.cmake, inventories |
| 3 (GREEN) | Public cogmaster_probe with cogmaster_cart_info | b7da844 | src/cart.c, src/internal.h, src/instance.c, docs/embedding.md, tests/cart/test_cart.c |

The tracer gate passed. Before Task 2 expanded on it, the tracer's automated verify (`mapper.sega.banks`) was run
again from build to test and passed.

## TDD Gate Compliance

- Task 2 RED: `4300e62 test(02-09)`. The command was
  `ctest --test-dir build/ci -R '^(mapper\.|core\.state\.)' --output-junit ../red/red-02-09-t2.xml`, and it exited
  8. The classifier returned `RED_EVIDENCE_OK` (target `core.state.mapper`, 1 of 15 tests failing). This evidence
  was recorded after the RED commit: `src/state.c` was restored from `4300e62`, rebuilt and run, then put back.
  Semantic assessment: the target failed on its planned assertions. No `mapper_board`, `mapper_ctrl`,
  `mapper_bank` or `cart_ram` field was recorded, and the loaded instance kept its own registers and RAM. Nothing
  failed to build, load or crash. The nine register-level mapper cases already passed because Task 1's tracer had
  built all three boards.
- Task 2 GREEN: `c0d283e feat(02-09)`.
- Task 3 RED: `5d98536 test(02-09)`. The command was
  `ctest --test-dir build/ci -R '^(cart\.|core\.load\.board$|core\.api\.statuses$|abi\.api_v2$)' --output-junit ../red/red-02-09-t3.xml`,
  and it exited 8. The classifier returned `RED_EVIDENCE_OK` (target `cart.probe.branches`, 5 of 6 tests failing).
  Semantic assessment: every failure is on a planned assertion. The stub returned ERR_ARGUMENT and filled no
  field, so the board, `header_at`, `image_size` and status checks all failed. The load part of `core.load.board`
  already passed from Task 1, and so did `core.api.statuses`. To make the tests compile, the RED commit declared
  the header names and added a `cogmaster_probe` stub.
- Task 3 GREEN: `b7da844 feat(02-09)`. The full ci workflow passes.
- REFACTOR: none needed.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] `COGMASTER_STATUS_ERR_UNSUPPORTED_BOARD` moved into Task 1**
- **Found during:** Task 1
- **Issue:** The plan puts the status in Task 3, but Task 1's load already has to refuse a board the probe rejects.
  Without the status, the tracer commit could not refuse an image and keep the tree green.
- **Fix:** Task 1 added the constant, its line in `tests/api/v2.c` and its row in `docs/embedding.md`. Task 3 added
  the rest of the public probe API.
- **Commit:** b342878

**2. [Rule 3 - Blocking] All three boards built in the Task 1 tracer**
- **Found during:** Task 1
- **Issue:** The switch in `src/mapper.c` needs every board's arms to compile and to load the existing no-mapper
  images. Adding only the Sega arm would have broken the 1 KiB images that the existing tests load.
- **Fix:** Task 1 wrote the Codemasters and no-mapper arms too. Task 2's register-level cases then tested them,
  and its RED failure came from the state test.
- **Commit:** b342878

**3. [Rule 1 - Bug] cogmaster_cart_info first size is 52, not 56**
- **Found during:** Task 3 GREEN
- **Issue:** The plan's field list has 13 `uint32_t` fields, which is 52 bytes. A first size of 56 failed the
  layout assert.
- **Fix:** `COGMASTER__FIRST_SIZE_CART_INFO` is 52, and the docs table says 52.
- **Commit:** b7da844

**4. [Rule 2 - Missing] Codemasters RAM and wrap tests also cover register addresses**
- `mapper.codemasters.banks` also checks that `$4001` and `$FFFF` page nothing on the Codemasters board. The
  register addresses are exact, so a write anywhere else in a slot must not page.

### Deferred (out of scope)

`hygiene.format` and `hygiene.no_float` fail in the hygiene lane on files this plan did not touch (`src/vdp.c`,
the `src/z80*` sources and `tests/bus/*`). The failure predates this plan: `src/vdp.c` at `af64ae3` already gives
8 format errors. The details are in `deferred-items.md`; clear them before `/gsd-ship`.

## Threat Flags

| Flag | File | Description |
|---|---|---|
| threat_flag: untrusted-input | src/cart.c | `cogmaster_probe` is a new public entry point that parses untrusted image bytes. It reads only `[image, image + n)`: every header read is guarded by `at + 16 <= body` and the scan stops 2 bytes short of its end. `cart.probe.pure` runs on exact-size heap buffers, so an address-sanitizer build catches an overread. A local `-fsanitize=address,undefined` build ran the 24 cart, mapper, load and state tests and exited 0. |

## Self-Check: PASSED
