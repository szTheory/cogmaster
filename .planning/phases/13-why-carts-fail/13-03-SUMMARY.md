---
phase: 13-why-carts-fail
plan: 03
subsystem: library-judge
tags: [sh, awk, cmake, diagnose, library, taxonomy]

requires:
  - phase: 13-why-carts-fail
    provides: sh scripts/library.sh diagnose with the leaf classifier and counters (plan 02)
provides:
  - D-18 sub-rows (pcslot, where, exact pc at 5 or more rows) under each leaf and the final leaf order
  - mapper x class and source x leaf cross-cuts and the pass-row histogram as counts-only diag-other lines
  - the ledger cross-check totals (ledger_fail, matched, ledger_fail_not_in_run, ledger_fail_now_pass, run_fail_not_in_ledger)
  - test library.diagnose.taxonomy, canned runner lines through the real library.sh and library.cmake path with no emulation
affects: [13-04, 13-05, library diagnose]

actuals:
  tokens: 3750
  tasks: 2
  commits: 4
  plan_head_before: a9c6d61946ecaa9b427cdfb4995a831fb25f02cf
  plan_head_after: 5dd9ff7a65f4fb86d41fc4cb8d19d558840e7c68

tech-stack:
  added: []
  patterns:
    - "awk emits numeric sort keys ahead of each output line; sh orders with explicit sort keys and strips them with cut"
    - "canned runner lines behind a two-line wrapper script reached through the COGMASTER_LIBRARY_RUN and COGMASTER_LIBRARY_EMULATOR seam"
    - "the ledger is read as the set(compat_N_field [[value]]) lines of compat-common.sh cases, never split on commas"

key-files:
  created: []
  modified:
    - scripts/library.sh
    - tests/library/diagnose.cmake
    - tests/library/tests.cmake
    - tests/inventory/ci.txt
    - tests/inventory/wasm32.txt

key-decisions:
  - "Sub-rows print only for facts that exist: pcslot=none and where=none are not printed, so the non-fault leaves carry no sub-rows"
  - "An exact pc is compared as a string and printed only when 5 or more rows share it; a 4-row pc never appears in any diag-group line"
  - "The ledger cross-check keys a fail row as id, machine, region, tv and fm, the key a record line carries; matched wins over now-pass, and the rest is not-in-run"
  - "The empty-directory, stray-row and malformed-line cases run over the same stand-in catalog and ledger as the main taxonomy call, in their own directories"

requirements-completed:
  - DIAG-01

coverage:
  - id: D1
    description: "diag-group sub-rows pcslot, where and pc print under each leaf; leaves order by rows, class precedence, split, kind and device; an exact pc prints at 5 rows and not at 4"
    requirement: DIAG-01
    verification:
      - kind: integration
        ref: library.diagnose.taxonomy
        status: pass
    human_judgment: false
  - id: D2
    description: "the mapper x class and source x leaf cross-cuts and the pass histogram print as counts-only diag-other lines, and input_mismatch prints with 0 rows over an empty directory"
    requirement: DIAG-01
    verification:
      - kind: integration
        ref: library.diagnose.taxonomy
        status: pass
    human_judgment: false
  - id: D3
    description: "the totals line carries ledger_fail, matched, ledger_fail_not_in_run, ledger_fail_now_pass and run_fail_not_in_ledger read only through compat-common.sh cases, and a fail row with no leaf exits 1 after printing its totals"
    requirement: DIAG-01
    verification:
      - kind: integration
        ref: library.diagnose.taxonomy
        status: pass
    human_judgment: false
  - id: D4
    description: "three malformed runner lines give malformed=3, a CRLF line is accepted, a title of 80 bytes prints and one of 81 prints title=- with title_withheld counted"
    requirement: DIAG-01
    verification:
      - kind: integration
        ref: library.diagnose.taxonomy
        status: pass
    human_judgment: false
  - id: D5
    description: "in the owned scenario in-a-00.sms prints pcslot=0 and where=port sub-rows and boot_to_play.sms appears only as one pass histogram line"
    requirement: DIAG-01
    verification:
      - kind: integration
        ref: library.diagnose
        status: pass
    human_judgment: false
  - id: D6
    description: "the runner diagnose line grammar this output reads is pinned by the runner test from plan 01"
    requirement: DIAG-01
    verification:
      - kind: integration
        ref: runner.diagnose
        status: pass
    human_judgment: false
  - id: D7
    description: "the tracked ledger and page are unchanged against origin/main, the ci workflow exits 0 and the budgets check exits 0"
    requirement: DIAG-01
    verification:
      - kind: other
        ref: git diff --quiet origin/main -- tests/compat/ledger.csv docs/compatibility.md; cmake --workflow --preset ci; cmake -DROOT=$PWD -P tests/cmake/budgets.cmake
        status: pass
    human_judgment: false

