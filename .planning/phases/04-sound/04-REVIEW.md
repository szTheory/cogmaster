---
phase: 04-sound
reviewed: 2026-11-09T00:00:00Z
depth: standard
files_reviewed: 20
files_reviewed_list:
  - include/cogmaster.h
  - libretro/libretro.c
  - libretro/options.h
  - runner/args.c
  - runner/args.h
  - runner/audio.c
  - runner/audio.h
  - runner/main.c
  - src/bus.c
  - src/frame.c
  - src/instance.c
  - src/internal.h
  - src/io.c
  - src/opll.c
  - src/opll.h
  - src/opll_reg.c
  - src/psg.c
  - src/state.c
  - src/state.h
  - src/synth.c
findings:
  critical: 0
  warning: 2
  info: 4
  total: 6
  necessity: 3
status: issues_found
---

# Phase 4: Code Review Report

**Reviewed:** 2026-11-09
**Depth:** standard
**Files Reviewed:** 20
**Status:** issues_found

## Summary

All 20 listed files were read in full, with the call chains between the PSG, OPLL, synthesiser, I/O ports and
state visitor traced across files. No structural (fallow) findings were supplied. Necessity was assessed using
`necessity-review.md`.

The core is in better shape than the brief's worst-case framing suggests. No Critical defect was found. To avoid
trusting my own reading of the state validator, I built the whole library with AddressSanitizer and
UndefinedBehaviorSanitizer (`-fno-sanitize-recover=undefined`) and ran three throwaway harnesses from the scratchpad:

- A hostile-state fuzzer. It runs random Z80 programs that hit the PSG, $F0-$F2 and the Game Gear $06 port, over
  SMS1, SMS2 and GG profiles, NTSC and PAL, with the FM unit on and auto. It saves a state, then rewrites in-range
  fields (single fields, and a mass rewrite of every `synth_*`, `psg_*` and `opll_*` field to min, max or random
  within the recorded bounds), reseals the CRC, loads, runs 3 frames, and requires save then load to round-trip.
  About 5,400 mutated states across 6 seeds produced no sanitizer report, no call longer than 1 s, and no
  round-trip failure.
- A targeted case with the PSG origin set to its oldest accepted value (a frame plus 117 ticks behind), all four
  channels loud at period 2. The largest ring slot reached about 1e13, three orders under the 2^56 load bound, and
  the state still round-tripped.
- A save-then-replay check over 240 random runs (including trace records). A fresh instance and the same instance
  restored from the state produce identical PCM and trace bytes.

Bounds in `synth_io`, `psg_io` and `opll_io` match what the emulator can legitimately produce, and every table
index and shift count in `opll.c` is held by a state bound. The real issues are one false public contract, one
wrong derivation comment, and three guards that defend against states that cannot occur.

## Warnings

### WR-01: The trace log is not "in causal order", and its ticks do fall within one run

**File:** `src/synth.c:230-240` (also `src/internal.h:602-605`, `docs/audio.md:145`)
**Issue:** `docs/audio.md` promises "the log is in causal order and the ticks never fall within one run", and
`internal.h` repeats "in causal order". It is not true across sources. The two chips are run lazily and
independently. At the end of a frame `cogmaster__synth_frame_end` runs `cogmaster__psg_catch_up(deadline)` and
appends every PSG record up to the deadline, and only then runs `cogmaster__fm_catch_up(deadline)`, which appends the FM
records for the whole frame, starting at a tick far earlier than the PSG records just written. `fm_out`
(`io.c:196`) catches up only the FM on an $F1 write, so the same inversion happens mid-frame.

I confirmed it with a ROM that keys one FM voice and toggles PSG tone 0 on a Japanese profile with
`trace_records=32768`: 1,489 records over 2 frames, with the tick falling at record 289 (source 5, tick 14,256
after 178,848) and at record 1,011 (source 5, tick 179,280 after 358,272). `runner/audio.c:112-118` only
checks that each source's ticks are monotonic, and sorts before hashing, which is why nothing in CI trips. The
guarantee that actually holds is "ordered within a source". A host or tool that follows the published contract and
consumes the log linearly (for example to replay level changes into its own mixer) would apply FM changes
out of order.

