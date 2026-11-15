---
phase: 16-graphics-i-and-ii-draw
plan: 06
subsystem: state
tags:
  - state-format
  - behaviour-revision
  - legacy-sprites
  - loader-bounds
  - record-sweep

requires:
  - phase: 16-graphics-i-and-ii-draw
    provides: the Graphics I and II renderer and the mode class of plan 04, the legacy_modes images and the base hashes file of plans 02 and 05
provides:
  - struct cogmaster__vdp with slots of x, tile, row and attr and sprite_legacy, sprite_size and fifth_index appended after vram, size pin 16520
  - behaviour revision 19 and state format 17, so every older state is refused by the fixed header fields
  - a status read that returns the latched fifth sprite index in bits 4 to 0 while bit 6 is set and resets it to 31
  - class-dependent sprite bounds in the VDP record, keyed on the class of the last scan and applied to the selected slots
  - core.state.legacy_roundtrip and core.state.legacy_sprite_bounds in both inventories
affects:
  - 16-07
  - 16-08
  - 16-09

actuals:
  tokens: 5251
  tasks: 2
  commits: 4
plan_head_before: 6e7104e4eeebbbaa5c3d79bf49fda8888dc21cd6
plan_head_after: f5aac66c50e4dc78679fefb033d7175ea6f04b05

tech-stack:
  added: []
  patterns:
    - "a saved field set with two meanings is bounded by the union in the field and narrowed by a check on the class the last scan ran in, read from the record itself"
    - "slot bounds apply to the selected slots only, because a slot past the count keeps what a scan of the other class left and nothing reads it"
    - "a round trip compares the loaded instance's fields as well as the saved bytes, because a field missing from both states makes equal bytes"

key-files:
  created: []
  modified:
    - src/internal.h
    - src/vdp.c
    - src/state.c
    - include/cogmaster.h
    - tests/api/v14.c
    - tests/bios/test_bios.c
    - tests/core/test_state.c
    - tests/core/test_core.c
    - tests/core/tests.cmake
    - tests/inventory/ci.txt
    - tests/inventory/wasm32.txt

key-decisions:
  - "The loader bounds rows and colour bytes for the slots below sprite_count only: a legacy scan followed by a mode 4 scan leaves legacy colour bytes and 16-row values in the slots past the new count, nothing reads them, and checking all eight would refuse a state the core can produce"
  - "The bounds follow sprite_legacy, the class at scan time, and not the live mode, so a save taken after a mid-line mode switch loads; sprite_size is bounded 0 to 1 in both classes and sprite_legacy 1 is not refused on the native Game Gear profile, which the plan did not ask for"
  - "A latched fifth index with status bit 6 clear is refused in both classes: the status read clears the bit and the index together and the scan sets them together, so the pair cannot occur"
  - "The status read test lives in the round trip helper (equal bytes on the source and the loaded instance, the saved index in bits 4 to 0, a reset to 31, 0x1F on the second read), so the behaviour change has a ci test now; plan 07 adds the engine side"
  - "core.state.legacy_sprite_bounds runs one whole record sweep over a legacy state and takes the 60 s tier of the other sweep cases, provisional, 1.2 s locally"

requirements-completed: []