duration: 15min
completed: 2026-11-12
status: complete
---

# Phase 13 Plan 03: Sub-rows, cross-cuts and ledger totals Summary

**`sh scripts/library.sh diagnose` now prints each cause leaf with its pcslot, where and 5-row exact-pc sub-rows in a fixed order, the mapper x class and source x leaf cross-cuts, the pass-row histogram and the ledger cross-check totals, all as counts; `library.diagnose.taxonomy` pins every leaf, threshold, order, validator and title edge over hand-written runner lines with no emulation.**

## Performance

- **Duration:** 15 min
- **Started:** 2026-11-12T23:14:00Z
- **Completed:** 2026-11-12T23:29:00Z
- **Tasks:** 2
- **Files modified:** 5

## Accomplishments

- `scripts/library.sh`: the awk now records the class precedence, split order and kind and device codes per leaf, and prints `diag-group: leaf=<leaf> pcslot=<k>`, `where=<class>` and `pc=<hhhh>` sub-rows (the pc only at 5 or more rows, compared as a string). Output is sorted by explicit numeric keys (`-k1,1nr` and the class, split, kind, device keys) and the keys are cut off. It also reads the ledger cases file as a second awk input and prints `diag-other: mapper=.. class=..`, `diag-other: source=.. leaf=..`, `diag-other: pass mapper=.. first_change=<lo>-<hi>` (lo = int((first_change-1)/60)*60+1) and the totals line `judged run_fail ledger_fail matched ledger_fail_not_in_run ledger_fail_now_pass run_fail_not_in_ledger groups_total fail_total`.
- `tests/library/diagnose.cmake`: CASE taxonomy (about 70 lines). A two-line wrapper answers `--probe` and `--diagnose` from `.probe` and `.line` files beside each image; one table of 16 rows (repeat count, id digit, catalog board, title, line) writes 22 images in the main directory and one stray image in its own directory. It pins the 12 leaf kinds in order, the 5-row `pc=0002` line and the absence of `pc=0100`, the cross-cuts, the histogram, the exact counter and totals lines (candidates=22 attempted=22 evaluated=19 malformed=3, ledger_fail=3 matched=1 ledger_fail_not_in_run=1 ledger_fail_now_pass=1 run_fail_not_in_ledger=17 groups_total=18 fail_total=18), the 80 and 81 byte titles, the byte order of the diag lines against a `list(SORT)` copy, the empty directory, and the exit-1 totals gate (stray fail row with no leaf, by exit status and non-empty stdout). CASE main gains the in-a-00 sub-row and the boot_to_play histogram assertions and a looser diag-other shape.
- Registration: 3 lines in `tests/library/tests.cmake` (running total 9 of 25), and the inventory rows in `ci.txt` and `wasm32.txt`.

Measured locally on one arm64 macOS host: `library.diagnose.taxonomy` takes about 1 to 2 seconds (timeout 60 s, provisional; the comment at the registration says about 2 s). `library.diagnose` takes 15 to 30 seconds under load (timeout 120 s). `tests/library/diagnose.cmake` is 156 lines, the plan's cap. `cmake --workflow --preset ci` exits 0 (670 tests), `tests/cmake/budgets.cmake` exits 0, and the tracked ledger and page are unchanged against origin/main.

## Task Commits

1. **Task 1 RED: failing taxonomy case for the sub-rows and group order (tracer)** - `79f9af3` (test)
2. **Task 1 GREEN: sub-rows and the final leaf order** - `8992d09` (feat)
3. **Task 2 RED: failing taxonomy checks for cross-cuts, ledger totals and the totals gate** - `d92b43a` (test)
4. **Task 2 GREEN: cross-cuts, pass histogram and ledger cross-check totals** - `5dd9ff7` (feat)

**Plan metadata:** the docs commit that follows this file (SUMMARY, then STATE, ROADMAP and REQUIREMENTS).

Tracer gate: interactive, `human_verify_mode` end-of-phase, automated-only verify, so the tracer verify was re-run end to end (exit 0; `library.diagnose`, `library.diagnose.taxonomy` and `library.synthetic` green) and expansion continued.

## TDD Gate Compliance

