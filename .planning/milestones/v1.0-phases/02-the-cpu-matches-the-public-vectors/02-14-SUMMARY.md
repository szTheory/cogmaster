---
phase: 02-the-cpu-matches-the-public-vectors
plan: 14
subsystem: fixtures
status: complete
tags:
  - fixtures
  - mailbox
  - mapper
  - codemasters
requires:
  - "02-09: the Sega and Codemasters mappers and their register-level tests"
  - "02-12: pinned WLA-DX, tests/fixtures/tests.cmake, tests/built/expected.txt"
  - "02-13: mailbox.inc, cogmaster-run --mailbox, the CSV judge, the mapper_sega fixture"
provides:
  - "tests/roms/src/mapper_codemasters.asm: 128 KiB, 8 stamped banks, a literal Codemasters header, records 01-07"
  - "fixtures.mapper_codemasters (--machine sms, labels fixtures and portable)"
  - "mapper_sega records 05-0A: wrap, the $DFFF readback, cartridge RAM, its bank and the bank shift"
  - "The mailbox CASE of tests/fixtures/tests.cmake takes an optional MACHINE"
  - "docs/conformance.md: the mailbox protocol, the judge's five messages, how to add a CSV row, both mapper fixtures"
  - "ARCHITECTURE sections 3 and 11 and the CONFORMANCE section 2 matrix place mapper_codemasters in phase 2"
affects:
  - "Plan 17: measured timeouts for fixtures.mapper_codemasters along with the other phase tests"
actuals:
  tokens: 9000
  tasks: 2
  commits: 2
commits: 2
plan_head_before: 6d5c6f18f3b5e34f516216f078a5f4b8d1d5f911
plan_head_after: bfb38f74b159d8c1fee20a2566bbff9f61c373d2
tech-stack:
  added: []
  patterns:
    - "A slot-0 bank loop on a board with no fixed first 1 KiB runs from a position-independent copy in work RAM, the restore bank passed in A'"
    - "A fixture with no TMR SEGA header names its machine with -DMACHINE in its mailbox test"
key-files:
  created:
    - tests/roms/src/mapper_codemasters.asm
    - tests/roms/src/mapper_codemasters.link
  modified:
    - tests/roms/src/mapper_sega.asm
    - tests/fixtures/tests.cmake
    - tests/built/expected.txt
    - tests/diag/expected.csv
    - tests/inventory/ci.txt
    - tests/inventory/wasm32.txt
    - docs/conformance.md
    - .planning/preparation/ARCHITECTURE.md
    - .planning/preparation/CONFORMANCE.md
decisions:
  - "mapper_codemasters carries no TMR SEGA header, as Codemasters games do not; its test passes --machine sms through a new optional MACHINE variable"
  - "The Codemasters stamp sits at bank offset $1F00, so slot 2's stamp at $9F00 stays ROM while cartridge RAM covers $A000-$BFFF"
  - "One position-independent bank loop is copied to $C200 and used for all three Codemasters slots"
  - "The mapper_sega shift record writes $FFFC b1-0, which the core records as its first fault (no shipped game uses the shift); the runner does not stop on a fault, so the record is still judged"
requirements-completed:
  - CPU-03
coverage:
  - id: D1
    description: "fixtures.mapper_codemasters: the digest matches and the records for reset banks 0,1,0, the sums through $0000, $4000 and $8000, wrap and 8 KiB RAM at $A000 via $4000 bit 7 are inside their expected.csv rows"
    requirement: CPU-03
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci --no-tests=error -R '^fixtures\\.mapper_codemasters$' --output-on-failure"
        status: pass
    human_judgment: false
  - id: D2
    description: "The runner's probe classifies the Codemasters image as codemasters"
    requirement: CPU-03
    verification:
      - kind: integration
        ref: "build/ci/cogmaster-run --probe build/ci/fixtures/mapper_codemasters.sms | grep -Eq '^mapper_guess=codemasters$'"
        status: pass
    human_judgment: false
  - id: D3
    description: "fixtures.mapper_sega also judges wrap, the $DFFF readback of $FFFF, cartridge RAM via $FFFC bit 3 (pattern, ROM, pattern), its bank via bit 2 and the bit 1-0 shift"
    requirement: CPU-03
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci --no-tests=error -R '^fixtures\\.mapper_sega$' --output-on-failure"
        status: pass
    human_judgment: false
  - id: D4
    description: "The fixtures label selects exactly seven tests and all pass; every inventoried test ran once"
    requirement: CPU-03
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -L '^fixtures$' --show-only=json-v1 | jq -e '.tests | length == 7'"
        status: pass
      - kind: integration
        ref: "cmake -DJUNIT=build/ci/junit.xml -DINVENTORY=tests/inventory/ci.txt -DEXCLUDE=release -P tests/cmake/check_junit.cmake"
        status: pass
    human_judgment: false
  - id: D5
    description: "docs/conformance.md describes the mailbox protocol, the judge's five messages and how to add a CSV row; the preparation matrix places mapper_codemasters in phase 2"
    requirement: CPU-03
    verification:
      - kind: other
        ref: "! grep -E '^[|] `mapper_codemasters` [|]' .planning/preparation/CONFORMANCE.md | grep -q 'P3'"
        status: pass
      - kind: other
        ref: "! grep -q 'P3 (Codemasters)' .planning/preparation/ARCHITECTURE.md"
        status: pass
    human_judgment: false
  - id: D6
    description: "Every CI lane builds and passes with both fixtures, including be32, wasm32 and the hash-equality job"
    requirement: CPU-03
    verification:
      - kind: integration
        ref: "gh run view 39372953419 --json conclusion --jq .conclusion"
        status: pass
    human_judgment: false
