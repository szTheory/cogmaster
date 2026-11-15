---
phase: 16-graphics-i-and-ii-draw
plan: 05
subsystem: fixtures
tags:
  - graphics-1
  - graphics-2
  - legacy-modes
  - oracle-compare
  - cross-lane-hashes
  - mode-switch

requires:
  - phase: 16-graphics-i-and-ii-draw
    provides: the closed-form oracle and the six pinned images from plan 02, the Graphics I and II renderer from plan 04, the base hashes file for the subset compare
provides:
  - a CASE=legacy branch of the fixtures script mode, with a profile map (sms1, sms2, gg_sms), an optional runner-hash judgement and a wrong-digest control
  - 16 fixtures.legacy_modes.* ci tests (15 image and profile tests and the wrong-hash control) in both inventories
  - the g2_switch line pinned by the SHA-256 of its 256 RGB444 words, one literal per profile, taken after the picture was viewed
  - hashes_playing in tests/cmake/hashes.cmake, a frame-count parameter for the playing rows, and 13 legacy_modes rows in the cross-lane hashes file with the hash_matrix assertions that name them
affects:
  - 16-07
  - 16-08
  - 16-09

actuals:
  tokens: 4600
  tasks: 3
  commits: 3
plan_head_before: 88ebfbcea0bc02e950b0286b38c4c6ad7690bdfc
plan_head_after: f8ac2e7b37c632f645ff5a2413ba27d775992497

tech-stack:
  added: []
  patterns:
    - "one script branch judges an image twice: every dot through the public API against the oracle, and the runner hash against the oracle digest, so a renderer and the digest it prints are never graded by themselves"
    - "a picture that is a mix of two modes is pinned by a hash literal that names its frame, after a check that every dot of it equals one of the two closed-form answers"
    - "the playing check takes its frame count as an argument, so one function writes the 14 frame rows and the longer legacy rows"

key-files:
  created: []
  modified:
    - tests/fixtures/legacy.c
    - tests/fixtures/tests.cmake
    - tests/cmake/hashes.cmake
    - tests/runner/tests.cmake
    - tests/inventory/ci.txt
    - tests/inventory/wasm32.txt

key-decisions:
  - "The static images are judged at frame 20 and not at the plan's 14: Graphics II is complete from frame 17 (compare fails at 14, 15 and 16 on the backdrop), Graphics I from frame 3, so one count of 20 covers all three with three frames of margin"
  - "g2_late is judged at frame 620, four frames after its switch at host frame 616; the plan's 610 is before the switch"
  - "legacy.c compare refuses any frame but 30 for g2_switch (exit 2), because the pinned switch line differs from frame to frame and a pin that silently skipped another frame would prove nothing"
  - "The switch-line pin was checked before it was written: lines 0 to 47 are flat colour RAM entry 0, lines 49 to 191 equal the oracle dot for dot, and line 48 is mode 4 before dot 119 or 120 and the oracle Graphics II dot from there, on all three profiles"
  - "hashes_playing derives nothing: the caller passes machine and video mode, and the legacy branch passes explicit profile and machine pairs, so the existing 14 frame rows stay byte-identical (the non-legacy lines of the hashes file are equal before and after)"
  - "The runner hash of g2_late is judged on sms2 only, and g2_switch on none (its whole frame is not closed form)"

requirements-completed:
  - MODE-01
  - MODE-06

