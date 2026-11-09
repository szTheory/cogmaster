# Phase 4: Sound - Research

**Researched:** 2026-11-09
**Domain:** Deterministic SN76489 and YM2413 audio in a C17 integer-only emulator core; band-limited synthesis; audio hash pins; libretro audio delivery; a hosted RetroArch recording check
**Confidence:** HIGH for the codebase facts and pins (every in-repo value below was read this session); MEDIUM for line-budget estimates and runtime costs on be32/wasm32 (not measured)

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
### Time base, synthesis and mix
- **D-01 (time unit):** Every audio timestamp is the absolute bus **tick** (3 ticks per Z80 cycle) since the last
  POWER reset or load; a SOFT reset does not restart the count. Never "cycle": where SMS-AUDIO §5 says `u64
  cycle`, read tick (DEC.34 and ARCHITECTURE §5 already say tick). The rational constants quoted below
  (176/13,125 and so on) are per Z80 cycle; per tick the modulus is three times larger (NTSC 39,375). —
  **Reversibility:** costly — the hash record layout and every pinned digest depend on it.
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
  phase is derived from absolute ticks (a frame is 3,733.5 steps, so it is not frame-aligned). Volume writes are
  combinational: catch up through τ, apply, stamp the record at τ, not at the next tick (this is what makes the
  GG "SEGA" chant and other PCM-by-volume tricks work). READY stall is not modelled.
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
- **D-18 (cadence, writes, state):** Sample k completes at T-state 72k since power-on (216 ticks). A write first
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
  counter bit 3, bit 6 = bit 7, bit 7 = bit 11), bits 4–2 zero; the counter derives from the absolute line index
  (no new state). Values: 0 PSG only, 1 FM only, 2 both off, 3 both on; muting is applied at the mixer, so the
  chips keep running. Power-on and both reset kinds: `$F2 = 0`, YM registers cleared, PSG silent (matches MAME's
  reset and the BIOS hand-off; whether a soft reset really clears the chips on hardware is unverified, so a test
  pins it and SMS-AUDIO §9 records it). On smsj, $F0–$F2 are independent of `$3E` bit 2. Without FM keep today's
  behaviour: writes ignored with no fault; export reads mirror $DC/$DD; GG and fm=off Japan read open bus; reads of
  $F0–$F2 never set `input_read` (DEC.38). The "%10 on export" value stays an unverified claim and is not
  implemented. Keep `io.decode_mirrors` and `bus.fault_not_faults` passing.
- **D-22 (libretro options and state):** exactly one new libretro option, `cogmaster_fm` (auto | off | on, reload
  content), added in all three places (`lr_definitions`, the v1 fallback `definitions[6]` with loop `i < 6`, and the
  `retro_variable` array). Per-voice or per-chip volume is deferred. `audio_ctrl` joins `struct cogmaster__io`
  (the `_Static_assert` of 8 forces an `io_io` update); one copy of the GG mask stays in `io.gg[5]`; one byte of
  the header's reserved space becomes `fm_unit` (`state.c` has a one-byte profile-reserved at offset 27 and a
  12-byte reserved block at 52; the planner picks one and states the remaining count); every new audio field gets a field-range sweep
  entry; LFSR ≠ 0 is cross-checked on load; undrained output is cleared on load.

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
  POWER reset; a SOFT reset happens at a later tick and does not restart the count, D-01), GG mask only on GG, FM
  and control only with an FM unit, so power-on state is hashed and the per-profile source set is checked.
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

### Claude's Discretion

- Exact Q12 gain constants (D-07) and the exact trace-record capacity and struct shape (D-23), subject to the
  stated proofs; file and function names inside the stated modules; the order of plans (the research agents'
  suggested spine: synth and PSG with units, then trace and `--hash-audio` with fixtures, then FM, then libretro and
  the hosted check); whether the lockstep oracle fetch is nightly or a build-time test.

### Deferred Ideas (OUT OF SCOPE)


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
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| SND-01 | SN76489 model, every tone/volume/noise setting, GG stereo (amended by D-33 to the owned `psg_sweep` fixture plus unit tests) | Findings F4 (native_gg gate), F9 (fixture machinery), Validation Architecture; `bus_out` no-op at `src/bus.c:247-259` is the wiring point |
| SND-02 | YM2413 for the Japanese profile, `$F2` PSG/FM/none/both, OPLL seed notice (amended by D-33 to lockstep + register units + owned `fm_sweep`) | emu2413 pin verified (200), file sizes, SHA-256, port plan; findings F2, F7, F8 |
| SND-03 | `--hash-audio` transition and PCM hashes identical on every platform; exact 3,504,512/4,375 samples per NTSC frame; no clipping | Runner symbols, `hashes.cmake`/`hash_matrix` extension, CI `hash-equality` (7 lanes), findings F5, F6, F10 |
| SND-04 | libretro batch and single-sample paths equal the runner's PCM; hosted RetroArch recording non-silent | Adapter symbols, `host.c` structure, 300-line e2e budget (exactly 300/300), findings F11, F12 |
| SND-05 | Spectral peak below 16 kHz under -80 dB; bounded synthesis cost under a register-hammering fixture | Kernel table facts (nesturbator pin), deterministic integer DFT, counter-based cost test |
</phase_requirements>

## Summary

