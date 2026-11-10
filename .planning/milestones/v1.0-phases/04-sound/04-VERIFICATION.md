---
phase: 04-sound
verified: 2026-11-10T00:30:00Z
status: passed
score: 5/5 must-haves verified
covered_files:
  - ".planning/phases/04-sound/04-01-PLAN.md"
  - ".planning/phases/04-sound/04-01-SUMMARY.md"
  - ".planning/phases/04-sound/04-02-PLAN.md"
  - ".planning/phases/04-sound/04-02-SUMMARY.md"
  - ".planning/phases/04-sound/04-03-PLAN.md"
  - ".planning/phases/04-sound/04-03-SUMMARY.md"
  - ".planning/phases/04-sound/04-04-PLAN.md"
  - ".planning/phases/04-sound/04-04-SUMMARY.md"
  - ".planning/phases/04-sound/04-05-PLAN.md"
  - ".planning/phases/04-sound/04-05-SUMMARY.md"
  - ".planning/phases/04-sound/04-06-PLAN.md"
  - ".planning/phases/04-sound/04-06-SUMMARY.md"
  - ".planning/phases/04-sound/04-07-PLAN.md"
  - ".planning/phases/04-sound/04-07-SUMMARY.md"
  - ".planning/phases/04-sound/04-08-PLAN.md"
  - ".planning/phases/04-sound/04-08-SUMMARY.md"
  - ".planning/phases/04-sound/04-09-PLAN.md"
  - ".planning/phases/04-sound/04-09-SUMMARY.md"
  - ".planning/phases/04-sound/04-10-PLAN.md"
  - ".planning/phases/04-sound/04-10-SUMMARY.md"
  - ".planning/phases/04-sound/04-11-PLAN.md"
  - ".planning/phases/04-sound/04-11-SUMMARY.md"
  - ".planning/phases/04-sound/04-12-PLAN.md"
  - ".planning/phases/04-sound/04-12-SUMMARY.md"
  - ".planning/phases/04-sound/04-13-PLAN.md"
  - ".planning/phases/04-sound/04-13-SUMMARY.md"
  - ".planning/phases/04-sound/04-14-PLAN.md"
  - ".planning/phases/04-sound/04-14-SUMMARY.md"
  - ".planning/phases/04-sound/04-15-PLAN.md"
  - ".planning/phases/04-sound/04-15-SUMMARY.md"
  - "docs/audio.md"
  - "include/cogmaster.h"
  - "libretro/cogmaster_libretro.info.in"
  - "libretro/libretro.c"
  - "libretro/options.h"
  - "runner/args.c"
  - "runner/args.h"
  - "runner/audio.c"
  - "runner/audio.h"
  - "runner/main.c"
  - "src/bus.c"
  - "src/frame.c"
  - "src/instance.c"
  - "src/internal.h"
  - "src/io.c"
  - "src/opll.c"
  - "src/opll.h"
  - "src/opll_reg.c"
  - "src/opll_tables.inc"
  - "src/psg.c"
  - "src/state.c"
  - "src/state.h"
  - "src/synth.c"
  - "tests/audio/pcm.txt"
  - "tests/audio/transitions.txt"
covered_digest: "v4:sha256:969ebf60efbb2de73d6a0bb77353f135bcec02b60cb2f1f137f34b2b2efc0662"
behavior_unverified: 0
overrides_applied: 0
re_verification:
  previous_status: passed
  previous_score: 5/5
  gaps_closed: []
  gaps_remaining: []
  regressions: []
---

# Phase 4: Sound Verification Report

**Phase Goal:** Games play with sound: the SN76489 with Game Gear stereo and the YM2413 for the Japanese profile are synthesised at an exact rational rate, delivered through the runner and both libretro audio paths, with transition and PCM hashes identical on every platform.
**Verified:** 2026-11-10T00:30:00Z
**Status:** passed (no blockers, no gaps)
**Re-verification:** Yes, regenerated after the two code-review fix commits `df8a27e` (WR-01) and `e1ac29f` (WR-02). The prior report's `covered_digest` no longer matched (`docs/audio.md`, `src/internal.h`, `src/synth.c` changed). Every truth was re-checked against the current head; the three changed files were read in full diff.

## What changed since the last report

`git diff 4c2079b..HEAD --stat -- src docs include libretro runner tests/audio tests/cmake` lists exactly three files, 15 insertions and 13 deletions:

