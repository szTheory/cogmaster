# Video

Native Game Gear CRAM has 64 bytes. Even writes update a shared low-byte
latch without changing colour; odd writes commit that latch and the low
nibble of the new byte at the odd address's entry. Address changes and wrap
do not discard the latch. SMS video mode uses immediate 32-byte six-bit
colours. Both paths feed the same background/sprite RGB444 renderer.
Sources: preparation SMS-VDP section 6.2 and
[GG VDP](https://www.smspower.org/Development/GGVDP).

The native LCD window is 160×144 at (48,24), or (48,40) in 224-line mode.
The selected 240-line model freezes the last valid LCD pixels and descriptor
until POWER reset, including across SOFT reset and later height changes.
Full-field diagnostic output continues independently. This is a
confidence-qualified hardware model, not a measured freeze edge. Explicit
SMS video exposes the full SMS field and PAR; physical LCD mixing/scaling
belongs to host presentation. Sources: preparation SMS-VDP section 6.3,
[GG LCD](https://www.smspower.org/Development/GGLCD).

The handheld's C0/C1 aliases answer the same controller pins as DC/DD.
Other full-decode gaps remain undriven. Native Start reads active low at
00; explicit SMS mode instead routes Start edges to the Pause NMI.
Sources: phase research “GG decode, pins and presentation” and SMS-SYSTEM
section 4. Local pad two, reset and TH remain absent on the handheld.

Content uses one production Mode 4 renderer. In the 192-line
path, the name table selects four-plane 8×8 tiles; its palette and flip bits
are decoded before SMS two-bit channels expand to RGB444 by multiplication
by five. Display-off pixels use CRAM entry 16 plus R7's low nibble.
Each working and completed field remains bounded by 256×240 pixels.

The 32×28 name map in 192-line mode wraps vertical coordinates at 224 pixels, horizontal
coordinates at 256. R8 shifts right: the first R8 modulo 8 dots show backdrop.
R0 bit 6 fixes the top 16 lines horizontally; bit 7 fixes source columns
24–31 vertically. Bit 5 masks the first eight display dots. Name bits 8–0
select the tile, 9/10 flip it, 11 selects palette two, and 12 gives priority
over sprites only for nonzero tile colours. An enabled empty background
shows CRAM[0]; an empty SAT contributes no pixels. Display-off shows backdrop.

The cursor advances by raster intervals. Before a stamped VRAM, CRAM or
register write it emits dots strictly before `tick / 2`, capped at 256;
earlier dots never change. The selected timing model samples R8 and display
enable at tick 600 for the next line. R9 samples there on the final physical
line for the following field, even with display disabled. These approximate
latch positions are sourced choices, not hardware measurements. The cursor,
latches and emitted pixels participate in state. The frame interrupt completes
the field and opens the next cursor. Native hashes serialize cropped RGB444
words low byte first; host palette conversion cannot change that identity.

A Light Phaser port reads emitted dots, so it may ask the renderer for them
early. It asks for dots strictly before `tick / 2` of the current line, the
bound every VDP write uses and never one dot more, which a status read would
use. It does not ask once the beam is past the last picture row, because the
field is final then and a call would flush it early, and it does not ask while
the display is on in a mode the renderer rejects: the renderer's own schedule
records the first fault there. Rendering earlier is idempotent and moves no
pixel, no status flag and no fault, so the sensor's polling schedule cannot
change a frame hash.

Sources: preparation SMS-VDP sections 2, 3, 4.7, 6 and 9;
[SMS Power tilemap](https://www.smspower.org/Development/Tilemap) and
[registers](https://www.smspower.org/Development/VDPRegisters).

`COGMASTER_MACHINE_SMS1` selects the 315-5124 through the same renderer.
For 192 lines R2 bit 0 gates name-table address A10: when clear, rows
16–27 mirror rows 0–11. R3 gates pattern A13–A6 for planes 0/1; R4 bits
2–0 gate A13–A11 for planes 2/3. Set R3=$FF and R4=7 for full addressing.
R5 bit 0 gates SAT address A7 (X/tile pairs mirror Y storage when clear).
R6 bit 2 supplies sprite A13; bits 1/0 gate A12/A11, or tile bits 7/6.
SMS2/GG ignore these mask bits. The address helpers are shared by tile
fetches and the sprite pipeline. Source: the register diagrams linked above.

Graphics I (mode select nibble 0000) and Graphics II (0010) render on
`COGMASTER_MACHINE_SMS1`, `COGMASTER_MACHINE_SMS2` and the Game Gear in
Master System video mode. These notes name a mode by that nibble, M4 M3 M2 M1
(SMS-VDP section 2.5), or by R0 and R1 bits, never by TI's M numbers, which
number the same bits from the other end (TI's M3 is R0 bit 1, SMS Power's M2).
With the display enabled, any other mode renders deterministic backdrop and
records `COGMASTER_FAULT_KIND_UNSUPPORTED_VIDEO_MODE` on the VDP, while the
CPU still returns at the fixed frame deadline: text (0001), multicolour (0100),
the mixed modes (0011, 0101, 0110, 0111), the invalid text modes (1001 and 1101
everywhere; 1011, 1110 and 1111 on the 315-5124, which supports only the Mode 4
combinations 1000, 1010 and 1100), and both legacy nibbles on native Game Gear
video, which has no such modes. The fault's address holds the nibble.

Both legacy modes are 192 lines of 32×24 cells with no scroll: R8, R9 and R0
bits 5–7 change nothing, so there is no left-column mask, and a line from 192
on shows the backdrop, which keeps a mid-frame switch from a 224 or 240-line
Mode 4 picture inside the name table. R1 bit 6 clear shows the backdrop alone.
Registers are constant inside one renderer call and a register write renders up
to its own dot first, so a switch is dot-exact. Every address wraps at 16 KiB
and none of the Mode 4 helpers (the 315-5124 masks of R2, R5 and R6) applies.
Graphics I reads the name at `(R2 & 15) << 10 | row << 5 | column`, the
pattern at `(R4 & 7) << 11 | name << 3 | line & 7` and the colour byte at
`R3 << 6 | name >> 3`; a set pattern bit shows its high nibble and a clear bit
its low nibble, and nibble 0 shows the backdrop, R7's low nibble. Graphics II
forms the index `idx = (line >> 6) << 8 | name`, reads the pattern at
`(R4 & 4) << 11 | idx << 3 | line & 7` and the colour byte at
`(R3 & $80) << 6 | idx << 3 | line & 7`. Sources: preparation SMS-VDP
section 7; the TMS9918A notes on SMS Power, `tms9918a.txt`, read from a
web.archive.org capture (the source of the unmasked addresses the in-process
oracle uses); TI's TMS9918A data manual, section 2.4.1 and table 3-2.

Graphics II masks are a selected model. On the 315-5124, R4 bits 1–0 and R3
bits 6–0 limit the index: the pattern uses `idx & ((R4 & 3) << 8 | $FF)` and the
colour `idx & ((R3 & $7F) << 3 | 7)`. The Master System 2 and the Game Gear in
Master System mode do not mask. The sources disagree three ways: msvdp says no
mask on the Master System 2 and Game Gear; MAME's `315_5124.cpp` masks on every
revision; ares applies R4 bits to both tables through address bits 11 and 12.
MAME's `tms9928a.cpp` also couples R3 into the pattern index on the TMS9918A
family; that is not adopted, since nothing shows the Sega chips do it. Every
source agrees on R3 $FF with R4 $03 and R3 $7F with R4 $07, the values games
write, so the model rarely shows. Colours are a model too: the Master System 1
and 2 show a fixed table of sixteen six-bit values ($00 $00 $08 $0C $10 $30 $01
$3C $02 $03 $05 $0F $04 $33 $15 $3F) through the CRAM two-bit expansion (SMS
Power palette page), the Game Gear in Master System mode shows CRAM entry 16 + n
(msvdp, SMS Power and MAME; ares uses the fixed table there), and no source
measures either against a console. The backdrop is a legacy palette entry, R7's
low nibble, only while the display is on in a legacy mode. With the display off
it is CRAM entry 16 + that nibble as in Mode 4, where MAME and ares show the TMS
colour. The deviation is bounded and pin-safe: the power-on registers select
Graphics I with the display off, so a TMS backdrop there would repaint the boot
frames of every Mode 4 game; a legacy game's frames before it enables the
display show the CRAM backdrop on a Master System.

Legacy sprites are a model as well. The table holds 32 entries of four bytes
(Y, X, name, colour byte) at `(R5 & 127) << 7`, the patterns are at
`(R6 & 7) << 11`, R1 bit 1 selects 16×16 (name & $FC, then top left, bottom
left, top right, bottom right) and R1 bit 0 doubles every dot both ways, plain
doubling on every profile: the 315-5124 slot rule of Mode 4 does not apply.
Colour 0 is transparent, a lower index wins and a sprite always covers the
background. `$D0` in a Y byte ends the list at every height. A Y of $E0 or more
is negative (the top line is Y − 255, otherwise Y + 1); MAME's `tms9928a.cpp`
tests above $E0, MAME's `315_5124.cpp` $F0 or more and ares $E0 or more, which
differ only for a sprite 32 lines high with Y from $E0 to $EF, and TI's manual
range fits above $E0 and $E0 or more alike. Colour byte bit 7 is the early
clock (X minus 32); R0 bit 3 is ignored here (MAME and ares; msvdp says it also
shifts by eight, which stays open). Four sprites fit a line. The fifth Y match,
counted even for a transparent or off-screen sprite, sets status bit 6 at the
first dot of its line, and its table index stays in bits 4–0, the first winning
until a status read; with bit 6 clear they read $1F, as in Mode 4, whereas
Young's notes and MAME's `tms9928a.cpp` give the last entry processed. Coincidence,
bit 5, follows the pattern bits, so a colour 0 sprite collides (TI's manual and
Young; MAME's `315_5124.cpp` counts opaque dots, ares a dot over an earlier
opaque sprite), only on dots 0–255 and not for a sprite after `$D0` or dropped
as fifth. A line from 192 on draws no sprite and sets no flag. These models are
revisited with the SG-1000 machine. A fixture proves fidelity to these notes,
not to silicon. Sources: preparation SMS-VDP section 7, TI's data manual
section 2.4.5, the TMS9918A notes `tms9918a.txt` section 4, and MAME's
`tms9928a.cpp` and `315_5124.cpp` and ares's `tms9918` files, read at the pins
in PROVENANCE.

Sprites share the same cursor. At tick 600 the SAT is scanned for the next
active line: at most 64 entries and eight retained Y matches. The selected
X, tile and source row are saved; pattern bytes and palette remain live for
unemitted dots. A direct renderer entry prepares a line once if needed.
Y denotes the line before the top, $F0–$FF wraps above line zero, and $D0
terminates the 192-line SAT. 8×16 ignores tile bit zero and selects the
following tile for its bottom half. R0 bit 3 shifts X left by eight; right
edges clip without wrapping. R1 zoom doubles vertical pixels and SMS2/GG
horizontal pixels. SMS1 doubles slot i horizontally only if slot i+4 exists:
two/four/six/eight matches give VV/VVVV/FFVVVV/FFFFVVVV. Opaque lower SAT
slots win; nonzero priority background covers them. Masked left dots and
display-off dots show backdrop. Normally hidden GG sprites still use slots.
Sources: preparation SMS-VDP sections 3.4–3.8 and ARCHITECTURE section 13;
[SMS Power sprites](https://www.smspower.org/Development/Sprites).

The ninth Y match sets overflow at the first enabled active dot of that
line, even for transparent or clipped sprites; no ninth slot is rendered.
Collision sets only on a dot shared by two opaque retained sprites, with
display enabled and outside the masked left column. Touching edges,
transparent dots and pixels outside the selected 256×H active field cannot collide.
Background priority hides sprite colour but does not suppress occupancy.
These are tests of the selected active-area model, not hardware timing
measurements. A status read catches up through the dot at `tick / 2`,
observes flags, then clears all flags and the control latch and recomputes
both IRQ sources. Bits 4–0 still read as ones. Re-reading that same dot
cannot reassert; a later real overlap can. Sprite flags alone do not drive
IRQ. Sources: SMS-VDP sections 2.6 and 3.5–3.8, and
[SMS Power collision detection](https://www.smspower.org/Development/CollisionDetection).

The selected frame/line IRQ edges remain ticks 607/608, the next-line latch
600, and the CPU-visible V step 604, all relative to active dot zero.
Frame pending is status bit 7; line pending is separate. Each pending source
drives /INT immediately while its IE is enabled (R1 bit 5 / R0 bit 4).
Disable releases that source without losing pending; a status read observes
then clears both sources and the control latch. Events at the access stamp
precede the access, including when a CPU step spans several events.
R10 reloads on blank lines; active lines and the first postactive line test
zero, reload and set pending, or decrement. A write to R10 affects the next
reload. Thus n first fires on line n and repeats every n+1 lines; $FF never
underflows within the active range. Frame pending rises on physical line H
for H=192/224/240, without moving the host's fixed frame origin/deadline.
SMS1 invalid extended modes retain 192-line timing with unsupported rendering.
Sources: preparation SMS-VDP sections 2.5, 4.2, 4.5–4.6 and SMS-SYSTEM section 2.
The 607 measurement and selected 608 placement have different confidence
from the approximate latch/V positions; passing tests proves this chosen
model, not a hardware measurement or agreement with a reference binary.

V-counter sequences are listed independently below. Each value lasts one
line; the next value becomes CPU-visible at tick 604, including jump/wrap.

| TV / height | V-counter intervals |
|---|---|
| NTSC / 192 | $00–$DA, $D5–$FF |
| NTSC / 224 | $00–$EA, $E5–$FF |
| NTSC / 240 | $00–$FF, $00–$06 |
| PAL / 192 | $00–$F2, $BA–$FF |
| PAL / 224 | $00–$FF, $00–$02, $CA–$FF |
| PAL / 240 | $00–$FF, $00–$0A, $D2–$FF |

The NTSC-240 sequence rolls every 263 counts, independently of the host's
262-line / 179208-tick deadline. Its saved phase advances across host frame
boundaries. On entry into that mode, the selected model seeds phase from
the current physical line (plus one at/after 604). This entry phase is an
explicit inference. Other modes use the physical 262/313-line beam; PAL
keeps its 214092-tick deadline. Native GG LCD behavior follows the separate
crop and freeze model above.

The selected H coordinate translates active-origin ticks by 32 modulo 684,
then reads $00–$93 followed by $E9–$FF, four ticks per value. In active
coordinates the intervals are ticks 0–559 = $08–$93, 560–651 = $E9–$FF,
652–683 = $00–$07. Thus active dot zero is $08 and GG left dot 48 is $20;
607 reads $F4 and 608 reads $F5. These anchors reconcile the final 2008
[Display Timing discussion](https://www.smspower.org/forums/8161-SMSDisplayTiming)
with the [official counter sequence](https://www.smspower.org/Development/GGOfficialDocs).
The 32-tick translation and its sub-dot phase are coordinate inferences;
they do not move the IRQ or latch events or establish chip-level accuracy.

Before the first capture H reads live. A low-to-high effective TH transition
on either SMS port captures H, which remains held until another capture or
reset. Readback and edge detection share pin mixing: input TH pulls high,
export outputs use their written levels, Japanese outputs read low, and
native GG has no TH transition. Releasing low output to input captures;
falling edges and unchanged effective levels do not. Capture value, held
flag and rolling V phase participate in state, validated before applying;
H values in the sequence gap are refused. Peripheral timing remains a
selected model; future hardware evidence may refine latch phase/edges.
Sources: SMS-SYSTEM section 4 and
[peripheral port directions](https://www.smspower.org/Development/PeripheralPorts).

The owned `boot_to_play` fixture writes a diamond in plane 0 and $55, $33,
$0F in planes 1–3, with palette entry n containing n. Its independent oracle
expands the diamond into eight explicit bit rows, adds 2 for odd columns,
4 for columns 2,3,6,7 and 8 for columns 4–7. Red is `(n & 3) * 5`, green
is `(n / 4) * 5`, blue is zero. Repeating this 8×8 construction across all
256×192 coordinates gives the native little-endian SHA-256 checked against
the runner. After two frame flags, the ROM polls button 1 once per frame.
Press edges increment a colour counter (CRAM 15) and horizontal scroll by
one; holding and release leave the counter unchanged. The first eight polls
report `counter | scroll << 8` in successive mailbox records, sealed once
after the eighth measurement. Live counter and previous button levels occupy
$C200 and $C201. In the scripted check, frame 5 presses, 6 holds, 7 releases,
8 presses again; counts are 1, 1, 1, 2 at those first boundaries.

Height is sampled at tick 600 of the final physical line, after outgoing
active intervals have completed; a write stamped exactly at that tick misses
the latch. This is a selected coherent field-boundary model, not a measured
chip latch. Rendering, IRQ height and counter tables use that latch. Supported
Mode 4 combinations are 1000/1010/1100 (192), 1011 (224), 1110 (240),
and 1111 (192) on SMS2; SMS1 retains its documented three 192-line modes.
Extended modes use the 32×32 map, wrapping at 256, with R2 bits 3–2
selecting $0700/$1700/$2700/$3700. $D0 only terminates sprites at 192 lines.
Sources: preparation SMS-VDP sections 2.5, 3.1, 3.4 and 5.2.

At the height-specific frame event the core publishes a pixel snapshot and
its height, crop and backdrop together. Both frame and info queries use that
completed descriptor, even while the following field is partially rendered.
The initial/reset descriptor is a black 192-line field. Fixed NTSC/PAL host
deadlines remain 179208/214092 ticks; extended geometry may first appear
one host call later. SMS PAR is 8:7 NTSC and 2950000:2128137 PAL; GG
remains 6:5. The two fixed buffers are instance-owned and saved with the
height latch and completed metadata; restoration validates heights before
applying. Pixel conversion and caller pitch remain outside emulation.

The owned `vdp_lines` guest runs the same source at 192/224/240 lines on
NTSC and PAL. Four tiles encode zero, a one-bit diagonal, solid colour 2,
and solid colour 3. Row one columns 2–5 select hflip, vflip, palette two and
priority. R0 enables the left mask and both scroll locks; R9=8. R10=31
interrupts on 31/63/...; R8 starts at 8 and advances by 8 after each IRQ.
The handler writes after the next line has started but before its tick 600,
so bands begin on lines 33/65/... . Top-16 horizontal and right-eight-column
vertical locks are checked independently at every returned coordinate.

Two overlapping sprites at (40,0)/(48,0) use zoomed 8×16 tiles: sixteen
green rows followed by sixteen white rows. The second line IRQ clears size
and zoom before (72,96), where two overlapping 8×8 sprites demonstrate SAT
order. Background priority hides only opaque diagonal dots. The oracle in
`tests/fixtures/lines.c` constructs these coordinates without calling any
renderer helper and hashes its expected little-endian RGB444 words.

The guest seals eight numeric records after four frame handlers: completed
line-IRQ count, frame-IRQ count, H/V at frame-handler entry, first frame
status and its immediate second read, then V samples $3F/$40 across a
selected visible-counter boundary with IRQs disabled. R10=31 gives floor((H+1)/32) =
6/7/7 interrupts. The frame handler reaches its H sample 249 ticks after
pending plus 0–38 ticks for an interrupted instruction tail: H=$33–$3D.
The following V read precedes the next tick 604: $C1/$E1/$F1 on the physical
beam; NTSC-240 reads $ED at the fourth IRQ because its rolling phase is four
counts behind that beam. Prior line handlers clear sprite flags, leaving
first frame status $9F and second $1F. These source-derived model checks
are not console measurements.
