---
phase: 11-pal-and-japanese-fm
plan: "07"
subsystem: fixtures
tags:
  - pause
  - nmi
  - pal
  - fm
  - hashes
  - ci
status: complete

requires:
  - 11-06
provides:
  - an owned WLA-DX program that counts between the frame interrupt and the Pause NMI and records the count, the V counter and the NMI count on PAL and NTSC, with the count band computed from the listing's cycle counts
  - mailbox rows, a device-row region and an fm sound-row field in the lane hash file, so the V and Pause fixtures and a PAL FM stream are printed and compared on every lane
  - the first hosted run (dispatch D1) and the one draft pull request that is the phase's ship pull request
affects:
  - 11-08
  - 11-09

actuals:
  tokens: 10800
  tasks: 3
  commits: 2

plan_head_before: fcc1697662b6900ef6d3e13a29ce633f7b2203e4
plan_head_after: 1c720da0f7daafdac63cd3591cab6c5f371f7606
commits: 2

tech-stack:
  added: []
  patterns:
    - "A count taken between two timed events is bracketed by a band computed from the listing's cycle counts (longest wait, accept, handler length, first increment, longest instruction); a shifted core falls outside it and the band is never fitted to a run"
    - "A 16-bit counter updated by one loop and zeroed by an interrupt handler is guarded with DI and EI around three instructions, so no lost update can look like a core defect"
    - "A fixture that paints nothing is compared across lanes by the digest of its printed records, and the same bytes are judged against the literals inside the hash writer"

key-files:
  created:
    - tests/roms/src/pause_nmi.asm
    - tests/roms/src/pause_nmi.link
    - .planning/phases/11-pal-and-japanese-fm/deferred-items.md
  modified:
    - tests/fixtures/tests.cmake
    - tests/diag/expected.csv
    - tests/built/expected.txt
    - tests/roms/manifest.txt
    - tests/cmake/hashes.cmake
    - tests/runner/tests.cmake
    - tests/audio/transitions.txt
    - tests/audio/pcm.txt
    - tests/inventory/ci.txt
    - tests/inventory/wasm32.txt
    - docs/conformance.md
    - docs/audio.md
    - docs/testing.md

key-decisions:
  - "The band edges come from the listing: n lies in [floor((C - 132) / 78), floor((C - 36) / 78)] with C the cycle count, B = 78 the guarded loop body, 76 the frame handler, 13 the IM 1 accept, 59 the longest wait, 42 and 61 the first-increment offsets and 16 the longest instruction; the plan's S-only shape became two edges because the main loop is stopped while the frame handler runs, so the handler length enters the lower edge and the NMI handler's cycles never enter it"
  - "Two loops of the same 78-cycle body (until frame 12, then until frame 36) keep every compare out of the frame handler, so the handler is 76 cycles and uses shadow registers holding DE' = 0 and HL' = the frame counter's address"
  - "The mailbox case of the fixtures script gains an optional VIDEO parameter so the native Game Gear run passes --video-mode gg"
  - "The hash writer judges each mailbox row in a function scope (its OUT names a file, the writer's OUT a directory) and writes the judged bytes with Unix newlines"

requirements-completed:
  - REG-02

