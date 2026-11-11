# Controllers

What a port can hold, how the core picks it, how the Sports Pad, the Paddle and the 3-D glasses behave, and how the
runner and the libretro core drive them. The calls and structs are in [embedding.md](embedding.md), the runner's
options in [runner.md](runner.md). Every timing below follows MAME and ares and is unverified on hardware.

## What each machine can hold

| Controller | Master System, any region | Japanese Master System | Game Gear |
|---|---|---|---|
| Pad, or nothing | yes | yes | yes |
| Sports Pad | yes | yes | no |
| Paddle | no | yes | no |

Either port can hold any controller its machine allows. The Game Gear has its own keys and no ports to attach to.
The Light Phaser is not modelled.

## Which controller a port gets

1. The hash table names a controller for some dumps (a pad, a Paddle or a Sports Pad per port). The library reports
   it with the loaded content and resolves it against the machine and region: a controller the profile cannot take
   becomes a pad, never an error.
2. An option from the user wins over the table. The runner takes `--port1` and `--port2`, libretro takes the core
   options `cogmaster_port1` and `cogmaster_port2`; both default to `auto`, the table's choice.
3. An explicit choice the profile cannot take is refused. The runner exits 77 with a message naming the fix
   (`--region ntsc-j`, or the Master System instead of the Game Gear). libretro cannot refuse a load option, so it
   uses a pad and logs one line.

A dump with no region in the table would normally resolve to the export console, where a Paddle cannot exist.
When the table row says Paddle and names no region, the library guesses Japan, so `--probe` shows it and `auto`
takes it. A row with a region keeps it, and an explicit `--region ntsc-u` wins and the Paddle becomes a pad. Forcing
`--port1 paddle` on a title the table does not call a Paddle title needs `--region ntsc-j` as well. A Japanese
profile turns the FM unit on by default, so those dumps play with FM.

`--probe` prints `port1_default` and `port2_default` (`pad`, `paddle` or `sports`), and the execution line names
the resolved controllers. A run with a pad only, or with no content, resolves to pads, so a pad run is unchanged
by any of this. A frontend that offers only a joypad cannot force a pad over a table default: set the option to
`pad`.

## Input

`cogmaster_input` has the pad bits and console buttons as before, then four signed axes: `x1` and `y1` for port 1,
`x2` and `y2` for port 2. An axis is a delta in counts, from -32767 to 32767, and a call outside that range is
refused whole. The host convention is +x right and +y down.

Each call adds its axes to what is already waiting, clamped to the same range, so two calls before one frame equal
one call with their sum while that sum is within range. Send each increment once; a host that repeats an absolute
position would move the pad twice. Buttons and controller choices are last-wins. Changing a port's controller
drops that port's waiting motion, and axes for a controller that takes none are dropped. At the start of each frame
the waiting motion is handed to the controller and cleared.

A soft reset keeps the waiting axes, so motion sent just before a reset applies on the next frame; a load clears
them. Both resets clear the controllers' own state and the 3-D eyes.

A caller built against the 32-byte `cogmaster_input` of API versions 1 to 11 stays valid: the call copies the
smaller of the caller's size and its own, whole fields only, and zeroes the rest.

## Sports Pad

The pad answers a read in five steps, started by TH falling. The first fall latches both axes and shows the high
nibble of X, the next rise shows the low nibble, the next fall and rise show Y the same way. D0 to D3 carry the
nibble, TL and TR are buttons 1 and 2, active low, and the pad ignores the direction bits of `p1` and `p2` while it is
on the port. Before its first read after a load, a reset or a device change it shows `$F`; after that the last nibble
stays on D0 to D3 until the next TH edge, however long TH stays put (MAME keeps its output nibble through the timeout, and
ares keeps presenting stored nibbles; neither returns to `$F`).

- Each fall hands out at most 127 counts per axis; what is left stays in the pad, which keeps at most 1023 counts
  per axis. That cap is a limit of the model, not of the hardware.
- The byte is the negated delta: moving the host +x or +y reads as a negative byte, as in MAME and ares. This is
  emulator agreement, not hardware proof.
- A fall that arrives at the second step before TH has been high for `SPORTS_IDLE_TICKS` (1,536 ticks, 512 Z80
  cycles or 143 microseconds, MAME `sms_ctrl/sports.cpp`) is the Y half of the same read. After that long, or from
  idle, it starts a new read. ares instead resyncs when TH has been low 60 microseconds; that rule is not modelled and
  would be added only if a game desynced.
