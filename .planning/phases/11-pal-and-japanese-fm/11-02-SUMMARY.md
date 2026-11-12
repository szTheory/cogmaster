---
phase: 11-pal-and-japanese-fm
plan: "02"
subsystem: runner
tags:
  - runner
  - fm
  - audio
  - pal
status: complete

requires:
  - 11-01
provides:
  - cogmaster-run --fm auto|on|off and the fm= field of the execution line
  - the audio@F=H expectation (kind 4) over the merged transition stream, one drain per frame
  - an exact per-frame sample count, floor(f x N / D) minus floor((f - 1) x N / D), on NTSC and PAL
affects:
  - 11-03
  - 11-05
  - 11-07

actuals:
  tokens: 7100
  tasks: 3
  commits: 3

plan_head_before: cab5ee2a459935087823a97f502be83b07c2f491
plan_head_after: aba0a5c14693662d8ed94946324092b1fdd189fa
commits: 3

tech-stack:
  added: []
  patterns:
    - "A running SHA-256 context is copied and finalised, so one run reads the same stream at several frames"
    - "A cumulative closed form in uint64_t replaces a tolerance of one sample"

key-files:
  created: []
  modified:
    - runner/args.c
    - runner/args.h
    - runner/main.c
    - runner/audio.c
    - runner/audio.h
    - runner/expect.c
    - runner/expect.h
    - runner/input.c
    - runner/input.h
    - docs/runner.md
    - tests/runner/tests.cmake
    - tests/runner/expect.cmake
    - tests/inventory/ci.txt
    - tests/inventory/wasm32.txt

key-decisions:
  - "--fm is parsed once and strictly like --port1; the core's profile refusal makes --fm on with a Game Gear exit 77, so the runner adds no second rule"
  - "main.c enables the trace and opens the audio object when --hash-audio or any audio case exists, drains once per frame, and hands the hash only to a frame that has a case"
  - "An audio case with reset=1 in the --input script is refused at load with exit 2, because a reset restarts the ticks the merge checks"
  - "The per-frame sample check is the cumulative closed form for both TVs; no tolerance of one sample remains"

requirements-completed: []

coverage:
  - id: C1
    description: "--fm auto|on|off is parsed once and strictly; auto resolves to on only for a Japanese Master System; the execution line reports fm= right after tv=; on with a Game Gear exits 77; any other word, an empty or missing word, a repeat and --probe exit 2; auto and no flag give one hash on every region"
    requirement: REG-03
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^runner\\.usage\\.fm$' exit 0; with media.fm forced to 0 the same test exits 8, restored exit 0"
        status: pass
    human_judgment: false
  - id: C2
    description: "audio@F=H is kind 4: 64 lowercase hex, F from 1 to --frames, judged against the merged transition stream of frames 1 to F; a flipped nibble, the 300 frame hash at frame 60, the PCM digest, the Japanese stream judged with the unit forced off and a PAL stream judged on NTSC all exit 3; malformed forms exit 2"
    requirement: REG-03
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^runner\\.expect\\.(audio|predicates)$' exit 0 (2 tests)"
        status: pass
    human_judgment: false
  - id: C3
    description: "One drain per frame: audio cases and --hash-audio in one run print the standalone audio-transitions line; the trace is on for an audio case alone; an audio case with a reset script exits 2 before any frame; a small-cap runner exits 1, not 3"
    requirement: REG-03
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^runner\\.expect\\.audio$' exit 0; copy-instead-of-original mutation of the hex function exits 8; trace left off for an audio case alone exits 8; both restored exit 0"
        status: pass
    human_judgment: false
  - id: C4
    description: "Every frame holds exactly floor(f x N / D) minus floor((f - 1) x N / D) samples (NTSC 3,504,512 / 4,375, PAL 685,094,400 / 709,379); the card's totals for F in 1, 2, 3, 50 and 60 on both TVs equal the CMake closed form and the literal anchors; a total one higher fails"
    requirement: REG-02
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^runner\\.audio_cadence' exit 0 (2 tests, the self-test expects exit 1 and passes); PAL denominator raised by one exits 8 and runner.hashes_file also fails; restored exit 0"
        status: pass
    human_judgment: false
  - id: C5
    description: "No file under include or src changed and no pin moved: API 14, behaviour 16 and state 16 stay, tests/audio is untouched, runner.hashes_file and runner.hash_audio stay green, the counted check code stays 845 and docs/runner.md is 34,054 bytes"
    requirement: REG-02
    verification:
      - kind: integration
        ref: "git diff --quiet origin/main -- tests/audio tests/testcard_pins.cmake include src exit 0; cmake -DROOT=$PWD -P tests/cmake/budgets.cmake exit 0 (845 check lines); cmake --workflow --preset ci exit 0 (627 tests); cmake --workflow --preset hygiene exit 0 (71 tests)"
        status: pass
    human_judgment: false

duration: about 75 minutes
completed: 2026-11-11
---