coverage:
  - id: C1
    description: "pause_nmi.asm and its link file are built three times (192, 224, 240); the three digests are in expected.txt and one comment line in the manifest; two clean builds give equal digests; no image is committed; include and src are unchanged"
    requirement: REG-02
    verification:
      - kind: integration
        ref: "cmake --build --preset ci exit 0; shasum -a 256 of the three images before and after removing images and objects and rebuilding, cmp exit 0; grep -c '^pause_nmi_' tests/built/expected.txt prints 3; git diff -U0 origin/main over expected.txt and the manifest has 0 removed lines; git diff --quiet origin/main -- include src exit 0"
        status: pass
    human_judgment: false
  - id: C2
    description: "On PAL 192, 224 and 240 and NTSC 192 and 224 the first-NMI count lies inside the closed-form band, V reads 00FF, the NMI count is 1 after the held press and 2 at the end; on the native Game Gear all four records are 0"
    requirement: REG-02
    verification:
      - kind: integration
        ref: "build/ci/cogmaster-run build/ci/fixtures/pause_nmi_<h>.sms --region pal|ntsc-u --frames 40 --input build/ci/tests/pause.input --mailbox exit 0 for the five profiles and with --machine gg --video-mode gg; observed values in the band table below"
        status: pass
    human_judgment: false
  - id: C3
    description: "Eleven fixtures.pause_nmi tests are registered with fixtures and portable labels (five mailbox runs, the Game Gear run, closed_form, three judge controls, the wrong-profile control) and listed in both inventories; closed_form recomputes the five tick distances against literals and the band edges from the listing's constants and compares them with the csv rows"
    requirement: REG-02
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci -R '^fixtures\\.pause_nmi' exit 0, 11 of 11; ctest -N prints Total Tests: 11; grep -c '^fixtures\\.pause_nmi\\.' prints 11 for ci.txt and wasm32.txt"
        status: pass
    human_judgment: false
  - id: C4
    description: "Controls: a core that asserts the NMI one line late moves n by three and V from 00FF to 0000 and fails the five mailbox tests; halving the lower slack fails closed_form; changing the body length in the test constants and regenerating the csv leaves closed_form green and fails the five mailbox tests; the judge and profile controls pass because the judge fails"
    requirement: REG-02
    verification:
      - kind: unit
        ref: "src/vdp.c vdp_nmi_line lines - 2u to lines - 1u with a rebuild: ctest -R '^fixtures\\.pause_nmi' exit 8, five mailbox tests failed, record 01=015D outside 015A-015B and 02=0000 outside 00FF; restored, git diff --quiet origin/main -- include src exit 0; pz_s_lo halved: ctest -R fixtures.pause_nmi.closed_form exit 8 (csv 103-104 computed 104-104); pz_body 78 to 79 with the csv regenerated: closed_form 100 percent, five mailbox tests exit 8; every file restored from a saved copy, 11 of 11"
        status: pass
    human_judgment: false
  - id: C5
    description: "The hash file prints ten mailbox rows (V and Pause, NTSC 192 and 224, PAL 192, 224 and 240) each judged against expected.csv, three PAL device rows for the Pause images, and the PAL FM row (export console, unit forced on, 300 frames, 289,729 samples); the two sound pins gain exactly one line each and every older line, pin and hash row is unchanged"
    requirement: REG-02
    verification:
      - kind: integration
        ref: "ctest -R '^(runner\\.hashes_file|runner\\.hashes_compare|runner\\.hash_audio|fixtures\\.)' exit 0, 107 of 107; grep -c '^mailbox ' build/ci/hashes.txt prints 10; grep -c 'fm_sweep.sms sms pal fm-on' prints 2; grep -c '^playing pause_nmi' prints 3; git diff -U0 origin/main over the two pins has 0 removed and 1 added line each, 12 lines per file; hashes.cmake has 3 removed lines; the old-script comparison is an empty diff apart from the three continuation lines of the new device rows"
        status: pass
    human_judgment: false
  - id: C6
    description: "Negative controls of the hash rows: one expectation row of vdp_vtable_192 pal changed in expected.csv makes runner.hashes_file fail naming the rom and profile; one hex digit of the new transitions pin makes it name that line"
    requirement: REG-02
    verification:
      - kind: unit
        ref: "AF26 to AF27: ctest -R '^runner\\.hashes_file' exit 8 with 'mailbox: vdp_vtable_192 (pal)' and 'record 02=AF26 is outside its row'; last digit of the fm-on transitions line changed: exit 8 with 'audio-transitions line 7 differs'; both files restored, exit 0"
        status: pass
    human_judgment: false
  - id: C7
    description: "Dispatch D1 of ci.yml on head 1c720da is green on all eight lanes (six build legs, be32, wasm32) with asan, nofp, retroarch, perf and hash-equality, and the first hosted runs of the hygiene read-once and parallel analyzer, the runner --fm and audio tests, the ledger parser and the new fixtures"
    requirement: REG-02
    verification:
      - kind: e2e
        ref: "https://github.com/szTheory/cogmaster/actions/runs/43111533403 conclusion success; hash-equality job https://github.com/szTheory/cogmaster/actions/runs/43111533403/job/130830634062 success; jq checks of the plan exit 0"
        status: pass
    human_judgment: false
  - id: C8
    description: "The draft pull request exists and its pull_request run, the first real run of the ledger bridge, the rehearsal and the title job, printed the migration line and the protected comparison line and ended green (after one rerun of the cleanup job)"
    requirement: REG-02
    verification:
      - kind: e2e
        ref: "https://github.com/szTheory/cogmaster/pull/31 (draft); https://github.com/szTheory/cogmaster/actions/runs/43112269415 attempt 2 success; the log holds 'compat: ledger format migration' and 'compat: protected comparison passed'; title, rehearsal, cleanup and CI required success"
        status: pass
    human_judgment: false
  - id: C9
    description: "docs/conformance.md names what is still missing and describes pause_nmi (+389 bytes, 31,938); docs/audio.md names the PAL FM row (+123, 16,539); docs/testing.md names the mailbox rows and the device-row region (+254, 31,936); the counted check code stays 852 and include and src do not change"
    requirement: REG-02
    verification:
      - kind: integration
        ref: "wc -c of the three files; planning-identifier grep over added docs lines prints 0; cmake -DROOT=$PWD -P tests/cmake/budgets.cmake exit 0 (852 check lines, 9402 src lines); cmake --workflow --preset ci exit 0 (651 tests plus 5 release); cmake --workflow --preset hygiene exit 0 (72 tests)"
        status: pass
    human_judgment: false

