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

## The PSG

The sound chip on every machine is the PSG (an SN76489 variant inside the VDP): three tone channels and a noise
channel. Any write to a port from $40 to $7F reaches it; $7F is the usual port and some games use $7E. Reads of $7E
and $7F stay the VDP's beam counters.

- **Time.** The chip steps once every 48 ticks (16 Z80 clocks), counted from tick 0 at a load or a reset of either
  kind. It is brought up to date lazily: when a write arrives, every step up to the write's tick runs first, then
  the byte applies. A write stamped exactly on a step tick therefore sees that step already taken. A volume write
  takes effect at its own tick, not at the next step, which is what lets a game play samples through the volume
  register. The end of a frame runs the chip to the frame's last tick, that tick included, and no further. The
  bus stall a write causes on some chips is not modelled.
- **Tone.** A channel with period N toggles its output every N steps; the first step toggles every output from 0
  to 1. A period of 0 or 1 holds the output high at the channel's volume and records nothing, though the counter
  keeps running underneath. This is the model's choice for the integrated chip, a default that hardware could
  overturn.
- **Noise.** A 16-bit shift register that starts at $8000 shifts right once every 32, 64 or 128 steps (control 0,
  1 or 2) or on tone 2's rising edge (control 3, where a tone 2 period of 0 counts as 1). White noise feeds bits 0
  and 3 back into the top bit; periodic noise feeds bit 0 back and repeats every 16 shifts, and white noise every
  57,337. The channel's output is the bit shifted out. Writing the noise control, by latch byte or by data byte,
  puts the register back to $8000; writing the noise volume does not. That reset rule and the interval counting
  are defaults, not measured on hardware.
- **Records.** Sources 0 to 3 report `output << 4 | attenuation` as the trace log's level, from the registers
  alone, so changing the volume law or the mix gain moves the samples and none of the records.
- **Game Gear stereo.** In the Game Gear's own video mode, a write to port $06 brings the chip up to date and sets
  the mask: bit n + 4 sends channel n to the left and bit n to the right, so $FF sounds both sides, $0F the right
  only, $F0 the left only and $00 neither. The change is a record for source 4 at the write's tick, and the mask
  powers on at $FF. In Master System video mode a Game Gear does not decode $06 at all: a write latches nothing,
  is no fault, and every channel reaches both outputs.
  That mode also decodes $3E and $3F as a Master System does ($3E powers on at $AB), and keeps its pad, its full
  decode and Start as Pause.
- **Cost.** However fast a game writes the sound ports, a frame takes at most one PSG step per 48 ticks and a
  bounded number of kernel additions per level change, so the work per frame is bounded and no clock is read.
- **Reset.** A load and both kinds of reset put the chip back to its power-on values: every attenuation off,
  periods and noise control 0, shift register $8000, counters and outputs clear, a data byte that follows no
  latch byte writing the volume of channel 1, and the step origin at tick 0, where the tick count restarts. The
  chip then reports its levels to the synthesiser at tick 0, so what the synthesiser holds is the chip's. After a
  SOFT reset a held tone falls to silence in the first samples of the next frame, and the volume a game wrote
  before the reset is heard again when it writes it again.

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

The libretro core's option `cogmaster_fm` (Automatic, Off, On; reload content) sets `cogmaster_media.fm` at load
to AUTO, OFF or ON. On with Game Gear content is read as Automatic, so the content loads without a unit. The core
delivers each frame's samples as the library returns them, mono machines as equal left and right, through the batch
callback when it is registered and otherwise one sample at a time; it never calls with zero frames.

With the unit attached a game drives it through three ports, which answer ahead of the I/O chip, so they work
whether or not a game has set bit 2 of port $3E to turn that chip off (the way games probe for the unit):

- **$F0 and $F1.** A write to $F0 latches a register number; a write to $F1 stores a byte in the latched register
  of the YM2413. The chip makes one sample every 72 Z80 clocks (216 ticks), the first at tick 216 after a load or
  reset, and is brought up to date lazily like the PSG: a write to $F1 first completes every sample due at or
  before its tick, then applies, and is heard from the next sample. No write is dropped or delayed; the unit's
  busy time after a write is not modelled. The end of a frame runs the chip to the frame's last tick, after the
  PSG. Reads of $F0 and $F1 drive nothing and read as an undriven bus.
- **$F2, the audio control.** A write stores its two low bits (the rest are ignored): 0 sounds the PSG only, 1 the
  FM unit only, 2 neither and 3 both. The mute is applied where the two are mixed, so both chips keep running
  while one is muted and the log still reports their levels. A read returns the stored value in bits 1 and 0,
  zero in bits 4 to 2, and in bits 7, 6 and 5 bits 11, 7 and 3 of a 12-bit counter of lines since the last load
  or reset (the tick count divided by 684). Reads of the three ports are never counted as input.
- **Without the unit** writes to the three ports are ignored and are no fault. The export console reads them as
  its pad ports (even ports as $DC, odd as $DD), and the Japanese console without the unit and the Game Gear read
  an undriven bus.

