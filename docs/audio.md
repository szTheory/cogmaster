# Audio

What a host sees of the sound: the sample stream, the FM unit choice, the trace log and what a reset does to
them. The calls and structs are in [embedding](embedding.md).

## The sample stream

`cogmaster_run_frame` produces one frame of 48,000 Hz samples and `cogmaster_get_audio` hands them over. Call it
once after every frame, with a buffer of at least 966 sample frames: that holds any frame on either TV standard.

- **Cadence.** The count per frame comes from an exact ratio, never from rounding the frame rate: 801 or 802
  sample frames on NTSC and 965 or 966 on PAL. After F frames the running total is exactly
  floor(F x 3,504,512 / 4,375) on NTSC and floor(F x 685,094,400 / 709,379) on PAL, so a host that plays what it is
  given stays in step with the video for as long as it runs. The library pads and skips nothing.
- **Channels.** `channels` is 1 on the Master System, a mono stream, and 2 on the Game Gear, left then right, in
  native and in SMS video mode alike. `cogmaster_info.audio_channels` reports the same number without running a
  frame, and `cogmaster_get_audio` with `capacity` 0 and a NULL `samples` reports it too.
- **written.** The sample frames copied: the smaller of `capacity` and what is pending. What you leave behind
  stays pending until the next `cogmaster_run_frame`; take it in a second call with the rest of the buffer.
  Before the first frame after a load, nothing is pending and `written` is 0.
- **dropped.** The total of sample frames that were pending when the next frame finished and so were discarded,
  since the last load or POWER reset. It counts a host that took too little or took nothing; a host that drains
  after every frame keeps it at 0. It saturates at the largest 32-bit value rather than wrapping.
- **Types.** Samples are `int16_t` in the host's byte order. Hash the little-endian bytes if you compare digests
  across platforms.

## Synthesis

The library turns each change in a sound source's level into a band-limited step and sums the steps, so a
square wave faster than the output rate leaves no audible alias. A host hears this; nothing in it is configurable.

- **Kernel.** Each change adds a 16-tap kernel, in one of 32 sub-sample positions, to a ring of pending
  impulses; the table is a Nyquist-sinc integral under a Kaiser window, credited in `PROVENANCE.md`. A change is
  heard about 8 samples after it happens (the kernel's peak tap), with nothing earlier. The position costs one
  integer division per change.
- **Volume and mix.** The attenuation law is 2 dB a step from full scale (steps 0 to 14; 15 is off). The level
  of each source is gained before it enters the ring, as a level rather than as a difference, so the gain cannot
  drift. The Master System and Game Gear PSG gets a gain of 840 over 4096. With the FM unit the PSG gets 491 and
  the FM 1402, which keeps their 4096 to 1434 ratio. Four PSG channels at full level stepping together peak at
  about 28,900, under the -1 dBFS line at 29,204: the kernel overshoots a step by under 8 percent. A run of
  quick changes can ring a little higher; the output saturates at the 16-bit limits and never wraps.
- **DC.** One leaky estimator per channel follows the signal and is subtracted, so a held level fades to silence
  with a time constant of 256 samples (a corner near 30 Hz). Nothing else filters the output.
- **Game Gear stereo.** The mask byte sends PSG channel n to the left with bit n + 4 and to the right with bit n,
  and changing it mid-tone is a band-limited step on the affected side only. The Master System has one channel.
- **What a change moves.** The kernel, the volume law, the gains and the DC corner decide the samples, so a change
  to any of them moves the PCM hash and no recorded transition. Sources, levels and ticks decide the transitions.

## The FM unit

The Japanese Master System has a YM2413 FM sound unit beside the PSG; the export Master System and the Game Gear
have none. `cogmaster_media.fm` chooses it at load:

| `fm` | Attached when |
|---|---|
| `COGMASTER_FM_AUTO` (0, the default) | the resolved region is Japan and the machine is not a Game Gear |
| `COGMASTER_FM_OFF` | never |
| `COGMASTER_FM_ON` | always, on any Master System; a Game Gear returns `COGMASTER_STATUS_ERR_PROFILE_REJECTED` |

A value above ON also returns `COGMASTER_STATUS_ERR_PROFILE_REJECTED`, and a refused load changes nothing.
`cogmaster_info.fm` reports the resolved choice, `COGMASTER_FM_ON` or `COGMASTER_FM_OFF`, never AUTO. An
export Master System with `COGMASTER_FM_ON` is the way to exercise the unit's detection routine; a game cannot
tell it from the Japanese console. The unit is fixed for the life of a load, and a saved state loads only into
an instance with the same choice.

## The trace log

An instance can record every change a sound source makes, for tools that check the chips rather than listen to
them. `cogmaster_config.trace_records` is the log's size in records; 0, the default, keeps none.

- **Record.** `tick`, `source` and `level`. `tick` counts video-clock ticks (the unit a frame is counted in, 179,208 per
  NTSC frame) since the last load or reset of either kind; the log is in causal order and the ticks never fall
  within one run. `source` names what
  changed: 0 to 3 are the PSG's three tone channels and its noise channel, 4 is the Game Gear stereo mask, 5 is the
  FM unit's output sum and 6 is the FM control register as the profile leaves it. `level` is the new raw value of
  that source, before any mixing gain, so a later change to the mix moves the samples and not the log. For sources
  0 to 3 bit 4 is the channel's output and bits 3 to 0 its attenuation; for 4 and 6 it is the byte itself.
- **Drain.** `cogmaster_get_audio_trace` copies the oldest records first, up to `capacity`, and empties the log.
  Drain after every frame; a frame holds at most a few tens of thousands of records. An instance created with no
  log answers `COGMASTER_STATUS_ERR_ARGUMENT`.
- **dropped.** The records the full log refused, plus those that did not fit your `capacity`, since the last
  drain. A non-zero value means the trace has a gap; the library never drops a record without counting it.
- **Restarts.** Ticks restart at a load and at both kinds of reset, and the log is emptied there. A consumer that
  spans a reset therefore sees the tick count go back to 0 and must split its stream at that point; drain before
  you reset, and treat the first record after it as a new run.

The log is allocated once, by the first load, through your allocator. Nothing is allocated after it.

## Reset

- **Load and POWER reset** start from silence: no pending samples, the cadence restarts at 801 (or 965), the
  `dropped` count is 0 and every sound chip is in its power-on state.
- **SOFT reset** keeps the output continuous: the cadence phase, the pending impulses, the DC estimate, each
  source's last level and any samples not yet taken carry over, so a host sees no gap or repeat in the stream.
  The tick count restarts at 0 and the chips return to their power-on values, as the console's reset leaves them; this is the model's choice
  and is not yet measured on hardware.

The test card is silent and follows the same cadence.
