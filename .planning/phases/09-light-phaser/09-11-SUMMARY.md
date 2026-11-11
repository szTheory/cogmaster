---
phase: 09-light-phaser
plan: "11"
subsystem: cart-table-and-ledger
tags:
  - light-phaser
  - cartdb
  - compat-ledger
  - docs
status: complete

requires:
  - 09-06
  - 09-10
provides:
  - table controller code 1 resolves to the Light Phaser on the port that carries it (13 rows, Gangster Town on both ports), cut back to a pad on a Game Gear by the existing availability rule
  - the ledger key token lphaser accepted on either port in every copy of the key grammar, with auto still refused
  - the controller-tests table of docs/testing.md lists every test family of the phase, and the README names Light Phaser titles
affects: [09-12]

actuals:
  tokens: 6200
  tasks: 3
  commits: 3

plan_head_before: 895cdc4b3ca17796b31d1276adcfb993a5f6bd32
plan_head_after: 4281c0b9cbd9a645229d6929acf593a089518a52
commits: 3

tech-stack:
  added: []
  patterns:
    - "The table intent is an unconditional mapping of a code to a device; the Game Gear degrade stays in one place (the availability rule behind cogmaster_get_info), and the per-row test writes both again from the table's codes"
    - "A key grammar widened in place: each regex copy gains a word, so no counted check line is added"

key-files:
  created: []
  modified:
    - src/cart.c
    - tests/cart/test_cart_device.c
    - scripts/compat-common.sh
    - scripts/library.cmake
    - tests/compat/tests.cmake
    - tests/compat/ratchet.cmake
    - docs/conformance.md
    - docs/testing.md
    - README.md

key-decisions:
  - "Code 1 maps to the Light Phaser with no region or machine side effect: region_guess and machine_guess stay as the row left them, and only the Paddle keeps its Japan rule"
  - "The ledger keeps its pad-by-default rule: a row without a cfg: prefix is port1=pad;port2=pad, so no ledger row moves"
  - "No test name is added: the existing cart.device cases follow the new mapping per row, so no inventory file changes"

requirements-completed: []

coverage:
  - id: C1
    description: "Table code 1 maps to COGMASTER_DEVICE_LIGHT_PHASER on the port that carries it, code 4 and every other code stay a pad, and the resolved defaults degrade to a pad on a Game Gear and keep the gun on every other profile; cart.device.defaults and cart.device.degrade assert it per row from the table's codes and the availability rule, and region_guess is untouched by a phaser row"
    requirement: GUN-01
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci -R '^(cart\\.|runner\\.device\\.|fixtures\\.(paddle|sports_pad|light_phaser)\\.|libretro\\.host\\.devices)' exit 0 (52 tests); control that maps code 1 to a pad again (python edit, git diff --stat HEAD confirmed, build exit 0) turned cart.device.defaults and cart.device.degrade red, reverted, exit 0"
        status: pass
    human_judgment: false
  - id: C2
    description: "A ledger key names lphaser on port 1 or port 2 in the shell, cmake and test copies of the grammar; the key test loops 5 by 5 (25 distinct pairs, a duplicate is exit 2), the rejection test loops auto, phaser and lightgun in both positions (6 refused), and the ratchet treats a changed lphaser key as exit 2 and an adjacent lphaser key as exit 0 on each port"
    requirement: GUN-03
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^(compat\\.|policy\\.compat_doc_fresh|docs\\.budgets)' exit 0 (17 tests); control that restored the old set in scripts/compat-common.sh turned compat.ratchet and compat.device_key red, reverted, exit 0"
        status: pass
    human_judgment: false
  - id: C3
    description: "The grammar paragraph of docs/conformance.md names pad|none|paddle|sports|lphaser and says only auto is refused; docs/testing.md lists ports.gun.*, ports.state.gun, fixtures.light_phaser.*, runner.gun.*, host.aim, libretro.host.gun.*, policy.no_title_table* and abi.api_v13 and says a ledger key names the Light Phaser too; README.md names Light Phaser titles; docs stay under their caps"
    requirement: GUN-03
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^(docs\\.|policy\\.)' exit 0 (39 tests); grep -cF 'only `auto` is refused' docs/testing.md prints 1 and the old sentence prints 0; wc -c docs/testing.md prints 35785 (cap 38912)"
        status: pass
    human_judgment: false
  - id: C4
    description: "No ledger row, hash, count pin or existing pin moved, and the counted check code did not grow: the compat ledger, the table count pins and generated table, tests/io, tests/audio and the test card pins are unchanged against origin/main, and the budget check reports 735 check lines before and after"
    requirement: GUN-03
    verification:
      - kind: integration
        ref: "git diff --quiet origin/main -- tests/io tests/audio tests/testcard_pins.cmake tests/compat/ledger.csv tests/cart/test_cartdb.c src/cartdb.inc exit 0; cmake -DROOT=$PWD -P tests/cmake/budgets.cmake exit 0 (735 check lines, 9225 src lines); cmake --workflow --preset ci exit 0; cmake --workflow --preset hygiene exit 0 (50 tests)"
        status: pass
    human_judgment: false
---

# Phase 9 Plan 11: The table default and the ledger token Summary

A bare run of any of the 13 dumps the hash table flags for the Light Phaser now attaches the gun on its port (both ports for Gangster Town), a Game Gear cuts it back to a pad, and a ledger key may name `lphaser` on either port while only `auto` stays refused.

