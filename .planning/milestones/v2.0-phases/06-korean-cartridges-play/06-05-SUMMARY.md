---
phase: 06-korean-cartridges-play
plan: "05"
subsystem: cartdb
tags:
  - cartdb
  - quality
  - ci
  - docs
status: complete

requires:
  - phase: 06-korean-cartridges-play
    provides: plan 06-03 compact binary table, catalog and the generator's --pin and --catalog options
provides:
  - size.cartdb ceiling of 102400 bytes with a reason in quality.json
  - Linux x64 Size ceilings step measures cartdb.c.o against it, with an oversize control
  - cart.generator pin check on the first line of src/cartdb.inc, with a wrong-pin control
  - maintainer text for regeneration, slot counts, gates and where titles live; notices and provenance cover the catalog
affects: [06-06, 06-07, 06-11]

actuals:
  tokens: 7000
  tasks: 2
  commits: 2

plan_head_before: 168d058c7278d0ee5a36ba6f90f7ee2f2f13cb8f
plan_head_after: da095b0b349373e82d6f7a92a1ad7e11b09818cb
commits: 2

tech-stack:
  added: []
  patterns:
    - "The generator test script has a pin mode selected by -DCASE=pin; the main run calls it for the real files and for a pins file with another commit"

key-files:
  created: []
  modified:
    - quality.json
    - .github/workflows/ci.yml
    - tests/cart/generator.cmake
    - tests/cart/tests.cmake
    - docs/maintaining.md
    - THIRD-PARTY-NOTICES.md
    - PROVENANCE.md

key-decisions:
  - "size.cartdb starts at the 100 KB hard bound (102400) because no hosted value can exist before this key does; a follow-up sets it to the first hosted Linux x64 value plus about 15 percent"
  - "The pin check is a mode of the existing cart.generator script, so no test name is added and the inventories do not change"

requirements-completed:
  - MAP-01
  - MAP-02

coverage:
  - id: C1
    description: "quality.json carries size.cartdb 102400 with one reason entry and no planning ID; the local object is 64752 bytes (Mach-O) and passes the ceiling"
    requirement: MAP-02
    verification:
      kind: other
      ref: "jq -e on quality.json exit 0; cmake -DQUALITY=quality.json -DMEASURED=size.cartdb=bytes:build/ci/CMakeFiles/cogmaster.dir/src/cartdb.c.o -P tests/cmake/quality.cmake exit 0; grep -cE 'MAP-[0-9]' quality.json prints 0"
      status: pass
    human_judgment: false
  - id: C2
    description: "The Linux x64 Size ceilings step measures cartdb.c.o and a control with size.cartdb=999999 must exit 1 naming size.cartdb"
    requirement: MAP-02
    verification:
      kind: integration
      ref: "cmake with -DCMD for quality.cmake at size.cartdb=999999 and -DCODE=1 -DREGEX=size.cartdb -P tests/cmake/expect_fail.cmake exit 0 locally; grep -c size.cartdb .github/workflows/ci.yml prints 3; hosted Linux x64 measurement is confirmed at ship"
      status: pass
    human_judgment: false
  - id: C3
    description: "cart.generator fails unless the first line of src/cartdb.inc names the commit that the three MAME rows of pins.txt share, and a pins file with another mame-sms commit exits 1 matching table pin"
    requirement: MAP-02
    verification:
      kind: unit
      ref: "ctest --test-dir build/ci -R '^cart\\.generator$' exit 0"
      status: pass
    human_judgment: false
  - id: C4
    description: "Maintainer docs give the regeneration command with --pin and --catalog, each slot's seen and emitted counts, the slot table location and where titles live; the notices and provenance cover the catalog and rom.cpp"
    requirement: MAP-01
    verification:
      kind: other
      ref: "grep checks on docs/maintaining.md, docs/conformance.md, THIRD-PARTY-NOTICES.md and PROVENANCE.md exit 0; cmake -DROOT=$PWD -P tests/cmake/budgets.cmake exit 0 at 723 check lines; cmake --workflow --preset hygiene exit 0"
      status: pass
    human_judgment: false
---

# Phase 6 Plan 05: Table gates and maintainer docs Summary

The compact cartridge table is now gated twice: a `size.cartdb` ceiling at the 100 KB bound, measured on Linux x64, and a
`cart.generator` check that the table's first line names the MAME commit `pins.txt` pins; the maintainer text describes
regeneration with `--pin` and `--catalog`.

## What was built

- `quality.json`: `size.cartdb` 102400 and a reason saying it is the hard bound, to be replaced by the first hosted value plus about 15 percent.
- `.github/workflows/ci.yml`: the linux-x64 branch of Size ceilings measures `cartdb.c.o`; a control with `size.cartdb=999999` must exit 1 naming the key.
- `tests/cart/generator.cmake` and `tests/cart/tests.cmake`: `-DPINS` and `-DTABLE` are passed in; the script's `pin` mode requires the three MAME rows of `pins.txt` to agree and the table's first line to contain that commit; the main run proves the real files pass and a pins file with 40 zeros for `mame-sms` exits 1 matching `table pin`.
- `docs/maintaining.md`: regeneration command, slot-table pointer, per-slot seen and emitted counts, titles in `tests/compat/catalog.tsv`, the two gates. `THIRD-PARTY-NOTICES.md` and `PROVENANCE.md` cover the catalog titles and the BSD-3 `rom.cpp` source read through `gh api`.

## Evidence

- Local `cartdb.c.o` size: 64752 bytes (`wc -c < build/ci/CMakeFiles/cogmaster.dir/src/cartdb.c.o`, a Mach-O value on this host).
- `cmake --workflow --preset ci` exit 0; `cmake --workflow --preset hygiene` exit 0; budgets check at 723 check lines (limit 745).
- Task verify commands exit 0 (cart.generator and policy.quality tests, the measured quality run, the oversize control).
- Hosted Linux x64 measurement of `cartdb.c.o` is a ship-time confirmation (plan 06-11).

## Deviations from Plan

**1. [Acceptance criterion not usable as written] `git diff --quiet origin/main -- tests/inventory`**
- **Found during:** Task 1
- **Issue:** exits 1 because earlier plans on this phase branch (06-01 and 06-03) already added inventory lines; it cannot hold on any plan after the first.
- **Fix:** checked the intent instead: `git diff --quiet HEAD -- tests/inventory` exits 0 after this plan's changes, and no test name was added.
- **Files modified:** none.

**2. [Doc baseline] `docs/conformance.md` needed no edit**
- **Found during:** Task 2
- **Issue:** plan 06-03 had already changed the titles line to point at `tests/compat/catalog.tsv`; `grep -c 'Titles come from `src/cartdb.inc`'` already printed 0.
- **Fix:** none needed; the verify grep for `catalog.tsv` in that file passes.

**Total deviations:** 2 (both plan-text drift, no code impact).

## Known Stubs

None.

## Threat Flags

None.

## Next

Plans 06-06 and 06-07 regenerate the table; run `ctest --test-dir build/ci -R '^cart\.generator$'` again after the later of the two.

## Self-Check: PASSED

- `quality.json`, `tests/cart/generator.cmake` and `docs/maintaining.md` exist and were committed (db0b0f2, da095b0 are ancestors of HEAD).
