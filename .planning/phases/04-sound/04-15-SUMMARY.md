---
phase: 04-sound
plan: "15"
subsystem: testing
tags: [retroarch, afinfo, hosted-recording, ci, perf]

requires:
  - phase: 04-sound
    provides: psg_sweep fixtures, libretro audio sessions and the cogmaster_fm option (plans 04-08 and 04-14)
provides:
  - tests/retroarch/audio.cmake, a cmake -P checker that judges a RetroArch recording against a silent control
  - tests/retroarch/record.cfg, the three-line recorder config
  - hosted retroarch job steps for the control and the two psg_sweep sound sessions
  - a dated boot_to_play perf allow line measured from hosted runs
  - hosted evidence run on the pushed phase head with seven equal hashes.txt files
affects: [04-sound code review, 04-sound ship]

actuals:
  tokens: 5297
  tasks: 2
  commits: 4

plan_head_before: 4b1faee59faa9b4e68f6b439f3031fa42dd8b654
plan_head_after: 9aaf3780e54e13b166ca44e7a63903d02d358c79
commits: 4

tech-stack:
  added: []
  patterns:
    - "Hosted recording assertions live in one cmake -P checker under tests/retroarch, never in workflow YAML"
    - "A negative control runs the comparator through the shared tests/cmake/expect_fail.cmake"

key-files:
  created:
    - tests/retroarch/audio.cmake
    - tests/retroarch/record.cfg
  modified:
    - tests/retroarch/run_retroarch.cmake
    - tests/retroarch/bmp.c
    - tests/retroarch/compare.c
    - tests/libretro/tests.cmake
    - .github/workflows/ci.yml
    - tests/perf/allow.txt
    - tests/core/tests.cmake
    - src/opll.c
    - tests/audio/test_psg.c
    - tests/audio/test_synth.c
    - docs/testing.md
    - docs/audio.md
  deleted:
    - tests/retroarch/flip.cmake

key-decisions:
  - "The full D-32 checker fits: hosted code is 299 of 300 lines, so the minimum fallback was not needed and the length bounds stayed"
  - "The perf allow line is 357 basis points: dispatch runs measure only one commit, so the base figure came from a dispatch on main"
  - "The two whole-sweep core.state cases take a 60 s timeout after macOS x64 timed out at 10 s"

patterns-established:
  - "Free hosted lines first (shared helper, trims), then add the checker; never raise the 300-line cap"

requirements-completed: [SND-04, SND-03]

coverage:
  - id: D1
    description: "A real RetroArch 1.22.2 records psg_sweep.sms and psg_sweep.gg with at least 8192 non-zero audio bytes each, judged by audio.cmake inside the 300-line budget"
    requirement: SND-04
    verification:
      - kind: e2e
        ref: "cmake -DMOV=build/ra-audio/sms.mov -DMODE=sound -P tests/retroarch/audio.cmake and the same for gg.mov (local exit 0 each, 127484 non-zero of 192244 bytes)"
        status: pass
      - kind: e2e
        ref: "hosted run 42714958262 retroarch job steps: Master System and Game Gear sound recording is non-silent"
        status: pass
    human_judgment: false
  - id: D2
    description: "The silent control (no-content session) has exactly 0 non-zero audio bytes, so the non-silence check cannot pass on silence; the checker rejects the wrong mode, a missing file and an unknown mode"
    requirement: SND-04
    verification:
      - kind: e2e
        ref: "cmake -DMOV=build/ra-audio/control.mov -DMODE=control -P tests/retroarch/audio.cmake (local exit 0, 0 non-zero of 192244 bytes)"
        status: pass
      - kind: e2e
        ref: "hosted run 42714958262 retroarch job step: RetroArch silent control recording"
        status: pass
    human_judgment: false
  - id: D3
    description: "Hosted code stays at 299 of 300 lines with flip.cmake replaced by expect_fail.cmake and the pixel control still failing its flipped comparison"
    requirement: SND-04
    verification:
      - kind: unit
        ref: "ctest -R policy.e2e_budget and libretro.screenshot (exit 0; e2e_budget: 299 lines)"
        status: pass
      - kind: e2e
        ref: "hosted run 42714958262 retroarch job step: The compare's negative control"
        status: pass
    human_judgment: false
  - id: D4
    description: "The pushed phase head is green on every hosted lane, and hash-equality passes over seven hashes.txt files that include every audio row"
    requirement: SND-03
    verification:
      - kind: integration
        ref: "gh run view 42714958262 with the plan's jq predicate (exit 0; hash-equality, be32 and retroarch succeeded)"
        status: pass
    human_judgment: false
  - id: D5
    description: "A dated perf allow line for boot_to_play written from the measured 3.56 percent change, expiring 2027-01-07"
    requirement: SND-04
    verification:
      - kind: unit
        ref: "cmake -DROM=boot_to_play -DHEAD=12302841 -DBASE1=11879630 -DBASE2=11879630 -DALLOW=tests/perf/allow.txt -DMODE=gate -P tests/cmake/perf_compare.cmake (exit 0, verdict PASS; exit 1 FAIL with an empty allow file)"
        status: pass
    human_judgment: false