coverage:
  - id: D1
    description: "The sprite slots widen to x, tile, row and attr and sprite_legacy, sprite_size and fifth_index follow vram in the VDP record; the pinned size is 16520"
    requirement: MODE-05
    verification:
      - kind: unit
        ref: "cmake --build --preset ci exit 0 with the 16520u size assert in src/state.c"
        status: pass
      - kind: unit
        ref: "ctest --preset ci -R core.state.legacy_roundtrip exit 0"
        status: pass
    human_judgment: false
  - id: D2
    description: "Behaviour revision 19 and state format 17 are in the header, tests/api/v14.c expects 17, and the fixed header fields refuse every older state"
    requirement: MODE-05
    verification:
      - kind: unit
        ref: "ctest --preset ci -R abi.api_v14 and core.info exit 0"
        status: pass
      - kind: unit
        ref: "ctest --preset ci -R core.state.record_sweep exit 0; the sweep sets behaviour_revision and format_version one step outside their bounds and every set is refused"
        status: pass
    human_judgment: false
  - id: D3
    description: "A status read returns the flags with the latched fifth sprite index in bits 4 to 0 while bit 6 is set and 31 otherwise, then resets the index; in mode 4 the index is always 31, so its byte is unchanged"
    requirement: MODE-05
    verification:
      - kind: unit
        ref: "tests/core/test_state.c#legacy_round_trip through ctest --preset ci -R core.state.legacy_roundtrip exit 0; mutations that dropped the index, the reset or the bit 6 gate failed the case"
        status: pass
      - kind: other
        ref: "test -z of LC_ALL=C comm -23 of the sorted build/p16-base-hashes.txt and the sorted build/ci/hashes.txt exit 0 (173 base lines, 192 head lines) after runner.hashes_file"
        status: pass
    human_judgment: false
  - id: D4
    description: "A legacy-mode state round trip is byte-equal, with no sprite selected and no overflow (the empty edge), with four 16x16 slots and a fifth match, and with a latched fifth sprite, and the loaded instance holds every sprite field"
    requirement: MODE-05
    verification:
      - kind: unit
        ref: "ctest --preset ci -R core.state.legacy_roundtrip exit 0; removing the visit of sprite_attr, vdp_fifth_index or sprite_legacy from vdp_io failed the case"
        status: pass
    human_judgment: false
  - id: D5
    description: "The loader refuses a legacy slot set that breaks the class bounds (row 8 with 8x8, five slots, a colour byte with bits 6 to 4 set, an overflow without four slots and five reads, 33 reads, a latched index with bit 6 clear, the field edges) with the instance unchanged and loads the neighbour of each bound, including the mode 4 bounds and a slot past the count"
    requirement: MODE-05
    verification:
      - kind: unit
        ref: "ctest --preset ci -R core.state.legacy_sprite_bounds exit 0; 20 refused and 12 accepted sub-cases; 11 mutations of the loader checks each failed a named sub-case"
        status: pass
    human_judgment: false
  - id: D6
    description: "Every new visitor field name is classified in eq_excluded and EQ_EXCLUDED_NAMES rises from 69 to 73, so bios.equivalence names no unclassified field; the record sweep names the four fields and runs over a legacy state"
    requirement: MODE-05
    verification:
      - kind: integration
        ref: "ctest --preset ci -R bios.equivalence exit 0 (three tests)"
        status: pass
      - kind: unit
        ref: "ctest --preset ci -R core.state.record_sweep and core.state.legacy_sprite_bounds exit 0"
        status: pass
    human_judgment: false
  - id: D7
    description: "The whole ci lane, the budgets, the format dry run and the hygiene lane (hygiene.analyzer and hygiene.no_float included) pass on the final commit"
    requirement: MODE-05
    verification:
      - kind: integration
        ref: "cmake --workflow --preset ci exit 0 at f5aac66 with 720 of 720 tests and 5 of 5 release tests"
        status: pass
      - kind: other
        ref: "cmake -DROOT with tests/cmake/budgets.cmake exit 0 (887 check lines, 9605 src lines); clang-format 21 dry run over git ls-files exit 0; cmake --workflow --preset hygiene exit 0, 84 of 84"
        status: pass
    human_judgment: false

duration: 13 min
completed: 2026-11-14
status: complete
---

# Phase 16 Plan 06: Format 17 and the legacy sprite fields Summary

**The VDP record now saves the legacy sprite fields (widened slots with a colour byte, sprite class, sprite size and the latched fifth sprite index) under state format 17 and behaviour revision 19, with class-dependent loader bounds and a byte-equal Graphics I round trip, before any sprite engine writes them**

## Performance

- **Duration:** 13 min
- **Started:** 2026-11-14T04:52:12Z
- **Completed:** 2026-11-14T05:06:00Z
- **Tasks:** 2 (the first a tracer), plus two follow-up commits
- **Files changed:** 11 (379 insertions, 29 deletions)