The discuss step locked 35 decisions; this research verified them against the repository and found the codebase
ready for the design with **one real conflict and several traps that the plans must absorb**. The conflict: D-01
and D-24 say a SOFT reset does not restart the tick count, but `cogmaster_reset` calls `cogmaster__bus_origin`,
which zeroes `ticks` and `frame` for **both** kinds, and that behaviour is documented in `docs/embedding.md`
and asserted by `tests/bus/test_bus.c` and `tests/core/test_state.c`. The traps: the state header's byte 27 is
already `video_mode` (the only reserved space is the 12 bytes at offset 52), the public counter `audio_dropped`
has no public accessor (the runner's "exit 1 unless `audio_dropped` is 0" needs one), the existing
`runner.usage.unknown` test asserts `--hash-audio` is an unknown option, `docs/embedding.md` (36,732 of 40,960
bytes) and `docs/testing.md` (38,796 of 40,960) have almost no room, `hygiene.no_float` scans **comments** in
`src/` for the pattern digit-dot-digit (so a pasted "v1.6.4" fails), `policy.notices` does not enforce an emu2413
section for a plain `MIT` SPDX line, and `policy.e2e_budget` is exactly 300/300.

Both pinned external sources answer 200 and were fetched with `gh api` into the scratchpad (not the workspace):
emu2413 `emu2413.c` is 1,552 lines / 47,757 bytes with SHA-256 `35d492c1...abe5`; nesturbator `src/synth.c` is
2,786 lines, and its kernel is 32 phases x 16 taps of Q15 with every row summing to exactly 32,768 (checked). I
estimate the pruned emu2413 logic at about 860 non-comment lines in the original 120-column layout, so roughly
900 to 1,000 after clang-format at 100 columns: under the 1,100-line trigger, but close enough that the FM plan
must measure early.

**Primary recommendation:** land the one-way doors first in a single small plan (API 9 header layout, behaviour
revision 11, state format 11 with the fm byte in the header's reserved block, `tests/api/v9.c`, the amendments),
resolve the reset-tick conflict by keeping the existing reset contract and amending D-01's SOFT-reset sentence,
then build synth+PSG with units, trace+`--hash-audio`+fixtures, FM, libretro, and finally the hosted recording.

## Findings that change or constrain the locked decisions

Each is evidence from this session; none reopens a decision's intent. "Plan action" is what the plan must do.

**F1. SOFT reset restarts ticks today (conflicts with D-01 and D-24).**
`cogmaster__bus_origin` sets `m->ticks = 0u; m->frame = 0u;` [VERIFIED: src/bus.c:36-40] and `cogmaster_reset`
calls it for both kinds [VERIFIED: src/instance.c:271]. The doc says "Both kinds set the frame count and the
tick count to zero" [VERIFIED: docs/embedding.md:305-306]; `test_frame_origin` runs both kinds and calls
`expect_origin` [VERIFIED: tests/bus/test_bus.c:217-228]. The state header's `ticks` is `time.ticks >= start`
checked against `frame` [VERIFIED: src/state.c:98-103]. Making SOFT keep ticks would change a documented,
tested contract and the state validation. **Plan action:** keep the contract; amend D-01/D-24 to "ticks since the
last load or reset of either kind; the trace snapshot is stamped tick 0 on load and on both resets". The PCM
accumulator still resets only on POWER reset or load (D-06), so PCM stays continuous across a SOFT reset. The
runner never calls `cogmaster_reset` (grep: only `libretro/libretro.c:289` and tests call it), so no pinned hash
crosses a reset; the per-source monotone-tick assertion in the runner is unaffected. The libretro `retro_reset`
calls SOFT on SMS and POWER on GG [VERIFIED: libretro/libretro.c:289-290].

**F2. State header: byte 27 is `video_mode`; the only reserved space is 12 bytes at offset 52 (corrects D-22).**
`header_io` writes machine, region, tv and `video_mode` as four fixed bytes: `fixed(io, "video_mode",
m->video_mode, 1u);` [VERIFIED: src/state.c:84-87], and the reserved block is `#define H_RESERVED 52u` and
`#define H_RESERVED_SIZE 12u` [VERIFIED: src/state.c:30-31]. The layout comment at `state.c:6-8` and
`state.c:27` still says "profile reserved 27" and is stale. **Plan action:** the first reserved byte (offset 52)
becomes `fm_unit` (fixed to the resolved value, like `machine`), `H_RESERVED` becomes 53 and
`H_RESERVED_SIZE` 11, the `_Static_assert` at `state.c:32` still holds, and the comments are corrected. Also
there is **no signed visitor helper**: `state.h` has only `cogmaster__sio_u8/u16/u32/u64` [VERIFIED:
src/state.h:119-157]. The int64 integrator, ring slots and DC state need a signed helper (bias by 2^63 in
VALIDATE) or sign-magnitude fields; add it to `state.h` once and use it for every signed audio field.

**F3. Struct tails and sizes (D-20, D-09).** Current asserts, verbatim: [VERIFIED: src/internal.h:234]
`_Static_assert(sizeof(cogmaster_config) == 4 * COGMASTER__P, "config size");` [VERIFIED: src/internal.h:253]
`_Static_assert(sizeof(cogmaster_media) == (COGMASTER__P == 8 ? 48 : 28), "media size");` [VERIFIED:
src/internal.h:269] `_Static_assert(sizeof(cogmaster_info) == 60, "info size");` and the first-size macros
`COGMASTER__FIRST_SIZE_CONFIG ((uint32_t)(4 * COGMASTER__P))`, `COGMASTER__FIRST_SIZE_MEDIA ((uint32_t)(4 *
COGMASTER__P))`, `COGMASTER__FIRST_SIZE_INFO 48u` [VERIFIED: src/internal.h:329-336]. Resulting layouts to assert
(computed from the field order read in `include/cogmaster.h:97-131`; all other offsets unchanged):

| Struct | Now | Append | New size |
|---|---|---|---|
| `cogmaster_config` | 32 (64-bit), 16 (32-bit) | `trace_records` at 4P; explicit `reserved` on 64-bit | 40 (64-bit), 20 (32-bit) |
| `cogmaster_media` | 48 (64-bit, pad at 44-47), 28 (32-bit) | 64-bit: `reserved2` at 44, `fm` at 48; 32-bit: `fm` at 28 | 56 (64-bit), 32 (32-bit) |
| `cogmaster_info` | 60 | `audio_channels` at 60, `fm` at 64 | 68 |

`cogmaster_load` reads later media fields through `media_field(media, off)`, which returns 0 unless `off + 4 <=
size` [VERIFIED: src/instance.c:109-117], and checks video with `media->size > (sizeof(void *) == 8u ? 40u :
24u)` [VERIFIED: src/instance.c:167-169]; read `fm` the same way so an API-8 caller (size 48) gets `fm = AUTO`.
`trace_records` is hostile input (a u32 times a record size on a 32-bit `size_t`): cap it (for example
1 << 20 records) and return `ERR_ARGUMENT` above, and compute the byte count in a width that cannot overflow.
The new `fm` field also needs `COGMASTER_FM_AUTO/OFF/ON` constants (values 0/1/2 so zero stays AUTO) and
`ERR_PROFILE_REJECTED` for ON with a GG machine and for any value above 2.

**F4. `machine == GG` versus `native_gg` (D-13) touches more than `io.c`.** The gate is `is_gg(m) { return
m->machine == COGMASTER_MACHINE_GG; }` [VERIFIED: src/io.c:50-53], used by `full_decode`, `tr_pin`, `port_dc`,
`th_pins`, `port_dd`, `io_reset` (`io->mem_ctrl = is_gg(m) ? IO_MEM_GG : IO_MEM_SMS;`, `src/io.c:73`), `io_in`
(`if (is_gg(m) && p <= 6u)`, `src/io.c:169`) and `io_out` (`if (is_gg(m)) { ... p >= 1u && p <= 6u ...`,
`src/io.c:193-196`). In `io_out` a GG console in SMS mode currently drops `$3E` into `unmapped_write`
(`src/io.c:214-216`). `state.c` has the same console-based gate: `int gg = m->machine == COGMASTER_MACHINE_GG;`
then `io_mem_ctrl` range `gg ? 0xA8u : 0u .. gg ? 0xA8u : UINT8_MAX` [VERIFIED: src/state.c:300-301]. Native
mode is `m->video_mode == COGMASTER_VIDEO_GG` [VERIFIED: src/internal.h:205-208]. **Plan action:** switch only the
ports `$00-$06` and `$3E` decode (and `io_io`'s `$A8` validation and `io_reset`'s `$A8` default) to
`native_gg`; the tests that pin the native behaviour are `io.gg_ports` (uses the native `gg()` helper,
`tests/io/test_io.c:185-209`) and the `ports_gg` mailbox rows in `tests/diag/expected.csv`; the `gg-sms`
boot_to_play hash rows (`tests/cmake/hashes.cmake`) read `$DC` and are not expected to move. Decide explicitly
what `full_decode` means for a GG console in SMS mode (D-12 names only `$06`); add a test for it. This is a
behaviour change and is covered by the single revision bump.

**F5. `--hash-audio` is pinned as an unknown option by an existing test.** `tests/runner/tests.cmake:533` loops
`foreach(opt --hash-audio --capabilities --boot --bios)` expecting exit 2 "unknown option", and `:547` fails if
`--help` mentions any of them [VERIFIED: tests/runner/tests.cmake:529-548]. Remove `--hash-audio` from both lists
in the same plan that adds the flag. `args.c` has a hand-written if/else chain (`--hash` at `args.c:248`) and a
synopsis string (`args.c:13-35`); `check_combination` (`args.c:166-191`) is where `--hash-audio` combinations
(for example with `--bench`) are decided. `print_hash` is `runner/main.c:113-141`; `run` is `main.c:264-473`.

**F6. `audio_dropped` has no public reader.** It exists in `struct cogmaster__counters` [VERIFIED:
src/internal.h:43] and in the state visitor [VERIFIED: src/state.c:117], and is never written today (grep). The
runner can only see public structs. **Plan action:** put a `dropped` count in the public audio struct (or in the
trace struct) and define it: samples (or frames) the previous `run_frame` produced that the host did not drain
before the next one. Recommended: `get_audio` returns what is pending; `run_frame` counts any undrained pending
samples into `audio_dropped` and replaces the buffer (bounded memory, never silent loss).

**F7. Docs budgets bind now.** `docs.budgets` limits each `docs/*.md` to 40,960 bytes, README to 12,288 and
`include/cogmaster.h` to 24,576 [VERIFIED: tests/cmake/budgets.cmake:2-6]. Measured: `docs/embedding.md` 36,732,
`docs/testing.md` 38,796, `docs/runner.md` 23,763, `docs/conformance.md` 28,966, `docs/maintaining.md` 14,017,
`include/cogmaster.h` 7,010, `README.md` 3,303 bytes. `docs.api_names` requires every `cogmaster_*` and
`COGMASTER_*` name in the header to appear in `docs/embedding.md` and vice versa [VERIFIED:
tests/cmake/repository.sh:`api_names` branch]; `embedding.md` already carries a full "Names" list (line 450)
and a stale "exact rational silence pacing" sentence (line 487). **Plan action:** put audio behaviour prose in a
new `docs/audio.md` (each new doc gets its own 40,960 budget), keep `embedding.md` to the new names, the version
bump and one pointer, and delete stale text in `embedding.md` and `testing.md` to make room (the hosted-RetroArch
section is `testing.md:379-413`). Stale sentences to fix: `README.md:8` ("audio is silent"), `docs/runner.md:3`,
`:9`, `:309`, `docs/conformance.md:60`, `docs/embedding.md:487`, `docs/testing.md:431`.

**F8. `hygiene.no_float` scans comments and identifiers in `src/` and `include/`.** The rule flags the words
`float`/`double`, `<math.h>`/`<float.h>`, hex floats, exponent literals and any digit-dot-digit such as `1.5`
[VERIFIED: tests/cmake/repository.sh:24-25, regex `[0-9]\.[0-9eE]`], after stripping hex literals and the word
"section N.N". It scans `*.c` and `*.h` only, so `src/opll_tables.inc` is not scanned but anything in the
`.c`/`.h` is, comments included. emu2413's header reads `emu2413 v1.6.4` and "Copyright (C) 2026": write the
provenance comment without any dotted version number (write the 40-hex
commit instead). `*.inc` is also outside the `core` line count: `budgets.cmake` globs `src/*.c` and `src/*.h`
[VERIFIED: tests/cmake/budgets.cmake:45]. Hygiene also runs `clang-format` over every tracked `*.c`/`*.h`
including tests, with `ColumnLimit: 100`, 4-space indent, `BreakBeforeBraces: Linux` [VERIFIED: .clang-format].

**F9. `policy.notices` will not enforce an emu2413 notice for a plain `MIT` SPDX.** The check accepts `MIT` or
`MIT AND (BSD-2-Clause|BSD-3-Clause|Zlib|BSL-1.0|ISC|CC0-1.0)` and only demands a notices section for the
`AND` form [VERIFIED: tests/cmake/repository.sh:115-119]; its "Applies to" path scan only matches lines that
begin with `**Applies to:**`, whereas the existing sections use a bulleted `- **Applies to:**`, so it matches
nothing today. SND-02's "notice present in `THIRD-PARTY-NOTICES.md`" therefore needs its own assertion: add the
cheapest one (a `string(FIND)` in an existing CMake test, or an awk line in the `notices` branch; each check line
costs 10 `src` lines of the one-tenth budget, which has room once `src/` grows by thousands of lines, but note
today's margin is 18 lines: 528 counted check lines x 10 = 5,280 against 5,298 `src` lines [VERIFIED: computed
from `tests/policy/repository-checks.txt` `check` rows and `cat src/*.c src/*.h | wc -l`]). Per `PROVENANCE.md`
5a, nesturbator (same author, MIT) seeds get **no** header and no notices section, only a commit-body line
`Seeded from nesturbator 9f9386c661f6 src/synth.c (same author, MIT).` [VERIFIED: PROVENANCE.md:88-89]; D-03's
"copied with its notice" is satisfied by that line plus a new strangler-fig row (section 4) and a section 10
source entry. emu2413 gets an SPDX line (`MIT`), the upstream path and commit, both copyright years, a notices
section with LICENSE verbatim, and the section 2 / 4 / 10 rows of `PROVENANCE.md`.

**F10. The test card must deliver audio.** `cogmaster_run_frame` has a no-CPU branch for the card
(`src/frame.c:62-78`) that advances `ticks`, `bus.at` and `deadline` by a whole frame. D-29 deletes the
adapter's silence generator, and `host.c` asserts 801/802 per frame, exactly 3,504,512 samples over 4,375 frames
and `batch_nonzero == 0` for the card [VERIFIED: tests/libretro/host.c:665-697], plus `run_one() == 801u` after a
reload (`host.c:717`). So the synth frame-end (zeros, correct count, correct accumulator) must run in **both**
branches of `run_frame`, and `cogmaster__synth` state must reset on load so a reload starts at 801.

**F11. `host.c` details the three-session test must respect.** `content_test` reads exactly 32,768 bytes
(`uint8_t image[32768]`, `CHECK_EQ_U64(n, sizeof image)`) and detects GG from the header (`(image[0x7FFFu] >> 4)
== 6u`) [VERIFIED: tests/libretro/host.c:431-439], which matches `tests/roms/src/header.inc` (one 32 KiB bank,
`ROMSIZE $C`). Every new fixture image used by the host must therefore be exactly 32 KiB. `set_callbacks()`
registers both audio callbacks (`p_set_audio_sample(sample); p_set_audio_sample_batch(rgb565 ? NULL : batch);`)
[VERIFIED: tests/libretro/host.c:413-420]; the "batch only", "single only" and "both" sessions need per-session
registration, not a new module. The `env` stub always answers core-options version 2
(`host.c:219-222`), so the v1 and v0 option fallbacks in `libretro/options.h` have **no test today**; D-22's
"all three places" is a manual guarantee unless a cheap env switch is added.

**F12. The adapter's v1 fallback sizes.** Today the array is `static struct retro_core_option_definition
definitions[6];` with `for (unsigned i = 0u; i < 5u; i++)` [VERIFIED: libretro/options.h:55-56], five options and
a zeroed terminator. Adding `cogmaster_fm` makes it `definitions[7]` and `i < 6u`, a sixth `lr_definitions`
entry before `{0}`, and a sixth `retro_variable` row before `{NULL, NULL}`; D-22's wording ("`definitions[6]` ...
`i < 6`") describes the result after one option is added to a 5-option base only if the array grows to 7.
The adapter's `LR_SAMPLE_RATE 48000u`, `LR_AUDIO_CAPACITY 966u`, `audio_acc`, `silence[LR_AUDIO_CAPACITY * 2u]`
and `lr_audio` are at `libretro/libretro.c:21-23, 57-58, 294-313`; `lr_unload` clears `audio_acc`
(`libretro.c:129`). `info.size` handling: the adapter memsets `cogmaster_config` and sets `cfg.size`
(`libretro.c:177-186`), so appending `trace_records` is transparent to it.

**F13. Perf gate and release rehearsal.** The CI `perf` job measures Cachegrind instructions per frame of
`boot_to_play.sms` and fails over +1.00 percent unless `tests/perf/allow.txt` has a dated line (max 60 days out;
format `<rom> <max % x 100> <expiry YYYY-MM-DD> <reason>`) [VERIFIED: tests/perf/allow.txt, tests/cmake/perf_compare.cmake:1-16];
the workflow currently sets `COGMASTER_PERF_MODE: report` [VERIFIED: .github/workflows/ci.yml:29], so it reports
and does not block, but add the allow line anyway (and write it in the same PR as the synth lands). Any edit to
`.github/workflows/ci.yml` sets `release_inputs=true` and triggers the release **rehearsal** job
[VERIFIED: scripts/classify-changes.sh:35-37]; the hosted-recording plan edits `ci.yml` (artifact list and the
`retroarch` job), so expect that job in the PR.

**F14. `be32` and `wasm32` run only `portable`-labelled tests** (`testPresets` filter `^portable$`,
timeout scales 8 and 5), and `abi.*` tests are not `portable`, so `abi.undefined`'s ppc32 allow-list
(`__udivdi3 __umoddi3 __ashldi3 __lshrdi3` [VERIFIED: tests/cmake/repository.sh:85]) is exercised only on 64-bit
legs. Consequence for new tests: label everything that must run on every platform `portable`, and keep each
one's native cost low (the integer DFT is about 22 million multiply-adds; the hash matrix adds about nine
300-frame runs; measured native cost today is 1.43 s for 3,000 `boot_to_play` frames, about 0.5 ms per frame
[VERIFIED: `time cogmaster-run ... --frames 3000` on this machine]). `runner.hashes_file` has `TIMEOUT 30`
(x8 on be32, x5 on wasm32); re-measure and raise it with a comment, and re-check the `be32` job's
`timeout-minutes: 5` (34 s measured for the whole job).

**F15. `OPLL_calc` needs `clk` to be a multiple of 72 for the lockstep oracle.** The loop is `while (opll->out_step
> opll->out_time) { opll->out_time += opll->inp_step; update_output(opll); mix_output(opll); }` with `out_step =
clk` and `inp_step = rate * 72` [CITED: emu2413 @a1d5120903b7 emu2413.c lines 1484-1495, 1121-1130]. If `clk` is
exactly `72 * rate` (for example `rate = 49716`, `clk = 3579552`), each call runs exactly one `update_output` and
`conv` is NULL (the `floor(f_inp) != f_out && floor(f_inp + 0.5) != f_out` test is false). With the real clock
3,579,545 the call count per sample is not 1. The oracle harness (the original, with its float tables) must use
`clk = 72 * rate`; the port's own sample is "72 T-states".

**F16. Fixture addresses to keep clear.** `boot_to_play.asm` uses `$C200-$C208` for its counters and the mailbox
occupies `$C100-$C1FF` [VERIFIED: tests/roms/src/boot_to_play.asm main, mailbox.inc header]. The compat ledger
pins `fixture/boot_to_play ... ram@14:$C200=00` and `frame@14=6d28fda9...` (SMS NTSC-U) [VERIFIED:
tests/compat/ledger.csv:2-3], so the added PSG writes and chime state must use other RAM (for example `$C210+`)
and must not move the frame-14 picture. The `compat.fixtures` test (portable) judges those rows.

**F17. Cadence statement refined.** Computed with integer math: NTSC long frames (802) fall at frames 32, 64, 96,
... with gaps of 32 early on; 31 of the first 1,000 frames are long and the sum is 801,031 [VERIFIED: python3
`k*3504512//4375` differences]. PAL over 1,000 frames: 766 frames of 966 and 234 of 965, sum 965,766 [VERIFIED:
same method with 685,094,400/709,379]. The long-frame spacing is 32 or 33 (137 long frames per 4,375), so a test
should assert the prefix-sum formula `floor(F x 3,504,512 / 4,375)` rather than a period of 32.

