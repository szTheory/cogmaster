---
phase: 05-tune-up
plan: "08"
subsystem: comments
tags: [comments, docs, review-findings, pointers]
status: complete

requires:
  - phase: 05-tune-up
    provides: docs/perf-fuzz.md and docs/saves.md (plan 05-01)
provides:
  - frame-rate and SYNTH_EVENTS comments that state the hardware and cite their preparation sections
  - true zlib-window and nightly-report comments
  - code comments that point at the docs files that now hold the prose
affects: [05-09]

actuals:
  tokens: 1500
  tasks: 1
  commits: 1

plan_head_before: d9ad048d93c198402957311b453238ba44ea107b
plan_head_after: 02274334b60bcae9167ce5e095df257896ace193
commits: 1

tech-stack:
  added: []
  patterns:
    - "A hardware comment states what the hardware does and cites a preparation section on one line, so a grep finds the citation"

key-files:
  created: []
  modified:
    - src/instance.c
    - src/synth.c
    - src/battery.c
    - runner/png.c
    - .github/workflows/nightly.yml
    - docs/embedding.md

key-decisions:
  - "The nightly report comment keeps only the true reason (the job decides from needs.*.result) and drops the false claim about pipefail"
  - "The citation SMS-SYSTEM section 1 sits on one source line, so the plan's grep and a reader both find it"

requirements-completed: []

coverage:
  - id: P1-IN-01
    description: "The NTSC frame-rate comment states the 262 by 684 tick frame and the 315/88 MHz x 3 tick clock (about 10,738,636 Hz) and cites ARCHITECTURE section 2 and SMS-AUDIO section 1; docs/embedding.md carries the same figure"
    requirement: TUNE-07
    verification:
      kind: other
      ref: "git grep -q '10,738,635 Hz' -- src docs/embedding.md exits 1 (baseline: one match in each file); grep -q '315/88 MHz' src/instance.c exits 0; grep -q 'ARCHITECTURE section 2' src/instance.c exits 0"
      status: passed
    human_judgment: false
  - id: P1-IN-02
    description: "The runner zlib header comment says CMF 0x78 is CINFO 7, a 32 KiB window, citing RFC 1950 section 2.2"
    requirement: TUNE-07
    verification:
      kind: other
      ref: "grep -q '256-byte' runner/png.c exits 1 (baseline: 1 match); grep -q '32 KiB' runner/png.c exits 0"
      status: passed
    human_judgment: false
  - id: P1-IN-03
    description: "The nightly report job comment keeps only the true reason, that the job decides from the jobs' results"
    requirement: TUNE-07
    verification:
      kind: other
      ref: "grep -q 'has no pipefail' .github/workflows/nightly.yml exits 1 (baseline: 1 match); actionlint -ignore 'label \"ubuntu-26.04' .github/workflows/nightly.yml exits 0"
      status: passed
    human_judgment: false
  - id: P4-IN-01
    description: "The SYNTH_EVENTS comment gives the corrected premise (PSG steps every 48 ticks, a port write is at least 33 ticks and changes at most five sources, a YM2413 sample every 216 ticks, about 50 changes under 128) and cites SMS-SYSTEM section 1 and SMS-AUDIO section 5"
    requirement: TUNE-07
    verification:
      kind: other
      ref: "grep -q 'PSG step every 16 ticks' src/synth.c exits 1 (baseline: 1 match); grep -q '48 ticks' src/synth.c exits 0; grep -q 'SMS-SYSTEM section 1' src/synth.c exits 0"
      status: passed
    human_judgment: false
  - id: POINTERS
    description: "The nightly.yml header comment points at docs/perf-fuzz.md and the src/battery.c header comment points at docs/saves.md, the files plan 05-01 moved the prose to"
    requirement: TUNE-07
    verification:
      kind: other
      ref: "grep -q 'docs/testing.md, \"Performance\"' .github/workflows/nightly.yml exits 1 (baseline: 1 match); grep -q 'docs/perf-fuzz.md' .github/workflows/nightly.yml exits 0; grep -q 'docs/saves.md' src/battery.c exits 0"
      status: passed
    human_judgment: false
---

# Phase 5 Plan 08: Comments that cite their sources Summary

Five comment corrections and two pointer fixes from the phase 1 and phase 4 review findings, as one `docs:` commit, `0227433`. No code path changed.

## Accomplishments

- `src/instance.c`: the NTSC frame-rate comment now says a frame is 262 lines of 684 ticks, that the tick is a third of a Z80 cycle, that the tick clock is 315/88 MHz x 3 (about 10,738,636 Hz) and that the rate reduces to 1,640,625 / 27,379 Hz, citing ARCHITECTURE section 2 and SMS-AUDIO section 1. `docs/embedding.md` carries the same figure. The old 10,738,635 figure had no other copy in `src` or `docs`.
- `runner/png.c`: the zlib header comment says a 32 KiB window (CINFO 7), not 256 bytes.
- `.github/workflows/nightly.yml`: the report job comment no longer claims the default shell has no pipefail; the header comment points at `docs/perf-fuzz.md`.
- `src/synth.c`: the `SYNTH_EVENTS` comment uses the corrected premise and cites SMS-SYSTEM section 1 and SMS-AUDIO section 5. The int64 bound reasoning above it is unchanged.
- `src/battery.c`: the header comment points at `docs/saves.md`.

## Evidence

Baselines before editing (from the repository root): `git grep -c '10,738,635 Hz' -- src docs` one match in each of `docs/embedding.md` and `src/instance.c`; `grep -c '256-byte' runner/png.c` 1; `grep -c 'has no pipefail' .github/workflows/nightly.yml` 1; `grep -c 'PSG step every 16 ticks' src/synth.c` 1; `grep -c 'docs/testing.md, "Performance"' .github/workflows/nightly.yml` 1 (a `docs/testing.md, "Fuzzing"` reference sat beside it); `src/battery.c` named `docs/embedding.md`.

After editing, each negated grep in the plan's verify command exits 0 and each positive grep exits 0. Also:

- `cmake --build --preset ci` exit 0; `ctest --test-dir build/ci -R '^docs\.'` passes (10 policy tests)
- `cmake --workflow --preset hygiene` exit 0
- `cmake --workflow --preset ci` exit 0 (no frame or audio hash moved)
- `actionlint -ignore 'label "ubuntu-26.04' .github/workflows/nightly.yml` exit 0
- `git diff --quiet 0227433^..0227433 -- scripts` exit 0 (no tooling file touched; P1 WR-02 and IN-04 stay seeds)

Commit for plan 05-09 to cite: `0227433`.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] Reflowed the SYNTH_EVENTS comment so the citation is on one line**
- **Found during:** Task 1 verification
- **Issue:** The first wording wrapped `SMS-SYSTEM section 1` across two lines, so the plan's `grep -q 'SMS-SYSTEM section 1'` exited 1.
- **Fix:** Reflowed the paragraph so the citation is on one source line.
- **Files modified:** src/synth.c
- **Commit:** 0227433 (made before committing)

Otherwise none; the plan executed as written.

## Known Stubs

None.

## Threat Flags

None.

## Self-Check: PASSED

- FOUND: src/instance.c, src/synth.c, src/battery.c, runner/png.c, .github/workflows/nightly.yml, docs/embedding.md
- FOUND: commit 0227433 (ancestor of HEAD)
