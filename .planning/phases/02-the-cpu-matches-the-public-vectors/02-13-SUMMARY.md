---
phase: 02-the-cpu-matches-the-public-vectors
plan: 13
subsystem: fixtures
status: complete
tags:
  - fixtures
  - mailbox
  - peek
  - runner
  - mapper
requires:
  - "02-12: pinned WLA-DX, header.inc, tests/fixtures/tests.cmake, tests/built/expected.txt"
provides:
  - "Public COGMASTER_SPACE_BUS and cogmaster_peek(m, space, addr): the byte the CPU would read, no side effect; a value above 255 for no answer"
  - "cogmaster-run --mailbox: stop at mailbox status 00 or --frames N; print status=, rom=, magic=, crc= and NN=VVVV flags=F lines"
  - "runner/mailbox.{c,h}: cogmaster__crc16_ccitt, cogmaster__mailbox_done, cogmaster__mailbox_print"
  - "tests/roms/src/mailbox.inc: mb_init (A = ROM id), mb_record (A = id, HL = value), mb_done (CRC, then status 00)"
  - "tests/roms/src/mapper_sega.asm: 512 KiB, 32 stamped banks, records 01-04"
  - "tests/cmake/mailbox.cmake: the CSV judge with five distinct failure messages"
  - "tests/diag/expected.csv: header and the four mapper_sega rows"
  - "tests/fixtures/tests.cmake CASE=mailbox (digest check, runner, judge); fixtures.mapper_sega and fixtures.judge.{wrong_value,missing_row,hang}"
  - "header.inc map is skipped when a fixture defines FIXTURE_MAP"
affects:
  - "Plan 14: append the remaining mapper_sega records and their CSV rows, add mapper_codemasters to the foreach with its own ROM id and a CASE=mailbox test"
actuals:
  tokens: 13200
  tasks: 3
  commits: 4
commits: 4
plan_head_before: 62d197178c4989d596bb108abaedbb9ef0e89268
plan_head_after: 9a7b4bb9ffebd9f5142001cc097c11c1e05b1806
tech-stack:
  added: []
  patterns:
    - "Fixture measures, CSV judges: the runner prints mailbox records and mailbox.cmake compares them, never PASS_REGULAR_EXPRESSION"
    - "CSV test column starts with the record id in two hex digits and a colon"
    - "A fixture finishes no earlier than its second frame (it waits for the VDP frame flag), so a one-frame run is a hang"
key-files:
  created:
    - runner/mailbox.c
    - runner/mailbox.h
    - tests/roms/src/mailbox.inc
    - tests/roms/src/mapper_sega.asm
    - tests/roms/src/mapper_sega.link
    - tests/cmake/mailbox.cmake
    - tests/diag/expected.csv
  modified:
    - include/cogmaster.h
    - src/bus.c
    - tests/api/v2.c
    - docs/embedding.md
    - CMakeLists.txt
    - runner/main.c
    - runner/args.c
    - runner/args.h
    - tests/roms/src/header.inc
    - tests/fixtures/tests.cmake
    - tests/built/expected.txt
    - tests/core/test_core.c
    - tests/core/tests.cmake
    - tests/runner/test_runner.c
    - tests/runner/tests.cmake
    - tests/inventory/ci.txt
    - tests/inventory/wasm32.txt
    - docs/runner.md
    - docs/conformance.md
    - docs/testing.md
decisions:
  - "cogmaster_peek returns 256 for no answer; the header and guide promise only 'above 255', so no extra constant was added"
  - "Peek of an address that port $3E takes off the bus returns $FF, the CPU's open-bus value, without counting or faulting"
  - "bus.c splits the read into mem_undriven and mem_driven, which the faulting read and peek share"
  - "The mailbox CRC is stored low byte first, like the record values"
  - "The judge checks magic, then status (hang), then CRC, so a run cut short reports a hang rather than a bad CRC"
  - "The CSV test column is '<id hex>:<name>' so rows map to records without a separate table"
  - "mapper_sega keeps all code below $0400, so selecting another bank in slot 0 never moves it"