| File | Change | Behaviour effect |
|------|--------|------------------|
| `docs/audio.md` | WR-01: the trace log's ordering sentence now says the records of one source are in tick order and never go backwards; across sources they are in catch-up order; consumers sort by tick, then source, then emission index | None (prose) |
| `src/internal.h` | WR-01: the same wording in the `cogmaster__synth_trace` comment | None (comment) |
| `src/synth.c` | WR-02: the `if (m->frame > 0u)` guard around the `frame--`, catch-up, `frame++` sequence in `cogmaster__synth_frame_end` is removed | None on any reachable path |

`tests/audio` and `tests/cmake` do not appear in the diff: `git diff 4c2079b..HEAD --stat -- tests/audio tests/cmake` prints nothing (exit 0). No hash pin and no budget file moved.

**WR-01 check.** The new wording is true to the code. `runner/audio.c:110-125` asserts `r->tick >= last_tick[source]` per source, then `qsort`s the entries by (tick, source, index) before hashing. That matches the rewritten sentence in `docs/audio.md` and `src/internal.h`. The false "causal order" claim is gone.

**WR-02 check.** The removed guard's invariant holds. `m->frame` is incremented before `cogmaster__synth_frame_end` on both call paths: `src/bus.c:102` (`m->frame++` inside `cogmaster__bus_run_frame`, called at `src/frame.c:60` before the call at `:61`) and `src/frame.c:74` (before the call at `:79`). The counter starts at 0 (`src/bus.c:39`), is `uint64_t`, so `m->frame--` cannot underflow here. The deadline arithmetic is unchanged. The unchanged PCM and transition pins confirm no reachable path moved.

## Goal Achievement

### Observable Truths (ROADMAP success criteria, as amended by plan 04-01)

| # | Truth | Status | Evidence |
|---|-------|--------|----------|
| 1 | SC1 (SND-01): `ctest -L psg` passes every tone, volume and noise setting through closed-form unit oracles and the owned `psg_sweep` fixture under frozen transition and PCM hashes; GG stereo register routes channels | VERIFIED | `ctest -L "psg\|fm\|audio"` from `build/ci` re-run on this head: exit 0, 100% of 50 tests (`psg` 16, `fm` 23). The PSG label holds `psg.periods`, `volumes`, `noise_controls`, `lfsr_period`, `tie_rule`, `psg.gg.routing`, `psg.gg.port06_sms_mode`, `fixtures.psg_sweep` and `.gg`. `src/psg.c` is a real lazy-stepped chip that feeds `cogmaster__synth_level`. The 60-frame psg_sweep runs print nonzero transitions (1,469 SMS, 1,470 GG) and peaks of 7,172. Frozen pins in `tests/audio/pcm.txt` and `transitions.txt` (11 lines each) are unchanged. Third-party ROM conformance is deferred per REQUIREMENTS. |
| 2 | SC2 (SND-02): `ctest -L fm` passes lockstep against the unmodified emu2413, register units, `fm_sweep` frozen hashes, `$F2` selecting PSG/FM/none/both, emu2413 notice present | VERIFIED | The 23 `fm` tests pass in the run above, including `fm.lockstep` (frozen digest from the unmodified upstream at the pinned commit), `fm.f2_write_mask`, `fm.f2_read_bits`, `fm.chip_off` and `fixtures.fm_sweep.*`. `fm_sweep.sms` run directly: Japan region exit 0, 28,715 transitions, peak 5,746 over 60 frames; export region exit 0, peak 0 over 60 frames. This is what the profile gating requires. `THIRD-PARTY-NOTICES.md` carries the emu2413 section; `pins.txt` pins it to a public commit (unchanged). |
| 3 | SC3 (SND-03): `--hash-audio` prints identical transition and PCM hashes for `psg_sweep` and `boot_to_play` on every platform, 801/802 (965/966 PAL) samples per frame, cumulative floor(F x 3,504,512 / 4,375) (PAL floor(F x 685,094,400 / 709,379)), no clipping | VERIFIED | `cogmaster-run --hash-audio` run on psg_sweep.sms, psg_sweep.gg, boot_to_play.sms, boot_to_play.gg, fm_sweep.sms: all exit 0, both hash lines printed, `clipped 0`. The `boot_to_play` 60-frame PCM hashes equal `tests/audio/pcm.txt` lines 8 and 10 byte for byte (`57dab102...`, `fd03f40f...`), so the fix moved no hash. Cadence check, NTSC F=1,2,3,60,600: 801, 1602, 2403, 48061, 480618; PAL: 965, 1931, 2897, 57945, 579459. These equal the closed forms computed independently by `python3` integer arithmetic. Cross-platform equality is carried by hosted run 42714958262 (see Hosted evidence). |
| 4 | SC4 (SND-04): batch and single-sample libretro paths deliver PCM equal to the runner's (stereo on GG, duplicated mono on SMS); hosted RetroArch recording is non-silent | VERIFIED | `libretro.host.audio.{psg_sweep,boot_to_play}.{sms,gg}` run inside the passing `ctest --preset ci` (399 of 399): each image runs batch-only, single-only and both, and is compared with the runner's `audio-pcm` hash and the running floor total. The libretro sources are not in the post-report diff. The RetroArch recording conclusions (control 0 non-zero bytes, sound at least 8,192) come from hosted run 42714958262; RetroArch was not re-run locally. |
| 5 | SC5 (SND-05): reference tones show the largest non-harmonic peak below 16 kHz under -80 dB; synthesis cost bounded under a register-hammering fixture | VERIFIED | `audio.spectral`, `audio.cost`, `fm.cost`, `audio.ring_overshoot` and `audio.amplitude_bound` are in the `audio` label and pass in the run above (exit 0). `src/synth.c` was re-read after WR-02: the mixer and the deadline math are untouched, so the spectral margins and cost bounds stand. |

