---
phase: 04-sound
plan: "10"
subsystem: sound-fm-port
tags: [ym2413, opll, emu2413, integer-port, oracle, notices, provenance]

requires:
  - phase: 04-sound
    provides: the boot_to_play sound rows and the pinned audio hashes (plan 04-09)
provides:
  - an integer-only, globals-free YM2413 module (src/opll.c, src/opll_reg.c, src/opll.h, generated src/opll_tables.inc) that equals the unmodified emu2413 sample for sample and channel for channel over a seeded register stream
  - a one-shot oracle (tests/audio/opll_oracle.c, modes tables, stream and compare) and the shared seeded register stream (tests/audio/opll_stream.h)
  - the emu2413 notice, pins and provenance rows, and a policy.notices check that now demands a notices section for every file with an SPDX line
affects: [04-11, 04-12, 04-13, 04-14, 04-15]

actuals:
  tokens: 18600
  tasks: 2
  commits: 3
plan_head_before: 9a11f3e488a86100a03ed5ca047bd4ad90069d45
plan_head_after: 16537d7213893b2c845bded8fc7ef17258526e20

tech-stack:
  added: []
  patterns:
    - "A permissive seed is ported, then proved by a one-shot oracle that links the unmodified original and the port in one program and compares every sample and every channel output"
    - "Generated constant tables are split into groups behind a guard macro so each source file includes only the tables it uses, and no static const is unused under warnings-as-errors"
    - "Slot partners and patch links are indices derived from the slot number, never stored pointers or stored types, so a hostile state cannot reach outside the slot array"

key-files:
  created:
    - src/opll.h
    - src/opll.c
    - src/opll_reg.c
    - src/opll_tables.inc
    - tests/audio/opll_oracle.c
    - tests/audio/opll_stream.h
  modified:
    - CMakeLists.txt
    - pins.txt
    - THIRD-PARTY-NOTICES.md
    - PROVENANCE.md
    - tests/cmake/repository.sh

key-decisions:
  - "The measured logic-line count of the port is 699 non-blank, non-comment lines (705 at the first measurement, before the analyzer fix), against the trigger of 1,100; src/opll.c is 424 lines and src/opll_reg.c 284, so the D-15 replacement path (ares, then ymfm) is not taken"
  - "cogmaster__opll_sample returns the exact int32 sum of the 14 channel outputs where the original wraps the sum into an int16; the compare checks each channel as well as the sum, and the bound is left to the sum_bound case of plan 04-11"
  - "The amplitude LFO phase is kept below 13,440 (210 steps of 64) instead of growing without bound, which gives the original's table index for every reachable phase and removes its signed overflow after about 675 seconds of a held test bit"
  - "Slot links are indices: the wave table is a ws flag (0 full sine, 1 half sine), the instrument is a patch index, and the partner slot is slot i ^ 1 for the two non-rhythm types; the original's slot number and address latch fields are dropped (the address latch belongs to the port decode in plan 04-12)"
  - "Right shifts of values that can be negative all go through opll_floor_shift (the feedback input, the carrier modulation and the tone channel inversion); the original's left shift of a negative value in its exponential lookup is written as a multiplication by 2"

patterns-established:
  - "The oracle's tables mode is the only writer of src/opll_tables.inc, and it exits 1 if the integer key scale table differs from the original's floating point table"

requirements-completed: [SND-02]

