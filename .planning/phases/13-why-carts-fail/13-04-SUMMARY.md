---
phase: 13-why-carts-fail
plan: 04
subsystem: library-judge
tags: [cmake, diagnose, leak-scan, canary, negative-control, docs]

requires:
  - phase: 13-why-carts-fail
    provides: sh scripts/library.sh diagnose with the closed grammar, leaf taxonomy and totals (plans 02 and 03)
provides:
  - diag_scan, diag_check, diag_need and diag_channels in tests/library/diagnose.cmake, a canary scan over stdout, stderr and every retained file after a vacuity guard
  - five expect_fail controls library.diagnose.leak_name, .leak_path, .leak_bytes, .leak_digest and .leak_title
  - library.diagnose.hostile, a hostile catalog title and a hostile runner on the COGMASTER_LIBRARY_RUN seam
  - the diagnostic described in docs/conformance.md, docs/testing.md and ASSET_POLICY.md section 4
affects: [13-05, library diagnose]

actuals:
  tokens: 3709
  tasks: 3
  commits: 4
  plan_head_before: 4bd03d03932c6bfa813b0762db33f96fa01be1a2
  plan_head_after: 2bbe0f76679f5fb1489bab387f0973ad47e5374b

tech-stack:
  added: []
  patterns:
    - "canaries assembled with string(CONCAT) from fragments so no whole canary sits in a tracked file"
    - "a static negative control plants its leak into real captured output of an empty library and must fail through expect_fail with a pinned per-class message"
    - "a scan baseline asserts the clean output passes before the plant is appended, with a message that cannot satisfy the control's own regex"

key-files:
  created: []
  modified:
    - tests/library/diagnose.cmake
    - tests/library/tests.cmake
    - tests/inventory/ci.txt
    - tests/inventory/wasm32.txt
    - docs/conformance.md
    - docs/testing.md
    - ASSET_POLICY.md
    - runner/diag_class.h
    - runner/expect.h
    - tests/runner/test_runner.c

key-decisions:
  - "The scan reads text as bytes and fires leak_bytes on anything outside 0x20-0x7E and LF; the printable bytes canary is caught only by the literal check, so its control pins leak_bytes and nothing else"
  - "The control baseline FATALs with a message that carries no leak class text, so a dirty baseline cannot satisfy the control's own regex"
  - "A catalog title with an escape or right-to-left byte never reaches the awk: file(STRINGS) in library.cmake stops a row at the first non-printable byte, the cart is excluded, and the hostile case pins skipped_excluded=1 beside a printable hostile title that is withheld"
  - "leak_path reports leak_name as well, because the planted absolute path holds the directory canary and a file extension; the control regex pins leak_path"

requirements-completed:
  - DIAG-02

coverage:
  - id: D1
    description: "library.diagnose plants a bytes canary in the halt image tail, a name canary and a home-path fragment in an uncatalogued image file name and a directory canary in the image directory, runs a vacuity guard first, and finds none of them, no digest, no path, no byte outside 0x20-0x7E, no slash, no 8-digit hex run and no title outside the allowed stand-ins in stdout, stderr or diagnose.txt"
    requirement: DIAG-02
    verification:
      - kind: integration
        ref: library.diagnose
        status: pass
    human_judgment: false
  - id: D2
    description: "the retained file set under the diagnose output directory is exactly diagnose.txt after a call"
    requirement: DIAG-02
    verification:
      - kind: integration
        ref: library.diagnose
        status: pass
    human_judgment: false
  - id: D3
    description: "the controls leak_name, leak_path, leak_bytes and leak_digest each plant their leak into captured output of an empty library and fail through expect_fail with exit 1 and the pinned diagnose leak message"
    requirement: DIAG-02
    verification:
      - kind: integration
        ref: library.diagnose.leak_name
        status: pass
      - kind: integration
        ref: library.diagnose.leak_path
        status: pass
      - kind: integration
        ref: library.diagnose.leak_bytes
        status: pass
      - kind: integration
        ref: library.diagnose.leak_digest
        status: pass
    human_judgment: false
  - id: D4
    description: "leak_title sets a catalogued fail cart's title to the name canary and goes through the real library.sh, library.cmake and awk path; expect_fail requires both leak_name and leak_title"
    requirement: DIAG-02
    verification:
      - kind: integration
        ref: library.diagnose.leak_title
        status: pass
    human_judgment: false
  - id: D5
    description: "a printable hostile catalog title is withheld as title=- with title_withheld=1, an escape and right-to-left title excludes its cart, and a runner that appends the image path and its first 16 bytes changes nothing in the output except a higher malformed count; the scan passes on both calls"
    requirement: DIAG-02
    verification:
      - kind: integration
        ref: library.diagnose.hostile
        status: pass
    human_judgment: false
  - id: D6
    description: "docs/conformance.md, docs/testing.md and ASSET_POLICY.md describe the diagnostic within their byte caps, with no README, ledger, page or release input changed"
    requirement: DIAG-02
    verification:
      - kind: other
        ref: cmake -DROOT=$PWD -P tests/cmake/budgets.cmake; git diff --quiet origin/main -- README.md tests/compat/ledger.csv docs/compatibility.md
        status: pass
    human_judgment: false
  - id: D7
    description: "the ci and hygiene workflows exit 0 on the full phase code"
    requirement: DIAG-02
    verification:
      - kind: other
        ref: cmake --workflow --preset ci; cmake --workflow --preset hygiene
        status: pass
    human_judgment: false