coverage:
  - id: D1
    description: "Every legacy_modes image (g1, g2, g2_masked, g2_late, g2_switch) equals the closed-form oracle dot for dot through the public API on the Master System 1, the Master System 2 and the Game Gear in Master System video mode, except the switch line of g2_switch, which a hash pins"
    requirement: MODE-01
    verification:
      - kind: integration
        ref: "ctest --preset ci -R fixtures.legacy_modes exit 0, 16 of 16 (15 image and profile tests and wrong_hash); cogmaster-test-legacy-fixture compare runs 256 by 192 dots per test"
        status: pass
    human_judgment: false
  - id: D2
    description: "The runner's --hash equals the oracle digest on every variant except g2_switch, and on the Master System 2 only for g2_late; a deliberately wrong digest fails"
    requirement: MODE-01
    verification:
      - kind: integration
        ref: "ctest --preset ci -R fixtures.legacy_modes exit 0; fixtures.legacy_modes.wrong_hash runs the same script branch with WRONG=1 through expect_fail.cmake, CODE 1, regex legacy runner hash mismatch"
        status: pass
    human_judgment: false
  - id: D3
    description: "MODE-02 per D-22: the fixture per mask combination is the two ROM images here (g2 with all ones, and g2_masked where both Master System 1 masks bite) plus the eleven in-process (R3, R4) pairs of vdp.legacy.masks from plan 04 under the same oracle; removing the Master System 1 masks from the renderer fails exactly fixtures.legacy_modes.g2_masked.sms1"
    requirement: MODE-02
    verification:
      - kind: integration
        ref: "ctest --preset ci -R fixtures.legacy_modes.g2 exit 0 on sms1, sms2 and gg_sms for g2 and g2_masked; mutation run by hand and reverted, only g2_masked.sms1 failed"
        status: pass
      - kind: unit
        ref: "ctest --preset ci -R vdp.legacy.masks exit 0 in the full ci workflow, eleven pairs on three profiles"
        status: pass
    human_judgment: false
  - id: D4
    description: "A mode switch takes effect at the register-write dot: the lines wholly before and after the switch in g2_switch equal the oracle on three profiles and the switch line is pinned by hash at frame 30; moving the catch-up in cogmaster__vdp_write by 8 dots fails all three tests"
    requirement: MODE-06
    verification:
      - kind: integration
        ref: "ctest --preset ci -R fixtures.legacy_modes.g2_switch exit 0; mutation run by hand and reverted, the three g2_switch tests failed and nothing else in fixtures.legacy_modes"
        status: pass
    human_judgment: false
  - id: D5
    description: "The late image equals the Graphics II oracle after it enters the mode following frame 600 (switch at host frame 616, compared at 620 on three profiles, the runner hash on the Master System 2)"
    requirement: MODE-06
    verification:
      - kind: integration
        ref: "ctest --preset ci -R fixtures.legacy_modes.g2_late exit 0; cogmaster-test-legacy-fixture compare 612, 614 and 615 fail on all three profiles and 616 to 640 pass"
        status: pass
    human_judgment: false
  - id: D6
    description: "The cross-lane hashes file gains 13 playing rows (g1, g2, g2_masked and g2_switch on sms, sms1 and gg-sms, g2_late on sms) from a function that takes the frame count, the hash_matrix case asserts each of them, and the existing rows are byte-identical"
    requirement: MODE-05
    verification:
      - kind: integration
        ref: "ctest --preset ci -R runner.hashes_file exit 0; a missing g2_late row (frame count changed to 621 in the writer) failed it with playing matrix missing and was reverted; the non-legacy lines of build/ci/hashes.txt equal those written before the parametrisation"
        status: pass
    human_judgment: false
  - id: D7
    description: "No shipped pin moved: every line of the base hashes file is in the regenerated head file, and the g2_late and g2_switch rows are new lines, not renamed ones"
    requirement: MODE-05
    verification:
      - kind: other
        ref: "build/ci/hashes.txt deleted and regenerated, then test -z of LC_ALL=C comm -23 of the sorted build/p16-base-hashes.txt and the sorted build/ci/hashes.txt exit 0 (173 base lines, 192 head lines, 6 fixture lines and 13 playing rows new); comm -13 lists six lines matching legacy_modes_g2_late or legacy_modes_g2_switch"
        status: pass
    human_judgment: false
  - id: D8
    description: "The agent viewed the pictures the pins stand for and the images run without a fault on every profile"
    verification:
      - kind: other
        ref: "build/ci/cogmaster-run --png 30 of g1 and g2 on sms1, sms2 and gg viewed (six pictures), the three g2_switch pictures viewed and line 48 magnified; cogmaster-run --diagnose on the five images by three profiles printed fault=none fifteen times, exit 0"
        status: pass
    human_judgment: false
  - id: D9
    description: "The whole ci lane, the five release tests, the budgets and the format dry run pass on the final commit, and no new fixture joins the diagnose tests"
    verification:
      - kind: integration
        ref: "cmake --workflow --preset ci exit 0 at f8ac2e7 with 718 of 718 tests and 5 of 5 release tests; cmake -DROOT with tests/cmake/budgets.cmake exit 0 (887 check lines, 9555 src lines); clang-format 21 dry run on tests/fixtures/legacy.c exit 0; grep -c legacy_modes on both diagnose scripts prints 0"
        status: pass
    human_judgment: false

duration: 13 min
completed: 2026-11-14
status: complete
---

# Phase 16 Plan 05: Legacy mode images against the oracle Summary