duration: 22min
completed: 2026-11-09
status: complete
---

# Phase 4 Plan 15: Hosted RetroArch sound recording Summary

**A real RetroArch 1.22.2 records psg_sweep on both consoles and a silent control, judged by a 19-line cmake -P checker inside the 300-line hosted budget, and the pushed phase head is green on every hosted lane with equal audio hashes.**

## Performance

- **Duration:** 22 min
- **Started:** 2026-11-09T22:29:00Z
- **Completed:** 2026-11-09T22:51:00Z
- **Tasks:** 2
- **Files modified:** 15 (one deleted, two created)

## Accomplishments

- `tests/retroarch/audio.cmake` fails unless the movie exists, `afinfo` exits 0, the audio length is 188,000 to 196,000 bytes, and the non-zero byte count (head, tail and tr under `LC_ALL=C`) is exactly 0 for the control and at least 8,192 for a sound session; it reads no log text and rejects an unknown mode.
- Lines were freed first: `flip.cmake` is replaced by the shared `expect_fail.cmake` (the unit test and three workflow steps), `run_retroarch.cmake`, `bmp.c` and `compare.c` are trimmed. `policy.e2e_budget` reports 299 of 300, so the full checker fits and the D-32 minimum fallback was not used.
- The retroarch job records the no-content session as the control and `psg_sweep.sms` and `.gg` (now in the `retroarch-owned-input` artifact) as sound sessions; the assertions stay in the checker.
- Hosted run 42714958262 on head 9aaf3780e54e13b166ca44e7a63903d02d358c79 succeeded with every lane, including `be32`, `hash-equality` and the four retroarch steps the plan's jq predicate names.

## Local RetroArch commands and exit statuses

Run from the repository root with `/Applications/RetroArch.app` 1.22.2 (PlistBuddy prints 1.22.2) and `afinfo` present. The driver command for each is `cmake -DRETROARCH=/Applications/RetroArch.app/Contents/MacOS/RetroArch -DCORE=build/ci/libretro/cogmaster_libretro.dylib -DINFO=build/ci/libretro/cogmaster_libretro.info -DTEMPLATE=tests/retroarch/test.cfg.in "-DRA_DIR=$PWD/build/ra-audio/<name>" -DFRAMES=60 "-DSHOT=$PWD/build/ra-audio/<name>.png" [-DCONTENT=...] "-DARGS=--record=$PWD/build/ra-audio/<name>.mov;--recordconfig=$PWD/tests/retroarch/record.cfg" -P tests/retroarch/run_retroarch.cmake`.

| Session | Driver exit | Checker command | Checker exit | Result |
| ------- | ----------- | --------------- | ------------ | ------ |
| control (no content) | 0 | `-DMOV=build/ra-audio/control.mov -DMODE=control` | 0 | 192,244 bytes, 0 non-zero |
| psg_sweep.sms | 0 | `-DMOV=build/ra-audio/sms.mov -DMODE=sound` | 0 | 192,244 bytes, 127,484 non-zero |
| psg_sweep.gg | 0 | `-DMOV=build/ra-audio/gg.mov -DMODE=sound` | 0 | 192,244 bytes, 127,484 non-zero |

Negative checks of the checker (each exit 1): control mode on the sms movie, sound mode on the control movie, sound mode on a missing file, an unknown mode.

Verification commands: `cmake --build --preset ci` then `ctest ... -R '^(policy\.e2e_budget|libretro\.screenshot\.)'` (exit 0, 7 tests); `cmake --workflow --preset ci` (exit 0) and `cmake --workflow --preset hygiene` (exit 0), both run before the first push.

## Hosted evidence

| Step | Command and result |
| ---- | ------------------ |
| First run on dc5f35c | https://github.com/szTheory/cogmaster/actions/runs/42713186331, `gh run watch --exit-status` exit 1: nofp, be32, asan, linux and windows build legs, macOS x64 (see Deviations) |
| Second run on 41b8f26 | https://github.com/szTheory/cogmaster/actions/runs/42713780658, exit 0, every job success |
| Base figure | dispatch on main https://github.com/szTheory/cogmaster/actions/runs/42714378451 (success): `perf control boot_to_play runs=11879630,11879630,11879630 spread=0` |
| Head figure | run 42713780658 perf job: `perf control boot_to_play runs=12302841,12302841,12302841 spread=0` |
| Final evidence | https://github.com/szTheory/cogmaster/actions/runs/42714958262, head SHA 9aaf3780e54e13b166ca44e7a63903d02d358c79, the plan's push, dispatch, watch (exit 0) and jq predicate (prints true, exit 0) |

Perf measurement: +423,211 instructions per frame on 11,879,630, 3.56 percent (356 basis points, floor as `perf_compare.cmake` computes it); the allow line is `boot_to_play 357 2027-01-07 synthesised sound runs every frame` (expiry 59 days out). `perf_compare.cmake` in gate mode on the two figures prints verdict PASS with the line and FAIL without it. Dispatch runs measure one commit only, so the base figure is main's own fixture rather than the head fixture a pull request would use for both builds; the job runs in report mode, and the pull request's own report will confirm.

