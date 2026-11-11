# Controllers

What a port can hold, how the core picks it, how the Sports Pad, the Paddle, the Light Phaser and the 3-D glasses
behave, and how the runner and the libretro core drive them. The calls and structs are in [embedding.md](embedding.md), the runner's
options in [runner.md](runner.md). Every timing below follows MAME and ares and is unverified on hardware.

## What each machine can hold

| Controller | Master System, any region | Japanese Master System | Game Gear |
|---|---|---|---|
| Pad, or nothing | yes | yes | yes |
| Sports Pad | yes | yes | no |
| Paddle | no | yes | no |
| Light Phaser | yes | yes | no |

Either port can hold any controller its machine allows. The Game Gear has its own keys and no ports to attach to,
so it holds no Light Phaser; the Mark III is not a profile.

## Which controller a port gets

1. The hash table names a controller for some dumps (a pad, a Paddle, a Sports Pad or a Light Phaser per port). The library reports
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

`--probe` prints `port1_default` and `port2_default` (`pad`, `paddle`, `sports` or `lphaser`), and the execution
line names the resolved controllers. A run with a pad only, or with no content, resolves to pads, so a pad run is
unchanged by any of this. A bare run of a dump the table calls a gun title attaches the gun with nothing aimed or
pressed, and `--port1 pad` restores the old run. A frontend that offers only a joypad cannot force a pad over a
table default: set the option to `pad`. For a gun title it gets the cursor path and a crosshair (below).

## Input

`cogmaster_input` has the pad bits and console buttons as before, then four signed axes: `x1` and `y1` for port 1,
`x2` and `y2` for port 2. An axis is a delta in counts, from -32767 to 32767, and a call outside that range is
refused whole. The host convention is +x right and +y down.

Each call adds its axes to what is already waiting, clamped to the same range, so two calls before one frame equal
one call with their sum while that sum is within range. Send each increment once; a host that repeats an absolute
position would move the pad twice. The Light Phaser is the exception: its axes are an absolute aim and the last call
wins, so a later call in the same frame with no axes, a tag-32 call or a 35-byte script line parks the aim. Buttons and
controller choices are last-wins. Changing a port's controller
drops that port's waiting motion, and axes for a controller that takes none are dropped. At the start of each frame
the waiting motion is handed to the controller and cleared.

A soft reset keeps the waiting axes, so motion sent just before a reset applies on the next frame; a load clears
them. Both resets clear the controllers' own state and the 3-D eyes. A Light Phaser's aim follows the same rule: a
soft reset and a power reset keep it, as they keep a Sports Pad's waiting motion, and only a load parks it.

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

## Light Phaser

The gun rides either port of a Master System. It has a trigger and a light sensor and no other control.

**Aim.** On a phaser port `x1 y1` (port 1) or `x2 y2` (port 2) are an absolute aim in 1-based field dots: `x` from 1 to
256 and `y` from 1 to 240, the dot being `x - 1`, `y - 1`. `0 0` means no aim, so a zeroed input points nowhere. The
call is last-wins, not summed. An axis outside -32767 to 32767 refuses the call whole, as for every device. A pair with
either coordinate outside the field is stored as `0 0`: an aim that is not a field dot senses nothing and is never moved
to an edge pixel. Changing the port's controller restarts its aim, a soft reset or a power reset keeps it, and a load
parks it. One real aim is given up by the 1-based zero: the dot just left of the field, whose disc would still light
the first six dots. The trigger is bit 4 of `p1` or `p2` and drives TL, low when pulled; D0 to D3 and TR are undriven
high and the direction bits are ignored while the gun is on the port.

**Runner.** On a phaser port the 75-byte script line's `x1 y1` or `x2 y2` are the aim and bit 4 of `p1` or `p2` is
the trigger; the 35-byte form is no aim. The runner is stricter than the core: a negative axis, `x` above 256, `y` above
240 or only one of the pair 0 is exit 2 before frame 1 naming the port and the controller. It draws no crosshair and
maps nothing, because every machine that holds a gun shows its picture at 0,0, so a script aim is the field dot. A gun
on a Game Gear is exit 77.