**Every Graphics I and II image now equals the closed-form oracle on three profiles through the public API and the runner hash, the mid-line switch is pinned by a line hash after its dots were checked, and the cross-lane hashes file carries 13 legacy rows from a frame-count parameter with no shipped pin moved**

## Performance

- **Duration:** 13 min
- **Started:** 2026-11-14T04:38:00Z
- **Completed:** 2026-11-14T04:51:00Z
- **Tasks:** 3 (the first a tracer)
- **Files changed:** 6 (212 insertions, 8 deletions), all under `tests/`; nothing under `src/`, `include/` or `runner/` changed

## Accomplishments

- **One script branch, two judgements.** `CASE=legacy` in `tests/fixtures/tests.cmake` checks the image's SHA-256 against `tests/built/expected.txt`, runs `cogmaster-test-legacy-fixture <image> <variant> <profile> compare <frames>` (every dot through the public API) and, with `RUNNER`, requires `cogmaster-run <image> --machine <m> --video-mode sms --region ntsc-u --frames <n> --hash` to print the oracle's digest. The profile map is `sms1` to machine sms1, `sms2` to sms and `gg_sms` to gg. `WRONG` swaps in a digest that no frame has. The tracer (`fixtures.legacy_modes.g1.sms2`) passed end to end before anything was expanded.
- **Sixteen tests.** `fixtures.legacy_modes.<variant>.<profile>` for `g1`, `g2`, `g2_masked`, `g2_late`, `g2_switch` by `sms1`, `sms2`, `gg_sms`, and `fixtures.legacy_modes.wrong_hash` (through `expect_fail.cmake`, CODE 1, regex `legacy runner hash mismatch`). The runner hash is judged on every variant except `g2_switch` and, for `g2_late`, on `sms2` only. Each test is labelled `fixtures portable` under a `# Fault prevented:` comment with a provisional timeout note; all 16 are in `ci.txt` and `wasm32.txt` in sorted position.
- **The switch line.** `legacy.c` pins line 48 of `g2_switch` at frame 30 by the SHA-256 of its 256 RGB444 words: `dfdc6a00...33a2` on the Master System 1 and 2 (R3 7F and R4 07 mask nothing, so the two agree) and `9b8d29f2...0036` on the Game Gear. Before pinning, the agent viewed the frame on all three profiles and magnified line 48, and a scratch script (not committed) re-derived the pictures from the closed forms: lines 0 to 47 are flat colour RAM entry 0, lines 49 to 191 equal the oracle on every dot, and line 48 is that mode 4 colour up to dot 119 (Game Gear) or 119 and 120 (Master System) and the oracle's Graphics II dot from there on. `compare` refuses any other frame for this image (exit 2), because the switch dot moves from frame to frame.
- **The hashes file.** `hashes_playing` in `tests/cmake/hashes.cmake` now owns the run, the exit and `^frame <N> sha256` check and the row append; the boot_to_play and vdp_lines rows call it with 14 and are byte-identical to before (the non-legacy lines of the regenerated file equal the lines written before the change). The new branch writes `playing legacy_modes_<variant>.sms <profile> ntsc-u legacy frame <N> sha256 <hex>` for g1, g2 and g2_masked at 20 frames and g2_switch at 30 on sms, sms1 and gg-sms, and one row for g2_late on sms at 620 frames: 13 rows. The `hash_matrix` case of `tests/runner/tests.cmake` asserts every one of them with its frame count.
- **Subset compare.** `LC_ALL=C comm -23` of the sorted base and head hashes files printed nothing (exit 0 as `test -z`). The head file is 192 lines against 173: six `fixture legacy_*` lines and the 13 playing rows, all new; six new lines match `legacy_modes_g2_late` or `legacy_modes_g2_switch` (their fixture lines and their playing rows), none renamed.

## Task Commits

1. **Task 1: Tracer, one legacy_modes image (g1 on sms2) from the built image through the runner hash to the oracle digest** - `e3c9e90` (test)
2. **Task 2: Every variant and profile, the wrong-hash control and the switch-line pin** - `e415d82` (test)
3. **Task 3: The cross-lane hash rows from a frame-count parameter, their assertions and the subset compare** - `f8ac2e7` (test)

