---
phase: 03-games-play-in-retroarch
plan: "11"
status: complete
subsystem: cartridge-metadata
tags:
  - sha1
  - metadata
  - battery
  - provenance
requires:
  - phase: 03-10
    provides: Shared profiles, input and production fixture execution
provides:
  - Public-pinned immutable whole-body cartridge classification
  - Separate persistence knowledge, declared length and supported geometry
  - Reproducible owned battery fixture on the production lookup path
  - Explicit unsupported Moldorian persistence geometry disposition
affects:
  - 03-12
  - 03-13
  - 03-14
  - 03-15
tech-stack:
  added: []
  patterns:
    - Bounded own C17 metadata converter with no XML runtime dependency
    - Shared core SHA-1 with runner compatibility includes
    - Fixed fixture identity checked after linking independently of the core
key-files:
  created:
    - tools/cartdb.c
    - src/cartdb.c
    - src/cartdb.h
    - src/cartdb.inc
    - src/sha1.c
    - src/sha1.h
    - tests/cart/generator.cmake
    - tests/cart/fixture.cmake
    - tests/cart/fixture_facts.txt
    - tests/roms/src/battery.asm
    - tests/roms/src/battery.link
    - docs/hardware/cartridges.md
  modified:
    - src/cart.c
    - src/instance.c
    - src/internal.h
    - include/cogmaster.h
    - runner/probe.c
    - runner/sha1.c
    - runner/sha1.h
    - CMakeLists.txt
    - pins.txt
    - PROVENANCE.md
    - THIRD-PARTY-NOTICES.md
    - docs/embedding.md
    - docs/maintaining.md
    - docs/runner.md
    - docs/conformance.md
    - tests/cart/test_cartdb.c
    - tests/cart/tests.cmake
    - tests/fixtures/tests.cmake
    - tests/api/v3.c
    - tests/runner/tests.cmake
    - tests/built/expected.txt
    - tests/roms/manifest.txt
    - tests/inventory/ci.txt
    - tests/inventory/wasm32.txt
key-decisions:
  - Moldorian retains its known battery identity and 24576-byte declaration with unsupported persistence geometry; no guessed battery span is authorized.
  - Card and cartridge title aliases coalesce only when hardware facts agree; the shared SMS and GG SMS-mode image defaults to SMS.
  - Owned fixture metadata is a fixed MIT fact consumed by the production converter, independently checked after assembly and excluded from public identity export.
requirements-completed:
  - GAME-03
coverage:
  - id: immutable-cartridge-identities
    description: Public-pinned CC0 identities, sorted deterministic generation, title aliases, conflicting facts, entities, numeric sizes and compound-record exclusion
    requirement: GAME-06
    verification:
      - kind: unit
        ref: cart.generator and cart.database
        status: pass
      - kind: other
        ref: Pinned three-file regeneration and byte-for-byte cmp
        status: pass
    human_judgment: false
  - id: automatic-machine-and-mode
    description: Table and header facts resolve AUTO profiles, including GG pin_42 SMS mode, while explicit host settings remain authoritative
    requirement: GAME-03
    verification:
      - kind: unit
        ref: cart.database and abi.api_v3
        status: pass
      - kind: integration
        ref: runner.probe.keys, runner.probe.headerless and runner.probe.unsupported
        status: pass
    human_judgment: false
  - id: persistence-classification
    description: Known nonbattery, unknown persistence, supported battery and known unsupported geometry remain independent of mapper allocation and ledger membership
    requirement: GAME-05
    verification:
      - kind: unit
        ref: cart.database and cart.generator
        status: pass
    human_judgment: false
  - id: owned-battery-production-identity
    description: Fixed fixture SHA-1 matches after linking; production classification survives rename and copier header, rejects a changed byte, and executes the guest counter mailbox
    requirement: GAME-05
    verification:
      - kind: integration
        ref: cart.fixture_identity, cart.fixture_hash and cart.fixture_wrong_hash
        status: pass
      - kind: other
        ref: Battery image rebuilt from source and checked by the post-link SHA-1 command
        status: pass
    human_judgment: false
  - id: moldorian-geometry-disposition
    description: Real 24576-byte declaration is retained with unsupported geometry after the narrow wiring investigation found insufficient physical evidence
    requirement: GAME-05
    verification:
      - kind: unit
        ref: cart.database
        status: pass
      - kind: other
        ref: docs/hardware/cartridges.md primary-source evidence and confidence disposition
        status: pass
    human_judgment: false
actuals:
  tokens: 65565
  tasks: 2
  commits: 4
commits: 4
plan_head_before: 4186faf7434b41b5ae95857b2cce9e3a90022f08
plan_head_after: d388617db31d7312229cc406d109023412bd9763
duration: 14min
completed: 2026-10-20
---

# Phase 03 Plan 11: Cartridge identity without guessed battery wiring Summary