duration: 13min
completed: 2026-11-12
status: complete
---

# Phase 13 Plan 04: Leak scan, controls and diagnostic docs Summary

**`library.diagnose` now scans stdout, stderr and every kept file for planted canaries, image digests, paths and structure after a vacuity guard, five expect_fail controls plant one leak each and must fail with a named class, a hostile case pins the title and runner edges, and the diagnostic is documented within the doc caps.**

## Performance

- **Duration:** 13 min
- **Started:** 2026-11-12T23:30:00Z (approximate)
- **Completed:** 2026-11-12T23:43:00Z
- **Tasks:** 3
- **Files modified:** 10

## Accomplishments

- `tests/library/diagnose.cmake`: canaries built with `string(CONCAT)` (a bytes canary of 12 letters g to z in the halt image tail, a name canary plus a home-path fragment in the uncatalogued image's file name, a directory canary as the images' parent). `diag_scan` fires the classes name, path, bytes, digest and title in that order, each from `string(FIND)` literals and one regex without intervals; `diag_check` FATALs with `diagnose leak: leak_<class>` joined by commas; `diag_need` fails with `diagnose vacuity: <what>`; `diag_channels` asserts the retained set is exactly `diagnose.txt` and scans stdout, stderr and the file with CR removed. CASE main runs the guard first (bytes canary in the halt image, name and directory canaries in the glob of the image directory, the never-draws row and group printed, the pass title absent).
- Static controls `leak_name`, `leak_path`, `leak_bytes`, `leak_digest` (one case code path chosen by CASE) capture real output of an empty library, assert the scan passes it, append the class canary and call `diag_check`. `leak_title` runs the real `library.sh`, `library.cmake` and awk path through a canned-line wrapper whose catalog title is the name canary, and expects `leak_name` and `leak_title` together. All five go through `tests/cmake/expect_fail.cmake`, exit 1, TIMEOUT 30, in one registration loop.
- `library.diagnose.hostile`: call 1 with the real runner (title withheld, `skipped_excluded=1`, `title_withheld=1`, scan clean), call 2 through a wrapper that runs the real runner then prints the image path and `dd` of its first 16 bytes; the output equals call 1's except `malformed` rises (0 to 2), and the scan passes.
- Docs: `docs/conformance.md` paragraph (579 bytes added, 35312 total), `docs/testing.md` paragraph with the command, the eight tests, `COGMASTER_LIBRARY_OUT` and the SIGKILL note (35024 total), `ASSET_POLICY.md` section 4 three lines (12334 total). Caps are 38,912; README, ledger, page and release inputs are unchanged against origin/main.
- Local timings on one arm64 macOS host: `library.diagnose` 16 to 25 s (timeout 120 s), `library.diagnose.hostile` 3.4 to 3.6 s (timeout 60 s, provisional), each leak control 0.15 to 0.20 s (timeout 30 s), `library.diagnose.taxonomy` 1.3 s.

Tracer gate: interactive, `human_verify_mode` end-of-phase, automated-only verify, so the tracer verify was re-run end to end (exit 0; `library.diagnose` and `library.diagnose.leak_name` green, hygiene green) and expansion continued.

## Task Commits

1. **Style fix: clang-format 21 on the three plan 01 runner files that fail `hygiene.format`** - `bbb6e2e` (style)
2. **Task 1 (tracer): canary scan, vacuity guard, retained set and the leak_name control** - `9170f6c` (test)
3. **Task 2: path, bytes, digest and title controls and the hostile case** - `2bdca0d` (test)
4. **Task 3: docs** - `2bbe0f7` (docs)

**Plan metadata:** the docs commit that follows this file (SUMMARY, STATE, ROADMAP and REQUIREMENTS).

## Files Created/Modified

- `tests/library/diagnose.cmake` - helpers, CASE main additions, leak cases, leak_title and hostile.
- `tests/library/tests.cmake` - 22 added lines: the hostile registration and the control loop.
- `tests/inventory/ci.txt`, `tests/inventory/wasm32.txt` - the six new rows at byte-order position (eight library.diagnose rows in each).
- `docs/conformance.md`, `docs/testing.md`, `ASSET_POLICY.md` - the diagnostic.
- `runner/diag_class.h`, `runner/expect.h`, `tests/runner/test_runner.c` - format only.