# Phase 11 Plan 02: Runner --fm, audio@F=H and exact samples Summary

The runner can now be told the FM state (`--fm`), names it on the execution line (`fm=`), judges the merged transition stream at chosen frames in one run (`audio@F=H`, kind 4) and holds every frame to its exact cumulative sample count on both TV standards. Nothing under `include/` or `src/` changed.

## Accomplishments

- `--fm auto|on|off`: parsed like `--port1` (once, exact lowercase words, error text "--fm needs auto, on or off, once"), default auto, added to the `--probe` conflict list and to the synopsis; `media.fm` carries it to the core, so `--fm on --machine gg` exits 77 by the core's own refusal. The execution line reads `... tv=T fm=F video=...` (fm 1 off, 2 on); `runner.device.names` is the one existing regex that spanned `tv=` and `video=` and was updated.
- `audio@F=H`: kind 4 shares the `=` plus 64 lowercase hex branch of `frame@` (the unknown-kind sentinel is 5); `cogmaster__expect_has_audio` and `cogmaster__expect_audio_at` tell main.c which frames need the hash; `cogmaster__expect_frame` takes `audio_hex` and `transitions`; the line is `predicate kind=4 frame=F address=0000 pass|fail observed_audio=<hex> transitions=<count>`.
- `cogmaster__audio_transitions_hex` copies the running SHA-256 context and finalises the copy (`cogmaster__audio_print` uses it too), so one run holds several F and `--hash-audio` beside audio cases prints the standalone line.
- main.c: trace and audio object are enabled by `--hash-audio` or any audio case; `cogmaster__audio_frame` is called once per frame; the two `audio-` lines print only with `--hash-audio`. `cogmaster__input_validate` gained `reset_seen`; an audio case with a reset in the script prints "an audio expectation cannot follow a reset in the --input script" and exits 2 before any frame.
- Exact samples: `frame_samples` became `rate_num` and `rate_den`; `hash_pcm` fails the frame, exit 1, unless it holds `f x N / D - (f - 1) x N / D` samples, with the message "frame N holds M samples, not W".
- docs/runner.md: the `--fm` row, the `fm=` clause, the audio record with its output line, exits and reset refusal, the sentence that an audio record only locks our own output, the PAL closed form beside the NTSC one and the exact per-frame rule.

## Task Commits

1. Task 1, `--fm` and the `fm=` field: `9381a6b`
2. Task 2, `audio@F=H`: `fdbebaa`
3. Task 3, exact samples: `aba0a5c`

## Evidence

All commands from the repository root on this Mac, in this working tree. No hosted run belongs to this plan.

Tests (each `ctest --test-dir build/ci -R <pattern>`, exit 0): `^runner\.usage\.fm$` (1 test), `^runner\.expect\.(audio|predicates)$` (2), `^runner\.audio_cadence` (2), `^runner\.(device|exit\.table|hashes_file)` (9 of 9), `^runner\.(usage|expect|exit|hash|device)` (26 of 26 after task 1). `cmake --workflow --preset ci` exit 0 (627 of 627, after each task) and `cmake --workflow --preset hygiene` exit 0 (71 of 71, after each task).

Hand runs (build/ci/cogmaster-run, card, `--frames 2 --expect` of `ram@2:$C200=00`): `--region ntsc-j --fm on` exit 0, `region=2 tv=1 fm=2`; `--fm off` exit 0, `fm=1`; `--region ntsc-j` alone `fm=2`; `--region ntsc-u` and `--region pal` alone `fm=1`; `--region ntsc-u --fm on` `fm=2`; `--machine gg --fm off` `fm=1`. `--fm sideways`, `--fm`, `--fm ''`, `--fm ON`, `--fm 1`, `--fm on --fm off` and `--probe x --fm on` each exit 2; `--fm on --machine gg` exits 77. `--help | grep -c -- --fm` prints 1.

Audio by hand: `fm_sweep.sms --region ntsc-j --frames 300 --hash-audio` exit 0 prints `audio-transitions 300 count 86278 sha256 add14027...`, equal to the line read from tests/audio/transitions.txt by `grep` and `sed`; an expectation `audio@300=<that hash>` exits 0; the same with the last digit changed to 0 exits 3. A file holding `audio@60=`, `audio@300=` and `frame@300=` exits 0 with two kind=4 lines and prints the same audio line as the standalone run with `--hash-audio`. `--region pal --frames 5 --hash-audio` exit 0, `samples 4828`.

Negative controls (python edit of the source, `git diff` / `diff` against the saved copy to confirm the edit, rebuild, ctest, restore from the saved copy, rebuild, ctest):