The FM sum is mixed as a level, like each PSG channel: the chip's sum, kept to 16 bits as the original keeps it,
is shifted to a positive range, scaled so that the loudest the sounding voices can be is the PSG's full scale,
and gained by 1402 over 4096. The voices that sound together (nine tones, or six tones and five drums) never
add up to more than 32,688, so the sum fits in 16 bits for every sounding voice; the silent group's last outputs
stay in the chip's sum and can carry it past that, and such a sum counts as its low 16 bits and is limited at
the ends of the scale. Four PSG channels at full level and the FM unit at full scale together peak under the
1 dB line. A reset gives the chip its power-on state (every register 0), the PSG its own, and $F2 the value 0, so
a note sounding before a SOFT reset is silent after it and is heard again when the game writes $F2 and the note
again. Whether a hardware reset really clears the FM chip is not measured; this is the model's default.

## The trace log

An instance can record every change a sound source makes, for tools that check the chips rather than listen to
them. `cogmaster_config.trace_records` is the log's size in records; 0, the default, keeps none.

- **Record.** `tick`, `source` and `level`. `tick` counts video-clock ticks (the unit a frame is counted in, 179,208 per
  NTSC frame) since the last load or reset of either kind; the records of one source are in tick order and
  its ticks never go backwards. Across sources the log is in the order the chips were brought up to date, so sort by
  `tick`, then `source`, then emission index before merging, as `cogmaster-run --hash-audio` does. `source` names
  what changed: 0 to 3 are the PSG's three tone channels and its noise channel, 4 is the Game Gear stereo mask, 5 is the
  FM unit's output sum (as 16 bits) and 6 is the FM control register as the profile leaves it. `level` is the new
  raw value of that source, before any mixing gain, so a later change to the mix moves the samples and not the log.
  For sources 0 to 3 bit 4 is the channel's output and bits 3 to 0 its attenuation; for 4 and 6 it is the byte
  itself.
- **Drain.** `cogmaster_get_audio_trace` copies the oldest records first, up to `capacity`, and empties the log.
  Drain after every frame; a frame holds at most a few tens of thousands of records. An instance created with no
  log answers `COGMASTER_STATUS_ERR_ARGUMENT`.
- **dropped.** The records the full log refused, plus those that did not fit your `capacity`, since the last
  drain. A non-zero value means the trace has a gap; the library never drops a record without counting it.
- **Restarts.** Ticks restart at a load and at both kinds of reset, and the log is emptied there. A consumer that
  spans a reset therefore sees the tick count go back to 0 and must split its stream at that point; drain before
  you reset, and treat the first record after it as a new run. The first records of the new run are one per
  active source at tick 0 (sources 0 to 3, 4 on a Game Gear in its own video mode, and 5 and 6 with the FM unit),
  the chips' power-on levels.

The log is allocated once, by the first load, through your allocator. Nothing is allocated after it.

## Reset

- **Load and POWER reset** start from silence: no pending samples, the cadence restarts at 801 (or 965), the
  `dropped` count is 0 and every sound chip is in its power-on state.
- **SOFT reset** keeps the output continuous: the cadence phase, the pending impulses, the DC estimate, each
  source's last level and any samples not yet taken carry over, so a host sees no gap or repeat in the stream.
  The tick count restarts at 0 and the chips return to their power-on values, as the console's reset leaves them; this is the model's choice
  and is not yet measured on hardware.

The test card is silent and follows the same cadence.

A real RetroArch session records this sound: the hosted job checks that a `psg_sweep` recording is not silent
(see [hosted](hosted.md#hosted-retroarch)).

## The frozen sound hashes

`runner.hashes_file` writes the lane hash file described in [testing.md](testing.md).

The file also carries the sound rows: 300 frames of `cogmaster-run --hash-audio` over `psg_sweep.sms` (Master System
on NTSC, PAL and Japan, and the Game Gear in Master System mode), `psg_sweep.gg` (native) and `fm_sweep.sms` (Master
System, Japan and NTSC, and PAL with `--fm on`, the export console with the unit forced on, so a PAL FM stream cannot
differ between lanes unseen). Each `audio-transitions` and
`audio-pcm` line is tagged `<file> <profile> <region> baseline` after its keyword. `tests/audio/transitions.txt` and
`tests/audio/pcm.txt` freeze those lines; `runner.hashes_file` requires the rows to equal them and names the first
line that differs, and `runner.hash_audio` requires its own `psg_sweep.sms` lines to be among them. The Japan row of
`psg_sweep.sms` differs from the NTSC one only because the Japanese console carries the FM unit, which adds the control
record and lowers the PSG; every other existing row is unchanged. `fm_sweep.sms` has its own detection routine for
port `$F2`: the three `fixtures.fm_sweep.*` tests judge its detect record on the Japanese console (1), the export
console (0) and the Game Gear (0); `fm.detect` loads the built image with the unit forced on for an export console
(1) and off for a Japanese one (0); and `runner.hash_audio.fm` shows its Japanese and export runs repeat, equal their
frozen lines and differ from each other. `ctest -L fm` runs all of these with the chip and unit tests. After an
intended change, regenerate them from a clean `ci` build:

```sh
grep '^audio-transitions' build/ci/hashes.txt > tests/audio/transitions.txt
grep '^audio-pcm' build/ci/hashes.txt > tests/audio/pcm.txt
```

The transitions hash covers every sound-chip output change in order, so it proves the emulated behaviour; the PCM
hash covers the samples after synthesis, so it also proves the mix. A mix retune changes only `pcm.txt`; a change
to `transitions.txt` means the emulated behaviour moved.