**Score:** 5/5 truths verified (0 present, behavior-unverified)

### Required Artifacts

| Artifact | Expected | Status | Details |
|----------|----------|--------|---------|
| `src/synth.c` | band-limited mixer, DC blocker, ring, trace log | VERIFIED | Real synthesis; the WR-02 edit only removed a dead guard |
| `src/psg.c`, `src/opll.c`, `src/opll_reg.c`, `src/opll.h` | PSG and ported emu2413 | VERIFIED | Substantive, wired from `src/io.c`, `src/bus.c`, `src/frame.c`, `src/state.c`; unchanged since the last report |
| `runner/audio.c`, `runner/args.c`, `runner/main.c` | `--hash-audio`, merge and hash | VERIFIED | Output seen above |
| `libretro/libretro.c`, `options.h`, `.info.in` | batch and single-sample paths, FM option | VERIFIED | `libretro.host.*` pass inside the 399 |
| `tests/roms/src/{psg_sweep,psg_sweep_gg,fm_sweep,boot_to_play*}.asm` | owned fixtures | VERIFIED | Built to `build/ci/fixtures/*.sms` and `.gg` by the workflow |
| `tests/audio/{pcm,transitions}.txt`, `opll_lockstep.txt` | frozen pins | VERIFIED | Unchanged; compared by `runner.hash_matrix` |
| `docs/audio.md`, `THIRD-PARTY-NOTICES.md` | docs and notice | VERIFIED | The trace-ordering sentence now matches the code |

### Key Link Verification

| From | To | Status | Details |
|------|----|--------|---------|
| Z80 OUT `$7F/$7E`, `$06` | `psg.c`, stereo mask (source 4) | WIRED | `psg.through_bus`, `psg.gg.routing` pass |
| Z80 OUT `$F0/$F1/$F2` | `opll.c`, mute mask (source 6) | WIRED | `fm.through_bus`, `fm.f2_write_mask`, `fm.chip_off` pass |
| chips | `cogmaster__synth_level` then `cogmaster__synth_frame_end` | WIRED | The frame_end catch-up (PSG then FM) is unchanged in behaviour after WR-02 |
| `cogmaster_get_audio` | runner hash and both libretro callbacks | WIRED | `libretro.host.audio.*` equality tests pass |
| state save/load | synth, PSG, OPLL fields | WIRED | `psg.state_continue`, `audio.state_continue`, `fm.state_round_trip` pass |

### Data-Flow Trace (Level 4)

| Artifact | Data | Source | Real data | Status |
|----------|------|--------|-----------|--------|
| PCM out | samples | Z80 ROM writes through the chips into the synth ring | Yes: psg_sweep peak 7,172 (60 frames), boot_to_play peak 7,357, fm_sweep (Japan) peak 5,746 | FLOWING |
| RetroArch recording | audio bytes | libretro callbacks | Yes per the hosted run (control 0 non-zero bytes; sound at least 8,192) | FLOWING |

### Behavioral Spot-Checks and Evidence Commands

All run from the repository root, with `/opt/homebrew/opt/llvm@21/bin` on PATH, in this session on head `85b7dd2`.