**Plan metadata:** the SUMMARY commit and the state and roadmap commit that follow this file, both local; no push (the next scheduled push is plan 08's, after the sprites).

The tracer gate was met by re-running the Task 1 verify chain (configure, build, the one ctest) before expanding: exit 0, 1 of 1.

## Files Created/Modified

- `tests/fixtures/tests.cmake` - the `CASE=legacy` branch, the 15 image tests and the wrong-hash control
- `tests/fixtures/legacy.c` - `line_hash`, the three switch-line literals, `SWITCH_FRAME`, the pin check in `compare`
- `tests/cmake/hashes.cmake` - `hashes_playing` and the legacy_modes branch
- `tests/runner/tests.cmake` - the `hash_matrix` assertions for the 13 rows
- `tests/inventory/ci.txt`, `tests/inventory/wasm32.txt` - 16 names each

## Decisions Made

- **Frame counts are measured.** Plan 02 recorded that the display turns on from frame 17 for the static images, and the compare confirms it: g2 and g2_masked fail at 14, 15 and 16 on the backdrop at dot (0,0) and pass from 17; g1 passes from frame 3. One count of 20 serves g1, g2 and g2_masked in the tests and in the hashes rows. g2_late is judged at 620 (it passes from 616 and fails at 612, 614 and 615 on all three profiles).
- **The pin names its frame.** `compare` takes `frames` as an argument for every image, and for `g2_switch` any value but 30 is a usage error, so a later edit that changed the frame count cannot make the line check disappear.
- **Controls, run by hand and reverted.** Removing the Master System 1 masks from `src/vdp_legacy.c` failed exactly `fixtures.legacy_modes.g2_masked.sms1`. Moving the catch-up in `cogmaster__vdp_write` by 8 dots (`m->bus.t / 2u - 8u`) failed exactly the three `g2_switch` tests, so the line pin is sensitive to the switch dot. A changed pin literal failed the matching profile. A WRONG digest, the wrong variant's oracle and a run of one frame (the picture not yet drawn) each failed with the named message. A g2_late frame count of 621 in the hashes writer failed `runner.hashes_file` with `playing matrix missing`. `git status` showed `src/` unchanged after each source mutation.
- **No new diagnose fixture.** `grep -c legacy_modes tests/runner/diagnose.cmake tests/library/diagnose.cmake` prints 0 for both.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Plan variant] The static images are judged at frame 20, not 14**
- **Found during:** Task 2 (measuring the first passing frame per image)
- **Issue:** the plan names 14 frames for g1, g2 and g2_masked, but Graphics II is only on the screen from frame 17; at 14 the compare fails on the backdrop. The tracer's registration at 14 passed only because Graphics I loads in three frames.
- **Fix:** `FRAMES` 20 for all three images in the tests, and the same 20 in the hashes rows, the matrix assertions and the wrong-hash control.
- **Files modified:** `tests/fixtures/tests.cmake`, `tests/cmake/hashes.cmake`, `tests/runner/tests.cmake`
- **Verification:** all 16 tests and `runner.hashes_file` pass; compare at 14 fails for g2 and g2_masked
- **Committed in:** `e415d82`, `f8ac2e7`

**2. [Rule 1 - Plan variant] g2_late is compared at frame 620, not 610**
- **Found during:** Task 2 (plan 02 deviation 4 already measured the switch at host frame 616)
- **Issue:** frame 610 is a mode 4 picture; the plan's own count would fail.
- **Fix:** 620 (four frames after the switch) in the test, the hashes row and its assertion.
- **Committed in:** `e415d82`, `f8ac2e7`

**3. [Plan number] This plan adds 890 frames to runner.hashes_file, not 826**
- **Found during:** Task 3
- **Issue:** the plan's total assumed 14 frames for the three static images and 610 for g2_late.
- **Fix:** the real count is 9 runs of 20 (180), 3 runs of 30 (90) and 1 run of 620, which is 890. Plan 08 adds 3 x N for legacy_sprites (about 1,200 in all for N up to 120), still under the 2,300 D-24 estimated. The 107 s timeout of `runner.hashes_file` is left for plan 09; the local run took 3.05 s.
- **Files modified:** none (the count is recorded here for plan 09)

---

**Total deviations:** 3 (two plan variants measured against the images, one corrected total)
**Impact on plan:** none widens scope; every acceptance count is met (16 inventory names in each file, 17 `fixtures.legacy*` tests, 13 hashes rows). No file outside the plan's list changed.

## Issues Encountered

- `clang-format` is not on the PATH of this shell; the dry run used `/opt/homebrew/opt/llvm@21/bin/clang-format` (version 21, the one the earlier summaries name) and exited 0.
- The plan's Task 1 verify and acceptance counts ran as written. The first manual negative-control invocation quoted two `-D` options as one argument and so passed vacuously; it was rerun with separate arguments and failed as it must.