An immutable SHA-1 database identifies public cartridges and the owned battery fixture while separating declared persistence from supported physical decoding.

## Tasks and Commits

1. Resolve image identities through the production database: RED `0adafa5`, GREEN `2c3c349`.
2. Prove fixture identity and settle the 24 KiB disposition: RED `0dfabde`, GREEN `d388617`.

## Results

The public MAME commit `1d7a454ed79e97d35d2a9f127ac029ead94711d6` resolves through the GitHub API. All three
downloaded hash files state CC0 in their own headers and have pinned SHA-256 values. The converter emits
1,656 unique commercial records, excludes and audits 21 compound/incomplete records, and coalesces aliases
only when hardware facts agree. The separately supplied owned fact brings the production table to 1,657.
No commercial image, save or trace was used. SG/unknown devices remain classification facts without support claims.

SHA-1 moved from the runner into the core; runner compatibility includes preserve existing callers.
The core still links only its permitted memory functions. Classification fields append to the existing
size-tagged probe structure, preserving the first 52 bytes. New runner probe keys append after the old keys.

The owned 64 KiB battery fixture has SHA-1 `d9f9fbc463a136cdd5349da772abd0b4347787cb` and SHA-256
`bebe7dd486c9e0f4c903d0f2b83ddba312b5bf8c1ef1d199d372e50f4845c7fb`. Its fixed fact is checked immediately
after linking, without a core/fixture build cycle. The production loader classifies renamed/copier-header
variants identically; changing one byte removes identification. The guest mailbox reports initial count 0
and updated count 1. Its CRC calculation finishes within the eight-frame test window.

The pinned Moldorian record and [SMS Power](https://www.smspower.org/Games/Moldorian-GG) substantiate the
24 KiB declaration. The narrow PCB/chip/address-line search found no verifiable physical wiring. Its geometry
therefore remains explicitly unsupported, independently of its known battery status and ordinary bounded
mapper allocation. No 24575 mask, guessed modulo, truncated physical RAM or invented battery span was added.

GAME-05 and GAME-06 remain pending overall: this plan provides their identity/geometry foundations; raw spans,
generation, persistence and compatibility ledger execution belong to subsequent plans. GAME-03 stays complete.

## Command Evidence

All commands ran from the repository root. Outputs stayed in ignored build directories, never commits.
No hosted run was dispatched by this delegated plan; hosted run URL is not applicable to these local checks.

- gh api repos/mamedev/mame/commits/1d7a454ed79e --jq .sha — exit 0; resolves the full public pin.
- Three gh api repos/mamedev/mame/contents/hash/LIST.xml?ref=FULL-PIN requests with the raw accept header — exit 0; individual CC0 headers checked before conversion.
- build/ci/cogmaster-cartdb build/cartdb-input/sms.xml build/cartdb-input/gamegear.xml build/cartdb-input/sg1000.xml --fixture tests/cart/fixture_facts.txt followed by cmp src/cartdb.inc build/cartdb-input/rebuilt.inc — exit 0; deterministic complete regeneration.
- cmake --workflow --preset ci — exit 0; 298 tests plus 4 release checks passed. The tracer integration workflow also passed before fixture expansion.
- cmake --workflow --preset hygiene — exit 0; 32 tests passed, including formatting, analysis, privacy, core link surface and proportional budgets.
- cmake -E rm -f build/ci/fixtures/battery.sms followed by the CI build — exit 0; assembly and the independent post-link identity check reproduced the image.
- shasum build/ci/fixtures/battery.sms — exit 0; matches the fixed SHA-1 above.
- git diff --check — exit 0.

## TDD Gate Compliance

Both tasks committed RED before GREEN. `cart.database` failed its executed GG AUTO machine assertion
(SMS2 instead of GG); `cart.fixture_identity` failed its executed identification/persistence assertions
before the owned fact existed. Actual unchanged CTest JUnit reports returned `RED_EVIDENCE_OK` before each
implementation. Semantic inspection confirmed the planned assertion failures, not build or discovery errors.
Global TDD runtime gating and auto-advance were disabled. No separate refactor commit was needed.

## Deviations from Plan

- Supporting private digest/database headers and focused CMake fixture/generator checks were added to keep
  the shared implementation small and the post-assembly identity check independent of core targets.
- Runner contract tests and docs were updated with the appended probe output fields; this preserves the
  documented ordering and tests the actual consumer surface.
- Requirement completion records retain GAME-05/GAME-06 as pending until their subsequent implementation plans.
  Marking those complete for metadata alone would overstate the delivered behavior.

## Deferred Issues

None. Unsupported Moldorian geometry is the plan's explicit evidence disposition, not a stub. No authentication
gate, dependency install, commercial-byte artifact or new unmodeled network/auth surface was introduced.

## Self-Check: PASSED

All twelve created artifacts exist. All four task commits are ancestors of HEAD. Both CI inventories include
the new cases, and the executed CI and hygiene workflows passed after the final implementation changes.