| Behaviour | Command | Result | Status |
|-----------|---------|--------|--------|
| Whole CI workflow | `cmake --workflow --preset ci` | exit 0; 399 of 399 tests, then 4 of 4 release tests, all passed | PASS |
| Audio, PSG and FM labels | `ctest -L "psg\|fm\|audio"` in `build/ci` | exit 0, 100% of 50 | PASS |
| Hygiene | `scripts/hygiene.sh` | exit 0 | PASS |
| Hash and PCM cadence | `build/ci/cogmaster-run --region ntsc-u\|pal --frames F --hash-audio build/ci/fixtures/psg_sweep.sms`, F = 1, 2, 3, 60, 600 | samples equal the closed form for both regions | PASS |
| Per-fixture `--hash-audio` | psg_sweep.sms, psg_sweep.gg, boot_to_play.sms, boot_to_play.gg, fm_sweep.sms (60 frames) | all exit 0, `clipped 0`, boot_to_play hashes equal pins | PASS |
| Pins not moved | `git diff 4c2079b..HEAD --stat -- tests/audio tests/cmake` | empty, exit 0 | PASS |

A first attempt at the per-fixture loop failed with `unknown option` (exit 2) because zsh does not word-split an unquoted variable holding `--machine gg`; this was a verifier shell mistake, not a runner fault. The runs were repeated with the arguments passed separately and all exited 0.

### Hosted evidence

Run https://github.com/szTheory/cogmaster/actions/runs/42714958262 on head `9aaf378` is `success` (confirmed with `gh run view`): `hash-equality`, `be32`, `wasm32`, `nofp`, `perf`, `asan`, `retroarch` and all `build` legs. **That run predates the two fix commits.** The head moved by `.planning/` documents and by the three files above, none of which changes a hash or a pin (local evidence above). CI on the pull request will cover the new head and is the cross-platform confirmation of the fix; this report does not claim hosted results for it.

### Probe Execution

SKIPPED: no `probe-*.sh` is declared or present for this phase; the equivalent checks are CTest cases.

### Requirements Coverage

| Requirement | Source Plans | Status | Evidence |
|-------------|--------------|--------|----------|
| SND-01 | 04-01, 04-05, 04-06, 04-07, 04-08 | SATISFIED | SC1 |
| SND-02 | 04-01, 04-03, 04-10, 04-11, 04-12, 04-13 | SATISFIED | SC2 |
| SND-03 | 04-01 to 04-04, 04-07, 04-08, 04-09, 04-13, 04-15 | SATISFIED | SC3 |
| SND-04 | 04-02, 04-03, 04-14, 04-15 | SATISFIED | SC4 |
| SND-05 | 04-04, 04-06, 04-12 | SATISFIED | SC5 |

SND-01..05 are checked `[x]` in `.planning/REQUIREMENTS.md` (lines 45-49) and marked `Phase 4 | Complete` in the traceability table (lines 121-125). No requirement mapped to Phase 4 is missing from a plan.

### Anti-Patterns Found

None. `TBD|FIXME|XXX|TODO|HACK|PLACEHOLDER` over the three changed files: no matches. WINDOWS.md: `open_count: 0` (10 of 10 rows fixed), so the ship gate is clear.

### Code Review Findings

| ID | Disposition now | Effect on verdict |
|----|-----------------|-------------------|
| WR-01 | fixed (`df8a27e`), verified here against `runner/audio.c` | Closed. The published trace contract is true. |
| WR-02 | fixed in code (`e1ac29f`), verified here against both callers; `04-REVIEW-DISPOSITION.md` still lists it `open` | Closed in code. The disposition table is stale for this row and is a bookkeeping item for the orchestrator, not a goal issue. |
| IN-01 to IN-04 | open, out of scope for the fix round | Info only (a comment premise, two impossible-state guards, a trace left across a state load with no public state API). No goal impact. |

### Human Verification Required

None. Per CLAUDE.md rule 6 the owner verifies nothing by default; every success criterion is covered by CTest or hosted CI, and nothing needs the owner's hardware or identity-bound consent.

### Gaps Summary

No gaps. The two review-fix commits are a docs-only rewording and the removal of a provably dead guard; they moved no hash and no pin, and the full CI workflow, the label subset, hygiene and the cadence checks all exit 0 on the current head. The remaining caveat is that the hosted run predates the fixes, which CI on the pull request will cover.

---

_Verified: 2026-11-10T00:30:00Z_
_Verifier: Claude (gsd-verifier)_