## Accomplishments

- **The record.** `struct cogmaster__vdp` holds `sprite[8]` as `{x, tile, row, attr}` and appends `sprite_legacy`, `sprite_size` and `fifth_index` after `vram`, so the colour and video RAM offsets of a state do not move relative to each other and the visit order of every existing field is unchanged. The pin in `src/state.c` is 16520, the number the research computed; the compiler agreed on the first build. `sprite_overflow` is documented where it is declared and where it is bounded: a 0 or 1 flag in mode 4, 0 or the fifth match's SAT index plus 1 (1 to 32) in Graphics I and II.
- **The status read.** It returns `(status & 0xE0)` with the latched index in bits 4 to 0 while bit 6 is set and `0x1F` otherwise, then resets the index to 31 (`src/vdp.c`). Nothing writes an index other than 31 yet, so mode 4 and every shipped pin are unchanged; the subset compare of the base and head hashes files printed nothing.
- **The loader.** Fields take the union of both classes (count 0 to 8, row 0 to 15, colour byte 0 to 0x8F, overflow 0 to 32, scanned 0 to 64); after `sprite_legacy`, `sprite_size` and `vdp_fifth_index` are read, class checks narrow them. Graphics I and II: count at most 4, rows at most 7 (15 with 16x16), no colour bits 6 to 4, at most 32 SAT reads, an overflow needs four slots and five reads. Mode 4: eight slots, rows at most 7, colour byte 0, overflow 0 or 1, the existing nine-read rule. Both: a latched index needs status bit 6.
- **Revision 19, format 17.** `include/cogmaster.h`, `tests/api/v14.c` (two literals) and a third literal in `tests/core/test_core.c` that the plan's list did not name (deviation 1).
- **Classified fields.** `sprite_attr`, `sprite_legacy` and `sprite_size` (`why_out`) and `vdp_fifth_index` (`why_beam`) joined `eq_excluded[]`; `EQ_EXCLUDED_NAMES` is 73 and `bios.equivalence` passes.
- **Tests.** `core.state.legacy_roundtrip` saves a Graphics I instance to mid-line three ways (no sprite selected and no overflow, four 16x16 slots with a fifth match, a latched fifth sprite), loads each into a fresh instance, requires equal bytes, compares the sprite fields of both instances, reads the status of both and runs two more frames. `core.state.legacy_sprite_bounds` has 15 refused legacy sub-cases, 9 accepted neighbours, 5 refused and 3 accepted mode 4 sub-cases and a whole record sweep over a legacy state (170907 fields recorded, 362 out-of-range values refused). The record sweep's name list holds the four new fields.

## Task Commits

1. **Task 1: Tracer, a legacy-mode state saves and reloads byte-equal under format 17 and revision 19** - `fb84cac` (feat)
2. **Task 2: The loader bounds test and the record sweep** - `779911a` (test)
3. **Version pin literal in core.info (deviation 1)** - `0c5b9c4` (fix)
4. **Status read assertions in the round trip (deviation 2)** - `f5aac66` (test)