Task 2 carried `tdd="true"`, and Task 1 followed test-first as well. Task 1 RED `79f9af3`: `library.diagnose` failed on the missing `pcslot=0` and `where=port` sub-rows and `library.diagnose.taxonomy` failed in `diag_expect` on the first sub-row token. Task 2 RED `d92b43a`: the taxonomy case passed its earlier tokens (rc=0, sub-rows, counter line with malformed=3 and title_withheld=1) and failed on the totals line `judged=19 run_fail=18 ledger_fail=3 ...`. Semantic assessment: in both cases the target test ran and failed on the planned assertion, not on a load, syntax or fixture fault; the CTest console output is not a report format `gsd_run check tdd-red-evidence` reads, so no classifier verdict is recorded. GREEN `8992d09` and `5dd9ff7` made them pass; no REFACTOR commit.

## Files Created/Modified

- `scripts/library.sh` - sub-rows, final order, cross-cuts, histogram, ledger totals.
- `tests/library/diagnose.cmake` - CASE taxonomy, new assertions in CASE main.
- `tests/library/tests.cmake`, `tests/inventory/ci.txt`, `tests/inventory/wasm32.txt` - registration and inventory rows.

## Decisions Made

See key-decisions.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] The 4-row pc absence check matched the per-row diag lines**
- **Found during:** Task 1
- **Issue:** `diag_out MATCHES "pc=0100"` also matched the per-row `diag: ... pc=0100 ...` lines, which legitimately carry the pc.
- **Fix:** the match is anchored to `diag-group:` lines.
- **Files modified:** tests/library/diagnose.cmake
- **Commit:** 8992d09

**2. [Rule 3 - Blocking] CASE main's closed-grammar shape rejected the new diag-other lines**
- **Found during:** Task 2
- **Issue:** the plan 02 shape allowed only `name=number` pairs in `diag-other:` lines; the cross-cuts (`mapper=none class=..`), the histogram (`pass mapper=.. first_change=1-60`) are not numeric.
- **Fix:** the shape accepts an optional `pass ` and word-or-range values; the counter and totals lines stay pinned by exact tokens in the same test.
- **Files modified:** tests/library/diagnose.cmake
- **Commit:** 5dd9ff7

**3. [Rule 3 - Blocking] Taxonomy case compressed to the line cap with every assertion kept**
- **Found during:** Task 2
- **Issue:** the first full draft of `tests/library/diagnose.cmake` was 160 lines against the plan's cap of 156.
- **Fix:** single-use settings (the stand-in ledger configuration string, the shared first-display fields, the leaf-line prefix and the cross-cut prefix) were inlined and the wrapper script became one `file(WRITE)`; one extra line, `set(stray_rc ...)`, is kept so the exit status has the name the acceptance check greps for.
- **Files modified:** tests/library/diagnose.cmake
- **Commit:** 5dd9ff7

**4. [Out of scope, not changed] One timeout under load in cart.probe.pure**
- The first `cmake --workflow --preset ci` run timed out `cart.probe.pure` (10 s) while heavy tests ran in parallel; alone it takes about 1 s and the second workflow run was clean. Not caused by this plan's files.

---

**Total deviations:** 3 auto-fixed (1 bug, 2 blocking), 1 unrelated flake noted
**Impact on plan:** none on scope; every listed assertion is kept and every acceptance criterion passes.

## Issues Encountered

None beyond the deviations above.

## Known Stubs

None.

## Threat Flags

None. The new surface (a second awk input holding the ledger as the cases parser prints it, one wrapper script under the test's own output directory) is the one in the plan's threat model; the exact-pc leak and the comma-split ledger mitigations are asserted by `library.diagnose.taxonomy` and by the acceptance greps.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

Plan 04 can add the leak scan and controls to `tests/library/diagnose.cmake` within its 221-line ceiling (156 used). Plan 05 can read the printed sub-rows, both cross-cuts, the pass histogram and the totals line into the planning note.

## Self-Check: PASSED

`tests/library/diagnose.cmake` and `scripts/library.sh` exist; commits 79f9af3, 8992d09, d92b43a and 5dd9ff7 are ancestors of HEAD; `cmake --workflow --preset ci` exits 0; `cmake -DROOT=$PWD -P tests/cmake/budgets.cmake` exits 0; `git diff --quiet origin/main -- tests/compat/ledger.csv docs/compatibility.md` exits 0.

---
*Phase: 13-why-carts-fail*
*Completed: 2026-11-12*