## Project Constraints (from CLAUDE.md)

Directives that bind plans and code (read from `CLAUDE.md` and `.claude/CLAUDE.md` this session):

- **Reuse permissively, never GPL.** MIT/BSD/ISC/zlib/CC0 seeds with notices; never open GPL/LGPL/unlicensed
  emulator source; reference emulators run only as released binaries; no copy kept in the workspace (fetch with
  `gh api` to a scratch directory only). Nuked-OPLL is GPL-2.0: closed.
- **Three deliverables** (library, runner, libretro core); no window, audio-device or input-device code.
- **Clean tree:** no ROM/BIOS/save bytes except `tests/roms/manifest.txt` fixtures; no personal paths, e-mail
  addresses or names; noreply identity; `scripts/hygiene.sh` on every commit and push.
- **Deterministic, bounded core:** integer-only, all state in the instance, same frame and audio hashes on every
  platform, every call bounded in emulated time (hostile or jammed ROM), no wall clock.
- **Tested and documented together, within budgets:** every behaviour change lands with a test that
  `cmake --workflow --preset ci` runs; README short and user-facing; testing/CI/maintainer docs in `docs/`; the
  public header terse with behaviour prose in `docs/`.
- **Automation-first verification; the owner verifies nothing by default.** The agent runs RetroArch 1.22.2
  itself and reads docs against code; ask the owner only for identity-bound consent or their own hardware.
- **Proportional gates:** repository-check code (CMake `-P` or POSIX sh) under a tenth of `src/` lines; real-host
  e2e (RetroArch) code under 300 lines in total, checked by a test; every gate names the fault it prevents; at
  most two code-review fix rounds and one gap round per phase; review findings about test or tooling code
  become seeds, not requirements.
- **No invented ceremony:** no attempt quotas, formal slots, admission/publication ceremonies, lifetime counters
  or qualification campaigns; a failing harness assertion is a test bug to triage.
- **One GSD step per session**, then stop; never run `/gsd-next`, `/gsd-autonomous`, `/gsd-manager`, `--auto` or
  `--chain`, `/gsd-profile-user`; never set `COGMASTER_ALLOW_CHAIN` or `COGMASTER_ALLOW_MAIN_COMMIT`; record
  runtime and model in STATE.md at every phase step.
- **Branch then pull request:** `phase/04-sound` (current branch is `phase/04-sound`); Conventional Commit PR
  title; `gh pr merge --auto --squash` in a shell; confirm a CI run on the head commit; confirm the release PR
  is green afterwards. Evidence is a command, an exit status and a run URL, never a committed log.
- **Pins are public:** a pin answers 200 for `gh api repos/<o>/<r>/commits/<sha>` (done for emu2413 and
  nesturbator); sibling code is fetched with `gh api`, never read from a local checkout.
- **Plan SUMMARY format:** `requirements-completed:` and a block-style `coverage:` list (id, description,
  requirement, verification {kind, ref, status}, human_judgment); no flow style `{}`, no inline `#` comments in
  that block; acceptance checks assert on exit status and tolerant regexes, never on exact tool output.
- **Style:** small modules, plain control flow, fixed-width integers; public names start `cogmaster_`, internal
  `cogmaster__<module>_`; a hardware comment says what the hardware does and cites its source; **no planning
  IDs (D-xx, SND-xx, DEC.xx) in code, test names or user docs**; follow the ponytail ladder (remove the need,
  reuse, standard library, platform, minimum new code); another copy-paste beats another dependency.
- `.claude/CLAUDE.md`: never run `/gsd-profile-user` here.

## Architectural Responsibility Map

This is a library, a runner and a libretro adapter, not a client/server system; the "tiers" are the three
deliverables plus the test tooling. Audio belongs in the library; hosts only deliver it.

| Capability | Primary Tier | Secondary Tier | Rationale |
|------------|-------------|----------------|-----------|
| PSG, OPLL, synth, mixer, DC blocker, int16 conversion, cadence | Library (`src/`) | none | Determinism and "no wall clock" (rule 4); same bytes on every platform |
| Transition log (tap) and sample drain | Library (`cogmaster_get_audio_trace`, `cogmaster_get_audio`) | Runner/adapter call them | Caller-owned buffers; allocation only at `create`/`load` |
| Merge-on-change of records, SHA-256, `--hash-audio` lines, exit statuses | Runner (`runner/`) | none | The core stays free of host helpers; sorting and hashing are host code |
| Mono-to-stereo expansion, batch vs single callback, pacing | libretro adapter | none | libretro allows one callback; stereo is a host format, core reports `channels` |
| `cogmaster_fm` option, `on`+GG to `auto` mapping | libretro adapter | Library validates `fm` | Option UI is host; PROFILE_REJECTED stays in `load` |
| Pin files for audio hashes, cross-lane equality | Test tooling (`tests/cmake/hashes.cmake`, CI `hash-equality`) | CI | Pins are text; comparison is byte-for-byte across 7 lanes |
| Hosted recording is non-silent | CI macOS job + `tests/retroarch` (300-line cap) | agent runs RetroArch 1.22.2 locally | Real-frontend evidence, bounded code (rule 7) |

## Standard Stack

There are **no external packages** in this phase (no npm, pip or cargo). Reuse is by permissive source seed
(CLAUDE.md rule 1, PROVENANCE section 4), so the Package Legitimacy Gate does not apply.

### Core (seeds and pins, all fetched this session)
| Item | Pin | Licence | Facts the planner needs |
|------|-----|---------|-------------------------|
| emu2413 `emu2413.c` | `a1d5120903b75f93d3eb7690e34a29c0b4add903` | MIT, header "Copyright (C) 2026 Mitsutaka Okazaki", LICENSE "2001-2019" | 1,552 lines, 47,757 bytes, SHA-256 `35d492c180320bd473ece0fddca771ef54d9b25884a094468560f865f540abe5`; `gh api repos/digital-sound-antiques/emu2413/commits/<sha>` answers 200 (message "Bump Version", 2026-10-10) [VERIFIED: gh api this session] |
| emu2413 `emu2413.h` | same | same | 246 lines, 6,917 bytes, SHA-256 `24d132a37fbc28426d24db90954f723b477d37277aae03db77c7dac8c4a637e3` [VERIFIED: shasum of the fetched blob] |
| emu2413 `LICENSE` | same | MIT | 21 lines, 1,089 bytes, SHA-256 `8fb093f26ed0d78c8ca033bbec7063ac5b34c5d2475e6bd121d03ccfaaf2355f`; first line "The MIT License (MIT)", "Copyright (c) 2001-2019 Mitsutaka Okazaki" |
| nesturbator `src/synth.c` | `9f9386c661f6fb8f6063bf756655713aedadae77` | MIT ("Copyright (c) 2026 nesturbator contributors") | 2,786 lines, 296,876 bytes (almost all NES mixer tables), SHA-256 `21a3e7d1efeb7df8efb00333fab7c1e59932edb555d439e45e286389ffcba3dc`; commit answers 200 [VERIFIED: gh api] |

Repository roots: emu2413 is `digital-sound-antiques/emu2413` (default branch `develop`, MIT); nesturbator is
`szTheory/nesturbator` (public, MIT). The emu2413 tree at the pin holds `emu2413.c`, `emu2413.h`, `LICENSE`,
`CHANGELOG.md`, `README.md`, `CMakeLists.txt`, `sample2413.c`, `.clang-format`, `.gitignore`: nothing else is needed.
The `emu2413.c` header also says it "refers to" andete's notes, `ymf262.c` by Jarek Burczynski, Nuke.YKT's VRC7
presets and Chabin's YMF281B presets; those are documentation credits, not copied files, but D-17 asks that the
provenance entry record the instrument table's origins, so keep that sentence.

