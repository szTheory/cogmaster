---
phase: 04-sound
verified: 2026-11-09T23:30:00Z
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
covered_digest: "v4:sha256:3057527d0703aaa5a08fdd1f67c27be19b177dda11e0129af9e2e3e4a2b976f5"
behavior_unverified: 0
overrides_applied: 0
---

# Phase 4: Sound Verification Report

**Phase Goal:** Games play with sound: the SN76489 with Game Gear stereo and the YM2413 for the Japanese profile are synthesised at an exact rational rate, delivered through the runner and both libretro audio paths, with transition and PCM hashes identical on every platform.
**Verified:** 2026-11-09T23:30:00Z
**Status:** passed (two warnings, no blockers)
**Re-verification:** No, initial verification

## Goal Achievement

### Observable Truths (ROADMAP success criteria, as amended by plan 04-01)

| # | Truth | Status | Evidence |
|---|-------|--------|----------|
| 1 | SC1 (SND-01): `ctest -L psg` passes every tone, volume and noise setting through closed-form unit oracles and the owned `psg_sweep` fixture under frozen transition and PCM hashes; GG stereo register routes channels | VERIFIED | `ctest -L "psg\|fm\|audio"` run by this verifier: 100% pass. `-L psg` holds 16 tests: `psg.periods`, `volumes`, `noise_controls`, `lfsr_period`, `tie_rule`, `psg.gg.routing`, `psg.gg.port06_sms_mode`, `fixtures.psg_sweep` and `.gg`. `src/psg.c` is a real 48-tick lazy-stepped chip (periods 0 and 1 held high, LFSR reset 0x8000) and feeds `cogmaster__synth_level`. Frozen pins in `tests/audio/pcm.txt` and `transitions.txt` (11 lines each) cover psg_sweep sms/pal/ntsc-j/gg-sms/gg. Third-party ROM conformance deferred per REQUIREMENTS (D-33). |
| 2 | SC2 (SND-02): `ctest -L fm` passes lockstep against the unmodified emu2413, register units, `fm_sweep` frozen hashes, `$F2` selecting PSG/FM/none/both, emu2413 notice present | VERIFIED | `-L fm` holds 23 tests, all pass: `fm.lockstep` (2M samples against the digest frozen from the unmodified upstream at the pinned commit `a1d5120903b7`, with `opll_oracle.c` as the one-shot regenerator), `fm.key_on`, `rhythm`, `noise`, `lfo`, `test_register`, `sum_bound`, `f2_write_mask`, `f2_read_bits`, `chip_off`, `fixtures.fm_sweep.*`. `src/io.c:197-203` implements `$F0/$F1/$F2` with `audio_ctrl = v & fm_mask`. `THIRD-PARTY-NOTICES.md` has the emu2413 section (MIT, SHA-256 of the unmodified source, changes). `pins.txt` pins `emu2413-c/-h/-license` to a public commit. |
| 3 | SC3 (SND-03): `--hash-audio` prints identical transition and PCM hashes for `psg_sweep` and `boot_to_play` on every platform, with 801/802 (965/966 PAL) samples per frame, cumulative floor(F x 3,504,512 / 4,375) (PAL floor(F x 685,094,400 / 709,379)), no clipping | VERIFIED | Ran `build/ci/cogmaster-run --hash-audio` on psg_sweep.sms/.gg, boot_to_play.sms/.gg and fm_sweep.sms: all exit 0, both hash lines printed, `clipped 0`. Cadence check: NTSC F=1,2,3,60 gives 801, 1602, 2403, 48061, and PAL gives 965, 1931, 2897, 57945, equal to the closed form computed independently. 600 frames gives 480,618 NTSC and 579,459 PAL, also equal. Cross-platform: hosted run 42714958262 on head 9aaf378 had `hash-equality` ("Hashes are byte-identical"), `be32`, `wasm32`, `nofp` x2 and all six `build` legs green. The head moved since only by `.planning/` docs (`git diff 9aaf378 HEAD` touches no non-`.planning` file). |
| 4 | SC4 (SND-04): batch and single-sample libretro paths deliver PCM equal to the runner's (stereo on GG, duplicated mono on SMS); hosted RetroArch recording is non-silent | VERIFIED | `libretro.host.audio.{psg_sweep,boot_to_play}.{sms,gg}` (tests 394-397) run the image three times (batch only, single only, both) and compare each against the runner's `audio-pcm` hash and the running floor total. All 12 `libretro.*` tests pass here. Hosted run steps "RetroArch silent control recording", "Master System sound recording is non-silent" and "Game Gear sound recording is non-silent" are all `success`; `tests/retroarch/audio.cmake` requires the control to hold 0 non-zero bytes and a sound recording at least 8,192, with 188,000-196,000 total audio bytes. I did not re-run RetroArch locally; the hosted conclusions were read through `gh run view`. |
| 5 | SC5 (SND-05): reference tones show largest non-harmonic peak below 16 kHz under -80 dB; synthesis cost bounded under a register-hammering fixture | VERIFIED | `audio.spectral` run directly prints worst-bin margins of 120.0 dB (111.9 kHz square) and 85.0 dB (18.6 kHz square, fundamental asserted present at bin 3181) against the 80 dB rule, using an integer Blackman-Harris DFT over every bin 5..2730. The margin on the second tone is 5 dB, deterministic and integer-only. `audio.cost`, `fm.cost`, `audio.ring_overshoot` and `audio.amplitude_bound` pass and assert the counters bound kernel applications, ring reach and sum range. Hosted `perf` job green. |

