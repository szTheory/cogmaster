---
phase: 08-sports-pad-paddle-and-3-d-glasses
plan: "07"
subsystem: cart
tags:
  - default-controller
  - region-trap
  - cart-database
  - paddle
  - sports-pad
status: complete

requires:
  - 08-05
provides:
  - cogmaster_cart_info.default_port1/2 filled from the table row's ctrl1/ctrl2 (2 is PADDLE, 3 is SPORTS_PAD, anything else is PAD)
  - cogmaster_info.default_port1/2 resolved at load against the profile (the table intent, or PAD where the availability rule refuses it)
  - the region rule inside cogmaster__cart_fact_to_info, so a paddle row with no region reads as Japan and the load-time AUTO step takes it
  - four cart.device cases that resolve every table row against the mapping, the region predicate and three profiles
affects: [08-08, 08-13]

actuals:
  tokens: 4100
  tasks: 2
  commits: 2

plan_head_before: b01feba6e27582e2416d3b3d6ffa17a4ed5ff3b0
plan_head_after: d2af3fcf73f9475e7c5c0837bc0ec336d92ad18f
commits: 2

tech-stack:
  added: []
  patterns:
    - "The table intent is held in two instance bytes outside the saved fields, written at every successful load (zero when no row matched) and resolved against cogmaster__device_ok in get_info, so a default is load-derived and never state"
    - "The library test restates the code mapping and the availability rule independently and compares them per row with the row index in the failure text; it never counts rows of one kind"

key-files:
  created:
    - tests/cart/test_cart_device.c
  modified:
    - src/cart.c
    - src/instance.c
    - src/internal.h
    - tests/cart/tests.cmake
    - tests/inventory/ci.txt
    - tests/inventory/wasm32.txt

key-decisions:
  - "A row whose region is 0 keeps whatever region the image header guessed unless a controller is a paddle, which gives Japan; a named region is kept and an explicit region still wins in the load step (the default then degrades to PAD)"
  - "The degrade case drives the instance through the two held bytes of an instance of each profile, because no image whose SHA-1 is in a paddle or sports row exists until the owned fixture rows land; the load path that writes the bytes is covered by a reload that must zero them"

requirements-completed: []

coverage:
  - id: D1
    description: "cogmaster_cart_info.default_port1/2 carry each row's raw table intent for every row of the table, asserted per row (ctrl 2 is PADDLE, 3 is SPORTS_PAD, anything else including codes 1 and 4 is PAD), with no count of paddle or sports rows"
    requirement: PAD-06
    verification:
      - kind: unit
        ref: "tests/cart/test_cart_device.c#test_defaults via ctest cart.device.defaults"
        status: pass
    human_judgment: false
  - id: D2
    description: "A row with region 0 and a paddle ctrl reads Japan and no other row's region_guess moves; a row with a named region keeps it. The predicate is recomputed in the test for every row"
    requirement: PAD-06
    verification:
      - kind: unit
        ref: "tests/cart/test_cart_device.c#test_region via ctest cart.device.region"
        status: pass
    human_judgment: false
  - id: D3
    description: "For every row and for SMS2 export, SMS2 Japan and Game Gear, cogmaster_info.default_port1/2 equal the table intent where the restated availability rule accepts it and PAD otherwise (Paddle on export and on a Game Gear and Sports Pad on a Game Gear degrade to PAD)"
    requirement: PAD-06
    verification:
      - kind: unit
        ref: "tests/cart/test_cart_device.c#test_degrade via ctest cart.device.degrade"
        status: pass
    human_judgment: false
  - id: D4
    description: "No content, a zero-length probe and an image whose SHA-1 is in no row resolve to PAD on both ports, and a reload clears a table intent held by the earlier load"
    requirement: PAD-06
    verification:
      - kind: unit
        ref: "tests/cart/test_cart_device.c#test_none via ctest cart.device.none"
        status: pass
    human_judgment: false
  - id: D5
    description: "Nothing about a pad-only run changed: the cart, core, io, abi, runner and fixtures tests pass unchanged (171 tests), the full ci workflow passes (498 tests) and tests/io, tests/audio and tests/testcard_pins.cmake are byte-equal to origin/main"
    requirement: PAD-06
    verification:
      - kind: integration
        ref: "cmake --workflow --preset ci exit 0; cmake --workflow --preset hygiene exit 0; git diff --quiet origin/main -- tests/io tests/audio tests/testcard_pins.cmake exit 0"
        status: pass
    human_judgment: false

duration: 25min
completed: 2026-11-11
---

# Phase 8 Plan 7: Default controller per dump, resolved against the profile Summary