The release rehearsal job was skipped on the dispatch (the classifier does not flag the edit on a dispatch); it runs in the pull request, where its failure is triaged like any other job.

## Task Commits

1. **Task 1: local recording, checker and freed lines** - `16b8f49` (test)
2. **Task 2: hosted steps, docs** - `dc5f35c` (ci)
3. **Task 2: failures the first hosted run of the phase head exposed** - `41b8f26` (fix)
4. **Task 2: measured perf allow line** - `9aaf378` (chore)

**Plan metadata:** the docs(04-15) commit that follows this file.

## Files Created/Modified

- `tests/retroarch/audio.cmake` - recording checker (exists, afinfo exit 0, length bounds, non-zero byte count, control and sound modes)
- `tests/retroarch/record.cfg` - format mov, acodec pcm_s16le, frame_drop_ratio 60
- `tests/retroarch/run_retroarch.cmake`, `bmp.c`, `compare.c` - trimmed to free lines (single required-variable guard, no closing status message, 40-byte BMP header only with dimensions bounded by file size, bare usage exit, one-line size message)
- `tests/retroarch/flip.cmake` - deleted; `tests/libretro/tests.cmake` and three workflow steps use `tests/cmake/expect_fail.cmake` with `--flip`, exit 1 and a regex for pixel 7,3
- `.github/workflows/ci.yml` - control record ARGS, control check step, two sound steps, psg_sweep images in the owned-input artifact
- `tests/perf/allow.txt` - one dated line
- `docs/testing.md`, `docs/audio.md` - hosted audio check described; testing.md is now 40,953 of 40,960 bytes, so the next edit there must trim first

## Decisions Made

- Hosted sound movies and screenshots go under `build/ra-audio`, so the owned-screenshots artifact (build/frames/*.png) stays the same set.
- The control writes its movie to `build/frames/control.mov` because the driver already creates that directory from the screenshot path.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] GCC sign-conversion error in the YM2413 attack step**
- **Found during:** Task 2 (first hosted run: nofp, be32, linux builds)
- **Issue:** `src/opll.c` computed the attack step in mixed int and unsigned arithmetic; GCC with `-Werror` refused it (clang did not)
- **Fix:** the same expression in `uint32_t` arithmetic; `fm.lockstep` and every fm and audio test still pass
- **Files modified:** src/opll.c
- **Committed in:** 41b8f26

**2. [Rule 1 - Bug] Out-of-bounds table read in psg.noise_controls under UBSan**
- **Found during:** Task 2 (asan job)
- **Issue:** `tests/audio/test_psg.c` evaluated `lfsr_after` before its shift bound, indexing the three-entry white-noise tail past its end
- **Fix:** test the bound first so the call is skipped; same value otherwise
- **Files modified:** tests/audio/test_psg.c
- **Committed in:** 41b8f26

**3. [Rule 1 - Bug] MSVC C4310 in test_synth.c**
- **Found during:** Task 2 (Windows build legs)
- **Issue:** `(uint16_t)(0u - COGMASTER__OPLL_SUM_MAX)` truncates a constant
- **Fix:** mask with `0xFFFFu` before the cast
- **Files modified:** tests/audio/test_synth.c
- **Committed in:** 41b8f26

**4. [Rule 3 - Blocking] Two core.state cases timed out on macOS x64**
- **Found during:** Task 2 (build macos-x64)
- **Issue:** `core.state.record_sweep` (4.0 s here) and `core.state.mapper` (1.6 s) ran past the 10 s floor on the Intel runner now that sound is on
- **Fix:** 60 s timeout for those two cases, comment records run 42713186331 and that it is provisional
- **Files modified:** tests/core/tests.cmake
- **Committed in:** 41b8f26

---

**Total deviations:** 4 auto-fixed (3 bugs, 1 blocking)
**Impact on plan:** All four came from the phase head's first hosted run, which no earlier plan had been able to run, and are the plan's "triage a failing job" path. No scope creep; the cap, the checker and the artifacts are as planned.

## Issues Encountered

- The first dispatch (run 42713186331) failed on lanes this Mac cannot run (GCC, MSVC, ASan with the installed runtime); two fix-and-redispatch cycles were enough, within the three-attempt limit.
- A workflow dispatch reports no percentage change, so the perf figure was assembled from two dispatch runs; see Hosted evidence.

## User Setup Required

None - no external service configuration required.

## Known Stubs

None.

## Threat Flags

None.

## Next Phase Readiness

All fifteen plans of phase 4 are complete. The next step is code review for the phase (`/gsd-code-review 04`); the release rehearsal and the perf report will run on the pull request.

## Self-Check: PASSED

Files audio.cmake, record.cfg, allow.txt and audio.md found, flip.cmake gone; commits 16b8f49, dc5f35c, 41b8f26 and 9aaf378 are ancestors of HEAD.

---
*Phase: 04-sound*
*Completed: 2026-11-09*