**Score:** 5/5 truths verified (0 present, behavior-unverified)

### Required Artifacts

| Artifact | Expected | Status | Details |
|----------|----------|--------|---------|
| `src/synth.c` (522 lines) | band-limited mixer, DC blocker, ring, trace log | VERIFIED | Real synthesis; WINDOWS row 9 (stub) fixed; PCM hashes nonzero, peaks 1.7k-11k |
| `src/psg.c` (178), `src/opll.c` (425), `src/opll_reg.c` (284), `src/opll.h` (127) | PSG and ported emu2413 | VERIFIED | Substantive, wired into `src/io.c`, `src/bus.c`, `src/frame.c`, `src/state.c` |
| `runner/audio.c`, `runner/args.c`, `runner/main.c` | `--hash-audio`, merge and hash | VERIFIED | Output seen above |
| `libretro/libretro.c`, `options.h`, `.info.in` | batch and single-sample paths, FM option | VERIFIED | `libretro.host.*` tests pass, including `libretro.host.fm_option` |
| `tests/roms/src/{psg_sweep,psg_sweep_gg,fm_sweep,boot_to_play*}.asm` | owned fixtures | VERIFIED | Built to `build/ci/fixtures/*.sms/.gg`; digests in `tests/built/expected.txt`, listed in `manifest.txt` |
| `tests/audio/{pcm,transitions}.txt`, `opll_lockstep.txt` | frozen pins | VERIFIED | Compared line for line by `runner.hash_matrix` |
| `docs/audio.md`, `THIRD-PARTY-NOTICES.md`, `PROVENANCE.md` | docs and notice | VERIFIED (see WR-01 on one sentence) | emu2413 section present |

### Key Link Verification

| From | To | Status | Details |
|------|----|--------|---------|
| Z80 OUT `$7F/$7E`, `$06` | `psg.c`, stereo mask (source 4) | WIRED | `psg.through_bus`, `psg.gg.routing` |
| Z80 OUT `$F0/$F1/$F2` | `opll.c`, mute mask (source 6) | WIRED | `fm.through_bus`, `fm.f2_write_mask`, `fm.chip_off` |
| chips | `cogmaster__synth_level` then `frame_end` | WIRED | `psg_emit` calls `cogmaster__synth_level`; `synth_frame_end` catches both chips up to the deadline |
| `cogmaster_get_audio` | runner hash and both libretro callbacks | WIRED | Hash equality tests in `libretro.host.audio.*` |
| state save/load | synth, PSG, OPLL fields | WIRED | `psg.state_continue`, `audio.state_continue`, `fm.state_round_trip` |

### Data-Flow Trace (Level 4)

| Artifact | Data | Source | Real data | Status |
|----------|------|--------|-----------|--------|
| PCM out | samples | Z80 ROM writes through the chips into the synth ring | Yes: psg_sweep 78k transitions and peak 7,222; fm_sweep peak 1,671 (export) and 6,682 (Japan) | FLOWING |
| RetroArch recording | audio bytes | libretro callbacks | Yes: control 0 non-zero bytes, sound at least 8,192 (hosted) | FLOWING |