duration: about 35 minutes
completed: 2026-11-11
---

# Phase 11 Plan 07: The Pause NMI fixture, lane hash rows and the first hosted run Summary

A self-authored Z80 program counts between the frame interrupt and the Pause NMI on PAL and NTSC and reports the count, V and the NMI count; the count lies inside a band computed from the listing's cycle counts; the V and Pause fixtures and a PAL FM stream are now printed by every lane and compared, dispatch D1 is green on all eight lanes, and the draft pull request that ship will reuse has run the ledger bridge for real. REG-02 is complete.

## Accomplishments

- `tests/roms/src/pause_nmi.asm` (204 lines): IM 1, a frame handler that zeroes the 16-bit count `n` and counts frames (76 cycles, shadow registers hold its two constants), a main loop that adds one to `n` in a 78-cycle body guarded by `DI` and `EI` (no lost update), and an NMI handler that copies `n` and V on its first call only and counts its calls. Records: `n` at the first NMI, V, the NMI count after a held press (frames 3 to 7, one NMI) and at the end (a second press at frame 20).
- The band: `n` in `[floor((C - 132) / 78), floor((C - 36) / 78)]`, C the cycle count of `(lines - 2 - H) x 684 + 4` ticks divided by 3. The header derives 132 and 36 from the listing: longest wait 59 (+1 rounding), IM 1 accept 13, handler 76, first increment 42 to 61 after the resume, longest instruction 16 (+1). `fixtures.pause_nmi.closed_form` recomputes the five tick distances against the literals 46,516, 24,628, 81,400, 59,512 and 48,568 and the five bands against the csv.
- Eleven tests (`fixtures.pause_nmi.mailbox.{ntsc.192,ntsc.224,pal.192,pal.224,pal.240}`, `.gg`, `.closed_form`, `.judge.{wrong_value,missing_row,hang}`, `.control.wrong_profile`), `pause.input` generated beside `play.input` (40 records, Unix newlines), three image digests, one manifest line, inventories.
- `tests/cmake/hashes.cmake`: a `mailbox` row kind (digest of the printed records, judged inside the writer against expected.csv in a function scope), an optional trailing region on device rows (three Pause rows on `pal`), and an optional ninth `fm` field on sound rows (one new row: `fm_sweep.sms` export, PAL, unit forced on). The two sound pins grew by exactly one line each (12 lines); `runner.hashes_file` requires the new rows.
- Docs: conformance (the Pause counter sentence corrected, a short pause_nmi section), audio (the PAL FM row), testing (mailbox rows and the device-row region).

## Band table (record 01, hand runs and the tests)

Command: `build/ci/cogmaster-run build/ci/fixtures/pause_nmi_<h>.sms --region pal|ntsc-u --frames 40 --input build/ci/tests/pause.input --mailbox`, exit 0, status 00 on every run.

| Profile | ticks | cycles C | band (hex) | observed | from low edge | from high edge |
|---|---:|---:|---|---:|---:|---:|
| PAL 192 | 81,400 | 27,133 | 346-347 (015A-015B) | 346 (015A) | 0 | 1 |
| PAL 224 | 59,512 | 19,837 | 252-253 (00FC-00FD) | 253 (00FD) | 1 | 0 |
| PAL 240 | 48,568 | 16,189 | 205-207 (00CD-00CF) | 206 (00CE) | 1 | 1 |
| NTSC 192 | 46,516 | 15,505 | 197-198 (00C5-00C6) | 197 (00C5) | 0 | 1 |
| NTSC 224 | 24,628 | 8,209 | 103-104 (0067-0068) | 104 (0068) | 1 | 0 |

Every run also gave V = 00FF, 03 = 0001 and 04 = 0002; the Game Gear native run (`--machine gg --video-mode gg`, 192 image) gave 0000 for all four. The band is 1 to 2 bodies wide against about 2.9 bodies in one line (228 cycles), so a core one line late is outside it by arithmetic; the observed values sit on or next to an edge on every profile, as a band from a worst-case derivation should, and no edge was moved to fit. Image digests (SHA-256): `pause_nmi_192` `9ed3c34c...cea04`, `pause_nmi_224` `2f22ef9c...2997a`, `pause_nmi_240` `25cff5b0...e6863` (full values in `tests/built/expected.txt`).

