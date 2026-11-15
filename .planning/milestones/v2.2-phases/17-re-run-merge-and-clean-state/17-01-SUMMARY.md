---
phase: 17-re-run-merge-and-clean-state
plan: 01
subsystem: runner-diagnose
tags: [diagnose, runner, library-sh, closed-grammar, docs]

requires:
  - phase: 16-graphics-i-and-ii-draw
    provides: the legacy renderer, after which text, multicolour, mixed, invalid and native Game Gear modes are the only video-mode faults still raised
provides:
  - the runner line `diagnose 2` with 21 words after the token (30 record fields once library.cmake prefixes it)
  - scripts/library.sh diagnose reducer that accepts token 2 and 30 fields and no removed word
  - diagnose tests, docs and requirement text in agreement with the new shape
affects: [17-02, 17-03, 17-04]

plan_head_before: 9527540cfc2d03c99dbbcb1913ab0024d79c3d75
plan_head_after: 87500605823dfcc8ed752851eecb80e7acb7daa3
commits: 3

actuals:
  tokens: 9992
  tasks: 2
  commits: 3

tech-stack:
  added: []
  patterns:
    - one line-shape change in every consumer at once, with the token and field count moved so a stale runner reads as malformed

key-files:
  created: []
  modified:
    - runner/diag.c
    - runner/diag_class.h
    - scripts/library.sh
    - tests/runner/diagnose.cmake
    - tests/runner/test_runner.c
    - tests/library/diagnose.cmake
    - docs/runner.md
    - docs/conformance.md
    - .planning/REQUIREMENTS.md

key-decisions:
  - "The runner token is `diagnose 2` and the reducer requires 30 fields, so a runner built before this change fed to the new library.sh yields malformed records, never shifted fields"
  - "The fault-kind and device name tables keep the ROM-write and controller names, so no designated-initialiser slot is NULL for printf in runner/main.c"
  - "cogmaster__diag_where answers unknown for the controller and the unnamed device; the vdpmode word with its mode4 canary and the faddr word stay"
  - "TUNE-15 wording amended to keep vdpmode, citing the blocked-mode raise site in src/vdp_render.c"

requirements-completed:
  - TUNE-15

coverage:
  - id: D1
    description: "cogmaster-run --diagnose prints one line starting diagnose 2 with the two class words and the port value of where removed"
    requirement: TUNE-15
    verification:
      - kind: integration
        ref: "ctest --preset ci -R '^runner\\.diagnose$'"
        status: pass
    human_judgment: false
  - id: D2
    description: "scripts/library.sh diagnose accepts only token 2 with 30 fields; closed lists lose the ROM-write kind, the controller device and the port class; sub-rows renumber with size as sub-row 6; taxonomy rows re-point to the cart device with unchanged counts"
    requirement: TUNE-15
    verification:
      - kind: integration
        ref: "ctest --preset ci -R '^library\\.diagnose' (diagnose, taxonomy, hostile and the five leak controls)"
        status: pass
    human_judgment: false
  - id: D3
    description: "cogmaster__diag_where maps the controller and the unnamed device to unknown, the two dead class helpers are gone, and the mode4 canary and faddr word are kept"
    requirement: TUNE-15
    verification:
      - kind: unit
        ref: "ctest --preset ci -R '^runner\\.diagnose\\.class$' (tests/runner/test_runner.c, diag_class)"
        status: pass
    human_judgment: false
  - id: D4
    description: "docs/runner.md shows the diagnose 2 line without the two class rows or the port value; docs/conformance.md names the classes still printed and says to judge a commit already on main through a detached worktree; both stay under the byte cap"
    requirement: TUNE-15
    verification:
      - kind: other
        ref: "ctest --preset ci -R '^docs\\.budgets$' and the grep and wc -c acceptance commands, all exit 0"
        status: pass
    human_judgment: false
  - id: D5
    description: "TUNE-15 wording in the requirements file keeps vdpmode and cites the raise site; nothing under src or include changed"
    requirement: TUNE-15
    verification:
      - kind: other
        ref: "git diff --quiet origin/main -- src include tests/api/v2.c docs/embedding.md tests/bus exits 0, and the TUNE-15 grep acceptance exits 0"
        status: pass
    human_judgment: false
  - id: D6
    description: "the whole lane passes with the new shape"
    requirement: TUNE-15
    verification:
      - kind: integration
        ref: "cmake --workflow --preset ci exits 0 (737 of 737 tests, then 5 of 5 release tests)"
        status: pass
    human_judgment: false