**Plan metadata:** the SUMMARY commit and the state and roadmap commit that follow this file, both local; no push (the next scheduled push is plan 08's).

The tracer gate was met by re-running the Task 1 verify chain (configure, build, the targeted ctest, `runner.hashes_file`, the subset compare) before expanding: exit 0, 134 of 134 and 1 of 1.

## Files Created/Modified

- `src/internal.h` - the widened slot, the three appended fields, the two meanings of `sprite_overflow`
- `src/vdp.c` - reset values, the status read with the latched index and its reset
- `src/state.c` - the 16520 pin, the visitor names, the class-dependent bounds, the Layout paragraph
- `include/cogmaster.h` - revision 19 and format 17
- `tests/api/v14.c`, `tests/core/test_core.c` - the version literals
- `tests/bios/test_bios.c` - four classified names, `EQ_EXCLUDED_NAMES` 73
- `tests/core/test_state.c` - the two cases, their helpers and the named-field list
- `tests/core/tests.cmake`, `tests/inventory/ci.txt`, `tests/inventory/wasm32.txt` - registration, the `# Fault prevented:` comment, the 60 s tier, the inventory lines

## Decisions Made

- **Selected slots only.** Rows and colour bytes are bounded for the slots below `sprite_count`. A legacy scan stores slots 0 to 3 and a later mode 4 scan rewrites only the slots it fills, so slots past the count can keep a legacy colour byte or a row of 15, and no render reads them. Checking all eight would refuse a state the core can produce; a mutation that checks all eight fails the "slot past the count" sub-cases of both classes.
- **Class from the record, not the mode.** `sprite_legacy` is the class at scan time, so a save after a mid-line switch loads (D-18). `sprite_legacy` 1 is not refused on the native Game Gear profile, which has no legacy class; the plan did not ask for it and the fields are bounded either way.
- **The latch and bit 6 travel together.** The scan sets both, the read clears both, so bit 6 clear with an index other than 31 is refused in both classes (D-19). A mutation of the read that ignores bit 6 is equivalent on every reachable state and cannot be told apart by a test.
- **The status read has a test now, not only in plan 07.** Rule 5 asks for a test with the behaviour change; the round trip reads the status of the source and the loaded instance and checks the saved index.
- **TDD note for Task 2.** The checks it tests were written in Task 1, as the plan orders them, so the sub-cases passed on their first run. The test is shown to bite by mutation: 11 loader mutations (the row bound ignoring the size, a legacy count bound of 8, the colour bits not checked in either class, the overflow bound of 32 in mode 4, 64 SAT reads in legacy, an overflow without a fifth read, an overflow with fewer than four slots, the bit 6 gate, a mode 4 scanned bound of 32, all eight slots checked) each failed a named sub-case, and removing the visit of `sprite_attr`, `vdp_fifth_index` or `sprite_legacy` failed the round trip. One sub-case ("five selected slots") was refused for a second reason in the first draft; it now clears the overflow so it isolates the count bound.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] core.info held the literal 18 beside the header macro**
- **Found during:** the full `cmake --workflow --preset ci` run after Task 2 (core.info failed, 719 of 720)
- **Issue:** `tests/core/test_core.c` line 437 pins `v.behaviour` to 18 as a literal; the plan's grep lists lines that name the macros, which hides a literal on the next line, and the targeted ctest selections of both tasks do not select `core.info`.
- **Fix:** the literal is 19 and the comment names what revision 19 is.
- **Files modified:** `tests/core/test_core.c`
- **Verification:** `ctest --preset ci -R '^core\.(info|state)'` 8 of 8; the full workflow exit 0, 720 of 720
- **Committed in:** `0c5b9c4`

**2. [Rule 2 - Missing test] The status read change had no test of its own**
- **Found during:** final review of the plan against CLAUDE.md rule 5
- **Issue:** the plan's tests cover the saved fields and the loader; the one behaviour change in `src/vdp.c` (the low bits of the status read) is reached only once the engine of plan 07 writes an index, and its case `status_low` lands there.
- **Fix:** `legacy_round_trip` reads the status of the source and the loaded instance, requires the saved index in bits 4 to 0 (31 when bit 6 is clear), the reset and `0x1F` on the second read; two mutations of the read failed it.
- **Files modified:** `tests/core/test_state.c`
- **Verification:** `ctest --preset ci -R core.state.legacy_roundtrip` exit 0
- **Committed in:** `f5aac66`

### Plan variants