**The library now reports the controller each dump defaults to (the table's ctrl codes, read into cogmaster_cart_info and resolved against the profile into cogmaster_info, degrading to a pad where the profile cannot take it) and gives a paddle row with no region Japan, so a Paddle dump runs under a Japan profile with no option; every table row is checked per row against an independent restatement of the mapping and the availability rule.**

## Performance

- **Duration:** about 25 min
- **Completed:** 2026-11-11
- **Tasks:** 2
- **Files:** 7 (1 new)

## Accomplishments

- `src/cart.c`: `cogmaster__cart_fact_to_info` maps `ctrl1` and `ctrl2` to the raw intent through one small helper (the codes for the light phaser and the graphic board read as a pad until a later release) and applies the region rule: region 0 with a paddle gives Japan, a named region is kept, every other row is as before. The hardware comment cites SMS Power, Paddle Control.
- `src/instance.c` and `src/internal.h`: `cogmaster_load` stores the probe's intent in two instance bytes outside the saved fields (`table_device`), zero for the test card and for an unmatched image, and `cogmaster_get_info` reports each as the intent where `cogmaster__device_ok` accepts it on the loaded profile, else a pad. State, the visitor and the saved format are untouched.
- `tests/cart/test_cart_device.c`: four cases in one executable (`cart.device.defaults`, `region`, `degrade`, `none`) over every row of the table, with the mapping, the region predicate and the availability rule written again in the test. Every failure message names the row index. The four names are on the ci and wasm32 inventories with `cart,portable`.
- Rows whose `region_guess` moved: exactly 1 of 1658 (row index 1164, 32768 bytes, ctrl1 2, ctrl2 0, board 1), the Woody Pop dump the plan expected, measured with a scratch program over the table (not in the tree).

## Negative controls

Each edit was made with a script, confirmed with `git diff --stat src/cart.c` (1 file) and built before the run, and reverted with `git checkout -- src/cart.c`.

| Control | Command | Exit | Result |
|---|---|---|---|
| region rule dropped (the paddle branch replaced by a no-op that keeps the variable used) | `ctest --test-dir build/ci -R '^cart\.device\.region$'` | 8 | fails; the other three cases pass |
| mapping of 2 and 3 swapped | `ctest --test-dir build/ci -R '^cart\.device\.defaults$'` | 8 | fails (`cart.device.degrade` fails too) |
| both reverted | `ctest --test-dir build/ci -R '^cart\.'` | 0 | 13 of 13 pass |

## Verification run

- Task 1 verify: `cmake --preset ci && cmake --build --preset ci && ctest --test-dir build/ci -R '^(cart\.|core\.|io\.|abi\.(globals|undefined|symbols)|runner\.|fixtures\.)'` exit 0, 171 of 171 pass, `-N` prints `Total Tests: 171`; `grep -c default_port1` gives 1 in src/cart.c and 2 in src/instance.c; `git diff --quiet origin/main -- tests/io tests/audio tests/testcard_pins.cmake` exit 0; no removed line in tests/built/expected.txt, tests/diag/expected.csv or tests/roms/manifest.txt; `cmake -DROOT=$PWD -P tests/cmake/budgets.cmake` exit 0 (723 check lines, 8793 src lines).
- Task 2 verify: `ctest --test-dir build/ci -R '^cart\.'` exit 0 (13 of 13); `-N -R '^cart\.device\.'` lists the four names; four matching lines in each of tests/inventory/ci.txt and wasm32.txt; `git diff --quiet origin/main -- tests/cart/test_cartdb.c` exit 0; the fault-prevented comment appears once.
- Plan: `cmake --workflow --preset ci` exit 0 (498 tests plus 4 release tests); `cmake --workflow --preset hygiene` exit 0 (48 tests); budgets exit 0.
- Nothing was pushed and no hosted run was dispatched; the plan names none. Run URLs: none for this plan.

## Task Commits

1. **Task 1: Defaults from the table, resolution against the profile and the region rule** - `c9b6004` (feat)
2. **Task 2: Per-row default, region trap, degrade and no-content cases** - `d2af3fc` (test)

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] hygiene.format rejected the first form of a comment**
- **Found during:** the plan-level hygiene workflow after Task 2
- **Issue:** a trailing comment on a `#define` line was wrapped by clang-format 21 in a form that its own dry run refuses.
- **Fix:** the comment moved above the two defines; the local task commit was amended (not pushed) before the SUMMARY.
- **Files modified:** tests/cart/test_cart_device.c
- **Committed in:** d2af3fc (part of the Task 2 commit)

### Notes that are not deviations

- The plan's behaviour text says `region_guess = fact->region` at the row level; the code reads it conditionally (region 0 keeps the guess the header gave). The test therefore pre-sets a sentinel and requires it to survive on every row that names no region and has no paddle, which is the stronger statement of "no other row's region_guess moves".
- The profile-resolved cases cannot load an image with a paddle or sports SHA-1 until the owned fixture rows land in a later plan, so `cart.device.degrade` and `none` put the table intent into an instance of each profile through the two held bytes (internal API) and read it back through the public `cogmaster_get_info`. A reload with the test card and with an unmatched image must clear the bytes (covered in `none`). The end-to-end proof that a load of a matched dump stores the intent is the plan that adds the owned rows.
- The table has 1658 rows at this head; the count pins in tests/cart/test_cartdb.c were not touched and pass.

**Total deviations:** 1 auto-fixed (a formatting rule). **Impact:** none.

## Known Stubs

None.

## Threat Flags

None. The plan reads two bytes of a static table into a result struct and adds no endpoint, file access or trust boundary.

## Issues Encountered

None.

## Next Phase Readiness

Ready for plan 08-08. The branch has the 08-05, 08-06 and 08-07 commits locally (pushed through `b05c7b4`).

## Self-Check: PASSED

- `tests/cart/test_cart_device.c` exists; commits `c9b6004` and `d2af3fc` are ancestors of HEAD.