**nesturbator kernel (what to take, what to drop).** The table is `const int16_t
nesturbator__synth_kernel[NESTURBATOR_SYNTH_PHASES][NESTURBATOR_SYNTH_TAPS]` at `src/synth.c` lines 12-45: 32
rows (lines 13-44) of 16 taps, Q15, and all 32 rows sum to exactly 32,768 [VERIFIED: awk over the fetched file].
Row 0 is `{8, -27, 70, -148, 293, -644, 2606, 28452, 2606, -644, 293, -148, 70, -27, 8, 0}` (peak at tap 7, last tap
0) and row 16 is the symmetric centre. The constants are `NESTURBATOR_SYNTH_TAPS 16u`, `NESTURBATOR_SYNTH_PHASES
32u`, `NESTURBATOR_SYNTH_RING_CAPACITY 1024u` (`src/internal.h` lines 19-21 at the pin). Port: the table only (a
`static const int16_t` in `src/synth.c`) and the endpoint rule for the interpolation partner of row 31: "Phase 32
is phase 0 shifted one tap to the right" (`b = tap == 0u ? 0 : kernel[0][tap - 1u]`), which D-04's `a*(65536-f) +
b*f` still needs for `row == 31`. Drop: `nesturbator__pulse_mix`, `nesturbator__tnd_mix` (NES mixer, lines 52-2650),
the two high-pass and one low-pass Q30 filters, `multiply_q30`, `rounded_q30`, `pcm_sample`, the correction tap
and the per-event divisions. Keep conceptually: reset = clear state, transition = add `delta * coefficient` into
a ring at `head + tap`, sample = add `impulse[head]` into the integrator, advance head.

**emu2413 port inventory (`emu2413.c`, line numbers at the pin).**

| Region | Lines | Action |
|---|---|---|
| includes, `INLINE`, `_PI_` | 13-29 | drop `<math.h> <stdio.h> <stdlib.h>`; use `static inline` |
| `default_inst[3][152]` | 33-97 | keep only tone row 0 (YM2413, 19 x 8 bytes); D-17 |
| constants `DP_BITS EG_STEP TL_STEP SL_STEP DAMPER_RATE PG_BITS` | 99-126 | keep integer ones; `EG_STEP 0.375` becomes x8/3 |
| `exp_table[256]`, `fullsin_table[1024]` (literal: 256 values, the quarter wave) | 128-166 | keep as `static const`; mirror at use or generate full `.inc` |
| `halfsin_table`, `tll_table[128][4]`, `rks_table[16][2]`, `default_patch[3][38]` | 167, 221-225 | **mutable globals, built at runtime**; become `static const` literals in `opll_tables.inc` or pure functions (abi.globals rejects any writable data symbol, local statics included) |
| `pm_table[8][8]`, `am_table[210]`, `eg_step_tables[4][8]`, `ml_table[16]` | 172-215 | keep, `static const` |
| `kl_table[16]` (`double`, multiples of 0.125) | 217-219 | replace by integer `kl8[]` (D-16); note `makeTllTable` truncates `tmp = (int32_t)(kl - dB2(3.000) * (7 - block))` to an integer **before** the shift, then `(tmp >> (3 - KL)) / EG_STEP`; the lockstep proves any rewrite |
| `min`, `max` helpers | 229-232 | keep as `static inline` |
| rate converter `OPLL_RateConv_*`, `blackman`, `sinc` | 236-336 | **delete** |
| `makeSinTable ... initializeTables` | 345-415 | replaced by generated tables |
| slot ops, key status, LFO, noise, EG, PG, outputs | 416-1050 | **the core to port** (about 600 lines of logic after the debug printers at 429-462 are dropped) |
| `mix_output` (int16 sum of 14 channel outs), `mix_output_stereo` | 1050-1083 | keep mono only |
| `OPLL_new/delete`, `reset_rate_conversion_params`, `setRate` | 1085-1147, 1213-1221 | delete (no malloc, no rate) |
| `OPLL_reset`, `forceRefresh` | 1149-1211 | keep reset; forceRefresh only if the state-apply path needs it |
| `OPLL_writeReg`, `OPLL_writeIO` | 1222-1403 | keep (about 175 lines + 7) |
| pan, patch API, mask, `save_state` / `load_state` | 1405-1552 | delete (dump-to-patch conversion at 1410-1437 is needed to build user and default patches) |
| `OPLL_calc` | 1484-1496 | keep as the one-sample step (`update_output` then `mix_output`) |

Rough size: counting non-blank, non-comment lines outside the dropped regions gives about 860 in the original
layout [ASSUMED: awk estimate; the planner must re-measure after the port]; clang-format at 100 columns plus
casts forced by `-Wconversion -Wsign-conversion` (every `ci` build is warnings-as-errors, `cogmaster_warnings` in
`CMakeLists.txt`) will add to that, so run the size check as the first FM task, before writing the state visitor.

### Supporting: none. Alternatives Considered
Locked by D-02/D-15 (polyBLEP, box average, ymfm, ares, Nuked-OPLL excluded). Do not open Nuked-OPLL (GPL-2.0).

**Installation:** none. Fetch for reading only, never into the workspace:
```bash
gh api "repos/digital-sound-antiques/emu2413/contents/emu2413.c?ref=a1d5120903b75f93d3eb7690e34a29c0b4add903" \
  -H 'Accept: application/vnd.github.raw' > "$SCRATCH/emu2413/emu2413.c"
```
For the one-shot oracle run, add two `pins.txt` rows (`name version url sha256`, https only, 64 lowercase hex)
for `emu2413.c` and `emu2413.h` using `https://raw.githubusercontent.com/digital-sound-antiques/emu2413/<sha>/...`
and fetch with `scripts/fetch-pin.sh NAME DEST` into `build/pins/` (git-ignored); `policy.pins` validates the rows.

## Package Legitimacy Audit

No external packages are installed in this phase. The two seeds are not packages: they are pinned commits whose
exact bytes are digested above, fetched from official repositories through `gh api`, and recorded in PROVENANCE
and THIRD-PARTY-NOTICES. Packages removed (SLOP): none. Packages flagged (SUS): none.

## Architecture Patterns

### Current code map (read this session)

| Area | Where | What exists now |
|---|---|---|
| Versions | `include/cogmaster.h:14-17` | `#define COGMASTER_API_VERSION 8`, `#define COGMASTER_ABI_VERSION 1`, `#define COGMASTER_BEHAVIOUR_REVISION 10`, `#define COGMASTER_STATE_FORMAT_VERSION 10`. Targets: API 9, behaviour 11, state 11 |
| Public structs | `include/cogmaster.h:97-131` | `cogmaster_config {size, allocator}`, `cogmaster_media {size, image, image_size, machine, region, tv, [reserved], video_mode}`, `cogmaster_info {..., region, tv, video_mode}`; **no audio type or function** |
| Instance | `src/internal.h:169-203` | `struct cogmaster` holds `frame`, `ticks`, `machine`, `video_mode`, `region`, `tv`, `loaded`, `counters`, `z80`, `bus`, `vdp`, `cart`, `mapper`, `io`, `input_read`, `ram`, `field`, `completed`, `lcd`; no audio fields; `audio_dropped` is a counter (line 43) |
| I/O struct | `src/internal.h:160-164` | `struct cogmaster__io { uint8_t mem_ctrl; uint8_t io_ctrl; uint8_t gg[6]; }` (8 bytes; the `_Static_assert` at `state.c:52` must change when `audio_ctrl` is added) |
| Bus tick | `src/bus.c:94-104`, `src/z80` seam | `cogmaster__bus_run_frame` runs `cogmaster__z80_step` until `ticks >= deadline`, then `bus_sync`, `deadline += frame_ticks`, `frame++`, returns overshoot (at most 68 ticks) |
| Port writes | `src/bus.c:247-259` | `cogmaster__bus_out`: VDP data/control to the VDP; `else if ((port & 0xC0u) != 0x40u) { cogmaster__io_out(...) }`; `$40-$7F` falls through to nothing (comment: "Writes to $40-$7F go to the sound chip, which is not modelled yet.") |
| Port reads | `src/io.c:163-184` | `$C0-$FF` chip reads, GG `$00-$06`, `$3E/$3F` reads fault; `$F0-$F2` currently read as open bus on full-decode consoles and mirror `$DC/$DD` on export |
| Load/info/reset | `src/instance.c:119-280` | `cogmaster_load` (profile resolution, `memset(&m->counters, 0, ...)`, `bus_origin`, `boot_handoff`, `vdp_reset`), `cogmaster_get_info` (NTSC fps 1640625/27379, PAL 13300856/267615), `cogmaster_reset` |
| Frame | `src/frame.c:25-92` | `cogmaster_run_frame` latches input, runs `bus_run_frame` or the test-card branch, counts lag |
| State | `src/state.c:23-369`, `src/state.h` | 64-byte header, field visitor with SIZE/WRITE/VALIDATE/APPLY/RECORD; unsigned helpers only; tripwires `counters == 40u`, `bus == 40u`, `vdp == 16510u`, `io == 8u` |
| libretro | `libretro/libretro.c`, `options.h`, `cogmaster_libretro.info.in` | silence generator `lr_audio`, five options, `.info` description "compatibility entries describe bounded predicates, not general gameplay or sound support"; `savestate = "false"` |
| Runner | `runner/main.c`, `args.c`, `args.h` | `print_hash` (pixels only), option chain, `cogmaster__run_options` has no audio fields; `cogmaster_config` is built at `main.c:280-284` |

### System architecture (data flow)

```
ROM bytes --> Z80 step (stamps every access with the bus tick)
                |-- OUT $40-$7F --> psg_write(tick, v) --catch up PSG steps (48 ticks each) <= tick--> synth_level(tick, src, level)
                |-- OUT $06 (native GG only) --> io_out: catch PSG up, store mask, emit source 4
                |-- OUT $F0/$F1/$F2 (FM unit) --> opll_write / audio_ctrl (mute at the mixer) --> emit sources 5, 6
                `-- run_frame end --> psg_flush(deadline), opll_flush(deadline) --> synth_frame_end
synth_level --> [optional trace log append {tick, source, level}]  (one branch per change)
            --> per-source gained level (Q12, non-negative rounding) --> delta --> impulse ring (1,024 slots, int64)