**Fix:** Pick one and make the code and the docs agree. The cheapest is to state the real guarantee in
`docs/audio.md` and `internal.h`: "records of one source are in tick order; across sources the log is in the order
the chips were brought up to date, so sort by tick (then source) before merging, as `cogmaster-run --hash-audio`
does". If a globally ordered log is wanted, merge in `synth_frame_end` (and in `fm_out`) by alternating
`psg_catch_up` and `fm_catch_up` to whichever of `p->at + 48` and `m->fm_next` is earlier, so records are appended
in tick order. That interleaving changes the transition hash's input order only, not its merged output.

### WR-02: Impossible-state guard: `if (m->frame > 0u)` in `cogmaster__synth_frame_end`

**File:** `src/synth.c:234`
**Category:** necessity
**Issue:** The guarded state (`m->frame == 0` when a frame ends) cannot occur. `cogmaster__synth_frame_end` has
exactly two callers, `frame.c:61` (after `cogmaster__bus_run_frame`, which does `m->frame++` at `bus.c:102`) and
`frame.c:79` (after `m->frame++` at `frame.c:74`). On both paths the frame count is at least 1 on entry. The
comment above the block ("The frame has counted itself by now") states the invariant. If the guard were ever false,
the effect would be silent: the chips would not run to the frame's deadline and the next frame would sum stale
levels, which is harder to notice than an assertion failure.
**Fix:** Drop the `if`, keep the `m->frame--` / `m->frame++` pair, and record the invariant in the existing comment
(or add `assert(m->frame > 0u)` in a debug build if the project permits `assert`).

## Info

### IN-01: Wrong PSG step interval in the ring-slot derivation

**File:** `src/synth.c:81-82`
**Issue:** The comment that justifies `SYNTH_EVENTS = 128` says "a PSG step every 16 ticks". The chip steps every
48 ticks (`psg.c:12`, `docs/audio.md` "every 48 ticks (16 Z80 clocks)"); 16 is the Z80-clock count, not the tick
count. The conclusion still holds (the estimate is three times too pessimistic, and my own count of the worst case
is about 50 changes per ring per sample position, under 128), but a bound whose stated premise is wrong cannot be
re-checked by the next person who changes the step rate or adds a source.
**Fix:** Say "a PSG step every 48 ticks changes up to four channels (about 19 per 224-tick sample), a CPU write
every 33 ticks at most five sources (the $F2 write), an FM sample every 216 ticks: about 50 in all".

### IN-02: Impossible-state guard: `a.written > LR_AUDIO_CAPACITY` in the libretro drain

**File:** `libretro/libretro.c:306-307`
**Category:** necessity
**Issue:** `cogmaster_get_audio` clamps `n` to `io->capacity` before it copies (`synth.c:462-464`), and the adapter
passes `a.capacity = LR_AUDIO_CAPACITY`, so `a.written` cannot exceed it. The library is statically linked into the
core and is not a trust boundary. This is a judgment call because the check does protect `audio_pcm[]`; if it is
kept as defense in depth, say so in the comment.
**Fix:** Remove `|| a.written > LR_AUDIO_CAPACITY`, or comment it as a deliberate second guard on the buffer size.

### IN-03: Impossible-state guard: `source >= COGMASTER__SYNTH_SOURCES` in `cogmaster__synth_level`

**File:** `src/synth.c:426-428`
**Category:** necessity
**Issue:** Every caller passes a literal or a bounded loop index: `psg.c:53,173,176`, `io.c:202,270`, `synth.c:147-148,171`.
No host input reaches this internal function, so the guard cannot fire. It sits after the trace append, so an
out-of-range source would be logged and then silently ignored rather than rejected.
**Fix:** Remove the branch and the dependence of `width[]` on a range check, or turn it into an `assert`.

### IN-04: Loading a state keeps the previous timeline's pending trace records

**File:** `src/state.c:349-352` and `src/state.c:389-392`
**Issue:** `synth_io` discards the undrained PCM on a state load so the next frame does not count it as dropped, but
leaves `trace_pending` and the log untouched. A restored state has an unrelated tick count, so any records not yet
drained keep ticks from the discarded timeline and are followed by records from the restored one. That breaks the
per-source monotonic rule that `runner/audio.c:112` enforces and that `docs/audio.md` documents. `docs/audio.md`
documents that a load and both resets empty the log but says nothing about a state load. There is no public state
API yet, so nothing can hit this today.
**Fix:** In the APPLY branch of `synth_io`, also set `s->trace_pending = 0u; s->trace_dropped = 0u;`, and add a
sentence to `docs/audio.md` and to the comment above `synth_io` before a public state load ships.

---

_Reviewed: 2026-11-09_
_Reviewer: Claude (gsd-code-reviewer)_
_Depth: standard_
