---
phase: 04-sound
plan: "11"
subsystem: sound-fm-tests
tags: [ym2413, opll, lockstep, frozen-digest, unit-tests, sum-bound]

requires:
  - phase: 04-sound
    provides: the integer YM2413 port, the one-shot oracle and the seeded register stream (plan 04-10)
provides:
  - cogmaster-test-fm with fm.lockstep (the port against the original's frozen 2,000,000-sample digest, fetching nothing) and seven chip-level cases
  - tests/audio/opll_lockstep.txt, the digest frozen once from the unmodified emu2413 at the pin, naming the original's source by its SHA-256
  - COGMASTER__OPLL_TONE_PEAK, COGMASTER__OPLL_DRUM_PEAK and COGMASTER__OPLL_SUM_MAX in src/opll.h, with the bound of the sounding voices proved and the bound of the returned sum stated
affects: [04-12, 04-13, 04-14, 04-15]

actuals:
  tokens: 4733
  tasks: 2
  commits: 3
plan_head_before: 696e840e135adbdce930983a36b95c2fd4485507
plan_head_after: c9a38825ef95c397500da83d7cb6e95a13748246

tech-stack:
  added: []
  patterns:
    - "A frozen oracle digest in a text file, parsed at test time, replaces fetching the original: the test proves the port and the file names the original's source by its SHA-256"
    - "Chip behaviours are asserted against closed forms written in the test (the noise register's taps, the AM triangle, a phase step of 8 per pitch offset unit), and each case was shown to fail under a mutation of the port"

key-files:
  created:
    - tests/audio/test_opll.c
    - tests/audio/opll_lockstep.txt
  modified:
    - tests/audio/tests.cmake
    - tests/inventory/ci.txt
    - tests/inventory/wasm32.txt
    - src/opll.h
    - .planning/WINDOWS.md

key-decisions:
  - "The stream digest was re-run before freezing and equals the one plan 04-10 reported: build/opll-oracle stream 2000000 exited 0 and printed sha256 eb45e9b6cf41fcd554277ea1d7b3db6a3001f6853080c4c2c02b187c689e4ec9; the frozen file is that line plus the SHA-256 of emu2413.c (35d492c1...), and the oracle hashes (uint16_t) of the original's int16 sample, which the port's (uint16_t) of its int32 sum equals"
  - "COGMASTER__OPLL_SUM_MAX is 6 tone peaks plus 5 drum peaks (32,688), the largest sum of the voices that sound together; it is not a bound of the returned sum, which also holds the silent group's last outputs (38,817 over all fourteen peaks) and is therefore not inside int16"
  - "Per-voice peaks are measured, not assumed: a tone voice reaches exactly -2042 to 2043 (nine in phase give exactly 9 times that), a drum reaches -4086 to 4084, and the test fails if either constant is one off"

patterns-established:
  - "Test timeouts for the fm cases follow the audio and psg comments: a 10 s floor, the measured native and sanitizer cost, and the be32 and wasm32 scales marked provisional until a hosted run"

requirements-completed: [SND-02]

coverage:
  - id: D1
    description: "The port equals the unmodified original over 2,000,000 seeded register-stream samples in every lane: the SHA-256 of the port's int16 samples equals the digest frozen from the original at the pinned commit, and the digest file names emu2413.c by SHA-256"
    requirement: SND-02
    verification:
      - kind: unit
        ref: "tests/audio/test_opll.c#lockstep (ctest fm.lockstep)"
        status: pass
      - kind: other
        ref: "grep -c 35d492c180320bd473ece0fddca771ef54d9b25884a094468560f865f540abe5 tests/audio/opll_lockstep.txt prints 1"
        status: pass
    human_judgment: false
  - id: D2
    description: "Key-on starts the carrier in the damp state and the attack follows within two samples, the voice is audible within 128 samples, and a key-off releases it as a decay to full attenuation and silence"
    requirement: SND-02
    verification:
      - kind: unit
        ref: "tests/audio/test_opll.c#key_on (ctest fm.key_on)"
        status: pass
    human_judgment: false
  - id: D3
    description: "Each of the five rhythm keys sounds its own drum channel and no other; the noise register steps 18 times per sample with taps at bits 0 and 9 in both modes"
    requirement: SND-02
    verification:
      - kind: unit
        ref: "tests/audio/test_opll.c#rhythm (ctest fm.rhythm)"
        status: pass
      - kind: unit
        ref: "tests/audio/test_opll.c#noise (ctest fm.noise)"
        status: pass
    human_judgment: false
  - id: D4
    description: "The AM LFO is a 0 to 13 triangle with 13,440 samples to the period, the PM counter counts one per sample with the pitch offset moving a phase generator by 8 per unit, and bits 0 to 3 of the test register behave as upstream"
    requirement: SND-02
    verification:
      - kind: unit
        ref: "tests/audio/test_opll.c#lfo (ctest fm.lfo)"
        status: pass
      - kind: unit
        ref: "tests/audio/test_opll.c#test_register (ctest fm.test_register)"
        status: pass
    human_judgment: false
  - id: D5
    description: "One call evaluates exactly 18 slots under a register-hammering stream (slot_evals equals 18 times the samples after every sample)"
    requirement: SND-02
    verification:
      - kind: unit
        ref: "tests/audio/test_opll.c#slot_count (ctest fm.slot_count)"
        status: pass
    human_judgment: false
  - id: D6
    description: "The voices that sound together (nine tone, or six tone and five drums, at full level) sum within COGMASTER__OPLL_SUM_MAX, which fits int16, with the per-voice peaks reached exactly; the silent group's held outputs stay in the returned sum"
    requirement: SND-02
    verification:
      - kind: unit
        ref: "tests/audio/test_opll.c#sum_bound (ctest fm.sum_bound)"
        status: pass
    human_judgment: false
  - id: D7
    description: "The fm tests run clean under UndefinedBehaviorSanitizer and AddressSanitizer (every floor-shift path), and the full ci workflow (378 tests plus 4 release tests) and the hygiene workflow (38 tests) exit 0"
    requirement: SND-02
    verification:
      - kind: other
        ref: "cc -std=c17 -O1 -fsanitize=undefined,address -fno-sanitize-recover=undefined over tests/audio/test_opll.c, runner/sha256.c, src/opll.c, src/opll_reg.c: all eight cases exit 0"
        status: pass
      - kind: integration
        ref: "cmake --workflow --preset ci: exit 0; cmake --workflow --preset hygiene: exit 0"
        status: pass
    human_judgment: false

duration: 9min
completed: 2026-11-09
status: complete
---

# Phase 04 Plan 11: YM2413 Lockstep and Chip Units Summary

**fm.lockstep proves the integer YM2413 equals the unmodified emu2413 over 2,000,000 seeded samples against a digest frozen in a text file (nothing fetched), seven chip-level cases pin key-on, the drums, the noise register, both LFOs, the test register, the slot count and the sum of the sounding voices, and the run found that the sum the chip returns can leave 16 bits.**

## Performance

- **Duration:** about 9 min by the system clock (the start time was not captured)
- **Completed:** 2026-11-09T21:30:00Z
- **Tasks:** 2
- **Files modified:** 7 (2 created, 5 modified, counting WINDOWS.md)

## Accomplishments

- The oracle was rebuilt and its stream mode re-run before anything was frozen; the digest equals plan 04-10's report. `fm.lockstep` reads `tests/audio/opll_lockstep.txt`, runs the same seeded stream through the port, hashes `(uint16_t)` of each int32 sum low byte first, and compares: it passes, and fails (exit 1, both digests printed) against a file with one hex digit changed.
- Seven chip-level cases, each shown to fail under a mutation of the port (noise tap, AM period, PM step, each test-register bit, the damp rate, the key-off path, the drum key mapping, the hi-hat output, the slot evaluation count) and each clean under address and undefined-behaviour sanitizers.
- The per-voice peaks are measured: a tone voice spans -2042 to 2043 and nine in phase give exactly nine times that; with the test register holding every envelope at zero attenuation a drum reaches -4086 and 4084. `COGMASTER__OPLL_SUM_MAX` is 6 x 2043 + 5 x 4086 = 32,688 for the voices that sound together, below 32,767.
- Finding for the state and mixer plans: the returned sum is not inside int16 (see Deviations).

## Measured values and commands

Every command ran from the repository root.

| Check | Command | Result |
|---|---|---|
| Pin | `shasum -a 256 build/pins/emu2413/emu2413.c` | 35d492c180320bd473ece0fddca771ef54d9b25884a094468560f865f540abe5, exit 0 |
| Oracle build | `cc -std=c17 -O2 -Isrc -Itests/audio -Ibuild/pins/emu2413 tests/audio/opll_oracle.c src/opll.c src/opll_reg.c -lm -o build/opll-oracle` | exit 0 |
| Oracle stream | `build/opll-oracle stream 2000000` | exit 0; `samples 2000000 seed 0x4f504c4c32343133 sha256 eb45e9b6cf41fcd554277ea1d7b3db6a3001f6853080c4c2c02b187c689e4ec9` (0.29 s) |
| Digest source named | `grep -c 35d492c180320bd473ece0fddca771ef54d9b25884a094468560f865f540abe5 tests/audio/opll_lockstep.txt` | prints 1 |
| Task 1 verify | `cmake --preset ci && cmake --build --preset ci && ctest --test-dir build/ci --no-tests=error --output-on-failure -R '^fm\.lockstep$'` | exit 0, 1 of 1 (0.22 s native, 0.20 to 0.29 s over four runs) |
| Lockstep under sanitizers | `cc -std=c17 -O1 -g -fsanitize=undefined,address -fno-sanitize-recover=undefined -Isrc -Irunner -Itests/audio tests/audio/test_opll.c runner/sha256.c src/opll.c src/opll_reg.c -o <scratch>/fm-san` then `<scratch>/fm-san lockstep tests/audio/opll_lockstep.txt` | exit 0, 0.59 s |
| Other fm cases | `ctest --test-dir build/ci --no-tests=error --output-on-failure -L fm` | exit 0, 8 of 8; every case but lockstep under 0.05 s |
| Cases under sanitizers | the sanitizer build above run with each of key_on, rhythm, noise, lfo, test_register, slot_count, sum_bound | all exit 0 |
| Full workflows | `cmake --workflow --preset ci` and `cmake --workflow --preset hygiene` | both exit 0 (378 of 378 plus 4 of 4; 38 of 38) |

Measured `fm.lockstep` times: 0.22 s native; 0.59 s under the sanitizers; the TIMEOUT stays at the 10 s floor (twice the slowest observed is below it). The be32 and wasm32 legs were not run: `qemu-ppc` and the pinned wasmtime are not installed here and no hosted run includes the test yet, so the comment in `tests/audio/tests.cmake` marks the timeout provisional; its 8 and 5 times scales leave 80 s and 50 s against 1.8 s and 1.1 s at those multiples of the native cost.

## Task Commits

1. **Task 1: the port equals the original's frozen digest (tracer)** - `df67d62` (test)
2. **Task 2 (behaviours that already existed from plan 04-10): key-on, rhythm, noise, LFO, test register, slot count** - `62e172f` (test)
3. **Task 2 (sum bound): constants and fm.sum_bound** - `c9a3882` (feat)

**Plan metadata:** the commit that adds this summary, then the state and roadmap commit.

## Files Created/Modified

- `tests/audio/test_opll.c` - `cogmaster-test-fm`: lockstep and the seven chip-level cases
- `tests/audio/opll_lockstep.txt` - the frozen oracle line plus the SHA-256 of emu2413.c
- `tests/audio/tests.cmake` - the executable, `fm.lockstep` and the foreach of seven cases, each with its fault prevented and its measured timeout
- `tests/inventory/ci.txt`, `tests/inventory/wasm32.txt` - eight `fm.*` rows (fm,portable)
- `src/opll.h` - the two per-voice peaks and `COGMASTER__OPLL_SUM_MAX`, with what they do and do not bound
- `.planning/WINDOWS.md` - one open `unmet-truth` entry for the finding below

## Decisions Made

- The frozen file is the oracle's own output line plus a second field naming the source; the test parses the first three fields with `sscanf` and ignores the rest, so the file stays one readable line.
- `COGMASTER__OPLL_SUM_MAX` bounds what sounds together rather than what is returned (see Deviations 1); the held-output behaviour is pinned by a test instead of being hidden by a larger constant.

## Deviations from Plan

### Auto-fixed Issues

None needed a code fix. Three places where the plan's text did not match the chip, and one TDD note:

**1. [Rule 1 - Bug in the plan's claim] The returned sum does not stay inside int16**
- **Found during:** Task 2 (sum_bound)
- **Issue:** the plan says the worst-case native sum fits the int16 hash field. The chip returns the sum of all 14 channel outputs, and the group that is not sounding keeps its last outputs (as the original does, and as plan 04-10 recorded). With rhythm mode on, three tone outputs are held; with it off, five drum outputs are held. A scratch run with nine tones keyed off for voices 7 to 9, the test register at 1 (every envelope at zero attenuation) and all five drum keys on reached a sum of -36,095 (below -32,768), so the claim is false for the returned value. The sum of the voices that sound together is within 32,688.
- **Fix:** none to the chip, because the lockstep digest requires the original's behaviour (held outputs included). `COGMASTER__OPLL_SUM_MAX` is the bound of the sounding voices and is proved; the header states that the returned sum is bounded only by all fourteen peaks (38,817); `fm.sum_bound` asserts the held values and that the returned sum is the total of all fourteen. The original wraps the total to 16 bits and the port's `(uint16_t)` of it is the same value, so the hashes are unaffected.
- **For plan 04-12 and the mixer:** the source-5 level cast `(uint16_t)` of the sum is the original's wrap; a test that the worst-case level "stays inside the int16 hash field" cannot pass for the returned sum. Either keep the wrap (two extreme levels then share a hash value, and a hostile ROM can reach it only with the test register or a rhythm switch) or take the level from the sounding voices only; that choice is the state plan's to make. One `unmet-truth` entry in `.planning/WINDOWS.md` keeps it visible to the ship gate until it is fixed or waived.
- **Files modified:** src/opll.h (the comment), tests/audio/test_opll.c, .planning/WINDOWS.md
- **Committed in:** c9a3882 (the ledger entry is in the plan metadata commit)

**2. [Plan text] Key-on is audible after 89 samples, not within two**
- **Found during:** Task 2 (key_on)
- **Issue:** the plan's behaviour line says non-zero samples within two samples as upstream. What happens within two samples is the end of the damp state (the carrier is in damp on the first sample and attacking on the second); the instrument-1 attack then takes 89 samples to reach an audible level at volume 0.
- **Fix:** `fm.key_on` asserts the damp state on the first sample, no damp on the second, audible within 128 samples, and the release behaviour; the same stream is proved sample for sample by `fm.lockstep`.
- **Files modified:** tests/audio/test_opll.c
- **Committed in:** 62e172f

**3. [Plan text] The be32 and wasm32 timings were not read**
- **Found during:** Task 1
- **Issue:** the plan asks to read the two scaled timings from the first CI run or run one pass of each if cheap. Neither emulator is installed here, and no hosted run includes the test.
- **Fix:** the timeout is the 10 s floor with a provisional comment in the form the audio and psg tests use; the scales leave 80 s and 50 s against 1.8 s and 1.1 s of scaled native cost.

**TDD note (task 2, not a plan-level TDD plan, `workflow.tdd_mode` is off).** The six behaviour cases test code that plan 04-10 wrote, so they were green on the first run (an expected green, not a wrong test); each was proved non-vacuous by mutating a scratch copy of the port, and every mutation made its case exit 1. The sum_bound case was written first and failed to build (`use of undeclared identifier 'COGMASTER__OPLL_SUM_MAX'`) before the constants existed; that state was not committed, so the test and the constants share the feat commit.

---

**Total deviations:** 1 plan claim found false (no code change), 2 plan-text mismatches handled in the tests
**Impact on plan:** no scope change; the finding is carried to plan 04-12 by the ledger entry.

## Issues Encountered

The local `asan` preset needs clang-21, which is not installed here, so the sanitizer runs above use `cc` with `-fsanitize=undefined,address` over the same sources. The hosted `asan` lane runs every test but `abi` and `release`, so it includes the eight fm tests. `clang-format` is also absent; the hygiene workflow's formatting step passed regardless.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- Plan 04-12 can use `cogmaster__opll_reset`, `cogmaster__opll_write` and `cogmaster__opll_sample` as proved here; `slot_evals` is exactly 18 per sample. It must settle the source-5 level question in Deviations 1 before it writes the hash field's test.
- No stubs and no skipped tests; the one open ledger entry is the finding above.

## Self-Check: PASSED

`tests/audio/test_opll.c` and `tests/audio/opll_lockstep.txt` exist; commits df67d62, 62e172f and c9a3882 are ancestors of HEAD; `ctest -L fm` lists 8 passing tests; `grep -c 35d492c180320bd473ece0fddca771ef54d9b25884a094468560f865f540abe5 tests/audio/opll_lockstep.txt` prints 1.

---
*Phase: 04-sound*
*Completed: 2026-11-09*