synth_frame_end --> integrate N samples (801/802; PAL 965/966) --> DC blocker --> saturating int16 --> pending PCM (1 or 2 ch)
host: cogmaster_get_audio --> PCM copy-out ; runner: cogmaster_get_audio_trace --> sort/merge --> SHA-256 lines
libretro: drain every frame (even with NULL callbacks) --> batch callback, else per-sample loop; SMS mono -> L=R in place
```

### Recommended structure (additions)
```
src/synth.c   src/psg.c   src/opll.c   src/opll.h   src/opll_tables.inc    (+ CMakeLists.txt source list)
include/cogmaster.h  (API 9)            docs/audio.md (new)
tests/psg/  tests/audio/  tests/fm/  tests/api/v9.c  tests/audio/{transitions,pcm}.txt
tests/roms/src/{psg_sweep,psg_sweep_gg,fm_sweep}.{asm,link}
tests/retroarch/ (one small checker; see the budget below)
```
`src/*.c` names inside the stated modules are free (Claude's discretion). Every non-static function must be
named `cogmaster__<module>_...` because `abi.symbols` fails on any external symbol outside `cogmaster__` that
the header does not declare [VERIFIED: tests/cmake/repository.sh:100]; keep file-local helpers `static`.

### Pattern 1: lazy catch-up with an absolute-tick divider
**What:** each chip keeps `last_tick`; a write stamped `t` first runs every step with `48k <= t` (PSG) or
`216j <= t` (OPLL), then applies the write; the frame end runs both to the deadline. **When:** D-11, D-18.
**Test:** the tie rule (a write at exactly `48k`), the frame-end flush, and "same stream with or without an
intermediate flush" equality (catch-up is associative).

### Pattern 2: the field visitor for every audio field
**What:** add `psg_io`, `synth_io`, `opll_io` to `state_io` after `io_io`, one field per variable through
`cogmaster__sio_u8/u16/u32/u64` and a new signed helper, with range bounds and cross-checks (LFSR not 0,
ring/head in range, `fm_unit` matching the header). The tripwire `_Static_assert(sizeof(struct ...) == N)` must
exist for each new state struct so a forgotten field fails the build. `core.state.record_sweep` already sweeps
every RECORD field's bounds; new fields are covered automatically **if** their bounds are not the full type
range. **Output buffers (the transient PCM and trace log) are not state** (undrained output cleared on load, D-22).

### Pattern 3: owned fixtures judged by frozen digests
**What:** `psg_sweep`/`fm_sweep` are table-driven WLA-DX programs; the mailbox reports frame and write counters
(measurements), the frozen transition and PCM digests judge the emulator. Same pattern as `ports`/`vdp_lines`.

### Anti-patterns
- Hashing a struct's memory or `int16_t` array natively (breaks be32): serialise little-endian byte by byte
  (the `print_hash` pixel loop at `runner/main.c:121-136` is the model).
- Using `>>` on a signed value or `<<` on a negative in new synth/PSG/mixer code (D-10); only the OPLL port uses
  the one named floor-shift helper.
- A second PSG model in tests (closed-form oracles only, D-14).
- Mutable file-scope tables or function-local statics (abi.globals).
- Putting audio prose in `embedding.md` (budget) or planning IDs in code, tests or user docs (CLAUDE.md style).

## Don't Hand-Roll

| Problem | Don't build | Use instead | Why |
|---------|-------------|-------------|-----|
| SHA-256 of records and PCM | new hash | `runner/sha256.c` (`cogmaster__sha256_init/update/final/hex`) | already shared by runner and tests, streaming |
| Mailbox protocol for fixtures | new reporting | `tests/roms/src/mailbox.inc` (`mb_init`, `mb_record`, `mb_done`) and `tests/cmake/mailbox.cmake` + `tests/diag/expected.csv` rows | CRC, magic, 60-record cap, judge exists; rows are `rom,test,profile,expected,source,citation` |
| Fixture assembly | new build rule | the `foreach(name IN ITEMS ...)` in `tests/fixtures/tests.cmake` (WLA-DX 10.7 pinned via `pins.txt`) | adds image digests to `hashes.txt` automatically (it globs `*.sms`/`*.gg`) |
| Exit-status-judged negative controls | new harness | `tests/cmake/expect_fail.cmake` (`-DCMD ... -DCODE -DREGEX`) | shared, 12 lines, **outside** `tests/retroarch` |
| State round trip, field sweep | new serializer | `state.c` visitor and `core.state.record_sweep` | one field list drives five passes |
| Cross-lane comparison | new job | `tests/cmake/hashes.cmake` MODE write/compare + CI `hash-equality` (COUNT=7) | audio lines ride the same `hashes.txt` |
| OPLL maths | new YM2413 model | emu2413 port (D-15); ares then ymfm only on the D-15 trigger | tables and quirks are the value |
| Kernel table | new generator | the sibling's 16 x 32 Q15 table; `tools/kernelgen` only if the integer spectral test fails (D-03) | measured margin is thin (3.4 dB), so the test, not the tool, decides |

## Test machinery the plans must extend (verified)

- **Inventories.** `tests/inventory/ci.txt` (348 lines) and `wasm32.txt` (330 lines, "the ci list without the
  libretro tests") list `name<TAB>labels`, sorted, with labels sorted alphabetically and comma-joined (for
  example `io.port3e	io,portable`, `fixtures.boot_to_play	fixtures,portable`); configure fails when the
  registered set differs [VERIFIED: cmake/CogmasterTests.cmake `cogmaster__check_inventory`, tests/inventory/ci.txt:1-2].
  `cogmaster_add_test(NAME ... COMMAND ... LABELS ... TIMEOUT ...)` is the only registration form (all four
  keywords required; timeout is multiplied by `COGMASTER_TIMEOUT_SCALE`: 1 ci, 3 asan, 8 be32, 5 wasm32). A test
  that must run on be32 and wasm32 is labelled `portable` and appears in **both** inventories; `libretro.*` tests
  only in `ci.txt`. `ctest -L psg` and `-L fm` work as soon as tests carry those labels, for example inventory
  `fixtures.psg_sweep.sms	fixtures,portable,psg` (alphabetical: fixtures, portable, psg).
- **API v9 plumbing.** `abi.api_version` requires `tests/api/v9.c` (the awk reads `v1.c ... v9.c` and every name in
  the header, including struct field names, must occur there); `abi.api_v9` is registered explicitly in
  `tests/abi/tests.cmake` (the v8 block is at its end: `add_executable ... cogmaster_add_test(NAME abi.api_v8 ...
  LABELS abi portable TIMEOUT 10)`) and in both inventories. `docs.api_names` needs each new name in
  `docs/embedding.md`.
- **Unit-test executables.** The pattern is one executable per area taking a case name (`tests/io/tests.cmake`:
  `add_executable(cogmaster-test-io ...)`, `target_include_directories(... ${PROJECT_SOURCE_DIR}/src)`,
  `target_link_libraries(... cogmaster::cogmaster)`, `foreach(case ...) cogmaster_add_test(NAME io.${case} COMMAND
  cogmaster-test-io ${case} LABELS io portable TIMEOUT 10)`), `tests/check.h` macros `CHECK`, `CHECK_EQ_U64`,
  `CHECK_EQ_HEX`, `CHECK_DONE()`. Tests may call `cogmaster__*` internals (static archive). Add the new directory
  to `tests/tests.cmake` (an `include(.../psg/tests.cmake)` line; the include list is at its lines 30-47; fixture-dependent tests go in `tests/fixtures/tests.cmake` (included only when `COGMASTER_FIXTURES`) and host tests in `tests/libretro/tests.cmake` (only when the libretro core is built)).
- **Fixture images.** `foreach(name IN ITEMS hello mapper_sega mapper_codemasters ports ports_gg boot_to_play
  boot_to_play_gg battery)` assembles `tests/roms/src/<name>.asm` with `wla-z80 -q -o <name>.o <name>.asm`, then
  `wlalink -r <name>.link <image>`; a `<name>.link` is two lines (`[objects]` / `<name>.o`); the gg wrapper is
  `.DEFINE FIXTURE_GG` + `.INCLUDE "boot_to_play.asm"`; `boot_to_play_gg` is special-cased to produce
  `boot_to_play.gg` (`if(name STREQUAL "boot_to_play_gg") set(extension gg) set(image_name boot_to_play)`), and the
  `DEPENDS` list names `boot_to_play.asm`, `ports.asm`, `header.inc`, `mailbox.inc`. Generalise that special case
  for `psg_sweep_gg` and add the new asm to `DEPENDS`. Images are 32 KiB (`header.inc`: `SLOT 0 START $0000 SIZE
  $8000`, `ROMSIZE $C`), built only, never committed; their SHA-256 goes in `tests/built/expected.txt` as
  `<name> <sha256>`; `fixtures.*` cases check it by `^${NAME} `.
- **Mailbox judging.** `fixtures_mailbox` sets `-DCASE=mailbox`; rows in `tests/diag/expected.csv` (119 lines);
  `PROFILE=*` matches every profile; every record needs a row and every row a record; add `psg_sweep` and
  `fm_sweep` rows (and, if wanted, profile-specific `fm_sweep` rows: `sms-japan`, `sms-export`, `gg`).
- **Hash matrix.** `tests/cmake/hashes.cmake` MODE write: card lines, then per image `fixture <file> sha256 <hex>`,
  then, for `boot_to_play.sms|.gg` and `vdp_lines_*`, `playing <file> <profile> <region> <variant> frame 14 sha256
  <hex>` with profiles `sms sms1 gg-sms` (+ `native` for GG images), regions `ntsc-u ntsc-j pal` (no PAL on
  machine gg), variants baseline/input/pad2/both. The `hash_matrix` case in `tests/runner/tests.cmake:67-87`
  asserts specific `playing ...` rows exist and re-runs the card-only mode. Add the audio loop here: for each
  (fixture, profile, region, variant) run `cogmaster-run --hash-audio` and write lines that begin with
  `audio-transitions ` and `audio-pcm ` (so `grep '^audio-pcm' build/ci/hashes.txt > tests/audio/pcm.txt` works as
  D-27 says); the runner's own line is `audio-pcm <frames> rate 48000 channels ...` with no fixture name, so
  `hashes.cmake` must insert the fixture/profile/region/variant tag after the keyword. Then compare the `audio-*`
  subsets against `tests/audio/{transitions,pcm}.txt` inside the same case. `runner.hashes_file` is `TIMEOUT 30`.
- **Cross-platform proof.** CI `hash-equality` (job in `.github/workflows/ci.yml`) compares exactly seven
  `hashes.txt` files (six `build` legs and `be32`) with `-DCOUNT=7`; wasm32 only reports.
- **Hosted RetroArch.** `tests/retroarch/run_retroarch.cmake` (38 lines) already accepts `-DARGS` and `-DCONTENT`
  and passes `${ARGS} ${content}` to RetroArch; `test.cfg.in` sets `audio_driver = "null"`.

### e2e budget (D-32): exact numbers and candidates

`policy.e2e_budget` prints `e2e_budget: N lines under tests/retroarch, at most 300` and fails above 300
[VERIFIED: tests/cmake/repository.sh:132-136]: it counts every file under `tests/retroarch` plus
`tests/fixtures/play.h`. Today: `bmp.c` 69, `compare.c` 64, `run_retroarch.cmake` 38, `replay.c` 36,
`checkpoint.c` 24, `oracle.c` 23, `test.cfg.in` 22, `flip.cmake` 8 = **284**, plus `play.h` 16 = **300** [VERIFIED:
`wc -l`]. Any added line must be paid for. Concrete candidates, each with the fault it protects so the plan can
judge it:

1. `flip.cmake` (8 lines): its only job is to run `compare ... --flip` and require exit 1 naming pixel 7,3. The
   shared `tests/cmake/expect_fail.cmake` (outside `tests/retroarch`, not counted) does exactly that with
   `-DCMD=... -DCODE=1 "-DREGEX=first difference at 7,3:"`. Callers to switch: `libretro.screenshot.pixel_control`
   (`tests/libretro/tests.cmake`) and three `ci.yml` steps. **Frees 8.**
2. `bmp.c` (69): `BMP_FILE_LIMIT`/`BMP_DIM_LIMIT` hardening and header-variant acceptance guard a file that CI's
   `sips` produces (verified locally: `sips` BMPs here have a 40-byte header, 24 bpp, negative height = top-down).
   Keep top-down and bottom-up (`oracle.c` writes bottom-up), trim the two limits and the `108`/`124` header cases.
   **Frees about 6 to 8.**
3. `run_retroarch.cmake` (38): the five-line required-variable loop and the closing `message(STATUS ...)`.
   **Frees about 6.**
4. `compare.c` (64): the usage print and the `size differs` print paths. **Frees about 4 to 6.**

The minimal audio check (a three-line record config, `afinfo` exit 0, byte-count and non-zero-byte assertions
through `head`/`tail`/`tr`) is estimated at 15 to 20 lines [ASSUMED: not drafted]. Candidates 1 to 3 free about
20 to 22, enough for the minimum with one or two lines spare; D-32's fallback (existence + `afinfo` + a
non-zero-bytes threshold only) is the escape hatch. Do not raise the cap, and do not move assertions to YAML.
The checker belongs under `tests/retroarch` (`docs/maintaining.md:181` requires it). An alternative with fewer
lines, if the planner wants it: have the checker read the recorded movie with CMake's `file(READ ... HEX LIMIT n
OFFSET m)` and `string(REGEX REPLACE ...)` (no `tr`); not recommended until measured, because hex-pair alignment
must be exact.

## Common Pitfalls

### Pitfall 1: warnings are errors on every compiled lane
**What goes wrong:** the ported OPLL and the 64-bit mixer arithmetic compile with `-Wall -Wextra -Wpedantic
-Wshadow -Wconversion -Wsign-conversion -Wstrict-prototypes -Wmissing-prototypes -Wvla -Wundef -Wcast-qual
-Wimplicit-fallthrough -Wmissing-variable-declarations` (GNU adds `-Wduplicated-cond -Wlogical-op`; MSVC `/W4`) and
`CMAKE_COMPILE_WARNING_AS_ERROR=ON` [VERIFIED: CMakeLists.txt `cogmaster_warnings`, CMakePresets.json `ci`].
**Avoid:** explicit casts in every int/uint16/uint32 mix from emu2413; `static` for every non-exported
function; name new unused-parameter handling with `(void)`. **Warning sign:** the first build of `src/opll.c`.

### Pitfall 2: abi gates on the library archive
`abi.globals` fails on any writable data symbol (type `B b D d C`, local statics included); `abi.undefined` allows
only `memcpy memmove memset memcmp` (+ OS extras) as undefined symbols, so no `printf`, `abs`, `qsort`, `pow`,
`malloc` in `src/`; `abi.symbols` demands the `cogmaster__` prefix for internal externals. The `nofp` lane builds
`src/` with GCC `-mgeneral-regs-only`, so any double (including a stray `kl_table`) breaks it. Everything in
`opll_tables.inc` must be `static const` integers.

### Pitfall 3: determinism traps in the mixer
Signed division and shifts: D-10's rules plus "gain the level, not the delta" (D-07). Unsigned 64-bit division is
`__udivdi3` on ppc32 (allowed) and native elsewhere; avoid signed 64-bit `/` and `%` in per-event code (slow on
be32/wasm32 and not in the allow-list, although `abi.undefined` is not run on be32). Bound every ring index with
a mask and assert `n + 32 < 1024` under hostile overshoot (D-05).

### Pitfall 4: undrained or oversized output
Without a bound a host that never drains grows memory. Keep one frame of PCM (at most 966 stereo frames, 3,864
bytes) and count overflow in `audio_dropped` (F6). The trace log is allocated at `load` from `trace_records`;
overflow increments a drop counter, never reallocates (`core.no_alloc_after_load` guards allocation after load).

### Pitfall 5: the existing tests that must change with the feature
`runner.usage.unknown` (F5), the `libretro.host.*` silence assertions only if they still hold (they should: the
card stays silent and 801/802), `core.api.first_released_sizes` and `core.info` (new sizes and fields),
`core.state.*` (header/offset changes, new fields), `io.gg_ports` and any test of GG-in-SMS-mode (F4), the
`playing` rows' expectations (unchanged), `abi.api_version`'s self-test (header copy with one extra function),
and `docs.api_names`. Run `ctest --test-dir build/ci` after each plan, not only the new labels.

### Pitfall 6: frames and the perf gate
Every frame now does synthesis; expect a large Ir/frame increase on `boot_to_play` (PSG steps up to 3,734, ring
adds 16 taps per event, 801 integrate steps). The gate is in report mode but a dated `tests/perf/allow.txt` line
is expected; the percentage is unknown until measured [ASSUMED]. Do not write a number into the allow file
before running `cmake --workflow --preset bench` or the CI job once.

### Pitfall 7: hygiene specifics
No digit-dot-digit, `float`, `double` or `<math.h>` in any `src/*.c`/`*.h` text, comments included (F8); tracked
`*.c`/`*.h` in `tests/` must also be `clang-format` 21 clean; no `/Users/` or `/home/` paths, e-mail addresses or
GPL licence text in any tracked file (the emu2413 sources contain none, checked); a file name ending in
`.sms .gg .bin .rom .sav ...` must be listed in `tests/roms/manifest.txt` if tracked (fixture images are build
outputs and are not tracked).

### Pitfall 8: a fixture that disturbs existing evidence
`boot_to_play` feeds the pixel oracle, three input scripts, the compat ledger, the perf workload and RetroArch
replays. Adding PSG writes before the VDP init shifts VDP write ticks by a few cycles; write them after the video
code (D-27) and re-run `cogmaster-test-play` oracle plus the hosted replays. Its `expected.txt` digest changes.

### Pitfall 9: libretro callbacks
libretro allows one audio path per frame; RetroArch registers both (`audio_cb` and `audio_batch_cb`) and the
adapter currently prefers batch. Drain before the video work and even when both callbacks are NULL (D-29); never
call with 0 frames; keep the 966 x 2 static buffer as the only audio storage in the adapter.

## Code Examples

### Header deltas the API-9 plan must contain (layout only; names are suggestions)
```c
/* include/cogmaster.h */
#define COGMASTER_API_VERSION 9
#define COGMASTER_BEHAVIOUR_REVISION 11
#define COGMASTER_STATE_FORMAT_VERSION 11
#define COGMASTER_FM_AUTO 0u
#define COGMASTER_FM_OFF 1u
#define COGMASTER_FM_ON 2u
/* cogmaster_config: ... allocator; uint32_t trace_records; [64-bit] uint32_t reserved; */
/* cogmaster_media:  ... video_mode; [64-bit] uint32_t reserved2; uint32_t fm; */
/* cogmaster_info:   ... video_mode; uint32_t audio_channels; uint32_t fm; */
typedef struct cogmaster_audio {
    uint32_t size;
    int16_t *samples; /* in: caller buffer */
    uint32_t capacity; /* in: stereo frames, or mono samples when channels is 1 */
    uint32_t written;  /* out */
    uint32_t channels; /* out: 1 or 2 */
    uint32_t dropped;  /* out: see the audio doc */
} cogmaster_audio;
cogmaster_status cogmaster_get_audio(cogmaster *m, cogmaster_audio *io);
```
`ARCHITECTURE.md:122` gives `{ size, samples, capacity, written, channels }`; `dropped` is the F6 addition. Pointer
members move offsets on 64-bit (pointer at 8): assert each offset on both pointer sizes like `cogmaster_frame`.

### Version-bump checklist (one-way door; do it once)
`COGMASTER_API_VERSION` 9, `COGMASTER_BEHAVIOUR_REVISION` 11, `COGMASTER_STATE_FORMAT_VERSION` 11 in
`include/cogmaster.h`; `tests/api/v9.c` (and `abi.api_v9`); `docs/embedding.md` version notes; state header `fm_unit`;
the CI compat ratchet reads the behaviour revision from the header and the ledger rows must still pass.

### The runner's audio lines (exact text from D-26)
```text
audio-transitions <frames> count <n> sha256 <hex>
audio-pcm <frames> rate 48000 channels <1|2> samples <n> peak <p> clipped <k> sha256 <hex>
```
Total-sample assertion in CMake integer math: `math(EXPR want "(${frames} * 3504512) / 4375")` (and
`685094400 / 709379` for PAL); CMake `math` is 64-bit signed, so `frames * 685094400` is safe below about 1.3e10
frames.

### PROVENANCE and notice entries (formats verified)
- `PROVENANCE.md` section 2 row (like the ares/ymfm rows at lines 46-48): name, pin, what was read, scope; section
  4 row replacing line 80 (YM2413: "emu2413 `emu2413.c`/`.h` @`a1d5120903b7`, MIT; ported to integer C17; replace
  when the logic exceeds about 1,100 lines or the lockstep cannot be exact; parity corpus: the seeded register
  stream") and a new nesturbator kernel row; section 10 source row with a `[PV.NN]` id.
- `THIRD-PARTY-NOTICES.md` section per upstream: bullets `**Applies to:**` (paths), `**Upstream:**` (URL, path,
  40-hex commit), `**SHA-256:**` of the unmodified file (`emu2413.c` above), `**Changes:**`, `**Licence:** MIT`,
  then the LICENSE text verbatim in a fenced block; for the header's 2026 versus LICENSE's 2001-2019, reproduce
  both, as D-15 says. `release.archive_contents` checks the notices file ships in every archive.
- `src/opll.c` first line: `/* SPDX-License-Identifier: MIT */` plus upstream path, 40-hex commit, copyright
  holders as the header names them and the change summary, **without** a dotted version number (F8).

### hash line tagging (shape)
```text
# One line per run in hashes.txt: the runner's keyword first, then a tag, then the runner's own fields,
# so grep '^audio-pcm' and grep '^audio-transitions' split the pins cleanly.
audio-pcm psg_sweep.sms sms ntsc-u baseline 300 rate 48000 channels 1 samples <n> peak <p> clipped 0 sha256 <hex>
audio-transitions psg_sweep.sms sms ntsc-u baseline 300 count <n> sha256 <hex>
```
In `hashes.cmake` this is `string(REGEX REPLACE "^(audio-[a-z]+) " "\\1 ${tag} " line "${line}")` applied to
each runner line before `string(APPEND lines ...)`. `tests/fixtures/tests.cmake` and `libretro` tests must not
depend on this tag format.

## State of the Art

| Old approach | Current approach | Impact |
|--------------|------------------|--------|
| Adapter-side silence with its own rational (965.766414, 19 ppb off the library's) | Library-delivered PCM, adapter drains | one source of truth; deletes `audio_acc` and `silence` |
| "Box bins" / sample-and-hold synthesis (SMS-AUDIO section 5) | band-limited step ring, 16 taps | passes the -80 dB spectral gate (D-02) |
| Third-party ROM conformance for SND-01/02 | owned fixtures + units + lockstep (D-33) | requirement text must be amended in the first docs task |
| emu2413 `OPLL_new` + `rate converter` | direct 72-T-state sample, no converter | float and malloc disappear |

**Deprecated/outdated statements** (CONTEXT lists them; confirmed or added by this research): the SMS-AUDIO,
CONFORMANCE and HOSTS items in CONTEXT's canonical refs; plus `state.c:6-8, 27` ("profile reserved 27"),
`docs/embedding.md:487` ("exact rational silence pacing"), `README.md:8`, `libretro/cogmaster_libretro.info.in`
description, and the stale `.info`/docs claims that audio is silent.

## Spine, waves and one-way doors

`.planning/config.json` has `parallelization: false`, so plans run in order; the "can run in parallel" column
says which are logically independent (touch different files) if that setting changes.

| Plan | Contents | Depends on | Independent of | One-way door? |
|---|---|---|---|---|
| 1 | REQUIREMENTS/ROADMAP/DEC/PROVENANCE amendments (D-33, D-15, F1 wording); API 9 + behaviour 11 + state 11; header structs and asserts; `tests/api/v9.c`; stub `get_audio` returning zero samples; `docs/audio.md` skeleton and doc trims; inventory entries | none | all | **Yes**: public struct layout and state header |
| 2 | `src/synth.c` (ring, kernel, mixer, DC, int16 conversion, cadence), `audio_dropped`, state visitor for synth with signed helper, frame-end in both `run_frame` branches, `audio.cadence`, `audio.spectral`, `audio.no_clip`, `audio.cost` | 1 | 3 (files), 6 | Q12 gains and the kernel row shape move PCM pins only (reversible) |
| 3 | `src/psg.c`, `bus_out` wiring, `$06` and the `native_gg` fix, `audio_ctrl` in `struct cogmaster__io` (+ `io_io`, tripwire), PSG state, `psg.*` units | 1 | 2 (mostly), 6 | Transition-level encoding (D-25) is hashed forever |
| 4 | trace log (`trace_records`, `cogmaster_get_audio_trace`), runner `--hash-audio`, `runner.usage.unknown` edit, `psg_sweep` (+gg) and `boot_to_play` chime, `expected.txt`, `diag/expected.csv`, hash matrix + `tests/audio/*.txt`, `runner.hash_audio`, docs/runner.md | 2, 3 | 6 | **Yes**: record layout, line formats, pin file names |
| 5 | `src/opll.c` + `.h` + `opll_tables.inc`, size checkpoint first, lockstep (frozen digest, one-shot oracle run), `$F0-$F2`, profile/`fm`, state, notices/provenance, `fm_sweep`, `fm.*` | 1, 2, 4 | 6 | Notices and provenance follow the seed |
| 6 | adapter: drain, callbacks, `cogmaster_fm` option in the three places, `.info`, `host.c` three-session audio test, pacing test updates | 1, 2 | 3, 5 | `.info` claims must map to tests (`libretro.info_claims`) |
| 7 | hosted RetroArch recording: free lines, checker, `ci.yml` artifact + steps, local agent run, `docs/testing.md` | 4, 6 | 5 | CI workflow change triggers rehearsal |
| 8 | perf allow line, docs/README/conformance stale text, STATE entry, final full-lane evidence | all | none | none |

A plan-checker's reversibility focus should be Plans 1, 3 (level encoding) and 4 (record layout, pin formats):
after they merge, changes move every published digest and need a new behaviour revision.

## Runtime State Inventory

Not a rename/refactor phase (new subsystem). The one migration-like item is the state format bump: every
older state is refused by design (`fixed(io, "format_version", ...)`, `state.c:76`), and no state API is public,
so there is no stored data to migrate. Build artifacts: `build/*` is git-ignored; fixtures are rebuilt by the
build. Nothing else carries the changed strings.

## Assumptions Log

| # | Claim | Section | Risk if Wrong |
|---|-------|---------|---------------|
| A1 | The pruned emu2413 port is about 900 to 1,000 logic lines after clang-format (about 860 in the original layout by awk) | Standard Stack | If it exceeds about 1,100 the D-15 trigger fires (ares, then ymfm); early measurement avoids rework |
| A2 | Minimal hosted audio checker plus record config is 15 to 20 lines; trims 1 to 3 free about 20 to 22 lines | e2e budget | If it needs more, use D-32's minimum fallback (never raise the cap) |
| A3 | The perf allow percentage is unknown until measured; Ir/frame will rise well past +1 percent | Pitfall 6 | Allow line too low leaves the report-mode job red; it is not a blocking gate today |
| A4 | be32 (qemu-ppc) and wasm32 (wasmtime) run the integer DFT (about 22M multiply-adds) and nine 300-frame hash-matrix runs inside timeout x8 and x5 | F14 | Timeouts need raising with a measured comment (`hygiene.timeout_comments` rules apply to workflow `timeout-minutes`) |
| A5 | A one-shot agent run of the lockstep oracle plus a frozen digest in CI meets D-16 at lower cost than a nightly lane | Open Questions | If the owner wants the nightly, add a preset + inventory + `nightly.yml` job (about 60 to 100 lines) |
| A6 | `COGMASTER_FM_AUTO/OFF/ON` numbered 0/1/2 | F3 | Any numbering works if AUTO is 0; a different one only changes docs and v9 test |
| A7 | `trace_records` should be capped near 1 << 20 and the byte count computed overflow-safely | F3 | A hostile config could request a huge allocation on 32-bit `size_t` |
| A8 | `get_audio` before the first `run_frame` after load returns 0 samples, and a drained buffer returns 0 until the next frame | F6 | Hosts that poll expect stable semantics; define it in `docs/audio.md` |
| A9 | `cogmaster_audio.dropped` is the right public carrier for D-26's "audio_dropped is 0" | F6 | Alternative: put it in the trace struct; either works if the runner can read it |

## Open Questions (RESOLVED)

1. **Reset and the tick count (F1).** The recommendation is to keep the documented contract and amend D-01/D-24
   (both resets restart ticks; PCM accumulator continuity per D-06). If instead D-01 stands literally, `ticks`,
   `frame`, `bus_origin`, `state_io` validation and `docs/embedding.md` change, and so do at least
   `tests/bus/test_bus.c` (`test_frame_origin`) and `tests/core/test_state.c` (`test_fault_reset_clears`). Plan 1 should record
   the choice with the evidence above; no owner action is needed (rule 6).
   **RESOLVED:** the documented contract is kept. Plan 04-01 Task 1 amends D-01/D-24 with this evidence and states one
   SOFT rule (the synth keeps phase, ring, integrator, DC state, stored levels and undrained PCM and clears the trace
   log; each chip's step origin restarts with the tick count and its reset levels reach the synth through
   `cogmaster__synth_level` at tick 0). Plans 04-02 and 04-04 implement the synth part, 04-05 the PSG part
   (`psg.reset_soft`) and 04-12 the FM part (`fm.reset_kinds`).
2. **The "hostile hammer fixture" (D-35) is not in D-28's fixture list.** Recommendation: do the hammer
   in-process through `cogmaster__bus_out` (a loop of writes at the fastest legal Z80 spacing, 11 T-states per
   `OUT (n),A`) inside `audio.cost` and `fm.cost`, plus the 29-cycle pop section already in `psg_sweep`; no new
   assembly program. The counters the test asserts are the synth's kernel-application count and the PSG step
   count against D-35's formula.
   **RESOLVED:** in-process hammer through `cogmaster__bus_out`: `audio.cost` in plan 04-06 (PSG and `$06`) and
   `fm.cost` in plan 04-12; no new assembly program.
3. **fm_sweep detection for "export + ON" and "Japan + OFF".** D-26 forbids a new runner flag, and `--region
   ntsc-j` only gives Japan + AUTO. Test the other cases in a C test that loads the built `fm_sweep.sms` through
   `cogmaster_load` with `media.fm` set and reads the mailbox through `cogmaster_peek` (the pattern of
   `runner/mailbox.c`), and keep the runner-driven mailbox judging for Japan + AUTO, export, and GG.
   **RESOLVED:** `fm.detect` in plan 04-13 Task 2 (C test through `cogmaster_load` and `cogmaster_peek`); the
   runner-driven `fixtures.fm_sweep.*` rows in plan 04-13 Task 1 cover Japan + AUTO, export and GG.
4. **GG console in SMS mode, `full_decode` (F4).** D-12/D-13 settle `$06` and `$3E`; the rest of the full-decode
   behaviour for that mode stays as it is unless a test shows a conflict. State this in the plan so the test
   encodes it.
   **RESOLVED:** plan 04-06 Task 1 gates only `$00-$06` and `$3E/$3F` on the current mode and keeps full decode, the
   pad pins and Start-as-Pause on the console; `io.gg_sms_mode` encodes the full table.
5. **Lockstep oracle cadence (Claude's discretion).** See A5.
   **RESOLVED:** one-shot oracle runs plus a frozen digest, no nightly lane: plan 04-10 Task 1 (`compare` mode
   against the unmodified upstream) and plan 04-11 Task 1 (`fm.lockstep` against the frozen 2,000,000-sample digest).
6. **Hosted check on Linux/Windows** is deferred (D-32/Deferred); nothing to decide.
   **RESOLVED:** deferred (D-32, Deferred Ideas); no plan covers it.
7. **Version number.** A `feat:` merge bumps the release minor (pre-1.0); the version is release-please's, not
   ours (`version.txt` must not be edited by hand; `release.single_version_source` guards it).
   **RESOLVED:** release-please owns the version; no plan edits `version.txt`.

## Environment Availability

Audited on the machine this research ran on (macOS arm64, Darwin 25.6.0); CI is the authority for the other lanes.

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| cmake | every task | yes | 4.4.3 [VERIFIED: `cmake --version`] | none needed |
| ninja | build | yes | on PATH [VERIFIED: `command -v`] | none needed |
| gh | pin checks, PR | yes | on PATH | none needed |
| Apple clang | build | yes | `/usr/bin/clang` | none needed |
| WLA-DX 10.7 | fixtures | yes (built by configure) | `build/ci/fixtures/*.sms` exist | none needed |
| clang-format / clang-tidy 21 | hygiene lane | yes | `/opt/homebrew/opt/llvm@21/bin` [VERIFIED: ls] | CI hygiene job |
| jq, git, sh | hygiene lane | yes | on PATH | none |
| RetroArch | agent's local audio check | yes | 1.22.2 at `/Applications/RetroArch.app` [VERIFIED: PlistBuddy `CFBundleShortVersionString`] | the CI `retroarch` job (pinned DMG) |
| afinfo | hosted recording check | yes | `/usr/bin/afinfo` | macOS-only by design (D-32) |
| ffmpeg | none required | yes | Homebrew | not used |
| GCC 15 (nofp), clang-21 + sanitizer runtimes (asan, fuzz) | CI lanes | not local (`gcc` here is Apple clang) | CI only | CI is the evidence (run URL) |
| qemu-ppc, wasmtime, valgrind | be32, wasm32, perf | not local | CI only | CI is the evidence |

**Missing with no fallback:** none. **Missing locally but provided by CI:** be32, wasm32, nofp, asan, perf,
hash-equality; plans must name the CI run URL as the evidence for those lanes.

## Validation Architecture

(`workflow.nyquist_validation` is `false` in `.planning/config.json`; the orchestrator asked for this section
anyway, so it is included.)

### Test Framework
| Property | Value |
|----------|-------|
| Framework | CTest through `cmake --workflow --preset ci` (configure, build, test, package, release tests); C test programs use `tests/check.h` (`CHECK`, `CHECK_EQ_U64`, `CHECK_EQ_HEX`, `CHECK_DONE`); CMake `-P` scripts for judging |
| Config file | `CMakePresets.json` (`ci`, `asan`, `nofp`, `be32`, `wasm32`, `hygiene`, `bench`), `tests/tests.cmake`, `tests/inventory/*.txt` |
| Quick run command | `ctest --test-dir build/ci -L psg --output-on-failure`, then the same with `-L fm` and `-L audio` (after `cmake --build --preset ci`) |
| Full suite command | `cmake --workflow --preset ci` (the repo's `test_command`) |
| Other lanes | `cmake --workflow --preset hygiene` (local, needs llvm@21 and jq); `asan`, `nofp`, `be32`, `wasm32`, `hash-equality`, `retroarch`, `perf` run in CI |

What the `ci` preset already runs: every registered test except label `release` in the first pass (346 entries; `tests/inventory/ci.txt`
has 348 lines including 2 comment lines), including `abi.*`, `core.*`, `io.*`, `runner.*` (incl. `runner.hashes_file`),
`fixtures.*`, `libretro.*`, `policy.*`, `docs.*`, `compat.*`, `perf.*`; the second test step runs the `release`
label. CI adds `check_junit` ("Every test ran") against the inventory on each lane.

### Phase Requirements to Test Map
| Req ID | Behaviour | Test type | Automated command | File exists? |
|--------|-----------|-----------|-------------------|-------------|
| SND-01 | Every tone period (N >= 2 toggles every 48N ticks; 0 and 1 constant high, no records), all 16 volumes (derived law, index 7 = 6538), 8 noise-control values, LFSR period 57,337, write protocol, tie rule, GG `$06` routing and the SMS-mode no-op | unit | `ctest --test-dir build/ci -R '^psg\.' --output-on-failure` (one `cogmaster-test-psg <case>` executable, closed-form oracles) | no, Wave 0 (`tests/psg/`) |
| SND-01 | `psg_sweep` runs and its mailbox measurements are inside their rows; frozen transition/PCM digests | fixture | `ctest --test-dir build/ci -R '^fixtures\.psg_sweep'` (labels `fixtures,portable,psg`) | no, Wave 0 (`psg_sweep*.asm/.link`, `diag/expected.csv` rows, `expected.txt`) |
| SND-02 | emu2413 port equals the original over a seeded 2M-sample register stream (frozen digest in CI; one-shot oracle run recorded as command + exit status) | lockstep | `ctest --test-dir build/ci -R '^fm\.lockstep$'` | no, Wave 0 (`tests/fm/`) |
| SND-02 | Register-level units (key-on, rhythm, noise, LFO, test register), `$F0-$F2` write mask/read bits, mute at mixer, power-on and both resets, `fm` AUTO/OFF/ON and `PROFILE_REJECTED`, 18 x samples slot evaluations, state round trip | unit | `ctest --test-dir build/ci -R '^fm\.'` | no, Wave 0 |
| SND-02 | `fm_sweep` mailbox judging (Japan + AUTO detects 1; export and GG 0) and frozen digests; export + ON and Japan + OFF in a C test | fixture + unit | `ctest --test-dir build/ci -R '^fixtures\.fm_sweep'` | no, Wave 0 |
| SND-02 | The emu2413 notice is in `THIRD-PARTY-NOTICES.md` and in the archives | policy | `ctest --test-dir build/ci -R '^policy\.notices$'` and `-R '^release\.archive_contents$'`, plus the new notice assertion (F9) | partly (archive check exists; the emu2413 assertion is new) |
| SND-03 | Two identical `--hash-audio` runs agree; per-frame counts 801/802 (965/966 PAL), total `floor(N x 3504512 / 4375)`, `clipped 0`, `peak <= 29204`; pins in `tests/audio/{transitions,pcm}.txt` match | integration | `ctest --test-dir build/ci -R '^runner\.hash_audio$'` and `-R '^runner\.hashes_file$'` | `runner.hashes_file` exists (extend); `runner.hash_audio` is new |
| SND-03 | Same lines on every platform | cross-lane | CI job `hash-equality` (`cmake "-DFILES=..." -DCOUNT=7 -DMODE=compare -P tests/cmake/hashes.cmake`); evidence = run URL | exists (extend) |
| SND-03 | Cadence: long frames positioned by `floor(F x 3504512 / 4375)` over 1,000 frames | unit | `ctest --test-dir build/ci -R '^audio\.cadence$'` | no, Wave 0 (`tests/audio/`) |
| SND-04 | Batch-only, single-only and both-registered sessions deliver PCM equal to the runner's (GG both channels, SMS L = R mono hash); the single callback is never called in the "both" session | in-process host | `ctest --test-dir build/ci -R '^libretro\.host\.audio'` (new cases in `tests/libretro/host.c`, patterned on `libretro.host.content`) | no, Wave 0 |
| SND-04 | Hosted RetroArch recording non-silent (control session exactly 0 non-zero bytes; `psg_sweep.sms`/`.gg` at least 8,192; length within 4 x 47,000..49,000) | hosted, macOS | CI `retroarch` job steps; local: command below | no, Wave 0 (checker under `tests/retroarch`, 300-line cap) |
| SND-05 | Largest non-harmonic peak below 16 kHz under -80 dB at the 111.9 kHz and 18.6 kHz stress tones (integer DFT, M = 8192, Blackman-Harris) | unit | `ctest --test-dir build/ci -R '^audio\.spectral$'` | no, Wave 0 |
| SND-05 | Cost bounded under a hammer: kernel applications, PSG steps and FM slot evaluations asserted against the formula (no clock) | unit | `ctest --test-dir build/ci -R '^audio\.cost$'` and `-R '^fm\.cost$'` | no, Wave 0 |
| all | Mix never clips: clamp counter 0 on every fixture; "four PSG channels at 0 dB + full-scale FM" test | unit | `ctest --test-dir build/ci -R '^audio\.no_clip$'` | no, Wave 0 |
| all | API 9 layout and names | api | `ctest --test-dir build/ci -R '^abi\.api_v9$'` and `-R '^abi\.api_version$'` | no, Wave 0 (`tests/api/v9.c`) |

Hosted recording run by the agent (adapted from the CI step; the evidence is the command and its exit status):
```bash
cmake -DRETROARCH=/Applications/RetroArch.app/Contents/MacOS/RetroArch \
  -DCORE=build/ci/libretro/cogmaster_libretro.dylib -DINFO=build/ci/libretro/cogmaster_libretro.info \
  -DTEMPLATE=tests/retroarch/test.cfg.in "-DRA_DIR=$PWD/build/ra-audio" -DFRAMES=60 \
  "-DSHOT=$PWD/build/frames/audio.png" -DCONTENT=build/ci/fixtures/psg_sweep.sms \
  "-DARGS=--record=$PWD/build/audio.mov;--recordconfig=$PWD/build/record.cfg" \
  -P tests/retroarch/run_retroarch.cmake && afinfo build/audio.mov
```
(`record.cfg` is the three-line config of D-32: `format = mov`, `acodec = pcm_s16le`, `frame_drop_ratio = 60`.)

### Sampling Rate
- **Per task commit:** the quick command above plus the touched area's tests (for example `-R '^(io|core)\.'` when
  `io.c` or the state visitor changes); the whole `ci` suite takes minutes locally.
- **Per wave merge:** `cmake --workflow --preset ci`; for plans that touch formatting-sensitive files also
  `cmake --workflow --preset hygiene` (clang-format 21, `no_float`, analyzer).
- **Phase gate:** full `ci` workflow green locally; PR CI green on every lane (asan, nofp, be32, wasm32 report,
  `hash-equality` with 7 files, `retroarch`, hygiene, perf report), the agent's local RetroArch recording exit
  status, and the release PR green after merge (`docs/maintaining.md` "Closing a phase"). Evidence is a command,
  an exit status and a run URL, never a committed log.

### Wave 0 Gaps
- [ ] `tests/psg/` (`test_psg.c`, `tests.cmake`), `tests/audio/` (`test_synth.c` with the integer DFT, cadence, no-clip,
  cost; `tests.cmake`; `transitions.txt`, `pcm.txt`), `tests/fm/` (`test_opll.c`, the frozen-stream digest file)
- [ ] `tests/api/v9.c`, `abi.api_v9` registration, both inventories
- [ ] `tests/roms/src/psg_sweep.asm/.link`, `psg_sweep_gg.asm/.link`, `fm_sweep.asm/.link`; `expected.txt` lines;
  `diag/expected.csv` rows; `foreach` list and `DEPENDS` edits in `tests/fixtures/tests.cmake`
- [ ] `runner.hash_audio` and the audio loop in `tests/cmake/hashes.cmake`; the edit of `runner.usage.unknown`
- [ ] `libretro.host.audio.*` cases in `tests/libretro/host.c` and `tests/libretro/tests.cmake`
- [ ] hosted recording checker and free-up edits under `tests/retroarch`; `ci.yml` artifact list and steps
- [ ] Framework install: none (CMake/CTest, WLA-DX, and RetroArch are present)

## Security Domain

`security_enforcement` is `false` in `.planning/config.json`, so no ASVS table is produced. The hostile-input
bounds this phase must still hold (they are determinism/safety rules of CLAUDE.md rule 4):

| Pattern | STRIDE | Standard mitigation |
|---------|--------|---------------------|
| ROM hammering PSG/OPLL ports to exhaust time or memory | Denial of service | Fixed work per frame (PSG steps <= 3,734, OUT <= 5,430, FM samples <= 830); counter-based cost test; ring masks and the `n + 32 < 1024` assertion |
| Hostile `trace_records` in `cogmaster_config` | Denial of service | Cap and overflow-safe size; allocate only in `load` |
| Hostile state bytes for new audio fields | Tampering | Range-bound every field in VALIDATE; LFSR != 0; ring/head cross-checks; refused state changes nothing |
| Period 0/1 toggling producing unbounded records | Denial of service | Constant-high rule, no records (D-12) |
| Third-party code provenance | Repudiation / legal | Pinned digest, notices, SPDX, `policy.notices`, no GPL source opened |

## Sources

### Primary (HIGH confidence)
- Repository files read this session: `include/cogmaster.h`, `src/{internal.h, io.c, bus.c, state.c, state.h,
  instance.c, frame.c, boot.c}`, `libretro/{libretro.c, options.h, cogmaster_libretro.info.in}`, `runner/{main.c,
  args.c, args.h}`, `tests/{tests.cmake, abi/tests.cmake, api/v8.c, core/tests.cmake, io/tests.cmake,
  fixtures/tests.cmake, libretro/tests.cmake, libretro/host.c, runner/tests.cmake, cmake/{hashes, budgets,
  perf_compare, mailbox, CogmasterTests}.cmake, cmake/repository.sh, policy/*, retroarch/*, hygiene/tests.cmake,
  inventory/*, built/expected.txt, roms/*, diag/expected.csv, compat/ledger.csv}`, `CMakeLists.txt`,
  `CMakePresets.json`, `.github/workflows/{ci.yml, nightly.yml}`, `scripts/{hygiene.sh, fetch-pin.sh,
  classify-changes.sh}`, `pins.txt`, `PROVENANCE.md`, `THIRD-PARTY-NOTICES.md`, `docs/*.md`, `.clang-format`.
- `gh api repos/digital-sound-antiques/emu2413/commits/a1d5120903b75f93d3eb7690e34a29c0b4add903` (200) and the
  fetched `emu2413.c`, `emu2413.h`, `LICENSE` at that commit (SHA-256 above).
- `gh api repos/szTheory/nesturbator/commits/9f9386c661f6` (200; full SHA
  `9f9386c661f6fb8f6063bf756655713aedadae77`) and the fetched `src/synth.c`, `src/internal.h` constants, `LICENSE`.
- `.planning/phases/04-sound/04-CONTEXT.md`, `.planning/preparation/ARCHITECTURE.md` (sections 4 to 7),
  `SMS-SYSTEM.md` (PSG port table), `DECISIONS.md` (DEC.22, 46, 47), `REQUIREMENTS.md`, `ROADMAP.md`, `LESSONS.md`.
- Local measurements: `time cogmaster-run ... --frames 3000` (1.43 s), python3 integer cadence computation,
  `PlistBuddy` RetroArch version, `sips`-produced BMP header bytes.

### Secondary / Tertiary
- None. No web searches or MCP documentation lookups were needed: every question was answerable from the
  repository or the pinned sources (the research-plan seam was not used for that reason).

## Metadata

**Confidence breakdown:**
- Standard stack (seeds, pins, digests): HIGH, fetched and hashed this session.
- Architecture/integration (symbols, sizes, offsets, tests, budgets, CI): HIGH, each read from the file.
- Port size and runtime costs on be32/wasm32/perf: MEDIUM to LOW; marked `[ASSUMED]` and to be measured.
- Pitfalls: HIGH for the repository gates; MEDIUM for emulator-accuracy items already settled by D-01 to D-35.

**Research date:** 2026-11-09
**Valid until:** 2026-12-09 (stable codebase on the phase branch; re-check if `main` moves `include/cogmaster.h`,
`src/state.c` or `tests/runner/tests.cmake` before planning completes)