requirements-completed: []
coverage:
  - id: D1
    description: "cogmaster_peek and COGMASTER_SPACE_BUS are in the header, called in tests/api/v2.c and documented in docs/embedding.md in one commit; the API gates pass"
    requirement: CPU-03
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci --no-tests=error -R '^(abi\\.api_v2|abi\\.api_version|docs\\.api_names)$'"
        status: pass
    human_judgment: false
  - id: D2
    description: "Peeking all 64 KiB mid-frame, with the frame flag pending and the read buffer filled, and again with the cartridge and work RAM off the bus, leaves the saved state and counters unchanged; each peek equals the guest's LD A,(nn)"
    requirement: CPU-03
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci --no-tests=error -R '^core\\.peek\\.no_side_effects$'"
        status: pass
    human_judgment: false
  - id: D3
    description: "A NULL instance, nothing loaded, a bad space and an address above $FFFF each peek a value above 255"
    requirement: CPU-03
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci --no-tests=error -R '^core\\.peek\\.args$'"
        status: pass
    human_judgment: false
  - id: D4
    description: "The runner's CRC-16 CCITT of '123456789' is $29B1"
    requirement: CPU-03
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci --no-tests=error -R '^runner\\.mailbox\\.crc$'"
        status: pass
    human_judgment: false
  - id: D5
    description: "fixtures.mapper_sega: the image digest matches, --mailbox prints crc=ok, and the slot 1 and slot 2 bank sums and the slot 0 fixed and paged reads are inside their expected.csv rows"
    requirement: CPU-03
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci --no-tests=error -R '^fixtures\\.mapper_sega$'"
        status: pass
      - kind: integration
        ref: "build/ci/cogmaster-run build/ci/fixtures/mapper_sega.sms --frames 120 --mailbox | grep -Eq '^crc=ok$'"
        status: pass
    human_judgment: false
  - id: D6
    description: "The judge's negative controls fail with their own messages: one wrong value, one missing row, a one-frame run (hang)"
    requirement: CPU-03
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci --no-tests=error -R '^fixtures\\.judge\\.'"
        status: pass
    human_judgment: false
  - id: D7
    description: "Every CI lane builds and passes, including be32, wasm32 and the hash-equality job over the new fixture digest"
    requirement: CPU-03
    verification:
      - kind: integration
        ref: "gh run view 39372102230 --json conclusion --jq .conclusion"
        status: pass
    human_judgment: false
---

# Phase 2 Plan 13: RAM mailbox, cogmaster_peek and the mapper_sega fixture Summary

A side-effect-free `cogmaster_peek` lets `cogmaster-run --mailbox` read a diagnostic ROM's records from work RAM.
`tests/cmake/mailbox.cmake` judges them against `tests/diag/expected.csv`. The `mapper_sega` fixture's bank sums
and slot 0 reads are the first records judged this way, and three negative controls show the judge fails when it
should.

## What was built

- **Task 1 (tracer), `e850542`:** `COGMASTER_SPACE_BUS` and `cogmaster_peek` added to the header, `src/bus.c`,
  `tests/api/v2.c` and the embedding guide together. Tracer gate: the full `ci` workflow and the three API
  gates were re-run and passed before Task 2 started.
- **Task 2, `b5b4663`:** `runner/mailbox.{c,h}` and `--mailbox`, `mailbox.inc`, `mapper_sega.asm`/`.link`, the
  judge, the CSV, `fixtures.mapper_sega`, the pinned digest and both inventories. The first assembly gave
  `crc=ok`, so the ROM's CRC routine agrees with the runner's.
- **Task 3 (TDD), `2d9680e` tests and `9a7b4bb` docs:** `core.peek.no_side_effects`, `core.peek.args`,
  `runner.mailbox.crc`, `fixtures.judge.{wrong_value,missing_row,hang}`, then `--mailbox` in `docs/runner.md`
  and the CSV and judge in `docs/conformance.md`.

