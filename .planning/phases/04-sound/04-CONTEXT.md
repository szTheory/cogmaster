# Phase 4: Sound - Context

**Gathered:** 2026-11-09
**Status:** Ready for planning

<domain>
## Phase Boundary

Games play with sound. The SN76489 PSG with Game Gear stereo, and the YM2413 for the Japanese profile, are synthesised
at an exact rational rate and delivered through the runner and both libretro audio paths, with transition-stream and
PCM hashes identical on every platform (SND-01 to SND-05; roadmap Phase 4). Depends on Phase 3.

How these decisions were reached: six research agents, one per open decision area (OPLL seed, synthesis kernel,
audio hashes and fixtures, libretro and the hosted RetroArch check, PSG and test-ROM conformance, FM control and
mix), each ran the role lenses, an adversarial pass and a recommendation; an adversarial cross-check then ran over
the reconciled set (it found two blockers that were fixed: a wrong volume-law formula and a rule-7 dodge in the
hosted check, plus gaps now covered by D-35 and the period 0/1 rule). The owner asked for the recommendations to be
followed, so they are locked here without per-area questions. Facts below marked "verified" were computed or fetched during this step (cadence, LFSR period,
volume table, mix arithmetic, pins, licences, RetroArch 1.22.2 behaviour).

</domain>

<decisions>
## Implementation Decisions

### Time base, synthesis and mix
- **D-01 (time unit):** Every audio timestamp is the absolute bus **tick** (3 ticks per Z80 cycle) counted from the
  last load or reset of either kind; both SOFT and POWER restart the count. Never "cycle": where SMS-AUDIO §5 says `u64
  cycle`, read tick (DEC.34 and ARCHITECTURE §5 already say tick). The rational constants quoted below
  (176/13,125 and so on) are per Z80 cycle; per tick the modulus is three times larger (NTSC 39,375). —
  **Reversibility:** costly — the hash record layout and every pinned digest depend on it.
  Amended at planning (new evidence: research finding F1; confirmed by the owner at planning, 2026-11-09): ticks and
  frames count from the last load or reset of either kind, because cogmaster_reset restarts both for SOFT and POWER
  (implemented by src/bus.c cogmaster__bus_origin, stated in the first paragraph of docs/embedding.md "## Reset",
  asserted by tests/bus/test_bus.c test_frame_origin and tests/core/test_state.c). This is the single SOFT rule every
  later plan implements. Snapshots are stamped tick 0 at load and at both resets. Across a SOFT reset the synth keeps
  its phase accumulator, ring and head, integrator, DC estimator, stored per-source levels and undrained PCM, and
  clears the trace log. Each chip returns to its power-on constants including its step origin (the PSG's last-step
  tick returns to 0 and the FM next-sample tick to 216), and its reset levels reach the synth through
  cogmaster__synth_level at tick 0, so the synth's stored levels equal the chips' levels and the step to silence is
  band-limited at the first sample of the next frame. Ring positions are relative to the head and an event's sample
  index is measured from the current frame's start, so no event lands at a stale ring position.
- **D-02 (kernel family):** Band-limited step synthesis, the sibling `szTheory/nesturbator` `src/synth.c` family
  (MIT, same author, pin `9f9386c661f6`), with an integer impulse ring that is summed once per frame. Alternatives
  were measured and fail SND-05: polyBLEP −27 dB, box average −15.6 dB, sample-and-hold −4 dB; the "box bins"
  fallback in SMS-AUDIO §5 is withdrawn. 12 taps give −63.6 dB, so 16 taps stay; Kaiser beta 8 (beta 10 fails).
- **D-03 (table; reuse first, regenerate only on evidence):** Start from the sibling's 16 taps × 32 phases Q15
  table, copied with its notice (reuse before writing, per the ponytail ladder; no new tool). The deterministic
  integer spectral test (D-35) is the arbiter. Measured in a float simulation, the sibling table's worst non-harmonic
  peak over a period sweep is −83.4 dB at 48 kHz (3.4 dB of margin) and it fails at 44.1 kHz (−72.6 dB); its
  claimed "−87 dB" could not be reproduced and it ships no generator. **Fallback, taken only if the D-35 test fails
  or a later phase adds other rates:** a new `tools/kernelgen` (float is allowed in `tools/`; `hygiene.no_float`
  scans only `src/` and `include/`) producing 16 taps × 64 phases, int32 Q20, rows summing to exactly 2^20
  (recipe: Nyquist sinc integrated over each unit tap interval times a Kaiser window, beta 8, half-width 7.5,
  normalised; it reproduced the sibling rows to within 2 LSB at the centre tap; measured about −95 dB at 44.1, 48
  and 96 kHz), the agent running it once and recording the command and exit status. — **Reversibility:** costly —
  table values move every PCM digest.