### Behavioral Spot-Checks

| Behavior | Command | Result | Status |
|----------|---------|--------|--------|
| Whole CI workflow | `cmake --workflow --preset ci` | exit 0, 399/399 (plus 4 release tests) | PASS |
| Audio, PSG and FM labels | `ctest -L "psg\|fm\|audio"` from `build/ci` | 100% passed | PASS |
| libretro host tests | `ctest -R "libretro.*audio\|libretro.host"` | 12/12 passed | PASS |
| Hygiene | `scripts/hygiene.sh` | exit 0 | PASS |
| Cadence closed form | `cogmaster-run --frames F --hash-audio` NTSC and PAL | matches floor formula | PASS |
| Spectral margin | `cogmaster-test-audio spectral` | 1200 and 850 tenths dB (rule 800) | PASS |

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

Every ID SND-01..05 appears in at least one PLAN `requirements:` field and in REQUIREMENTS.md (traceability rows mark Phase 4). No orphaned requirement IDs map to Phase 4 without a plan.

### Anti-Patterns Found

None blocking. `TBD|FIXME|XXX` and `TODO|HACK|PLACEHOLDER|not yet implemented` over the 78 non-`.planning` files changed since the merge base with `origin/main`: no matches. WINDOWS.md: `open_count: 0` (rows 9 and 10 fixed), so the ship gate is clear.

### Code Review Findings Weighed (04-REVIEW.md: 0 critical, 2 warning, 4 info; all dispositions still `open`)

| ID | Weight in verdict |
|----|-------------------|
| WR-01 | WARNING, not a blocker. See below. |
| WR-02 | WARNING (necessity): `if (m->frame > 0u)` guards an impossible state in `src/synth.c:234`; no goal impact. |
| IN-01..IN-04 | Info: a wrong comment premise, two impossible-state guards, and an undrained trace across a state load with no public state API yet. No goal impact. |

**WR-01 (confirmed by this verifier).** `docs/audio.md:145` and `src/internal.h:603` say the trace log is "in causal order and the ticks never fall within one run". `cogmaster__synth_frame_end` (`src/synth.c:233-239`) runs `cogmaster__psg_catch_up` and then `cogmaster__fm_catch_up` to the same deadline, so FM records, starting at an earlier tick, are appended after the PSG records. The claim is true only within one source. The shipped consumer is correct: `runner/audio.c:112-118` asserts monotone ticks per source and sorts by (tick, source, emission index) before hashing, which matches the decision D-24 merge rule, so the transition hash (SC3) is unaffected and `hash-equality` is green. No ROADMAP success criterion or PLAN must-have asserts global causal order, so this does not fail a truth. It is still a false published contract for the trace API (project rule 5 puts docs and code together), and no test would catch it.

Recommendation, within CLAUDE.md rule 7's two fix rounds: before ship, change the two sentences to the real guarantee ("records of one source are in tick order; across sources they are in the order the chips were brought up to date, so sort by tick, then source, before merging, as `cogmaster-run --hash-audio` does"). It is a documentation-only change (optionally with IN-01's comment and WR-02's guard removal). Apply the same fix to the D-24 wording in CONTEXT if it is to be reused. It does not change any hash.

### Human Verification Required

None. Per CLAUDE.md rule 6 the owner verifies nothing by default; every success criterion is covered by CTest or hosted CI, and no item needs the owner's hardware or identity-bound consent. The release pull request not existing yet is a ship-time fact (rule 8), not a phase requirement.

### Gaps Summary

No gaps. All five success criteria are verified by commands run in this session (CI workflow exit 0 with 399/399; label subsets; runner cadence and hash output; spectral margins; hygiene) together with hosted run https://github.com/szTheory/cogmaster/actions/runs/42714958262 on head 9aaf378, whose `hash-equality`, `be32`, `wasm32`, `nofp`, `perf`, `asan`, `retroarch` and all `build` jobs concluded success; the only commits after that head are `.planning/` documents. The local RetroArch counts (0 and 127,484 non-zero bytes) quoted in the hand-off were not re-measured by this verifier; the hosted steps that enforce the same bounds were confirmed instead. Two warnings remain (WR-01 documentation contract, WR-02 dead guard), neither of which affects the goal.

---

_Verified: 2026-11-09T23:30:00Z_
_Verifier: Claude (gsd-verifier)_