**libretro.** The core option decides what is attached; the frontend's device for the port decides only where the aim
comes from. With the Light Gun device, the pointer's screen position and off-screen flag give the aim through the
rectangle of the last completed frame, and the trigger is the gun's trigger. With a joypad (the guaranteed path) a
per-port cursor starts at the centre and moves by the mouse delta, the left stick past a 20 percent dead zone (4
counts a frame at full deflection) and the D-pad (2 a frame), clamped to the picture and never off screen; the trigger
is RetroPad B or A or the mouse's left button. The cursor resets at load, at reset and at unload. When port 0 is a
gun, Pause and Reset read that port's Start and Select. A mode change from 192 to 224 lines moves the rectangle one
frame late. The crosshair (`cogmaster_crosshair`, on by default, and `cogmaster_crosshair_colour`: white, red, green
or yellow) is up to 17 points around the aim, drawn into the presented picture only while a phaser port has an aim.
It is never in the hashed frame, the 3-D pictures or any recording.

**Model.** The sensor reads the finished field pixel the VDP emitted, never the palette. It sees dots within
`GUN_RADIUS` (6) of the aim dot, an integer disc of 113 dots clipped to the field. A dot is lit when
`(77 r + 150 g + 29 b) >> 8` is at least `GUN_LUMA` (127), each colour nibble first widened to 8 bits as `n * 17`; 32
of the 64 Master System colours are lit. The sensor sees a dot `GUN_DELAY_TICKS` (38 ticks, 19 dots) after the VDP
emits it, on both edges. All three constants are unmeasured: each is MAME's value (`sms_ctrl/lphaser.cpp`) or the
nearest whole number, and a real gun could change any of them. The disc is narrower than MAME's 133 dots, and every
dot is tested where MAME samples every fourth.

**TH.** TH reads low on `$DD` (bit 6 for port 1, bit 7 for port 2) while the sensor sees light, only while that
port's TH is an input; an output TH wins. A lit run of 13 dots is 26 ticks, and the shortest poll loop is 75 ticks or
more, so a purely geometric level would be missed on most lines, and a loop whose length divides the 228-cycle line
would never see a narrow pulse. A sticky bit therefore keeps a pulse: it is set at each fall while TH is an input,
shown as TH low on the next `$DD` read, cleared by that read and cleared at the wrap of every picture, so a pulse
never outlives its picture. MAME has a bit for the same purpose, but it shows only while the VDP's latch is set and a
`$7F` read clears it; here the bit is not gated by the latch and a `$7F` read does not clear it. Both differences are
unmeasured. The latch and the edge rule use the geometric level only.

**Latch.** The existing rising-edge latch applies: H is taken where the sensor releases TH. MAME and ares do the
same. SMS Power says the latch is taken when TH goes low and MacDonald says either edge, so the true edge, the delay,
the pulse width and the sticky bit are all unmeasured. With the release at dot `x_off` (the first dot after the lit
run) H is `(x_off + 35) >> 1`. The edge is one predicate where the core calls the latch, so a different rule is a
one-line change. MAME releases at the end of the aim disc's chord whatever the picture holds; this model releases at
the first dot that is not lit. The two agree on a uniformly bright area, which is what games draw for a shot, and
differ on a lit shape smaller than the disc, where this model releases up to 6 dots earlier.

**No shot.** The sensor responds to light whether or not the trigger is held, so a gun aimed at a lit area changes
what `$DD` and `$7F` read. The invariant that holds is the other one: with a gun attached, the trigger never pulled
and no light in its view (no aim, or an aim over dark dots), the frame and audio hashes, the fault record and all
machine state apart from the gun's own fields equal those of a pad with nothing pressed. The exception is a video mode
the renderer rejects with the display on: the sensor then asks for no dots, leaving the renderer's own schedule to
record the fault, and where a poll sees light there can depend on when the poll comes. No hash and no fault record
depends on it.

**Order and cost.** A sensor edge comes before a `$3F` write at the same tick. At one tick port 1 comes before port 2.
With two guns the edges reach the single H latch in time order whatever the poll schedule. Work is bounded per picture:
at most 113 sensed dots and 13 closing dots per gun, however often a program polls, and none at all when no gun is
attached.

**Tolerance.** The core is exact. The fixture that runs on a Z80 allows plus or minus one H count, one step of the
counter (4 ticks, 2 dots), and every control it must reject is at least 3.5 counts away: an aim 16 dots off, a delay of
16 ticks more, the leading edge, and a latch at the read, which is at least 16 counts late.

## The libretro core

The core options are `cogmaster_port1`, `cogmaster_port2` (`auto`, `pad`, `paddle`, `sports`, `lphaser`, `none`; reload content)
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
within a frame are not modelled. The Light Phaser has no per-title offset, no Game Gear or Mark III form and no
`$7F`-read clear of the sticky bit, and no hosted RetroArch session drives it; the delay, the edge, the pulse width
and the sticky bit are agreement between emulators and need a real gun to judge.