duration: 5min
completed: 2026-11-14
status: complete
---

# Phase 17 Plan 01: Diagnose line prune Summary

**The `--diagnose` line drops its two dead class words and the port value of `where`, moves to token `diagnose 2` with 30 record fields, and `scripts/library.sh`, three test files and two docs agree with it.**

## Performance

- **Duration:** 5 min
- **Started:** 2026-11-14T17:14:00Z (about; STATE.md records the step start at 17:13:54Z)
- **Completed:** 2026-11-14T17:19:00Z (about)
- **Tasks:** 2
- **Files modified:** 9 (net 55 insertions, 139 deletions)

## Accomplishments

- `runner/diag.c` prints `diagnose 2 ... pow2=%s size=%s header=%s`; `wclass` and `pclass` and their two arguments are gone, so the line has 21 words after the token.
- `scripts/library.sh` diagnose reducer: `$9 != "2"`, `NF != 30`, `size` at `$29` and `header` at `$30`, closed lists without the ROM-write kind, the controller device and the port class, sub-row labels `pcslot where pc faddr vdpmode size` with `size` as sub-row 6. The `library-diagnose 1` header and the `run` branch are untouched.
- `runner/diag_class.h` lost `cogmaster__diag_wclass` and `cogmaster__diag_pclass`; `cogmaster__diag_where` answers `unknown` for the controller and the unnamed device. `cogmaster__diag_vdpmode` (mode4 canary), `cogmaster__diag_faddr`, `pow2`, `size` and `header` are unchanged.
- Both diagnose tests carry the new shape. The two taxonomy rows that used removed values now read `unmapped_read dev=cart where=rom` and `unmapped_write dev=cart`; every row count and the malformed count of 3 are unchanged, and the expected leaf and sub-row names matched the reducer output without further edits.
- `docs/runner.md` (37,973 to 37,491 bytes) and `docs/conformance.md` (38,034 to 38,106 bytes) are under the 38,912 cap; the conformance page now says to judge a commit already on `main` through a detached worktree of `origin/main` and carry the ledger and page diff into the branch. No gate or test checks stamp ancestry.
- Nothing under `src/` or `include/` changed: behaviour revision 19 and state format 17 stand.

## Commands and exit statuses

All run from the repository root. Nothing was pushed in this plan, so there is no run URL; the lane is plan 03's push.

| Command | Exit |
|---|---|
| `cmake --preset ci` then `cmake --build --preset ci` | 0 |
| `ctest --preset ci -R '^(runner\.diagnose\|library\.diagnose(\..+)?)$' --output-on-failure --no-tests=error` (9 tests, run twice: once before the first commit, once as the tracer gate) | 0 |
| `ctest --preset ci -R '^runner\.diagnose\.class$' --no-tests=error` against the edited test and the old helper | 8 (RED, two where checks failed) |
| `ctest --preset ci -R '^(runner\.diagnose(\.class)?\|library\.diagnose(\..+)?\|docs\.budgets)$' --output-on-failure --no-tests=error` (11 tests) | 0 |
| clang-format 21 `--dry-run --Werror` over `git ls-files '*.c' '*.h'` (excluding `libretro/libretro.h`) | 0 |
| `cmake --workflow --preset ci` (737 of 737, then 5 of 5) | 0 |
| `git diff --quiet origin/main -- src include tests/api/v2.c docs/embedding.md tests/bus` | 0 |
| Task 1 acceptance greps (eight) and Task 2 acceptance greps and byte caps (ten) | 0 each |

## Task Commits

1. **Task 1: One diagnose line end to end in the new shape (tracer)** - `8f4fa1e` (refactor)
2. **Task 2, RED: expect unknown for the controller and unnamed device, drop the class tables** - `96ec766` (test)
3. **Task 2, GREEN: remove the dead class helpers, update the docs and the requirement wording** - `8750060` (refactor)

**Plan metadata:** the commit that carries this file (docs: complete plan).

## TDD notes (Task 2)