## Decisions Made

See key-decisions.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] `hygiene.format` failed on three files from plan 01**
- **Found during:** Task 1 (the acceptance check `cmake --workflow --preset hygiene`)
- **Issue:** `runner/diag_class.h`, `runner/expect.h` and `tests/runner/test_runner.c` were not clang-format 21 clean, so the hygiene workflow the plan requires green could not pass. They are not files this plan owns, but they block its acceptance and Task 3's verify.
- **Fix:** `clang-format -i` with the pinned 21.1.8; the diff is whitespace and comment wrapping only (14 insertions, 10 deletions).
- **Files modified:** runner/diag_class.h, runner/expect.h, tests/runner/test_runner.c
- **Verification:** `cmake --workflow --preset hygiene` exits 0 (84 tests); `cmake --workflow --preset ci` exits 0 (676 tests).
- **Committed in:** bbb6e2e

**2. [Rule 1 - Bug] A catalog title with an escape or right-to-left byte cannot reach the awk**
- **Found during:** Task 2
- **Issue:** the plan expected a catalogued fail cart with an ESC and RLO catalog title to print `title=-` with `title_withheld=1`. `file(STRINGS)` in `scripts/library.cmake` ends a row at the first non-printable byte (measured for 0x01, ESC, 0x7F and the RLO bytes), so such a row loses its status and port columns and the cart is counted `skipped_excluded` and never run; no title is printed at all.
- **Fix:** the hostile case has two carts. One has a printable hostile title (`Evil <b>bold</b> end`, outside the title charset) and expects `title=-` and `title_withheld=1`; the other has the ESC and RLO title built with `printf` octal escapes and expects `skipped_excluded=1`. The runner wrapper step is unchanged. Nothing in the scripts changed.
- **Files modified:** tests/library/diagnose.cmake
- **Verification:** `library.diagnose.hostile` passes; its output shows `malformed=2 skipped_excluded=1 title_withheld=1`.
- **Committed in:** 2bdca0d

**3. [Plan cap not met] `tests/library/diagnose.cmake` is 301 lines against a cap of 221**
- **Found during:** Task 2
- **Issue:** the plan says the executor stops and reports per-case line counts, rather than removing a check, when the cap cannot be met. Every listed check is kept. Per-case lines at HEAD: prologue (`diag_call`, `diag_expect`) 18; canaries and helpers (`diag_scan` 43, `diag_check` 6, `diag_need` 6, `diag_channels` 13, `diag_standin` 7, canaries 6) 81; CASE main 78 (the plan 03 figure plus about 8); CASE taxonomy 68; the four static controls 13; leak_title 11; hostile 32. The plan's share for this plan was about 65 lines; the checks need about 145. After one trimming pass (the plant lookup by class name) 301 is where the code reads plainly; getting to 221 would mean dropping the literal digest and path checks that the regexes subsume, or packing unrelated commands, which the plan forbids.
- **Fix:** none; the overage is reported here and in the plan return. `tests/library/tests.cmake` added 22 lines, within its cap of 25. No repository gate counts this file.
- **Files modified:** tests/library/diagnose.cmake
- **Committed in:** 9170f6c, 2bdca0d

---

**Total deviations:** 3 (1 blocking, 1 bug in the plan's assumption, 1 cap overrun reported)
**Impact on plan:** every listed check and test is in place and green; the escape and RLO title is pinned as an exclusion, which is where the product stops it. The line-cap overrun is a planning estimate that was too low and needs an owner decision on whether to raise the cap or split the file.

## Issues Encountered

None beyond the deviations above.

## Known Stubs

None.

## Threat Flags

None. The new surface is test-only: a canned-line wrapper and a hostile runner wrapper written under the tests' own output directories. `scripts/library.sh` and `scripts/library.cmake` carry no new switch; the only added `COGMASTER_` name anywhere is none, checked by the Task 2 grep.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

Plan 05 can run the diagnostic on the owner's library and record group counts only. Open item for the owner: the 221-line cap for `tests/library/diagnose.cmake` is exceeded by 80 lines with all checks kept; raise it or move the scan helpers into their own file.

## Self-Check: PASSED

`tests/library/diagnose.cmake`, `tests/library/tests.cmake`, `docs/conformance.md`, `docs/testing.md` and `ASSET_POLICY.md` exist; commits bbb6e2e, 9170f6c, 2bdca0d and 2bbe0f7 are ancestors of HEAD; `ctest -N -R '^library\.diagnose'` lists 8 tests; `cmake --workflow --preset ci` exits 0 (676 tests), `cmake --workflow --preset hygiene` exits 0 (84 tests), `cmake -DROOT=$PWD -P tests/cmake/budgets.cmake` exits 0.

---
*Phase: 13-why-carts-fail*
*Completed: 2026-11-12*