## Evidence

Commands from the repository root, exit status 0 unless stated; no push, so no run URL:

- `cmake --preset ci`, `cmake --build --preset ci` (warnings are errors; clean)
- `ctest --preset ci -R '^fixtures\.legacy_modes\.g1\.sms2$'` (1 of 1, tracer), then `ctest --preset ci -R '^fixtures\.legacy' -j8` (17 of 17)
- `ctest --preset ci -R '^(fixtures\.legacy|runner\.hashes_file$)' -j8` (18 of 18), `ctest --preset ci -R '^(fixtures|docs|policy|runner\.hashes_file)' -j8` (189 of 189)
- `rm build/ci/hashes.txt`, then `ctest --preset ci -R '^runner\.hashes_file$'` (3.05 s), then `test -z "$(LC_ALL=C comm -23 <(sort build/p16-base-hashes.txt) <(sort build/ci/hashes.txt))"` (exit 0, repeated after the workflow); `LC_ALL=C comm -13` listed the 19 new lines
- `cmake --workflow --preset ci` at `f8ac2e7`: 718 of 718 tests and 5 of 5 release tests
- `cmake -DROOT="$PWD" -P tests/cmake/budgets.cmake` (887 check lines, 9555 src lines); `/opt/homebrew/opt/llvm@21/bin/clang-format --dry-run --Werror tests/fixtures/legacy.c`
- acceptance counts: `grep -c 'CASE STREQUAL "legacy"' tests/fixtures/tests.cmake` 1; `grep -cE '^fixtures\.legacy_modes\.'` 16 in `ci.txt` and 16 in `wasm32.txt`; `grep -c legacy_modes` 3 in `hashes.cmake` and 2 in `tests/runner/tests.cmake`; `grep -c -- '--frames 14 --hash' tests/cmake/hashes.cmake` 0
- `build/ci/cogmaster-run <image> --machine <m> --video-mode sms --region ntsc-u --diagnose` for the five images on sms1, sms and gg: `fault=none` fifteen times
- the agent viewed (rule 6) `--png 30` of g1 and g2 on sms1, sms2 and gg: Graphics I shows four 64 dot colour-group bands with the fixed palette on the Master System and pastel colour RAM entries on the Game Gear; Graphics II shows dense per-row detail that changes at lines 64 and 128 in the fixed palette and the colour RAM palette; and `--png 30:` of `legacy_modes_g2_switch` on three profiles: a flat band for 48 lines, then the generated scene, with line 48 olive up to about dot 120 and Graphics II colour blocks after it

## Known Stubs

None. The images and tests are complete for their scope; the sprite scenarios, the state format and the rescan across a class change are plans 06 to 08.

## Threat Flags

None. The plan adds test scripts and literals only; no endpoint, file access or trust boundary in `src/`.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- **Plan 06 to 08.** The legacy_modes images are judged against the oracle on every push of the ci lane; a renderer change that moves a Graphics I or II dot, the mask choice or the switch dot fails a named test. `build/p16-base-hashes.txt` (173 lines) is still the base for the subset compare, which now has 19 new lines in the head file.
- **Plan 08** adds the legacy_sprites rows to the hashes file (3 x N frames) and the push; the hosted run is the first place the `ci.yml` lanes compare the 13 new playing rows byte for byte.
- **Plan 09.** Re-derive the `runner.hashes_file` timeout and the provisional `fixtures.legacy_modes.g2_late.*` timeouts from a hosted run (the late image runs 620 frames, three times in the tests and once in the hashes file); record the frame counts here (20, 30, 620) in `docs/conformance.md`, with the 16-04 note that Graphics II is complete from frame 17.
- **Requirements.** MODE-01 and MODE-06 are in `requirements-completed:` as the plan says; MODE-06 is also declared by plans 07 and 08, and MODE-02 completes with the plan 09 documents (the masks the sources disagree on are named there). MODE-05's subset compare is shown here and completes with the state format work.

---
*Phase: 16-graphics-i-and-ii-draw*
*Completed: 2026-11-14*

## Self-Check: PASSED

The six modified files exist; commits e3c9e90, e415d82 and f8ac2e7 are ancestors of HEAD and `gsd_run check evaluation-scope --plan 16-05 --commits-only` lists the same three; the acceptance counts above were re-run on the final tree; the base-versus-head subset compare printed nothing after the full workflow; `git diff --name-only 88ebfbc..HEAD` lists only the six files under `tests/`; the coverage block uses block style with no flow mappings and no inline comments.