coverage:
  - id: D1
    description: "The ported chip's samples and 14 channel outputs equal the unmodified emu2413 at the pinned commit over the seeded register stream (clock exactly 72 times the rate); run for 100,000 samples as the plan's gate and for 2,000,000 samples"
    requirement: SND-02
    verification:
      - kind: other
        ref: "build/opll-oracle compare 100000: exit 0; build/opll-oracle compare 2000000: exit 0"
        status: pass
    human_judgment: false
  - id: D2
    description: "The key scale table is produced by the integer formula ((d >> (3 - level)) * 8) / 3 from the eighths table and equals the original's floating point table entry by entry; the generated src/opll_tables.inc is reproduced identically by the tables mode"
    requirement: SND-02
    verification:
      - kind: other
        ref: "build/opll-oracle tables: exit 0, output identical to src/opll_tables.inc"
        status: pass
    human_judgment: false
  - id: D3
    description: "The port measures 699 logic lines (at most 1,100) with each .c file under 800 lines, holds no writable data symbol, no undeclared external symbol, no floating point and no undefined symbol beyond the allowed set"
    requirement: SND-02
    verification:
      - kind: other
        ref: "cat src/opll.c src/opll_reg.c src/opll.h | grep -Ev blank | grep -Ev comment | awk END: exit 0, prints 699"
        status: pass
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^(abi\\.(globals|undefined|symbols)|policy\\.notices|core\\.)': 35 of 35 passed, exit 0"
        status: pass
      - kind: other
        ref: "cc -std=c17 -O2 -Isrc -mgeneral-regs-only -c src/opll.c and src/opll_reg.c: exit 0"
        status: pass
    human_judgment: false
  - id: D4
    description: "The port runs clean under UndefinedBehaviorSanitizer and AddressSanitizer against the original over 500,000 samples, so every floor-shift path is defined"
    requirement: SND-02
    verification:
      - kind: other
        ref: "oracle built with the two port objects under -fsanitize=undefined,address: compare 500000 exit 0, no report from src/opll*.c"
        status: pass
    human_judgment: false
  - id: D5
    description: "THIRD-PARTY-NOTICES.md carries the emu2413 section (paths, upstream, commit, SHA-256 of emu2413.c, changes, MIT, LICENSE verbatim with both copyright years), and policy.notices fails when a file with an SPDX line has no notices section"
    requirement: SND-02
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^policy\\.notices$': passed"
        status: pass
      - kind: other
        ref: "negative run: cmake -DROOT=<scratch git repository without the emu2413 section> -DCHECK=notices -P tests/cmake/repository.cmake: exit 1, output names src/opll.c, src/opll.h and src/opll_reg.c; the same scratch with the section present: exit 0"
        status: pass
    human_judgment: false
  - id: D6
    description: "The emu2413 pins (emu2413-c, emu2413-h, emu2413-license) fetch and verify with scripts/fetch-pin.sh, the commit answers 200 from the commits API, and PROVENANCE.md has the section 2, section 4 and section 10 rows"
    requirement: SND-02
    verification:
      - kind: other
        ref: "gh api repos/digital-sound-antiques/emu2413/commits/a1d5120903b75f93d3eb7690e34a29c0b4add903 --jq .sha: exit 0; scripts/fetch-pin.sh for the three rows: exit 0; grep -c emu2413 PROVENANCE.md prints 3"
        status: pass
    human_judgment: false
  - id: D7
    description: "The full ci workflow (configure, build, 370 tests, package, release tests) and the hygiene workflow (clang-format 21, hygiene.no_float, the analyzer) exit 0 with the port in the tree"
    requirement: SND-02
    verification:
      - kind: integration
        ref: "cmake --workflow --preset ci: exit 0, 370 of 370 and 4 of 4 passed; cmake --workflow --preset hygiene: exit 0, 38 of 38 passed"
        status: pass
    human_judgment: false

duration: 14min
completed: 2026-11-09
status: complete
---

# Phase 04 Plan 10: YM2413 Port of emu2413 Summary

**An integer-only, globals-free YM2413 (699 logic lines in src/opll.c, src/opll_reg.c and src/opll.h) ported from MIT emu2413 and proved equal to the unmodified original over 2,000,000 seeded register-stream samples, with its notice, provenance and a tightened notices check landing in the same plan.**

## Performance

- **Duration:** 14 min
- **Started:** 2026-11-09T21:05:00Z (approximate; the start time was not captured)
- **Completed:** 2026-11-09T21:19:00Z
- **Tasks:** 2
- **Files modified:** 11 (6 created, 5 modified)

## Accomplishments