## Accomplishments

- `src/cart.c`: `CART_CTRL_LIGHT_PHASER` and the `ctrl_device` mapping; the comment on the table's codes says code 4 and any other read as a pad and a phaser row carries no region or machine of its own. No generator, table or count pin changed.
- `tests/cart/test_cart_device.c`: `CODE_PHASER`, the intent and the availability rule (a gun or a Sports Pad on any console but a Game Gear, a Paddle only on a Japanese one), written again from the table's codes; still per row and count agnostic.
- The device word list of each regex copy (`scripts/compat-common.sh`, `scripts/library.cmake`, `tests/compat/tests.cmake`) and the grammar paragraph in `docs/conformance.md` gain `lphaser`; `compat.device_key` loops 25 pairs, `compat.device_rejected` loops `auto phaser lightgun` and expects 6 refusals; `tests/compat/ratchet.cmake` has a changed-key and an adjacent-key case for each lphaser port.
- `docs/testing.md`: eight new rows in the controller-tests table and the corrected ledger sentence; `README.md` names Light Phaser titles.

## Task Commits

1. Task 1, the table default and its per-row test: 780ab75
2. Task 2, the ledger token, key cases and ratchet cases: 6e2d3eb
3. Task 3, the testing guide rows and the README line: 4281c0b

## Deviations from Plan

None - plan executed as written. One slip, not a deviation of the result: the first control of Task 2 ended with `git checkout -- scripts/compat-common.sh`, which also dropped the uncommitted widening of that file; the widening was re-applied, `ctest -R '^compat\.'` run again (exit 0, 7 of 7), and the diff re-checked before the commit, so no commit carries the loss.

## Phase notes

- The testing-guide rows and the README line land here, where `docs/testing.md` was already open for the token sentence, so the closing plan stays at ten files.
- The grep inventory of the regex copies found exactly three (the shell, cmake and test copies); the plan's "four regexes" count included the grammar sentence in `docs/conformance.md`.
- `docs/testing.md` is 35785 bytes (cap 38912); the budget check still reports 735 counted check lines (`ratchet.cmake` is not a counted file).
- No test name was added: `cart.device.*` already covers the mapping, so no inventory file changed.

## Verification evidence

- `cmake --preset ci`, `cmake --build --preset ci`, `ctest --test-dir build/ci -R '^(cart\.|runner\.device\.|fixtures\.(paddle|sports_pad|light_phaser)\.|libretro\.host\.devices)'`: exit 0 (52 tests); `ctest --test-dir build/ci -N -R '^cart\.'` lists 4 cart.device cases.
- Container compile, `docker run --rm -v "$PWD":/w -w /w gcc:13 gcc -std=c17 -Wall -Wextra -Wpedantic -Werror -Wtype-limits -Wconversion -fsyntax-only -I src -I include -I build/ci/generated -I tests tests/cart/test_cart_device.c src/cart.c`: exit 0. `/opt/homebrew/opt/llvm@21/bin/clang-format --dry-run -Werror src/cart.c tests/cart/test_cart_device.c`: exit 0.
- Control 1 (python edit removing the code 1 mapping, `git diff --stat HEAD` showed the change, build exit 0): `cart.device.defaults` and `cart.device.degrade` failed; reverted, 4 of 4 pass.
- `ctest --test-dir build/ci -R '^(compat\.|policy\.compat_doc_fresh|docs\.budgets)'`: exit 0 (17 tests); the counts `compat.device_*` 2, `policy.compat_doc_fresh` 2, old regex copies 0, `device tokens refused: 6` 1 and `: 4` 0, `lphaser` in ratchet.cmake 6, added lines equal removed lines in compat-common.sh and library.cmake.
- Control 2 (python edit restoring the old set in scripts/compat-common.sh, diff confirmed): `compat.ratchet` and `compat.device_key` failed (exit 8 class), restored, `ctest -R '^compat\.'` 7 of 7.
- `cmake -DROOT=$PWD -P tests/cmake/budgets.cmake`: exit 0, 735 check lines before and after (limit 737 for this plan).
- `ctest --test-dir build/ci -R '^(docs\.|policy\.)'`: exit 0 (39 tests, baseline 35).
- `ctest --test-dir build/ci -R '^(cart\.|compat\.|docs\.|runner\.|libretro\.|fixtures\.)'`: exit 0 (238 tests).
- `cmake --workflow --preset ci`: exit 0. `cmake --workflow --preset hygiene`: exit 0 (50 tests).
- `git diff --quiet origin/main -- tests/io tests/audio tests/testcard_pins.cmake tests/compat/ledger.csv tests/cart/test_cartdb.c src/cartdb.inc`: exit 0.
- Planning-id grep over the added lines of src, tests, scripts and docs: count 0.
- Hosted lanes not dispatched by this plan (no push); the closing plan reads the hosted run.

## Known Stubs

None.

## Threat Flags

None. The change maps a table code to an existing device and widens a key grammar that a regex still bounds; no file, network or core state is added.

## Next Phase Readiness

The closing plan can document the table default in `docs/controllers.md`, record the hosted run, and settle the provisional timeouts.

## Self-Check: PASSED

- src/cart.c, tests/cart/test_cart_device.c, tests/compat/ratchet.cmake, docs/testing.md exist; 780ab75, 6e2d3eb and 4281c0b are ancestors of HEAD.
