---
phase: 16-graphics-i-and-ii-draw
plan: 08
subsystem: fixtures
tags:
  - legacy-sprites
  - oracle-compare
  - mailbox
  - cross-lane-hashes
  - hostile-input
  - fuzz-seed
  - hosted-run

requires:
  - phase: 16-graphics-i-and-ii-draw
    provides: the sprite scenario image, the closed-form sprite oracle with legacy_status and the 22 derived rows (plan 02), the frame-count parameter of the playing rows (plan 05), the legacy sprite engine and the class-change rescan (plan 07), the hostile case (plan 04)
provides:
  - six fixtures.legacy_sprites tests (a pixel and runner-hash test and a mailbox test on each of sms1, sms2 and gg_sms) in both inventories
  - a compare-all mode of the oracle program, which steps the sprite image once and judges every scenario on both frames it is on screen
  - six legacy_sprites playing rows in the cross-lane hashes file (frames 15 and 47 on sms, sms1 and gg-sms) with the hash_matrix assertions
  - sprite-state bounds in vdp.legacy.hostile and the legacy-display.sms program seed
  - the second scheduled push, its hosted run, the perf read and the be32 duration plan 09 sizes its timeouts from
affects:
  - 16-09

actuals:
  tokens: 3300
  tasks: 3
  commits: 3
plan_head_before: 672a23f28786901fb2e0089f60fa1925a05ba73e
plan_head_after: fba1e2498bd85c01e51a382f12f3cc873dba90f8

tech-stack:
  added: []
  patterns:
    - "one oracle run steps a multi-scenario image frame by frame and compares each scenario on every frame it is on screen, so a picture one frame off fails and no scenario needs a run of its own"
    - "a hostile case counts how often the garbage reaches the bound it asserts, so the assertion cannot pass because the bound was never approached"
    - "an image-level mutation run (a source edit restored from a pristine copy kept outside the tree) shows which of the pixel test, the mailbox test or both catches each engine rule"

key-files:
  created: []
  modified:
    - tests/fixtures/legacy.c
    - tests/fixtures/tests.cmake
    - tests/cmake/hashes.cmake
    - tests/runner/tests.cmake
    - tests/vdp/test_legacy.c
    - tests/fuzz/seeds.c
    - tests/inventory/ci.txt
    - tests/inventory/wasm32.txt

key-decisions:
  - "The sprite image is judged by one compare-all run per profile (frames 1 to 48, scenario k on frames 9 + 2 (k - 1) and 10 + 2 (k - 1)) and the runner hash of the last scenario at frame 47, the frame the image holds from then on; the mailbox test reads 200 frames and the runner stops when the mailbox is done"
  - "The cross-lane rows take two pictures per profile, the magnified 16 by 16 scenario at frame 15 and the held last scenario at frame 47, 186 frames in all, because one hash at the end would leave the widest sprite path to the in-process cases alone"
  - "vdp.legacy.hostile already wrote random bytes over the whole of video memory, the table and patterns included, so the extension is the three bounds after each legacy frame and at every step inside it, plus a check that the garbage fills all four slots and latches a fifth index on some lines"
  - "The program seed writes a counting byte from address 0 for ever under Graphics I, so the name, pattern, colour and sprite tables all read counting bytes: every line scans 32 entries with Y values that match many lines, the picture shows the diagonal of sprites and the run is fault none"

patterns-established:
  - "a status flag that is judged through the guest's own read is also judged by moving the engine rule (a mutation) and naming the record that fails"

requirements-completed:
  - MODE-03
  - MODE-06