- The size trigger of D-15 was measured before any wiring: 699 logic lines against the 1,100 limit (705 at the first measurement), with src/opll.c at 424 lines and src/opll_reg.c at 284. The port stands; ares and ymfm are not needed.
- The oracle compiles the unmodified emu2413 (fetched at the pin into build/pins/emu2413, SHA-256 verified) and the port in one program. At a clock of exactly 72 times the rate each original call runs one update, and `compare` found no difference in sample or in any of the 14 channel outputs over 2,000,000 samples.
- The only floating point in the original's path, the key scale table, is now an integer formula over an eighths table; the `tables` mode compares it entry by entry with the float-built table (exit 0) and writes the whole src/opll_tables.inc.
- Every right shift that can see a negative value is one named helper; the port is clean under UndefinedBehaviorSanitizer and AddressSanitizer against the original, and compiles with `-mgeneral-regs-only` (no floating point instruction).
- The emu2413 section of THIRD-PARTY-NOTICES.md (LICENSE verbatim, both copyright years), the three pins, the PROVENANCE rows, and a policy.notices that now fails for any SPDX-marked file without a section: the negative run exits 1 naming src/opll.c.

## Measured values and commands

Every command ran from the repository root.

| Check | Command | Result |
|---|---|---|
| Logic lines | `cat src/opll.c src/opll_reg.c src/opll.h \| grep -Ev '^[[:space:]]*$' \| grep -Ev '^[[:space:]]*(/[*]\|[*]\|//)' \| awk 'END { print NR; exit (NR == 0 \|\| NR > 1100) }'` | prints 699, exit 0 |
| File sizes | `wc -l src/opll.c src/opll_reg.c` | 424 and 284 lines (limit 800) |
| Oracle, plan gate | `cc -std=c17 -O2 -Isrc -Itests/audio -Ibuild/pins/emu2413 tests/audio/opll_oracle.c src/opll.c src/opll_reg.c -lm -o build/opll-oracle && build/opll-oracle compare 100000` | exit 0 |
| Oracle, longer | `build/opll-oracle compare 2000000` | exit 0 |
| Tables | `build/opll-oracle tables` | exit 0; output identical to src/opll_tables.inc |
| Stream digest (for plan 04-11) | `build/opll-oracle stream 2000000` | exit 0; `samples 2000000 seed 0x4f504c4c32343133 sha256 eb45e9b6cf41fcd554277ea1d7b3db6a3001f6853080c4c2c02b187c689e4ec9` |
| Notice negative run | `cmake -DROOT=<scratch repository without the emu2413 section> -DCHECK=notices -P tests/cmake/repository.cmake` | exit 1, names src/opll.c, src/opll.h, src/opll_reg.c; the control with the section: exit 0 |
| Task 1 verify | `cmake --preset ci && cmake --build --preset ci && ctest --test-dir build/ci --no-tests=error --output-on-failure -R '^(abi\.(globals\|undefined\|symbols)\|policy\.notices\|core\.)'` | exit 0, 35 of 35 |
| Task 2 verify | `ctest --test-dir build/ci --no-tests=error --output-on-failure -R '^(policy\.\|docs\.budgets\|release\.)'` | all but release.archive_contents passed on the first run; that test compares the packaged notices file with the tree and needs the package step, so it passed in the ci workflow below |
| Full workflows | `cmake --workflow --preset ci` and `cmake --workflow --preset hygiene` | both exit 0 (370 of 370 plus 4 of 4; 38 of 38) |

## Task Commits

1. **Task 1: Measure, port, and match the original on a seeded stream, with the notice** - `5d0b69f` (feat)
2. **Task 1 follow-up: analyzer finding on the slot partner** - `289c419` (fix)
3. **Task 2: Provenance rows and the enforced notice check** - `16537d7` (docs)

**Plan metadata:** the commit that adds this summary, then the state and roadmap commit.

## Files Created/Modified