## Records

| Id | Measures | Value |
|---|---|---|
| 01 | sum of `bank \| ~bank << 8` over banks 0-31 via slot 1 | `F1F0` |
| 02 | the same via slot 2 | `F1F0` |
| 03 | after `$FFFD = 5`: `$0000` \| `$03FF` << 8 (bank 0's code byte and marker) | `5AF3` |
| 04 | after `$FFFD = 5`: the stamp word at `$0400` (bank 5) | `FA05` |

## Evidence

- `cmake --workflow --preset ci` exited 0 (173 tests, then 4 release tests).
- `ctest --test-dir build/ci -R '^(core\.peek\.|runner\.mailbox\.|fixtures\.judge\.)' --show-only=json-v1 | jq -e '.tests | length == 6'` exited 0, and the selected tests passed.
- `ctest --test-dir build/ci -L fixtures --show-only=json-v1 | jq '.tests | length'` prints 6.
- `cmake -DJUNIT=build/ci/junit.xml -DINVENTORY=tests/inventory/ci.txt -DEXCLUDE=release -P tests/cmake/check_junit.cmake` exited 0.
- `ctest --test-dir build/ci --no-tests=error -R '^(docs\.api_names|abi\.api_v2|abi\.symbols|docs\.budgets)$'` exited 0.
- The planning-ID grep over `src runner tests docs include` found nothing (exit 0 under `!`).
- `cmake --workflow --preset hygiene` exited 0; `sh scripts/hygiene.sh --tree` exited 0; `cmake --workflow --preset bench` exited 0.
- `gh workflow run ci.yml --ref phase/02-the-cpu-matches-the-public-vectors`, then `gh run watch 39372102230 --exit-status`, exited 0:
  https://github.com/szTheory/cogmaster/actions/runs/39372102230

## TDD Gate Compliance

Task 3's tests passed when they were added, because Tasks 1 and 2 had already built what they test (the same
situation as plan 12's self test). Their failure paths were checked by hand instead:

- With peek changed to count an open-bus read for undriven addresses, `core.peek.no_side_effects` failed with
  "peeking changed the saved state" and the counters check (ctest exit 8). The original `src/bus.c` was then
  restored and rebuilt.
- With the CSV changed (`5AF3` to `5AF4`), with row 02 deleted, with an extra row 05 and with a one-frame run,
  `mailbox.cmake` failed with "outside its row", "has no row", "has no record" and "(hang)". The three
  registered controls assert the first, second and fourth of these.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] `runner.usage.unknown` listed `--mailbox` as an option a later release would add**
- **Found during:** Task 2's CI run.
- **Issue:** The test required `--mailbox` to be refused and absent from `--help`, which this plan reverses.
- **Fix:** Removed `--mailbox` from the refused list and from the `--help` regex.
- **Files modified:** tests/runner/tests.cmake
- **Commit:** `b5b4663`

**2. [Rule 3 - Blocking] `header.inc` fixed the 32 KiB map that every fixture shared**
- **Issue:** `mapper_sega` needs 32 banks of 16 KiB.
- **Fix:** `header.inc` skips its map when the fixture defines `FIXTURE_MAP`. `fixtures.hello` still matches its
  pinned digest, so hello's bytes did not change.
- **Commit:** `b5b4663`

**3. [Scope note] The purity test does not detect a beam sync**
- Making peek call `cogmaster__bus_sync` did not fail `core.peek.no_side_effects`, because after a whole Z80
  step the beam is already caught up with the tick count. The test's fault-prevented comment names only what it
  catches: a status clear, a read-buffer refill, an open-bus count and a fault record. Peek does not sync the
  beam; only a review of the code shows that.

`tests/cmake/mailbox.cmake` is 71 lines; the plan estimated about 60.

## Known Stubs

None. The other `mapper_sega` records (wrap, `$DFFF` readback, cartridge RAM, shift) and `mapper_codemasters`
are plan 14's scope.

## Next

Plan 02-14.

## Self-Check: PASSED