- The decision runs when the edge arrives. There is no timer and a read that changes nothing costs one compare.

## Paddle

The Paddle sends its position, 0 to 255 with 255 fully clockwise, as two nibbles on D0 to D3, with TR saying which:
low on a 0, high on a 1. TL is the button, active low. The paddle drives TR at its own clock, a square wave whose
half period is `PADDLE_HALF_TICKS` (299 ticks) from tick 0, the same on both ports and with no state. Delta x adds to
the position (+x is larger) and y is ignored. A console TR output still overrides the clock.

SMS Power and ares give the clock as 18 kHz, which as the TR frequency is a half period of 298.3 ticks, and MAME
uses 300. The value 299 is within half a percent of both and is not a multiple of 3, so no polling loop of fewer than
299 whole Z80 cycles locks exactly to it; a lock would give 0 or 100 percent ones and is the failure that games with
a tight loop would show. A near lock exists at loops of 199 and 398 cycles, which advance the phase one tick a
sample; no known title polls at those lengths. The fixture sweep counts 126 to 150 ones in 256 samples at loops of 29,
31, 47, 64, 97, 100, 101 and 200 cycles, and 174 and 113 at 199 and 398. Whether 18 kHz is right for the hardware
only a real Paddle can show.

## 3-D glasses

A store to `$FFF8` to `$FFFB` selects the eye: bit 0 clear opens the right shutter, set the left. The core records
it on every Master System profile and reports it in the run result as `glasses`: none when nothing wrote it since
load or reset, right or left otherwise, and always none on the Game Gear. The write also reaches work RAM as it
always did, so it changes no value a game reads and no hashed byte; the frame hash is the same whether or not a host
composes the eyes, and a store never counts as a controller poll. `$3E` plays no part: the report does not depend on
the cartridge being enabled, and a store with RAM disabled faults as before.

Real glasses exist for the original Master System, the Mark III and the Japanese Master System; reporting on the
second-revision profile, the default, is a modelling choice so that the default profile shows 3-D titles, not a
claim about hardware. The cost is that a title which writes those mirror addresses for its own reasons, a clear or
a fill, reports an eye. The core cannot tell the two apart; the libretro adapter does (below).

A frame reports the eye in force at its start, unless a store lands in the first part of the frame's window: the
window start, all of vertical blanking and the top 96 active lines count for this picture, and later stores belong to
the next one. The cut at active line 96 is the core's own choice; MAME cuts at half the screen's total height, about
line 88 on NTSC, which is not claimed.

`--eyes` on the runner prints one line after the last frame with how many frames reported each eye, `flips`
(frames whose eye differs from the last non-zero one) and the last eye. It only counts: `--hash`, `--png` and
`--expect` stay the plain picture.

## The libretro core

The core options are `cogmaster_port1`, `cogmaster_port2` (`auto`, `pad`, `paddle`, `sports`, `none`; reload content)
and `cogmaster_3d_view` (`left`, `right`, `alternate`, `anaglyph`, `off`). Only the base device ids are polled; there
are no controller subclasses. A port holding a Sports Pad or a Paddle sums its sources each frame and clamps each
axis to 32767 before the call:

- the mouse delta, which RetroArch gives already relative;
- the left stick, `(axis * K) / 32768` outside a 20 percent dead zone, K 6 for a Sports Pad and 4 for a Paddle (the
  integer division makes the first Paddle count start at 25 percent);
- the D-pad, 2 counts a frame.

Buttons 1 and 2 come from RetroPad B and A or the mouse's left and right button. The stick and the D-pad are the
guaranteed path; whether RetroArch delivers a mouse to a port that is not a mouse port is unmeasured.

The 3-D view applies only while the eyes alternate. The adapter holds a count of 4 that is set whenever a frame's eye
differs from the last non-zero eye and falls by one a frame. While it is above zero `left` and `right` show that
eye's latest picture, `alternate` and `off` send each frame as drawn, and `anaglyph` mixes the last two pictures
(red from the left, green and blue from the right) through one buffer taken at load. At zero, for a title that
reports no eye or one that never changes, the frame goes out untouched. Only the adapter does this; nothing is saved
and no hash changes. The default is `left`.

## Limits

The timings follow MAME and ares and are unverified on hardware. The Sports Pad sign, the Paddle direction and
clock, and the cut at line 96 are agreement between emulators, judged only by a run on a real cartridge and pad.
The Japanese polled Sports Pad, a Paddle on the export console, the shutter's own transition lag and eye changes
within a frame are not modelled.
