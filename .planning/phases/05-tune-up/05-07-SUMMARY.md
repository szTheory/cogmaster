---
phase: 05-tune-up
plan: "07"
subsystem: cart
tags: [cart, probe, size-rule, docs, tests]
status: complete

requires:
  - phase: 05-tune-up
    provides: the probe section moved to docs/boards.md (plan 05-01)
provides:
  - cogmaster__cart_probe refuses a body over 1 MiB after the copier-header strip (1 MiB + 1 to 1 MiB + 511 bytes)
  - the embedding, boards and runner docs state the body limit rather than a total size
affects: [05-09]

actuals:
  tokens: 3500
  tasks: 1
  commits: 4

plan_head_before: aa1195a5e4f8ea7c0a1d77d35607d17900975955
plan_head_after: 90bb31fd7ab9b5ab7bf3952ab40a30af4cb93979
commits: 4

tech-stack:
  added: []
  patterns:
    - "The size rule is applied to the body after the copier-header strip, so a total size alone never decides it"

key-files:
  created: []
  modified:
    - src/cart.c
    - tests/cart/test_cart.c
    - tests/core/test_core.c
    - docs/embedding.md
    - docs/boards.md
    - docs/runner.md

key-decisions:
  - "The body limit is a named constant (CART_BODY_MAX) and CART_MAX derives from it, so the two bounds cannot drift apart"
  - "The core seeded-load oracle computes the body the same way the probe does, instead of comparing the total size with one bound"

requirements-completed: []

coverage:
  - id: IN-03
    description: "An image of 1 MiB + 1 to 1 MiB + 511 bytes with no copier header is refused with ERR_UNSUPPORTED_SIZE and image_size filled in, while 1 MiB and 1 MiB + 512 (a copier header) still load, and the docs say a body above 1 MiB is refused whatever the total size"
    requirement: TUNE-07
    verification:
      kind: unit
      ref: "ctest --test-dir build/ci -R '^cart\\.probe\\.' exit 8 before the cart.c change (RED) and exit 0 after; ctest -R core.load.rejected_unchanged exit 0; cmake --workflow --preset ci exit 0 with 399 of 399 tests"
      status: passed
    human_judgment: false
---

# Phase 5 Plan 07: Refuse a body over 1 MiB Summary

The cartridge probe now refuses a body above 1 MiB after the copier-header strip, so a 1 MiB + 1 to 1 MiB + 511 byte image without a header returns `COGMASTER_STATUS_ERR_UNSUPPORTED_SIZE` instead of loading a body the largest Sega-mapper board cannot address (review finding IN-03).

## Performance

- **Tasks:** 1 of 1, with four commits (RED test, fix with docs, one oracle follow-up, one formatting follow-up)

## Accomplishments

- `test_sizes` gained probes for 1 MiB + 1 and 1 MiB + 511 (refused, `image_size` filled, `body_offset` 0) and for 1 MiB (loads, `body_offset` 0). The 1,023, 1 MiB + 513 and 1 MiB + 512 cases are unchanged. The case name `cart.probe.sizes` is unchanged, so the test inventory did not move.
- `cogmaster__cart_probe` returns `ERR_UNSUPPORTED_SIZE` when `n - body_offset(n)` exceeds `CART_BODY_MAX` (1,048,576). The comment cites SMS-CARTRIDGES section 1. The change is integer-only. No fixture is that size, so no frame or audio hash moved.
- The docs say it plainly: `docs/embedding.md` (the "Loading and running" sentence and the status-table row), `docs/boards.md` (the probe sentence) and `docs/runner.md` (the `FILE` row and the "Refusals" bullet).

## Task Commits

1. RED test: `1c827ab` (`test(05-07)`)
2. Fix with docs: `d9f3a2e` (`fix(05-07)`), the commit plan 05-09 cites for IN-03
3. Core load oracle follow-up: `9162b4d` (`test(05-07)`)
4. Formatting follow-up: `90bb31f` (`style(05-07)`)

## Evidence

All commands ran from the repository root.

| Command | Exit |
|---|---|
| `ctest --test-dir build/ci -R '^cart\.probe\.sizes$'` before the cart.c change (RED) | 8 |
| `ctest --test-dir build/ci -R '^cart\.probe\.'` after the fix | 0 |
| `git grep -q -e '1,048,576 + 512 bytes; any other size' HEAD -- docs` on the commit before the docs edit | 0 |
| `git grep -q -e '1,048,576 + 512 bytes; any other size' -- docs` after the edit | 1 |
| `ctest --test-dir build/ci -R '^(cart\.probe\.\|docs\.)'` | 0 |
| `cmake --workflow --preset ci` (final) | 0, 399 of 399 tests |
| `cmake --workflow --preset hygiene` (final) | 0, 38 of 38 tests |

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] core.load.rejected_unchanged encoded the old size rule**
- **Found during:** the first `cmake --workflow --preset ci` after the fix
- **Issue:** the seeded-load oracle in `tests/core/test_core.c` expected `OK` for any size from 1,024 to 1 MiB + 512, so it expected a load of the sizes the plan now refuses (status 7 against 0 or 6).
- **Fix:** the oracle computes the body after a copier header (size 512 past a 16 KiB page) and expects the size refusal when that body is over 1 MiB. The refused list in the load-size test also gained 1 MiB + 1 and 1 MiB - 1 + 512 (IMAGE_MAX - 1).
- **Files modified:** `tests/core/test_core.c`
- **Commit:** `9162b4d`

**2. [Rule 1 - Bug] The oracle edit was not clang-formatted**
- **Found during:** `cmake --workflow --preset hygiene` (hygiene.format)
- **Issue:** my first `clang-format -i` call found no binary on PATH and did nothing; the long list line broke hygiene.format.
- **Fix:** formatted with the preset's clang-format (llvm@21).
- **Files modified:** `tests/core/test_core.c`
- **Commit:** `90bb31f`

### Observations

- `hygiene.tree` (60 second limit) timed out twice in the full hygiene workflow while the machine's load average was above 60, and passed alone in 57 seconds. It passed in the later full runs when the load fell. This is the same load-sensitivity plan 05-03 recorded; it is unrelated to this plan's files and was not changed.

## Known Stubs

None.

## Threat Flags

None. The change tightens a bound on untrusted image size and adds no new surface.

## Self-Check: PASSED

- Commits `1c827ab`, `d9f3a2e`, `9162b4d` and `90bb31f` are ancestors of HEAD.
- `src/cart.c`, `tests/cart/test_cart.c`, `tests/core/test_core.c`, `docs/embedding.md`, `docs/boards.md` and `docs/runner.md` exist with the changes.