- **D-04 (row arithmetic):** One `t = (acc << 22) / MOD` per event in u64 (MOD per Z80 cycle, D-01), row = t >> 16,
  fraction = t & 0xFFFF, each tap `a*(65536−f) + b*f` (exact). Do not copy the sibling's 16 int64 divisions per
  event or its correction tap. A `_Static_assert` and a max-amplitude test bound the int64 accumulation (about
  2^58.6 for the 16 × 64 Q20 shape; the planner re-derives the bound for the table actually used).
  Amended at planning (new evidence: the shipped kernel is the sibling's 16 x 32 table, plan 04-04): with 32 phases
  the shift is << 21, t = (frac << 21) / mod, row = t >> 16 (frac < mod <= 2,128,137 < 2^22, so frac << 21 < 2^43 and
  row < 32); << 22 applies only to the 64-phase D-03 fallback table.
- **D-05 (ring):** A 1,024-slot int64 **impulse ring** with a global head. Each event adds at
  `head + sample_index_in_frame + k`; additions commute, so lazy and unordered chip catch-up (PSG, FM) is safe.
  Integrate only at the frame end for the nominal sample count; events past it land in the next frame. Save the
  integrator, the DC state, the phase accumulator, the per-source levels and the **32** slots at and beyond the
  head (overshoot events can land up to 69 ticks past the frame end, so 16 is not enough); the PCM buffer is
  transient. Ring indices are masked and a test asserts `n + 32 < 1024` under hostile overshoot.
- **D-06 (rate and cadence):** **Phase 4 ships 48 kHz only** (SND-03 and SND-04 name nothing else). The
  `audio_rate_hz` config field, the 44.1 and 96 kHz rates (77/6,250 NTSC for 44.1 kHz; 352/13,125 for 96 kHz) and
  any rate-dependent pins are deferred with the kernel fallback in D-03; the `(inc, mod)` constants are written so
  a rate argument can be added by appending a config field (reduced by gcd). Per Z80 cycle: NTSC 48 kHz =
  176/13,125; PAL 48 kHz = 9,600/709,379. The accumulator resets to 0 on every POWER reset or load. Verified
  cadence: NTSC is **801 samples with a 802 once every 32 frames** (not "801, 801, 801, 802"), **801,031 per 1,000
  frames** (SMS-AUDIO §7's 801,034 is wrong); PAL is 685,094,400/709,379 per frame, 965,766 per 1,000 frames (766
  long frames). Frames carry 801 or 802 (PAL 965 or 966), never padded.
- **D-07 (mix and headroom):** The FM:PSG ratio stays 4096:1434 (DEC.36). The earlier claim "cannot clip by
  construction" is false at those Q12 gains (4 PSG channels give 45,886; with full-scale FM 78,654). The absolute
  gains are therefore chosen so the worst-case mixed sum times the kernel overshoot (+8.28%, measured) stays at or
  below 32,767: per-source Q12 gains, applied to each source's delta before it enters the ring, using non-negative
  rounding division, never a signed shift. **Gain the level, not the delta:** the mixer computes each source's gained
  level `(level × g + 2048) / 4096` from non-negative levels, stores the last gained level per source, and feeds
  the difference to the ring (per-delta truncation would not sum to the gained level and the integrator would
  drift under volume and FM sequences). The FM gain also absorbs the normalisation from the OPLL's native signed sum
  (bounded by about 17k, asserted by a test) to the PSG full-scale unit. Starting point from the research: PSG-only
  profiles g_psg = 920/4096; the Japanese FM profile g_fm = 1550 with g_psg = 543 (ratio 2.855, within 0.07% of
  4096:1434). The planner fixes the exact constants and proves them with a no-clip test (a clamp counter that stays
  0 on every fixture, plus a "four PSG channels at 0 dB plus full-scale FM" test). Saturating int16 conversion
  happens at output only.
  Mixing exists only at the PCM stage; transition records are pre-gain and pre-mute. Mix retunes bump the
  behaviour revision and move PCM digests, never the transition hash.
- **D-08 (DC blocker):** One leaky estimator `dc += (x − dc) / 2^k`, `out = x − dc`, x in Q16, k = 8 at 44.1 and
  48 kHz, k = 9 at 96 kHz (about 30 Hz, settles under 50 ms). The sibling's two high-pass filters and 14 kHz
  low-pass are NES-specific and would remove the 16–18 kHz content games use; do not copy them.
- **D-09 (channels and modules):** One ring on SMS, two on GG (the kernel row is computed once and applied to
  both). `src/synth.c` (ring, kernel, mixer, DC blocker, conversion), `src/psg.c`, `src/opll.c`. The host-facing
  API follows ARCHITECTURE §4: `cogmaster_get_audio` and `channels` in the returned audio struct; **no
  `cogmaster_audio_format()`**. `cogmaster_config` gains `trace_records` (its `sizeof` static assert is updated);
  `cogmaster_info` gains the audio channels and the resolved fm. `get_audio` writes `min(capacity, available)`.
- **D-10 (determinism rules):** New synth, PSG and mixer code uses no signed right shift and no left shift of a
  negative value (use `* 2`, `/`, explicit rounding). The OPLL port is the exception: emu2413 relies on arithmetic
  right shifts of negative values (for example `(output[1]+output[0]) >> (9-FB)` and `fm >> 1`), and replacing
  them with `/` changes rounding and breaks the exact lockstep of D-16, so the port routes every such shift
  through one named floor-shift helper (`x >= 0 ? x >> n : ~(~x >> n)`, portable and exact). UBSan runs on the
  audio tests. Every serialised synth field is range-bounded on load (about ±2^24); hash records and samples are
  serialised field by field little-endian, never as struct memory.

### PSG and Game Gear stereo
- **D-11 (PSG module):** One file, per-/16-tick loop with lazy catch-up. Tie rule: before a write stamped τ, run
  every step with `48k ≤ τ`, then apply the write; document and test it, plus the frame-end flush. The divider
  phase is derived from absolute ticks, counted from the last load or reset of either kind (a frame is 3,733.5
  steps, so it is not frame-aligned). Volume writes are
  combinational: catch up through τ, apply, stamp the record at τ, not at the next tick (this is what makes the
  GG "SEGA" chant and other PCM-by-volume tricks work). READY stall is not modelled.
  Amended at planning (new evidence: research finding F1, the D-01/D-24 amendment): the tick origin is the last load
  or reset of either kind, so the PSG last-step tick returns to 0 at both resets.
- **D-12 (open-conflict defaults, each pinned by a cheap test, each flippable only by hardware evidence):**
  **period 0 and period 1 both hold the channel output constant high at its volume, with no transition records**
  (SMS-AUDIO §2.2 PSG-0, DEC.35; the flip-flop still counts internally; ares' literal toggle-every-tick would
  double the records for a hostile ROM and is identical after band-limiting); the LFSR resets to $8000 on every
  noise-control byte (latch or data) and not on a noise volume write; **the volume table is the derived law
  `round(32767 × 10^(−0.1 i))` for index i = 0..14 (2 dB steps) and 0 at index 15, which fixes index 7 at 6538
  (the wiki's 6568 is a typo; the other 14 entries match the law exactly; verified)**; the `sms2_clip` flag is
  **omitted** (single weak anecdote; adding it later changes no state format); GG port $06 is decoded only in
  native GG mode (not in SMS mode, per ARCHITECTURE §13 and DEC.35), power-on mask $FF held only in the I/O
  state; power-on constants are fixed (volumes $F, periods 0, noise control 0, LFSR $8000, counters and
  flip-flops 0, latch 3), not DEC.42 profile fields; noise rates 32/64/128 **PSG steps** (one step = 16 clocks =
  48 ticks) per shift for modes 0–2, mode 3 uses tone 2 live with "0 acts as 1" as a labelled model rule; the
  first step toggles the flip-flops 0→1; the noise output bit is sampled before the shift (ares order);
  SG-1000 constants are `#define`s only (deferred). The synth spectral test (D-35) drives the synth interface
  directly and does not depend on the PSG period 0/1 rule.
- **D-13 (wiring and a Phase 3 defect):** Replace the no-op in `bus_out` with `psg_write` for $40–$7F (reads of
  $7E/$7F stay VDP counters; the PSG is write-only). Port $06 goes through `io_out`: catch the PSG up, store the
  mask, emit source 4. **Fix in this phase:** `io.c` and `state.c` decide whether GG ports $00–$06 exist from
  `machine == GG` (the console), but they must follow `native_gg` (the current mode): today, with a GG console in
  SMS mode, $3E is dropped and $06 is stored, so `psg.gg.port06_sms_mode` cannot pass until the gate uses
  `native_gg`, including `io_io`'s $A8 validation.
- **D-14 (PSG tests, label `psg`, PR gate):** one `cogmaster-test-psg <case>` executable with closed-form oracles,
  not a second PSG model: all 1,024 periods (for N ≥ 2 the toggle spacing is 48N ticks; for N = 0 and 1 the
  output is constant high with no records), all 16 volumes, all 8 noise-control values;
  **LFSR period 57,337** from $8000 for 16-bit taps 0 and 3 (verified; SMS-AUDIO §7's "65,535" is wrong; 15-bit taps
  0/1 would give 32,767, periodic mode 16); the write protocol (data after a volume latch rewrites the volume; the
  latch persists); GG mask routing and the SMS-mode no-op; a save/load/continue stream equality.

### FM (YM2413)
- **D-15 (seed):** **emu2413** (MIT) at commit `a1d5120903b75f93d3eb7690e34a29c0b4add903` (cite the SHA only; it is
  an untagged "Bump Version" commit on the default branch), `emu2413.c` and `.h` only, ported to integer-only C17
  in `src/opll.c`, `src/opll.h` and a generated `src/opll_tables.inc`. This **widens DEC.22** (which names ymfm or
  ares): ymfm's OPLL sits on the templated OPL engine (about 4,800 lines to prune to about 1,500) and ares is nall
  bit-width types with float table generation. Size: the research estimated about 800–900 lines of logic after
  pruning, but making every table `static const` means generating literal tables for `tll`, `rks`, `halfsin`,
  `pm` and the like (emu2413.c is 1,552 lines today), so that estimate may be low; the trigger counts logic lines,
  excluding the generated `.inc`. Replacement trigger: the logic exceeds about 1,100 lines, or the lockstep test
  cannot be made exact; then ares (ISC), then ymfm. Never
  open Nuke.YKT's Nuked-OPLL (GPL-2.0) or any unlicensed OPLL repository. Amend DEC.22, PROVENANCE.md row 80 and
  ARCHITECTURE line 79; add an emu2413 section to `THIRD-PARTY-NOTICES.md` (paths, pin, SHA-256 of `emu2413.c`,
  changes, LICENSE verbatim; the header says 2026 and LICENSE says 2001–2019, reproduce both). — **Reversibility:**
  costly — notices, provenance and the digests follow the seed.
  Amended at planning (new evidence: the 800-line file rule of ENGINEERING.md, plan 04-10): the port spans src/opll.c
  (per-sample path) and src/opll_reg.c (register writes, reset, patch setup) with src/opll.h and src/opll_tables.inc;
  the 1,100-line replacement trigger counts both .c files and the .h.
- **D-16 (integer conversion and proof):** The only float in the core path is `kl_table` (multiples of 0.125) and
  `/ EG_STEP`: use an integer `kl8[]` (the dB2 value ×8) and `tll = ((tmp >> (3 − KL)) * 8) / 3` (derived by the
  research; the lockstep test is what proves it exact). Delete the rate
  converter, pan, mask, VRC7/YMF281B tables, debug code, malloc and the memcpy state save; make every table
  `static const` or an inline integer function (no mutable globals). Proof: a lockstep test of the port against the
  unmodified original (rate = clk/72) over a seeded PRNG register stream of at least 2M samples. The oracle is
  fetched at the pin with its SHA-256 recorded and runs nightly; PR CI keeps a frozen digest of the same stream so
  it stays hermetic.
- **D-17 (one instrument table):** emu2413's YM2413 rows only, no A/B knob (nothing in-repo can arbitrate between
  lineages). Remove the "ymfm (emu2413 selectable)" knob from ARCHITECTURE line 168; record the table's origins
  (andete's notes, Nuke.YKT VRC7 rhythm rows, Chabin YMF281B rows) in the provenance entry.
- **D-18 (cadence, writes, state):** Sample k completes at T-state 72k since the last load or reset of either kind (216 ticks). A write first
  catches up every sample ending at or before its tick, then applies and takes effect from the next sample; no busy
  state, no dropped or delayed write (the 12/84-clock gaps are ignored, as MAME does; they are chip clocks = T-states
  = 36/252 ticks if a diagnostic ever needs them). Serialise **every non-pointer field**: latch, `reg[0x40]`,
  `test_flag`, key status, rhythm mode, EG/PM/AM/LFO counters, noise LFSRs, patch numbers, the last sum, the
  72-cycle phase, and per slot `output[2]`, phase, EG state, volume, key and sustain flags **and the derived
  values plus the pending `update_requests`** (re-deriving after a load would change the first sample). Pointers
  become indices; never copy emu2413's raw-`memcpy` save. The FM block is always present at a fixed size when
  `fm_unit` is on. Source-5 level is the native signed sum as `(uint16_t)`, merged on change, independent of
  `fm_gain` and the `$F2` mute; a test asserts the worst-case sum stays inside the int16 hash field.
  Cost: a test counts slot evaluations and asserts exactly 18 × samples under a register-hammering fixture.
  Amended at planning (new evidence: research finding F1, the D-01/D-24 amendment): the tick origin is the last load
  or reset of either kind, so the FM next-sample tick returns to 216 at both resets.
- **D-19 (accuracy bought):** emu2413-grade: rhythm and noise LFSR, AM/PM LFO, the test register. Not bought: DAC
  multiplexing, analogue filtering, die-level timing.

### FM control, profiles, versions
- **D-20 (FM is a load-time profile field):** `media.fm` ∈ {AUTO, OFF, ON}; AUTO means FM only when region is Japan
  and the machine is not GG; export SMS and GG get none; ON with a GG machine returns PROFILE_REJECTED at load; ON
  with an export SMS attaches the YM2413 (the test hook that proves the detector, and what an FM add-on would be);
  the libretro option maps `on` with GG content to `auto` instead of failing the content. v1.0 reaches only `none`
  and `smsj` by default. The Mark III + FM unit is not exposed (no `COGMASTER_MACHINE_MARK3`; ARCHITECTURE puts it
  in v2); keep `fm_mask` (3 for smsj, 1 for the unit) as data and unit-test mask 1 by setting the field. API trap
  (64-bit): `cogmaster_media` is 48 bytes with tail padding at 44–47, so append an explicit `reserved2` at 44 and
  `fm` at 48 (sizeof 56); on 32-bit the struct is 28 bytes and `fm` goes at 28 (conditional like the existing
  `reserved`, sizeof 32). Update the sizeof static asserts for media, config and info. One bump to API 9, behaviour revision
  11, state format 11, recorded in `docs/embedding.md`; later mix retunes are one more bump plus regenerated PCM
  pins. — **Reversibility:** one-way — the public struct layout and the state format are published contracts.
- **D-21 (`$F2`):** Write stores `v & fm_mask`, bits 7–2 ignored. Read returns stored | counter bits (bit 5 =
  counter bit 3, bit 6 = bit 7, bit 7 = bit 11), bits 4–2 zero; the counter derives from the line index of the tick count since the last load or reset of either kind
  (no new state). Values: 0 PSG only, 1 FM only, 2 both off, 3 both on; muting is applied at the mixer, so the
  chips keep running. Power-on and both reset kinds: `$F2 = 0`, YM registers cleared, PSG silent (matches MAME's
  reset and the BIOS hand-off; whether a soft reset really clears the chips on hardware is unverified, so a test
  pins it and SMS-AUDIO §9 records it). On smsj, $F0–$F2 are independent of `$3E` bit 2. Without FM keep today's
  behaviour: writes ignored with no fault; export reads mirror $DC/$DD; GG and fm=off Japan read open bus; reads of
  $F0–$F2 never set `input_read` (DEC.38). The "%10 on export" value stays an unverified claim and is not
  implemented. Keep `io.decode_mirrors` and `fault.not_faults` passing.
  Amended at planning (new evidence: research finding F1, the D-01/D-24 amendment): the tick origin is the last load
  or reset of either kind, so the $F2 counter bits come from the line index of that tick count, not from a count kept
  across resets; the test is registered as fault.not_faults in tests/bus/tests.cmake and both inventories.
- **D-22 (libretro options and state):** exactly one new libretro option, `cogmaster_fm` (auto | off | on, reload
  content), added in all three places (`lr_definitions`, the v1 fallback `definitions[6]` with loop `i < 6`, and the
  `retro_variable` array). Per-voice or per-chip volume is deferred. `audio_ctrl` joins `struct cogmaster__io`
  (the `_Static_assert` of 8 forces an `io_io` update); one copy of the GG mask stays in `io.gg[5]`; one byte of
  the header's reserved space becomes `fm_unit` (`state.c` has a one-byte profile-reserved at offset 27 and a
  12-byte reserved block at 52; the planner picks one and states the remaining count); every new audio field gets a field-range sweep
  entry; LFSR ≠ 0 is cross-checked on load; undrained output is cleared on load.
  Amended at planning (new evidence: research finding F12): the v1 fallback holds five options plus a zeroed
  terminator today (definitions[6], loop i < 5), so adding cogmaster_fm makes it definitions[7] with loop bound i < 6.

### Observability: transition stream, hashes, runner
- **D-23 (tap is a caller-owned change log, not a v5 hook):** no hook machinery exists (hooks are TOOL-01 /
  ARCHITECTURE §8 "Hooks (v5)"). `cogmaster_config.trace_records` (0 = off) makes `load` allocate that many records
  (no allocation after load); `synth_level()` appends a raw `{tick, source, level}` when the log exists (one branch
  per change); `cogmaster_get_audio_trace(m, io)` drains it after `run_frame`; a drop counter turns overflow into a
  runner failure, never silence. Capacity 32,768 records per frame (bound: 4 × 3,735 tone/noise steps + 5,430 CPU
  writes + 830 FM samples ≈ 21.2k NTSC, 25.4k PAL), about 512 KiB, allocated only on request.
- **D-24 (record and merge):** 11 bytes, no padding: `u64 LE tick | u8 source | u16 LE level`; the public struct
  may be 16 bytes and is never hashed as memory. The core emits raw events in causal order (PSG steps at tick ≤ T
  before a CPU write stamped T). The **runner** sorts each frame's records by (tick, source, emission index) with
  an index tie-break (no subtraction comparators), keeps the last level per (tick, source) and drops it if it equals
  the source's previous merged level; the previous-level array lives in the runner and carries across frames.
  Frame end catches PSG and FM up to the deadline; the runner asserts per-source ticks are monotone. At load and at
  every reset the core emits one snapshot record per active source **at the current tick** (tick 0 for load and
  both reset kinds, because ticks count from the last load or reset of either kind, D-01), GG mask only on GG, FM
  and control only with an FM unit, so power-on state is hashed and the per-profile source set is checked.
  Amended at planning (new evidence: research finding F1; confirmed by the owner at planning, 2026-11-09): the
  snapshot is stamped tick 0 at load and at both reset kinds, and the SOFT rule of D-01 applies: across a SOFT reset
  the trace log is cleared and each chip's reset levels reach the synth through cogmaster__synth_level at tick 0.
- **D-25 (levels, so retunes never move the primary hash):** sources 0–3 = `out << 4 | att` from registers only
  (not Q15); 4 = the mask byte; 5 = the OPLL signed sum, pre-gain; 6 = the effective `$F2` after profile masking.
  Correcting the volume table or any mix constant therefore moves PCM digests only. Source 5 depends on the FM
  core by nature; SMS-AUDIO §7's "table switch never changes the transition count" no longer holds with
  merge-on-change.
- **D-26 (`--hash-audio`):** independent of `--hash`; machine and profile come from the existing `--machine`,
  `--region`, `--video-mode` (`--region ntsc-j` implies the built-in FM); **no new flag**. Prints
  `audio-transitions <frames> count <n> sha256 <hex>` and
  `audio-pcm <frames> rate 48000 channels <1|2> samples <n> peak <p> clipped <k> sha256 <hex>`. The PCM hash is
  SHA-256 over the whole run's int16 LE bytes, interleaved L,R on GG, continuous across frames. The runner exits 1
  unless every frame's count is 801 or 802 (965 or 966 PAL) and `audio_dropped` is 0; tests assert total samples =
  `floor(N × 3,504,512 / 4,375)` (PAL `floor(N × 685,094,400 / 709,379)`) in CMake integer math, clipped = 0 and
  peak ≤ 29,204 (−1 dBFS).
- **D-27 (pins and cross-platform proof):** add `tests/audio/transitions.txt` and `tests/audio/pcm.txt`, the
  `audio-*` subsets of `hashes.txt`, checked inside the existing `hash_matrix` case; regeneration is
  `grep '^audio-pcm' build/ci/hashes.txt > tests/audio/pcm.txt` (and likewise transitions), documented in
  `docs/testing.md`. A mix retune diffs only `pcm.txt`; a `transitions.txt` diff means semantics moved. Audio lines
  ride the existing 7-lane `hashes.txt` byte-compare (six native lanes plus be32 are gated; wasm32 produces the
  lines too but its comparison is report-only today, so "identical on every platform" is gated on those seven).
  Matrix of about 9 runs: `psg_sweep.sms` on sms/ntsc-u, sms/pal, sms/ntsc-j and gg-sms, `psg_sweep.gg`,
  `boot_to_play.sms` baseline and with input, `boot_to_play.gg` baseline and with input. At most 1–2 new CTests
  (`runner.hash_audio`), each added to `tests/inventory/ci.txt` and `wasm32.txt`. Fixture image digests go in
  `tests/built/expected.txt` (adding PSG writes changes them; re-run the pixel oracle and the RetroArch input
  replays; writes after the video code should not shift pixels).
- **D-28 (own fixtures, WLA-DX):** `psg_sweep.asm`: a table-driven player (WAIT n frames, PSG byte, `$06` byte
  under `FIXTURE_GG`, END) with a frame counter and a write counter in mailbox records per segment. Coverage means
  every register bit set and cleared plus extremes, not all 1,024 periods: protocol cases (including the Alex Kidd
  case), periods 0, 1 (constant high, no records), 2, 3, $0FE, $1FF, $3FF and walking ones on all three tones, all
  16 volumes on all 4 channels, all 8 noise modes with mode 3 at tone 2 ∈ {0, mid, high} plus
  reset-on-data, volume-write pops at 29-cycle spacing for a frame, GG `$06` walking bit with $FF/$0F/$F0/$00;
  about 300 frames. **A loud constant-volume tone must play before frame about 30 through frame 60 with no input**
  (the hosted check needs it). `boot_to_play` keeps its edge-only counter and gains a frame-driven boot chime (tone
  0 at 440 Hz, about 20 frames) plus, on each 0→1 edge, a period from a table indexed by the counter and a decaying
  note timer; on GG `$06` toggles left-only or right-only by counter parity; it writes $FF to `$06` and the
  volumes at boot. One owned FM fixture, `fm_sweep`, which also does the `$F2` detection routine (an own WLA-DX
  routine; the SMS Power code is unlicensed): it reports detect = 1 on Japan + auto and on export + ON, and 0 on
  export, GG and OFF, writes instrument-1 notes, rhythm key-ons and `$F2` = 0..3, and reports through the mailbox
  (add it to DEC.47). Frozen transition and PCM hashes judge it.

### libretro and the hosted check
- **D-29 (adapter):** drain `cogmaster_get_audio` every frame, before the video work, even when both callbacks are
  NULL; delete the adapter's `audio_acc` and the static `silence` buffer (the adapter's PAL rational 965.766414
  differs from the library's 685,094,400/709,379 by 19 ppb, and `audio_acc` is outside the state visitor). Use the
  batch callback when registered, else one `audio_cb(l, r)` loop over the same drained buffer (libretro allows only
  one; RetroArch registers both). SMS mono is expanded to stereo in place from the end into one static 966 × 2
  buffer keyed on the returned `channels`, not the machine. Never pad, skip, or call with 0 frames.
- **D-30 (AV info and lifecycle):** keep `sample_rate = 48000.0` and `fps = (double)fps_num/fps_den`; FRAME-03 pins
  `1640625.0/27379.0` and `48000.0` (host.c L647–648). (SMS-AUDIO §5's 3,579,545.45/59,736 is the same value in
  rounded form.) No rate core option. POWER reset or load zeroes all audio state including the output accumulator;
  SOFT reset follows D-21. Pause needs nothing; fast-forward does not use `GET_AUDIO_VIDEO_ENABLE`,
  `SET_MINIMUM_AUDIO_LATENCY` or `SET_AUDIO_BUFFER_STATUS_CALLBACK` (skipping synthesis would break determinism).
  `retro_serialize_size` stays 0 and `.info` keeps `savestate=false`, so run-ahead and rewind stay out of scope; the
  state visitor still carries every audio field, proven by a library save/run/hash vs load/run/hash test. Update
  the `.info` description ("not sound support").
- **D-31 (in-process proof):** in `host.c`, for SMS and GG `psg_sweep` and `boot_to_play`, run three sessions:
  batch only, single only, both registered (the "both" session asserts the single-sample callback is never
  called). Hash SHA-256 per channel over little-endian bytes (native int16 would break the be32 lane); copy each
  batch at call time (the adapter reuses its buffer); SMS asserts L == R and compares one mono hash with
  `cogmaster-run --hash-audio`; GG compares both channel hashes; per-frame counts 801/802 (965/966) and prefix sums
  equal `floor(F × 3,504,512 / 4,375)`. Expected values arrive through the existing `-DRUN`/`-DHOST` pattern of
  `tests/fixtures/tests.cmake`; register new names in `tests/inventory/ci.txt`.
- **D-32 (hosted RetroArch recording, macOS only):** record with `--record` into a mov using a 3-line config
  (`format = mov`, `acodec = pcm_s16le`, `frame_drop_ratio = 60`), supplied through the driver's existing
  `-DARGS="--record=F.mov;--recordconfig=C.cfg"`; analyse with `afinfo`, `tail`, `head` and `tr` (`LC_ALL=C`
  required), no new dependency. Sessions: the existing no-content step with `--record` added is the **control**
  (exactly 0 non-zero bytes, so the check is not vacuous); `psg_sweep.sms` and `psg_sweep.gg` (added to the
  `retroarch-owned-input` artifact) assert at least 8,192 non-zero bytes; all three assert
  4 × 47,000 ≤ length ≤ 4 × 49,000 (never an exact length). Assert that the file exists and `afinfo` exits 0
  (a failed recording start still exits 0); never assert on log text (localised; `audio_driver = "null"` logs a
  failure but the recorder still receives audio). **Budget (rule 7, hard):** `policy.e2e_budget` reports exactly
  300/300 today (284 under `tests/retroarch` plus 16 in `fixtures/play.h`), and `docs/maintaining.md` requires all
  hosted RetroArch code, including helpers, to stay under `tests/retroarch` so the independent 300-line check
  sees the whole driver; moving the assertions into workflow YAML would dodge that check and is **not** allowed.
  The non-zero-byte and length assertions therefore live in a small `cmake -P` checker under `tests/retroarch`,
  and the plan must first **free lines there** (trim `replay.c`, `flip.cmake` or equivalent duplicated logic) to
  make room; the cap is not raised. If the lines cannot be freed, reduce the hosted audio check to its minimum
  (file exists, `afinfo` exits 0, non-zero data bytes above a threshold) rather than exceed the cap. A raw `--pcm-out` dump with
  `audio_resampler = "nearest"` (the recording was an exact slice of core output in 8 of 8 runs) is deferred.
  The agent can run RetroArch 1.22.2 locally (as verified this step), so it verifies audio itself (rule 6);
  HOSTS-AND-INTEGRATION's "aborts in the agent sandbox" is stale.

### Conformance against third-party test ROMs
- **D-33 (requirement amendment, stated plainly):** SND-01, SND-02 and ROADMAP criteria 1 and 2 literally say the
  model "passes JoppyFurr's test ROM". New evidence shows that cannot be claimed: the SN76489 and YM2413 ROMs are
  keyboard-driven exercisers that print nothing (README), the repositories have no release and no binary
  (MIT; build needs SDCC 4.3.0, devkitSMS and Sneptile), and committing a built ROM does not fit ASSET_POLICY
  (our own SDCC build embeds GPL-2+exception crt0 bytes; `policy.notices` admits only MIT or permissive).
  **This phase therefore amends those requirements** (the planner edits REQUIREMENTS.md and ROADMAP.md as the
  plan's first docs task, citing this evidence): `ctest -L psg` is the owned `psg_sweep` fixture plus the unit tests
  of D-14 (every tone, volume and noise setting and the GG stereo routing against SMS-AUDIO §1–3), and `ctest -L fm`
  is the lockstep test, the register-level units and the owned `fm_sweep` fixture, judged by frozen transition and
  PCM hashes. Third-party ROM conformance (building JoppyFurr's ROMs from source at a pin and checking closed-form
  properties of their transition streams under a recorded pad script) is **deferred**, not scheduled, so no
  nightly lane, label or ceremony is created in this phase. Also amend DEC.22 (seed), DEC.46 ("may be committed"
  is moot) and DEC.47 (add `fm_sweep`). The pins gathered for that later work are in the canonical references.

### Verification of SND-05
- **D-35 (spectral and cost tests):** The spectral test drives the synth interface directly (independent of the PSG
  period 0/1 rule): a square wave whose half-period is 16 Z80 cycles (the 111.9 kHz "period 1" stress tone) and one
  at 96 cycles (18.6 kHz, "period 6"), rendered for about 12k samples with the first 4k discarded, then a
  deterministic **integer** DFT so the verdict is identical on be32 and wasm32 (floating point in tests is not
  forbidden, but an integer test removes a platform variable): a Q30 sin/cos table for M = 8192 built by half-angle
  isqrt from (0,1) then rotation, a 4-term Blackman-Harris window, a direct DFT to 16 kHz (about 22M
  multiply-adds), harmonics of the tone masked, reference = the programmed step height, pass when
  `peak² × 10^8 ≤ ref²` (−80 dB). A cadence test sums 1,000 frames and checks the position of the long frames. The
  cost bound is a deterministic counter, not a clock: count kernel applications and PSG steps per frame and assert
  them against the formula (PSG steps ≤ 3,734 + OUT writes ≤ 59,736/11 + FM samples ≤ 830, times 16 taps times the
  ring count) under the hostile hammer fixture; FM's 18 × samples slot-evaluation count (D-18) is the FM part.
  Amended at planning (new evidence: per-event accounting, plans 04-06 and 04-12): one PSG step can change up to four
  channel levels (three tones toggle and the noise shifts on the same step) and one OUT write at most five (a GG $06
  write re-routes up to four PSG sources plus itself; an $F2 write on the FM profile re-feeds PSG sources 0-3 and FM
  source 5, five ring sources, source 6 being only a gate), so the asserted bound is kernel applications <= 16 x rings
  x (4 x PSG steps + 5 x OUT writes + FM samples), the FM term being one source-5 event per sample and zero without
  an FM unit; PSG steps <= ceil(frame_ticks / 48), FM samples <= ceil(frame_ticks / 216) and FM slot evaluations
  exactly 18 x samples are asserted alongside.

### Claude's Discretion
- Exact Q12 gain constants (D-07) and the exact trace-record capacity and struct shape (D-23), subject to the
  stated proofs; file and function names inside the stated modules; the order of plans (the research agents'
  suggested spine: synth and PSG with units, then trace and `--hash-audio` with fixtures, then FM, then libretro and
  the hosted check); whether the lockstep oracle fetch is nightly or a build-time test.



</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Requirements and decisions
- `.planning/REQUIREMENTS.md` SND-01 to SND-05, and `.planning/ROADMAP.md` Phase 4 — scope and success criteria
  (SND-01/02 wording to be amended per D-33).
- `.planning/preparation/DECISIONS.md` — DEC.04, 08, 22 (amended), 34, 35, 36, 38, 42, 46, 47, 50, 61.
- `.planning/preparation/SMS-AUDIO.md` — §1 time base, §2 PSG, §3 Game Gear, §4 YM2413, §5 synthesis and hashes,
  §6 A/V sync, §7 tests, §9 open questions. Known stale items listed below.
- `.planning/preparation/ARCHITECTURE.md` — §4 public API, §5 I/O contracts, §6 profiles, §7 state, §8
  observability, §13 open conflicts (PSG and FM rows).
- `.planning/preparation/CONFORMANCE.md` §2, §6, §7; `.planning/preparation/SMS-SYSTEM.md` §5, §8, §9;
  `.planning/preparation/HOSTS-AND-INTEGRATION.md` (libretro, RetroArch); `.planning/preparation/ENGINEERING.md`.
- `PROVENANCE.md` (PV.03 ymfm, PV.02 ares, PV.05 nesturbator; add emu2413), `THIRD-PARTY-NOTICES.md`,
  `ASSET_POLICY.md` §2, `.planning/LESSONS.md` (XFER-011, XFER-016, XFER-044, XFER-067).
- `.planning/phases/03-games-play-in-retroarch/03-CONTEXT.md` and `.continue-here.md` — what phase 3 left.

### Stale or wrong statements the research found (correct them in the plan that touches the file)
- SMS-AUDIO §7: cadence sum 801,034 (is 801,031); "65,535-step" LFSR (is 57,337); "801, 801, 801, 802" (a 802 every
  32 frames); FM-MIX and ARCHITECTURE §5 "cannot clip by construction" (false at Q12 4096:1434); §2.3 volume entry 6568
  (is 6538); §4.3/§9 "emu2413 calls sin/pow/log at init" (only the deletable rate converter does; `exp_table` and
  `fullsin_table` are literals); §4.1 "%10 on export" stated as fact (open); §4.1 "detection sets `$3E` bit 2"
  (unsupported); "transition hash unaffected by retunes" holds only with attenuation-coded levels (D-25); `u64
  cycle` should read tick; SMS-AUDIO §5 fps rounded form; "table switch never changes the transition count".
- CONFORMANCE §2: `boot_to_play` "already changes PSG channel 0" and `expected.txt` "holds frame and audio hashes"
  (the asm has no PSG writes; `expected.txt` holds image SHA-256 only); a unit test deriving the stream "without
  the Z80" cannot match write stamps (use model-level tests plus pinned ROM-run digests).
- DEC.42 lists PSG power-on values as profile fields (they are constants); DEC.46 "may be committed";
  HOSTS-AND-INTEGRATION "RetroArch aborts in the agent sandbox" and "`audio_driver=null` isolates audio".

### External primary sources (pinned)
- emu2413 at `a1d5120903b75f93d3eb7690e34a29c0b4add903` (MIT); nesturbator `src/synth.c` at `9f9386c661f6`
  (MIT); ares `9408cb43d4948fc3ea6e152a307a34348df3fe04` (ISC, fallback); ymfm `81aec25ccbb9` (BSD-3, fallback).
- For the deferred third-party ROM lane only (MIT): JoppyFurr SN76489 TestRom `7d2068dfc1713890844f47a0adc00a6561d07cf0`,
  YM2413 TestRom `571018fe8bfb0680545bf7a60240e32ae8cfcb5d`, devkitSMS `e11dc64e51ad` (the 2024-04-08 commit that adds
  `crt0_BASIC`); const mode needs Right, Down, then button 2; the ROMs report nothing.
- libretro.h (vendored pin) audio callbacks; RetroArch 1.22.2 recording behaviour (measured this step).

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `src/io.c`, `src/bus.c`, `src/state.c`, `src/instance.c`: the PSG joins `bus_out` and the state visitor; the I/O
  state struct takes `audio_ctrl`.
- `tests/cmake/hashes.cmake`, `tests/retroarch/run_retroarch.cmake`, `tests/libretro/host.c`,
  `tests/fixtures/tests.cmake`, `tests/roms/src/mailbox.inc`, `boot_to_play.asm`: the hash matrix, the RetroArch
  driver, the in-process host, the fixture runner pattern and the mailbox protocol audio rides on.
- `print_hash` in the runner (byte-by-byte pixel serialisation) is the model for LE serialisation of records and PCM.

### Established Patterns
- Every behaviour lands with a test in `tests/inventory/ci.txt` (and `wasm32.txt` where it runs there); digests are
  pinned in text files and regenerated by a documented command; no self-skipping tests (DEC.50); repository-check
  code stays under a tenth of `src/` lines; RetroArch e2e code is capped at 300 lines and checked by
  `policy.e2e_budget` (284 + 16 today).
- Lazy catch-up per chip with raw causal event emission; no hooks yet (hooks are v5); API structs grow by appended,
  explicitly padded fields with sizeof asserts.

### Integration Points
- `cogmaster_config` (+`trace_records`), `cogmaster_media` (+`reserved2`, `fm`), `cogmaster_info`
  (+channels, fm), `cogmaster_get_audio`, `cogmaster_get_audio_trace`; the libretro adapter's `lr_audio`,
  core options and `.info`; the runner's `--hash-audio`; the state visitor (`psg_io`, synth, `opll`).

</code_context>

<specifics>
## Specific Ideas

- Owner's standing preference: another copy and paste beats another dependency; smaller, flatter dependency trees.
  Applied here: emu2413 is copied with its notice (no package), the kernel generator is one tool run once, no new
  runtime dependency anywhere in the phase.
- Owner's standing verification stance: automation-first; the agent runs RetroArch locally and verifies audio
  itself; nothing in this phase asks the owner to listen.

</specifics>

<deferred>
## Deferred Ideas

- Third-party ROM conformance: building JoppyFurr's SN76489 and YM2413 ROMs from source at the pinned commits and
  checking closed-form transition-stream properties under a recorded pad script (a nightly locator; see D-33).
- The FM ledger predicate over transition counts ("`$F2` bit 0 set and at least 1 YM data write by frame N", with
  the export-profile negative as the Monster World 2 regression catch, Gearsystem #122/#125 titles only), to the
  tune-up (DEC.61), with the `docs/conformance.md` line about unavailable sound predicates.
- 44.1 and 96 kHz output, the `audio_rate_hz` config field and the 16 × 64 Q20 kernel fallback with `tools/kernelgen`
  (D-03, D-06).
- `--vgm` replay input (ARCHITECTURE §11 lists it; ROADMAP and REQUIREMENTS omit it; it adds a hostile-input parser
  and bypasses the Z80 and bus path); a VGM-rendered fixture from the MIT test ROMs.
- A runner `--pcm-out` raw dump (would give `cmp`-exact hosted recording checks with the nearest resampler; needs
  shift stability measured on the hosted runner first); a Linux or Windows hosted audio lane (needs xvfb and an
  FFmpeg build; `afinfo` is macOS-only).
- Hash pins at 44.1 and 96 kHz; runtime rate change; `--audio-rate` runner flag; promoting the wasm32 hash
  comparison to required; a minimum-phase kernel; a GG speaker low-pass or `gg_speaker` hint.
- Per-chip or per-channel volume and mute options; Mark III + FM unit profile; a table switch (YMF281B, VRC7 or the
  ymfm lineage) once a hardware capture exists; an idle-chip FM fast path; skipping FM work while muted; stereo pan
  for FM add-ons; a diagnostic counter for early YM writes.
- Run-ahead and rewind (needs public states, SEED-002); `GET_AUDIO_VIDEO_ENABLE`, `SET_MINIMUM_AUDIO_LATENCY` and
  buffer-status frame-skip.
- SG-1000 / SC-3000 15-bit LFSR profile constants and the $400 period rule; READY stall; noise phase coupling to
  tone 2; `sms2_clip`; a ticks-stripped "levels" hash and a per-source count in the audio line; a ROM-sharing
  generator giving the `psg_sweep` script one source for both asm and C.
- Hardware captures (period 0 and SMS1, SMS2 clipping, FM/PSG ratio against the Japanese BIOS theme, export `$F2`
  read, soft-reset chip state, early-write behaviour): none can be obtained by the agent; the defaults here are
  recorded with their flip evidence and stay open in SMS-AUDIO §9.

</deferred>

---

*Phase: 04-Sound*