- **The slot bounds apply to the selected slots**, not to all eight (the plan's action does not say which); see Decisions Made.
- **The record sweep.** The plan allowed adding nothing if the sweep already walks every visited name. It does, so the change is the four names in the sweep's required list and one sweep run over a legacy state inside `core.state.legacy_sprite_bounds`, which moved that case to the 60 s timeout tier.

---

**Total deviations:** 2 auto-fixed (1 bug, 1 missing test)
**Impact on plan:** no scope widening beyond one extra test file line of context (`tests/core/test_core.c`); every acceptance count is met (16520u 1, 16510u 0, revision and format 1 each, `EQ_EXCLUDED_NAMES 73u` 1, `vdp_fifth_index` 1, the two meanings of `sprite_overflow` in `src/internal.h` and `src/state.c`, 3 `Fault prevented` comments, 2 `core.state.legacy_` lines in each inventory).

## Issues Encountered

- `clang-format` is not on the PATH of this shell; the dry runs used `/opt/homebrew/opt/llvm@21/bin/clang-format` (version 21), exit 0 over every tracked `.c` and `.h` file.
- The first mutation runs of Task 2 compiled with an unused-variable error and so ran the previous binary; each mutation was then written so the variable stays used, and the run script checks the build status before reading a verdict.

## Evidence

Commands from the repository root, exit status 0, no push so no run URL:

- `cmake --preset ci`, `cmake --build --preset ci` (warnings are errors; the 16520 assert held on the first build)
- `ctest --preset ci -R '^(vdp\.|core\.state|abi\.|api\.|bios\.|ports\.gun)' -j8` (134 of 134), `ctest --preset ci -R '^(core\.state|bios\.|api\.)' -j8` (15 of 15), `ctest --preset ci -R '^core\.(info|state)' -j8` (8 of 8)
- `rm build/ci/hashes.txt`, `ctest --preset ci -R '^runner\.hashes_file$'`, then `test -z "$(LC_ALL=C comm -23 <(sort build/p16-base-hashes.txt) <(sort build/ci/hashes.txt))"` (exit 0; 173 base lines, 192 head lines, the same as plan 05)
- `cmake --workflow --preset ci` at `f5aac66`: 720 of 720 tests and 5 of 5 release tests
- `cmake --workflow --preset hygiene`: 84 of 84 (`hygiene.analyzer` 13.8 s, `hygiene.no_float` and its self test pass); `cmake -DROOT="$PWD" -P tests/cmake/budgets.cmake` (887 check lines, 9605 src lines); clang-format dry run over `git ls-files '*.c' '*.h'`
- acceptance counts from the task blocks, all as expected

## Known Stubs

None. The slot attribute, the three fields and the bounds are complete; nothing writes a legacy colour byte, a class of 1 or an index other than 31 until plan 07's scan.

## Threat Flags

None. The loader only narrows what a state may hold; no endpoint, file access or trust boundary was added.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- **Plan 07** writes the fields: the legacy scan stores `attr = colour byte & 0x8F`, `sprite_legacy = 1`, `sprite_size`, and `1 + index` in `sprite_overflow` with count 4 and at least five SAT reads; the mode 4 scan must store `attr = 0` and `sprite_legacy = 0` for the slots it fills (slots past the count are not bounded and need no clearing). Its dot-0 rule sets `fifth_index` and status bit 6 together, which is the pair the loader requires. The `core.state.legacy_roundtrip` sub-case it adds runs a scan with sprites to mid-line; the helpers `legacy_instance`, `set_legacy_slots` and `legacy_round_trip` are in `tests/core/test_state.c`.
- **Plan 09** owns the revision 19 and state format 17 sentence in `docs/embedding.md` (lines 59 to 61 carry the earlier revisions) and the `docs/hardware/video.md` text on the latched index; this plan changed no document.
- **Timeouts.** `core.state.legacy_sprite_bounds` has the provisional 60 s tier (1.2 s locally); the CI-speed seeds re-derive it from a hosted run.
- **Requirements.** `requirements-completed` is empty as the plan says: MODE-05 completes with plan 09's head evidence (the subset compare is shown here and in plan 05).

---
*Phase: 16-graphics-i-and-ii-draw*
*Completed: 2026-11-14*

## Self-Check: PASSED

The eleven changed files exist; commits fb84cac, 779911a, 0c5b9c4 and f5aac66 are ancestors of HEAD and `gsd_run check evaluation-scope --plan 16-06 --commits-only` lists them; the acceptance counts above were re-run on the final tree; `git diff --name-only 6e7104e..HEAD` lists only the eleven files; the base-versus-head subset compare printed nothing after the full workflow; the coverage block uses block style with no flow mappings and no inline comments.