coverage:
  - id: D1
    description: "The legacy_sprites image equals the closed-form sprite oracle on every one of its 20 scenarios on both frames each is on screen, through the public API, on the Master System 1, the Master System 2 and the Game Gear in Master System video mode, and the runner hash of the last scenario at frame 47 equals the oracle digest"
    requirement: MODE-03
    verification:
      - kind: integration
        ref: "ctest --preset ci -R fixtures.legacy_sprites exit 0, 3 pixel tests and 3 mailbox tests; cogmaster-test-legacy-fixture legacy_sprites.sms sprites sms1, sms2 and gg compare all each exit 0"
        status: pass
      - kind: integration
        ref: "hosted CI run 43562888383 on fba1e24 concluded success; the six tests passed on all nine test lanes (slowest 1.24 s)"
        status: pass
    human_judgment: false
  - id: D2
    description: "Every status record of the image (fifth sprite and coincidence flags, the end-of-list Y value, 8x8 and 16x16, magnification, the early clock, five on a line, two overflow lines) equals its derived row of tests/diag/expected.csv on all three profiles, 22 records each, with no record lacking a row and no row lacking a record"
    requirement: MODE-03
    verification:
      - kind: integration
        ref: "ctest --preset ci -R fixtures.legacy_sprites.mailbox exit 0, 3 of 3; mutations of the fifth index offset, the coincidence count and a colour 0 collision each failed the sms2 mailbox test and nothing else, a Y threshold of F0 failed the pixel test only, and ignoring the early clock bit failed both"
        status: pass
    human_judgment: false
  - id: D3
    description: "The tail-read records are present and judged: a status read placed between the scan of line 80 at tick 600 and dot 0 of line 81 reads 001F, the H counter next to it lies in 0000 to 0013, and the frame-end read still reports the fifth index (00C4)"
    requirement: MODE-03
    verification:
      - kind: integration
        ref: "ctest --preset ci -R fixtures.legacy_sprites.mailbox exit 0 judges records 14, 15 and 16 on three profiles; a mutation that set the fifth flag and its index at scan time instead of dot 0 failed with record 14=0044 outside its row 14:tail_status (001F)"
        status: pass
    human_judgment: false
  - id: D4
    description: "The cross-lane hashes file gains six legacy_sprites playing rows (frames 15 and 47 on sms, sms1 and gg-sms), the hash_matrix case asserts each, no shipped line moved and the eight lanes agree byte for byte"
    requirement: MODE-05
    verification:
      - kind: integration
        ref: "ctest --preset ci -R runner.hashes_file exit 0; a writer that printed frame 16 for frame 15 failed it with playing matrix missing and was reverted; test -z of LC_ALL=C comm -23 of the sorted build/p16-base-hashes.txt and build/ci/hashes.txt exit 0 (173 base lines, 198 head lines)"
        status: pass
      - kind: integration
        ref: "hosted CI run 43562888383 job hash-equality success: exactly eight hashes files equal, the 13 legacy_modes rows and the six legacy_sprites rows included"
        status: pass
    human_judgment: false
  - id: D5
    description: "Hostile legacy input with sprites stays bounded: with video memory, the attribute table and the sprite patterns full of garbage and the sprite size and magnification bits varying, sprite_count stays at 4 or less, sprite_scanned at 32 or less and fifth_index at 31 or less after each legacy frame and at every step inside it, and the garbage fills all four slots and latches a fifth index on some lines"
    requirement: MODE-05
    verification:
      - kind: unit
        ref: "ctest --preset ci -R vdp.legacy.hostile exit 0; raising the slot count to five and the entries read to 33 each failed it"
        status: pass
    human_judgment: false
  - id: D6
    description: "A Graphics I program seed (legacy-display.sms) feeds the fuzzers, runs fault none, and draws a Graphics I picture with sprites from counting bytes"
    requirement: MODE-05
    verification:
      - kind: other
        ref: "build/ci/cogmaster-run build/ci/tests/seeds/legacy-display.sms --frames 30 --trace first-fault printed fault none, exit 0; --frames 1800 on sms1 gave the same frame hash as frame 30; cmake --workflow --preset ci exercises the fuzz replay over the seeds directory"
        status: pass
    human_judgment: false
  - id: D7
    description: "The second scheduled push has a hosted run on the pushed head: the CI run concluded success, the perf job reports verdict PASS with no allow line and no workflow edit, and the be32 lane ran runner.hashes_file in 64.72 s"
    requirement: MODE-07
    verification:
      - kind: other
        ref: "git push origin phase/16-graphics-i-and-ii-draw exit 0 (d7da071 to fba1e24); gh run list --branch phase/16-graphics-i-and-ii-draw --limit 1 headSha equals git rev-parse HEAD, exit 0; hosted CI run 43562888383 success (20 jobs: 18 success, rehearsal and cleanup skipped); perf boot_to_play head=10089912 base=12288947 delta_bp=-1789 verdict=PASS; git diff --quiet origin/main -- tests/perf/allow.txt .github/workflows/ci.yml exit 0"
        status: pass
    human_judgment: false
  - id: D8
    description: "The whole ci lane, the hygiene workflow, the text scan, the budgets, the format dry run and the subset compare pass on the final commit, and no file under src, include or runner changed"
    verification:
      - kind: integration
        ref: "ctest --preset ci -j8 exit 0, 737 of 737; cmake --workflow --preset hygiene exit 0, 84 of 84; sh scripts/hygiene.sh --tree exit 0; cmake -DROOT with tests/cmake/budgets.cmake exit 0 (887 check lines, 9738 src lines); clang-format 21 dry run over git ls-files exit 0; git diff --name-only 672a23f..HEAD -- src include runner prints nothing"
        status: pass
    human_judgment: false
  - id: D9
    description: "The agent looked at the pictures the hashes stand for: the last scenario on each profile, the magnified 16 by 16 scenario, the overlapping pair and the legacy program seed"
    verification:
      - kind: other
        ref: "build/ci/cogmaster-run build/ci/fixtures/legacy_sprites.sms --machine sms, sms1 and gg --video-mode sms --region ntsc-u --frames 47 --png 47:build/p16-sprites-NAME.png exit 0 for each, plus frames 15 and 31 on sms and the seed at frame 30, all viewed by the agent"
        status: pass
    human_judgment: false