- `src/opll.h` - chip, slot and patch structures, update flags, slot helpers and the three entry points
- `src/opll.c` - the per-sample path: LFOs, noise, envelope and phase generators of 18 slots, the channel outputs and the sum, with `opll_floor_shift`
- `src/opll_reg.c` - register writes with mirrors, key status, rhythm mode switch, patch handling, power-on reset
- `src/opll_tables.inc` - generated constant tables in two guarded groups (waves, voices)
- `tests/audio/opll_oracle.c` - one-shot oracle: tables, stream, compare
- `tests/audio/opll_stream.h` - seeded splitmix64 register stream shared with the lockstep test of plan 04-11
- `CMakeLists.txt` - the two new sources in the library
- `pins.txt` - emu2413-c, emu2413-h, emu2413-license at the pinned commit
- `THIRD-PARTY-NOTICES.md`, `PROVENANCE.md`, `tests/cmake/repository.sh` - notice, provenance rows, and the notices check

## Decisions Made

- The port returns the exact int32 sum of the 14 channels; the original wraps its sum into an int16. The two are equal over the whole 2,000,000-sample stream. The two rhythm voice groups keep their last values while inactive (as in the original), so a theoretical worst case of 38,807 exists if every slot peaked together; plan 04-11's sum_bound case owns that bound.
- The amplitude LFO phase wraps at 13,440 instead of counting up: the table index is the same for every phase the original can reach and the signed overflow after about 675 seconds with the test bit held is gone.
- Instrument and wave-table links are a patch index and a `ws` flag (state-friendly, no pointers), the slot's number and the chip's address latch are dropped, and slot fields use the narrowest unsigned type that holds their range, so the state work of plan 04-12 can bound every field.
- The generated tables are two guarded groups; `opll.c` defines `OPLL_WANT_WAVES` and `opll_reg.c` defines `OPLL_WANT_VOICES` before including the file, so neither sees an unused `static const` under warnings-as-errors.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] The slot partner was found through a stored type field**
- **Found during:** Task 2 (the hygiene workflow's analyzer)
- **Issue:** `buddy = &o->slot[i - 1u]` ran for any slot whose type read 1, so a slot 0 with type 1 (impossible from the registers, possible from a hostile saved state) reached slot index 4294967295. `clang-analyzer-security.ArrayBound` failed `hygiene.analyzer`.
- **Fix:** the partner is `&o->slot[i ^ 1u]` for the two non-rhythm types and NULL otherwise; the same slot for every reachable state, in range for any value.
- **Files modified:** src/opll.c
- **Verification:** `compare 2000000` exit 0 again; hygiene workflow exit 0; ci workflow exit 0
- **Committed in:** 289c419

### Notes on the plan text

- The nofp lane builds with GCC on Linux and cannot run here; the port was checked locally with `clang -mgeneral-regs-only` (arm64), which refuses any floating point instruction. The hosted lane is the second check.
- The first run of the Task 2 verify showed release.archive_contents failing because the package step had not been re-run after the notices file changed; it is part of the ci workflow and passed there.

**Total deviations:** 1 auto-fixed (1 bug, found by the analyzer)
**Impact on plan:** none on scope; the fix removed a hostile-state hazard before any state code exists.

## Issues Encountered

None beyond the deviation above. The unmodified original has its own undefined behavior (a left shift of a negative value in its exponential lookup), which is why the sanitizer check builds only the port objects with sanitizers.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- Plan 04-11 can write the lockstep test against `cogmaster__opll_reset`, `cogmaster__opll_write` and `cogmaster__opll_sample` and the shared `opll_stream.h`, freezing the digest printed above (or the one its own run records) next to the SHA-256 of emu2413.c. `slot_evals` already counts 18 per sample.
- The state plan (04-12) can serialise the structures field by field: no pointers remain, and `am_phase` is bounded by 13,440.
- No stubs, no skipped tests and no unrun verify commands.

## Self-Check: PASSED

All six created files exist, commits 5d0b69f, 289c419 and 16537d7 are ancestors of HEAD, `grep -q '^## emu2413' THIRD-PARTY-NOTICES.md` exits 0 and `grep -c emu2413 PROVENANCE.md` prints 3.

---
*Phase: 04-sound*
*Completed: 2026-11-09*