- `--fm`: `media.fm = 0u` in main.c (`diff` showed the single line): `ctest -R '^runner\.usage\.fm$'` exit 8; restored exit 0.
- Hex function finalises the original instead of a copy (the const parameter dropped in audio.c and audio.h so it compiles; a first attempt that cast away const failed to build under -Wcast-qual and was discarded): `runner.expect.audio` exit 8 (frame 60 passes, frame 300 fails); restored exit 0.
- main.c leaves `cfg.trace_records` unset when only an audio case exists: `runner.expect.audio` exit 8 with "cogmaster_get_audio_trace failed with status 1" (see deviations); restored exit 0.
- PAL denominator 709,379 changed to 709,380: `runner.audio_cadence` exit 8 ("frame 47 holds 966 samples, not 965" during the 50 frame PAL run), `runner.hashes_file` also fails, `runner.hash_audio`, `runner.hash_audio.fm` and the self-test still pass (they use NTSC rows or expect the failure); restored exit 0.
- `hash_pcm` accepting `want` or `want + 1` again: `runner.audio_cadence`, `runner.hash_audio` and `runner.hashes_file` all still pass (the cadence measures totals), so per-frame exactness is proved by the denominator control and the small-cap test, which now also asserts `not 801`; restored.

Checks: `docker run ... gcc:13 gcc -std=c17 -Wall -Wextra -Wpedantic -Werror -Wtype-limits -Wconversion -fsyntax-only -I runner -I host -I src -I include -I build/ci/generated -D_POSIX_C_SOURCE=200809L` over runner/args.c, main.c, audio.c, expect.c and input.c exit 0 (task 1 files after task 1, all five after tasks 2 and 3 for audio.c). `/opt/homebrew/opt/llvm@21/bin/clang-format --dry-run --Werror runner/*.c runner/*.h` exit 0. `git diff --quiet origin/main -- tests/audio tests/testcard_pins.cmake tests/io tests/vdp tests/bus tests/ports include src` exit 0. `git diff --numstat origin/main -- tests/inventory/ci.txt tests/inventory/wasm32.txt` prints `4 0` for each. Planning-identifier grep over added docs lines prints 0.

Counted check code: `cmake -DROOT="$PWD" -P tests/cmake/budgets.cmake` exit 0, 845 check lines and 9,402 src lines after every task (baseline 845; no counted file touched).

Docs bytes (`wc -c docs/runner.md`): 32,710 before; 32,986 after task 1 (bound 33,300); 33,800 after task 2 (bound 33,800); 34,054 after task 3 (bound 34,300).

## Deviations from Plan

### Interpretations (no behaviour lost)

- **gcc:13 include path.** The plan's command lacks `-I host`; runner/main.c includes `bios.h`, which lives in `host/`. The check ran with `-I host` added.
- **Trace control fails with exit 1, not an empty-stream mismatch.** With the trace left off, the core refuses the log read of a zero-capacity request (`cogmaster_get_audio_trace` status 1) and the runner exits 1 before any hash. The control is red either way; the plan expected a mismatch.
- **Synopsis text.** `--expect FILE` now reads "frame/RAM/mailbox/boot/audio predicates" (one word), so the help does not omit the new kind.
- **Empty --fm word.** An empty string does not survive CMake's expanded argument list, so the test passes it as a literal argument of its own `execute_process`.
- **docs/runner.md task 2 bound.** The audio paragraph was trimmed to land exactly on the 33,800 byte bound; task 3 added the rest within 34,300.
- **First mutation attempt.** Cast-away-const mutation of the hex function did not compile (-Wcast-qual); a dropped-const signature made it compile (noted above).
- **No `state advance-plan`.** It fails with `plan_position_unreadable`; the Plan line was set by hand as plan 11-01 did.

**Total deviations:** 0 rule-driven fixes; 7 interpretations above. **Impact:** none on the plan's outcome.

## Known Stubs

None.

## Threat Flags

None. T-11-04 (trace bound) is unchanged: the buffer stays 32,768 records, drained once per frame, with the dropped-record, backward-tick and sample-count exits still 1. T-11-05 is mitigated by the reset refusal and its test. T-11-06 is mitigated by the doc sentence and the two profile controls (Japanese stream with the unit forced off, PAL stream on NTSC), both exit 3. T-11-07 is mitigated by the denominator control.

## Notes for the next plans

- Plan 05 can put `--fm` in the ledger row identity and read the execution line `region= tv= fm=`; the raw values are region export 1, japan 2; tv ntsc 1, pal 2; fm off 1, on 2.
- `audio@F=H` accepts F up to `--frames` and is unsupported with `--boot bios`, `--trace`, `--mailbox` and `--bench` like every `--expect` record.
- docs/runner.md has 246 bytes of room under its 34,300 bound.

## Self-Check: PASSED

- Commits `9381a6b`, `fdbebaa` and `aba0a5c` are ancestors of HEAD (`git rev-list --count cab5ee2..HEAD` prints 3) and each message ends with the attribution trailer.
- The edited files exist, `git diff --quiet origin/main -- include src tests/audio tests/testcard_pins.cmake` exits 0 and the test names registered by this plan appear in tests/inventory/ci.txt and wasm32.txt.