## Local evidence and negative controls

- Task 1 verify: `cmake --preset ci` and `cmake --build --preset ci` exit 0, `ctest -R '^fixtures\.pause_nmi'` 11 of 11, `git diff --quiet origin/main -- include src` exit 0. Two clean builds: `shasum -a 256` listings equal (`cmp` exit 0). `grep -Ec '^[[:space:]]*(di|ei)([[:space:]]|;|$)'` prints 8 and the header states the wait inequality (59 at most 78).
- Control 1 (one-shot, never committed): `src/vdp.c` `vdp_nmi_line` `lines - 2u` to `lines - 1u` (`git diff --stat` 1 line), rebuild exit 0, `ctest -R '^fixtures\.pause_nmi'` exit 8 with the five mailbox tests failed and `record 01=015D is outside its row ... (015A-015B)`, `record 02=0000 is outside its row ... (00FF)`; file restored from a saved copy, `git diff --quiet origin/main -- include src` exit 0, 11 of 11 again.
- Control 2: `pz_s_lo` halved in the test constants: `ctest -R fixtures.pause_nmi.closed_form` exit 8 (`csv 103-104 computed 104-104`); restored, exit 0.
- Control 3: body constant 78 changed to 79 in the test constants with the csv bands regenerated from the printed closed form (194..195, 102..103, 341..343, 249..250, 203..204): `closed_form` stays green and the five mailbox band tests fail (gg, judge and profile controls stay green); both files restored from copies, 11 of 11.
- Task 2 verify: `ctest -R '^(runner\.hashes_file|runner\.hashes_compare|runner\.hash_audio|fixtures\.)'` 107 of 107; `grep -c '^mailbox ' build/ci/hashes.txt` 10; `grep -c 'fm_sweep.sms sms pal fm-on'` 2; `grep -c '^playing pause_nmi'` 3; the new PCM line has 289,729 samples (the PAL closed form, equal to the existing PAL pcm count).
- Pin diffs against origin/main: transitions.txt and pcm.txt each 0 removed, 1 added, 12 lines. `hashes.cmake` has 3 removed lines (the device-row match, its execute_process and its output line). The older output is unchanged: the origin/main `hashes.cmake` run on the same build gives a file whose lines, filtered of mailbox, pause_nmi, fm-on and fixture lines, differ only by three `lag_frames=40` continuation lines of the new device rows; the fixture lines lose nothing.
- Hash-row controls: `vdp_vtable_192` PAL row `AF26` to `AF27` in a copy of expected.csv, `ctest -R '^runner\.hashes_file'` exit 8 naming `mailbox: vdp_vtable_192 (pal)` and `record 02=AF26 is outside its row`; one digit of the new transitions line changed, exit 8 `audio-transitions line 7 differs`; restored, exit 0.
- Full workflows after each task: `cmake --workflow --preset ci` exit 0 (651 tests plus 5 release) and `cmake --workflow --preset hygiene` exit 0 (72 tests). `cmake -DROOT=$PWD -P tests/cmake/budgets.cmake` exit 0, 852 check lines / 9,402 src lines (ceiling 894, unchanged).

## Local proxies

- wasm32 compare: not run, wasm32 does not configure here (`cmake --workflow --preset wasm32` exit 1, no `wasm-ld`).
- mawk page proxy: `docker run --rm -v "$PWD:/src" -w /src ubuntu:26.04 sh -c 'sh scripts/compat-common.sh docs tests/compat/ledger.csv | cmp - docs/compatibility.md'` exit 0.
- Container compile of changed C: not needed, no C file changed in this plan.

## Hosted runs