---

# Phase 2 Plan 14: The Codemasters fixture and the full Sega record list Summary

A 128 KiB Codemasters image built with the pinned WLA-DX reports its reset banks, bank sums through all three
slots, wrap and its 8 KiB RAM at `$A000` through the RAM mailbox. `mapper_sega` now also reports wrap, the
`$DFFF` readback, cartridge RAM and its bank, and the bank shift. `expected.csv` judges every record.

## What was built

- **Task 1 (tracer), `9f5de1a`:** `mapper_codemasters.asm`/`.link`, the foreach entry, `fixtures.mapper_codemasters`
  with `-DMACHINE=sms`, the pinned digest, seven CSV rows and both inventories. The first assembly gave exactly the
  values predicted from SMS-CARTRIDGES section 2.2. Tracer gate: the task's `<verify>` command was re-run and passed
  (all 7 fixtures tests), and only then did Task 2 start.
- **Task 2, `bfb38f7`:** mapper_sega records 05-0A, the re-pinned digest, six CSV rows, the protocol, judge and
  row-adding sections of `docs/conformance.md`, and the phase corrections in ARCHITECTURE sections 3 and 11 and the
  CONFORMANCE section 2 matrix.

## Records

| ROM | Id | Measures | Value |
|---|---|---|---|
| mapper_codemasters | 01 | stamp numbers at reset: slot 0 \| slot 1 << 4 \| slot 2 << 8 | `0010` |
| mapper_codemasters | 02-04 | sum of `bank \| ~bank << 8` over banks 0-7 via `$0000`, `$4000`, `$8000` | `DC1C` |
| mapper_codemasters | 05 | slot 2 after `$8000 = 35` \| after `$FF` << 8 | `0703` |
| mapper_codemasters | 06 | RAM on: `$A000` \| `$BFFF` << 8 after writing `$A5`, `$3C` | `3CA5` |
| mapper_codemasters | 07 | RAM off: ROM marker at `$A000` \| RAM on again: `$A000` << 8 | `A55A` |
| mapper_sega | 05 | slot 1 after `$FFFE = 35` \| after `$FF` << 8 | `1F03` |
| mapper_sega | 06 | after `$FFFF = 61`: `$DFFF` \| slot 2 stamp << 8 | `1D3D` |
| mapper_sega | 07 | RAM on: `$8000` \| RAM off: slot 2 stamp << 8 | `02A5` |
| mapper_sega | 08 | RAM on again: `$8000` \| `$BFFF` << 8 | `C3A5` |
| mapper_sega | 09 | RAM bank 1: `$8000` \| RAM bank 0: `$8000` << 8 | `A55A` |
| mapper_sega | 0A | shift 1, `$FFFE = 0` \| shift 3, `$FFFE = 1` << 8 | `0918` |

## Evidence

- `cmake --workflow --preset ci` exited 0 (174 tests, then 4 release tests).
- `ctest --test-dir build/ci --no-tests=error -L '^fixtures$' --output-on-failure` exited 0, and
  `ctest --test-dir build/ci -L '^fixtures$' --show-only=json-v1 | jq -e '.tests | length == 7'` exited 0.
- `cmake -DJUNIT=build/ci/junit.xml -DINVENTORY=tests/inventory/ci.txt -DEXCLUDE=release -P tests/cmake/check_junit.cmake` exited 0.
- `grep -Ec '^mapper_codemasters,' tests/diag/expected.csv` prints 7; `grep -Ec '^mapper_sega,' tests/diag/expected.csv` prints 10.
- `build/ci/cogmaster-run --probe build/ci/fixtures/mapper_codemasters.sms | grep -Eq '^mapper_guess=codemasters$'` exited 0.
- Both preparation-matrix greps and the planning-ID grep over `src runner tests docs include` exited 0.
- `cmake --workflow --preset hygiene` exited 0; `sh scripts/hygiene.sh --tree` exited 0.
- `gh workflow run ci.yml --ref phase/02-the-cpu-matches-the-public-vectors`, then `gh run watch 39372953419 --exit-status`, exited 0:
  https://github.com/szTheory/cogmaster/actions/runs/39372953419

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] The mailbox test case had no way to name the machine**
- **Found during:** Task 1.
- **Issue:** The plan asks for `--machine sms`, but the mailbox case ran the runner with no machine option.
- **Fix:** An optional `MACHINE` variable adds `--machine ${MACHINE}`; `fixtures.mapper_sega` and the judge
  controls are unchanged.
- **Files modified:** tests/fixtures/tests.cmake
- **Commit:** `9f5de1a`

### Notes

- The Codemasters image includes no `header.inc`: Codemasters games carry no `TMR SEGA` header (SMS-CARTRIDGES
  section 1.3), so the fixture defines its own map and writes the `$7FE0` header as bytes.
- `cogmaster-run --trace first-fault` on mapper_sega now reports a `mapper_range` fault at the shift write to
  `$FFFC`. That is the core flagging a feature no shipped game uses, as designed; the mailbox run does not stop on
  faults. mapper_codemasters records no fault.
- CPU-03 is marked complete: register-level tests, padding and wrap (plan 9), the pinned WLA-DX build (plan 12)
  and both fixtures judged in CI (plans 13 and 14) cover every clause.

## Known Stubs

None.

## Next

Plan 02-15.

## Self-Check: PASSED