- **RED:** `tests/runner/test_runner.c`, case `diag_class`, run as `runner.diagnose.class`. The two `where` checks for `COGMASTER_FAULT_DEVICE_CTRL` and `COGMASTER_FAULT_DEVICE_NONE` now expect `unknown`; the test failed on exactly those two assertions (lines 251 and 252) with the helper still returning `port`, for the planned reason. The ctest output format is outside the formats the RED classifier reads, so the evidence is the unmodified ctest report and this semantic assessment; `workflow.tdd_mode` is off for this project.
- **GREEN:** `cogmaster__diag_where` lost the two case labels and falls to `default`; the test then passed and the 11-test subset and the full lane passed.
- **REFACTOR:** none needed; the two class helpers were deleted in the GREEN commit, as the plan placed them with the header edit.

## Files Created/Modified

- `runner/diag.c` - the `diagnose 2` format string and arguments
- `runner/diag_class.h` - two helpers removed, `where` mapping, file comment
- `scripts/library.sh` - the diagnose reducer: token, field count, closed lists, sub-rows, labels
- `tests/runner/diagnose.cmake` - shape regex and halt expectation
- `tests/library/diagnose.cmake` - grammar regex, canned lines, re-pointed rows, leaf names, leak_title line
- `tests/runner/test_runner.c` - `where` checks and the two removed class tables
- `docs/runner.md` - example line, `where` row, two table rows removed
- `docs/conformance.md` - evidence-class list and the worktree sentence
- `.planning/REQUIREMENTS.md` - TUNE-15 wording

## Decisions Made

- Followed the plan's shape: token 2 and 30 fields rather than a compatibility field, so a stale runner against the new `library.sh` is malformed, not mismatched.
- Kept the public `COGMASTER_FAULT_KIND_ROM_WRITE` constant and both name tables whole; only the diagnose vocabulary was pruned.
- Left the First fault table in `docs/runner.md` alone: its `kind` and `dev` names come from the kept name tables and are not part of the diagnose line.
- Left the TUNE-15 checkbox unticked and its traceability row `Pending`, as the plan says (the verifier closes it).
- `.planning/STATE.md` carries no hand-written Runtime Log row from this plan (the plan assigns that row to plans 03 and 04); only the position, progress and metrics handlers ran.

## Deviations from Plan

None - plan executed exactly as written.

The plan's acceptance wrote every criterion as `HEAD=1 EXPECT==0` or similar; all were run as written and pass. The Task 2 test-first step was split into a `test(17-01)` RED commit and a `refactor(17-01)` GREEN commit, which the task's `tdd="true"` flow calls for; the plan counted tasks, not commits.

## Issues Encountered

None.

## Known Stubs

None.

## Threat Flags

None. No new network endpoint, auth path or file access; the reducer's closed grammar only shrank (T-17-01), and the token and field-count move is the version-skew mitigation (T-17-02).

## For the owner: TUNE-15 wording amended

TUNE-15 in `.planning/REQUIREMENTS.md` no longer lists `vdpmode` among the removed words, because the blocked-mode fault in `src/vdp_render.c` (`cogmaster__bus_fault` with `COGMASTER_FAULT_KIND_UNSUPPORTED_VIDEO_MODE`) still raises it for the text, multicolour, mixed, invalid and native Game Gear modes. The requirement now says why the word stays, so the requirement closed is the one delivered. The checkbox stays unticked until the phase verifier closes it.

## Next Phase Readiness

- Plan 02 (wave 2) runs after this plan on the same `build/ci` tree; it can start from this branch head.
- Plan 03 pushes once for plans 01 and 02, then asks the owner for the judge and diagnose runs; the diagnose counts will come from the `diagnose 2` shape.

## Self-Check: PASSED

- Files found: `runner/diag.c`, `runner/diag_class.h`, `scripts/library.sh`, `tests/runner/diagnose.cmake`, `tests/runner/test_runner.c`, `tests/library/diagnose.cmake`, `docs/runner.md`, `docs/conformance.md`, `.planning/REQUIREMENTS.md`.
- Commits found as ancestors of HEAD: `8f4fa1e`, `96ec766`, `8750060`; the evaluation-scope resolver lists all three for plan 17-01.

---
*Phase: 17-re-run-merge-and-clean-state*
*Completed: 2026-11-14*