- D1: `gh workflow run ci.yml --ref phase/11-pal-and-japanese-fm` on head 1c720da (pushed first), run https://github.com/szTheory/cogmaster/actions/runs/43111533403, `gh run watch --exit-status` exit 0, conclusion success, head SHA equal to HEAD. All jobs success except the PR-only `title`, `rehearsal` and `cleanup`, skipped on a dispatch. The six `build` legs (linux-x64, linux-arm64, macos-x64, macos-arm64, windows-x64, windows-arm64), `be32`, `wasm32`, `asan`, both `nofp`, `retroarch`, `perf`, `hygiene` and `CI required` are success; `hash-equality` (the REG-02 eight-lane equality, 8 s) https://github.com/szTheory/cogmaster/actions/runs/43111533403/job/130830634062 success. No DNS failure, no rerun, no fix commit.
- Hosted timing, first sight of plan 01: the hygiene job took 43 s (22:24:45 to 22:25:28) against the earlier hosted 138 s; `hygiene.analyzer` 31.25 s against 47 s and `hygiene.tree` 24.82 s against 28 s (run 43111533403); the pull request run gave 50 s, 27.19 s and 20.86 s. First hosted sight of the new tests: all passed; on be32 `runner.hashes_file` took 47.43 s under its provisional 30 s floor and the 8x lane scale (passed; plan 09 settles it), and the Pause tests 0.5 to 0.7 s there and 0.09 s on Linux x64.
- Draft pull request: https://github.com/szTheory/cogmaster/pull/31, number 31, `gh pr create --draft`, title `feat: judge PAL timing and Japanese FM in the compatibility ledger` (`sh scripts/check-title.sh "<title>" < body` exit 0), still a draft and OPEN; never edited, readied or merged.
- Pull request run: https://github.com/szTheory/cogmaster/actions/runs/43112269415 (head 1c720da, attempt 1 red on `cleanup`, attempt 2 `gh run rerun 43112269415 --failed` success). The log of the build (linux-x64) job step "Protected compatibility comparison" holds `compat: ledger format migration` and `compat: protected comparison passed` (the bridge ran on the real v1 base ledger and the v2 head ledger and passed). Jobs: `title` success, `rehearsal` success, `cleanup` success on the rerun, `hash-equality` success, `CI required` success (job https://github.com/szTheory/cogmaster/actions/runs/43112269415/job/130836722225); no Nightly run started.
- The attempt-1 `cleanup` failure was not DNS: `publish: tag rehearsal-43112269415-1 still exists` right after the delete, while the tag and the release were gone a minute later and the previous nine pull request runs passed. It is a read-after-delete race in `rehearsal_delete`; the rerun was the answer, the finding is in `deferred-items.md`, no workflow or script was edited.

## Deviations from Plan

**1. [Rule 1 - Derivation] The band is two edges, derived with the handler length in the lower edge**
- **Found during:** Task 1, designing the listing
- **Issue:** the plan's closed form `[floor((C - S) / B), floor(C / B)]` takes S as the worst latency before the zeroing store and counts the NMI handler's cycles before its capture. In the listing the main loop is stopped for the whole frame handler, after the zeroing store too, and while the NMI handler runs, so n cannot change in the NMI handler and the capture cycles do not matter; the handler's remaining cycles do.
- **Fix:** `S_LO = (wait + 1) + accept + handler + first_increment_max - B = 132` and `S_HI = accept + handler + first_increment_min - (longest + 1) - B = 36`, both from the listing's comments, kept as named constants in the test beside the five tick literals; B is 78 (the guard and the frame-count compare), not 31.
- **Files modified:** tests/roms/src/pause_nmi.asm, tests/fixtures/tests.cmake
- **Commit:** f75af8a

**2. [Rule 3 - Blocking] A VIDEO parameter for the mailbox case**
- **Found during:** Task 1
- **Issue:** the native Game Gear run needs `--video-mode gg`, which the script-mode mailbox case could not pass.
- **Fix:** an optional VIDEO variable appended as `--video-mode` (four lines).
- **Files modified:** tests/fixtures/tests.cmake
- **Commit:** f75af8a

**3. [Not a code fix] Cleanup flake on the pull request run**
- **Found during:** Task 3, the first real pull request run
- **Issue:** `cleanup` failed with "tag ... still exists" although the tag was already deleted (read-after-delete race), not a DNS error.
- **Fix:** `gh run rerun 43112269415 --failed` (cleanup and the required roll-up only); recorded in `deferred-items.md` as an out-of-scope release-path finding.
- **Files modified:** .planning/phases/11-pal-and-japanese-fm/deferred-items.md
- **Commit:** this plan's summary commit

**Total deviations:** 3 (one derivation difference, one blocking parameter, one rerun). **Impact:** none on scope; no core, workflow or script file changed.

## Known Stubs

None.

## Threat Flags

None. The new surface is an owned assembly source, test and hash-file code, and the runs of one dispatch and one draft pull request; no secret or network path was added.

## Self-Check: PASSED

- Files found: tests/roms/src/pause_nmi.asm, tests/roms/src/pause_nmi.link, .planning/phases/11-pal-and-japanese-fm/deferred-items.md.
- Commits found: f75af8a, 1c720da (ancestors of HEAD, both pushed).