duration: 23 min
completed: 2026-11-14
status: complete
---

# Phase 16 Plan 08: Legacy sprite image against the oracle, hostile bounds and the second push Summary

**The legacy_sprites image equals the closed-form sprite oracle on every scenario and both frames it is on screen, and every status record (the tail read included) equals its derived row, on three profiles; six cross-lane hash rows, sprite-state bounds under hostile input and a legacy program seed landed, and the second push has a green hosted run with the perf gate PASS and be32 runner.hashes_file at 64.72 s**

## Performance

- **Duration:** 23 min
- **Started:** 2026-11-14T05:32:37Z
- **Completed:** 2026-11-14T05:56:00Z
- **Tasks:** 3 (the first a tracer)
- **Files changed:** 8 (182 insertions, 39 deletions), all under `tests/`; nothing under `src/`, `include/` or `runner/` changed

## Accomplishments

- **One run judges the whole image.** `cogmaster-test-legacy-fixture <image> sprites <profile> compare all` steps the image for 48 frames and, from frame 9, compares every dot of scenario k on frames 9 + 2 (k - 1) and 10 + 2 (k - 1) against the closed form (40 pictures, about 2 million dots per profile), stopping at the first difference. `CASE=legacy` gained two optional arguments, `COMPARE` (default the frame count) and `SCENARIO` (the scenario whose digest the runner's hash at `FRAMES` must equal), so the sprite image uses the same branch as the legacy_modes images. The runner's `--frames 47 --hash` equals the oracle digest of scenario 20 on every profile (`b0d6e211...` on both Master Systems, `c11f91b3...` on the Game Gear), and frame 60 gives the same hash because the image holds its last picture.
- **Six tests, three profiles.** `fixtures.legacy_sprites.<profile>` (pixels and runner hash) and `fixtures.legacy_sprites.mailbox.<profile>` (the 22 records against `tests/diag/expected.csv`, run with `--machine`, `--video-mode sms` and `--region ntsc-u`) for `sms1`, `sms2` and `gg_sms`, labelled fixtures portable under `# Fault prevented:` comments with provisional `# Timeout:` comments, six lines each in `ci.txt` and `wasm32.txt`. The tail records are judged like every other row: the H counter against `0000-0013`, the status rows against `legacy_status`.
- **Cross-lane rows.** `hashes.cmake` writes `playing legacy_sprites.sms <profile> ntsc-u legacy frame <N> sha256 <hex>` for N 15 (the magnified 16 by 16 scenario) and 47 on sms, sms1 and gg-sms through the frame-count parameter of plan 05; `hash_matrix` asserts all six. The hashes file is 198 lines against 192 and 173 at the base; `comm -23` of the sorted base and head prints nothing. The hosted `hash-equality` job compared the eight lanes' files byte for byte with the legacy_modes and legacy_sprites rows in them.
- **Hostile sprites.** `vdp.legacy.hostile` asserts `sprite_count <= 4`, `sprite_scanned <= 32` and `fifth_index <= 31` after each legacy frame and at each of the 40 steps inside it, and that the garbage fills all four slots and latches a fifth index on some lines. The case already wrote random bytes over all of video memory (so the attribute table and the patterns), with the size and magnification bits varying since plan 07; the new part is the bounds and the proof that they are approached.
- **Legacy program seed.** `legacy-display.sms` in `tests/fuzz/seeds.c` sets R0 `$00` and R1 `$C0` and writes a counting byte from address 0 for ever. It runs `fault none` and draws a Graphics I picture with a diagonal of sprites; no seed count is pinned anywhere, and nothing globs the seeds directory.
- **The push and its run.** `git push origin phase/16-graphics-i-and-ii-draw` (fast forward d7da071 to fba1e24, 27 commits) triggered hosted CI run 43562888383, which concluded success. The perf job read `perf boot_to_play head=10089912 base=12288947 delta_bp=-1789 verdict=PASS` with no allow line; be32 ran `runner.hashes_file` in 64.72 s (610 tests, job 132350296827, 552 s in all).

## Task Commits

1. **Task 1: Tracer, the legacy_sprites image on sms2 from the built image through pixels, runner hash and mailbox rows** - `74d20c8` (test)
2. **Task 2: All three profiles and the legacy_sprites hash rows** - `83eff87` (test)
3. **Task 3: Hostile sprite input and the fuzz seed** - `fba1e24` (test); the push and the hosted run are this task's evidence, not files

**Plan metadata:** the SUMMARY commit and the state and roadmap commit that follow this file, both local; the push was the one scheduled for this plan and is not repeated for them.

The tracer gate was met by re-running the Task 1 verify chain end to end (configure, build, the three targeted tests, exit 0) after the formatting fix and before the commit, then expanding.

## Files Created/Modified

- `tests/fixtures/legacy.c` - `compare_frame`, the `all` mode of `compare` and its argument handling
- `tests/fixtures/tests.cmake` - `COMPARE` and `SCENARIO` in `CASE=legacy`, the six sprite registrations
- `tests/cmake/hashes.cmake`, `tests/runner/tests.cmake` - the six playing rows and their assertions
- `tests/vdp/test_legacy.c` - `sprite_bounds` and its calls in `hostile`
- `tests/fuzz/seeds.c` - the `legacy-display.sms` seed
- `tests/inventory/ci.txt`, `tests/inventory/wasm32.txt` - six names each

## Decisions Made

- **Compare all, one run.** A run per scenario would repeat the load frames 20 times; one run of 48 frames judges each picture twice, so a compare one frame off meets another scenario's backdrop and fails.
- **Two pictures per profile in the hashes file.** Frame 15 is the magnified 16 by 16 sprite (the widest path) and frame 47 the held last scenario; the cost is 3 x (15 + 47) = 186 frames, about 2.7 s locally, so `runner.hashes_file` stays far inside its limit (64.72 s hosted on be32 against 107 s).
- **Hostile case.** The bounds are asserted at every step inside the frame, not only at its end, because the state at the end is the scan of one line past the picture and rarely holds four sprites; the vacuity check proves the garbage reaches the bounds.

## Deviations from Plan

### Plan variants

- **A compare mode the plan did not name.** Task 1 lists `tests/fixtures/legacy.c` and says the test compares "every scenario frame"; the existing `compare` took one scenario, so `compare all` was added. No rule fix was involved.
- **Counts in the acceptance lines.** `grep -c '^legacy_sprites,[^,]*tail' tests/diag/expected.csv` is marked `HEAD=0`, but plan 02 had already written the three tail rows, so it prints 3 (the criterion is `>=1`).
- **Hash rows.** The plan names one frame count N; two (15 and 47) were chosen, see Decisions Made.

**Total deviations:** none under rules 1 to 4. No hosted job was red, so no triage round, no second push and no cause to record.
**Impact on plan:** none widens scope; no file the plan did not list changed, and `tests/perf/allow.txt` and `.github/workflows/ci.yml` are unchanged.

## Issues Encountered

- `clang-format` rejected a 101-column comment in `legacy.c` after the first edit; it was reflowed with `clang-format -i` before the Task 1 commit.
- The mutation runs touched `src/vdp_legacy.c` ten times. Each restored it from a pristine copy kept in the session scratch directory (the script refuses to run without one), `cmp` against the copy printed equal after every run, and `git status` never listed a `src/` file.
- The CI watcher moved to the background after 180 seconds; the run was read by `gh run view` once it had concluded.

## Evidence

Commands from the repository root; exit status 0 unless stated.

- `cmake --preset ci`, `cmake --build --preset ci`; `ctest --preset ci -R '^(fixtures\.legacy\.oracle|fixtures\.legacy_sprites\.(sms2|mailbox\.sms2))$'` (3 of 3, the tracer); `ctest --preset ci -R '^(fixtures\.legacy|runner\.hashes_file$)' -j8` (24 of 24); `ctest --preset ci -j8` (737 of 737, run twice)
- `rm -f build/ci/hashes.txt`, then the `runner.hashes_file` test, then `test -z "$(LC_ALL=C comm -23 <(sort build/p16-base-hashes.txt) <(sort build/ci/hashes.txt))"` exit 0; `grep legacy_sprites build/ci/hashes.txt` prints the six rows and the fixture line
- `git ls-files -z '*.c' '*.h' ':!:libretro/libretro.h' > build/p16-format.lst`, then `xargs -0 /opt/homebrew/opt/llvm@21/bin/clang-format --dry-run --Werror < build/p16-format.lst`; `cmake --workflow --preset hygiene` (84 of 84, `hygiene.tree` 28.8 s); `sh scripts/hygiene.sh --tree`; `cmake -DROOT="$PWD" -P tests/cmake/budgets.cmake` (887 check lines, 9738 src lines)
- `git push origin phase/16-graphics-i-and-ii-draw` (d7da071..fba1e24); `gh run list --branch phase/16-graphics-i-and-ii-draw --limit 1 --json headSha` equals `git rev-parse HEAD`; hosted run: https://github.com/szTheory/cogmaster/actions/runs/43562888383 (CI, success; the Nightly run 43562888372 on the same head also succeeded)
- hosted perf, from the job log: `perf boot_to_play head=10089912 base=12288947 delta_bp=-1789 verdict=PASS`. Plan 03's read was 9893026; the 197k (2 percent) rise since is the sprite dispatch and the class compare of plans 04 to 07, still 18 percent under the base
- hosted `sh scripts/ci-durations.sh tests 43562888383`, be32 job: `runner.hashes_file` 64.72 s, `runner.diagnose` 407.78 s, `fixtures.legacy_modes.g2_late.sms2` 14.02 s (sms1 7.94, gg_sms 6.72), `core.state.legacy_sprite_bounds` 9.06 s, `vdp.legacy.hostile` 2.68 s, the six `fixtures.legacy_sprites` tests 1.24 s at most; the be32 preset scales every timeout by 8
- controls, run by hand and reverted (nothing mutated was committed): in `src/vdp_legacy.c` the fifth index offset, the coincidence count, a colour 0 collision, a Y threshold of F0, the early clock bit, the fifth flag at scan time, five slots and 33 entries; in `tests/cmake/hashes.cmake` a writer frame of 16 for 15. Each failed the named test (above) and the working tree equalled the pristine copy afterwards
- the pictures the agent viewed (rule 6): `--png 47` on sms, sms1 and gg: on the Master Systems a dark red backdrop with four 8 by 8 sprites in dark green, green, dark blue and blue at X 20, 50, 80 and 110 on lines 81 to 88 and a dark red fifth sprite at X 140 that stops a line short at 74 to 80 (its last line is the one with five matches), identical on sms and sms1; on the Game Gear a cyan backdrop and the same layout in pastel colour RAM entries. Frame 15 on sms: a blue backdrop and a 32 by 32 magnified 16 by 16 sprite in yellow at X 100, Y 40. Frame 31: a magenta backdrop and the overlapping pair at X 100 and 104, the lower index in front. The seed at frame 30: a Graphics I picture of counting names and colours with a diagonal run of sprites

## Known Stubs

None. Every test and row this plan adds judges a real run; nothing returns a placeholder.

## Threat Flags

None. The plan adds test scripts, a seed and assertions only; no endpoint, file access or trust boundary in `src/`.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

For plan 09, from run 43562888383 (https://github.com/szTheory/cogmaster/actions/runs/43562888383):

- **Timeouts to re-derive** (twice the slowest, with the run id): `runner.hashes_file` 64.72 s on be32 (now 107 s from 2 x 53.07 s; the file gained 25 lines this phase), the `fixtures.legacy_modes.g2_late.*` tests at 14.02 s on be32 against a nominal 10 s that the be32 scale of 8 turns into 80 s, and `core.state.legacy_sprite_bounds` at 9.06 s against its provisional 60 s tier. The six sprite tests and `vdp.legacy.hostile` need nothing (1.24 s and 2.68 s at most on be32). The be32 job took 552 s against its 21 minutes, so the job timeout is not near.
- **Perf.** `boot_to_play` at head 10089912 instructions per frame (PASS, no allow line); plan 09 may quote it with the plan 03 read of 9893026.
- **Docs.** Frame counts for the conformance paragraph: static images 20 (Graphics II complete from 17), `g2_switch` 30, `g2_late` 620, sprites 47 (scenario k on frames 9 + 2 (k - 1) and 10 + 2 (k - 1)); the sprite models to name with their disagreeing sources are in the notes of plan 07.
- **Requirements.** MODE-03 and MODE-06 are in `requirements-completed:`; MODE-05 and MODE-07 complete with plan 09's head evidence, as the plan says.
- Nothing more is pushed by this plan; the work tree is clean after the SUMMARY and state commits.

---
*Phase: 16-graphics-i-and-ii-draw*
*Completed: 2026-11-14*

## Self-Check: PASSED

The eight changed files exist; commits 74d20c8, 83eff87 and fba1e24 are ancestors of HEAD and `gsd_run check evaluation-scope --plan 16-08 --commits-only` lists the same three; `git diff --name-only 672a23f..HEAD` lists only the eight files; the acceptance counts (six inventory lines in each file, the hashes rows, `fifth_index` and `legacy` in the two C files) and the plan-level verification were re-run on the final tree, all exit 0, the subset compare printing nothing; `git diff --quiet origin/main -- tests/perf/allow.txt .github/workflows/ci.yml` exits 0; the hosted run on the pushed head exists and concluded success; the coverage block uses block style with no flow mappings and no inline comments.
