# Video

Content uses one production Mode 4 renderer. In the 192-line
path, the name table selects four-plane 8×8 tiles; its palette and flip bits
are decoded before SMS two-bit channels expand to RGB444 by multiplication
by five. Display-off pixels use CRAM entry 16 plus R7's low nibble.
The fixed field remains bounded by 256×240 pixels.

The 32×28 name map wraps vertical coordinates at 224 pixels, horizontal
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

Legacy TMS modes and invalid text modes render deterministic backdrop and
record `COGMASTER_FAULT_KIND_UNSUPPORTED_VIDEO_MODE` on the VDP, while the
CPU still returns at the fixed frame deadline. SMS1 only supports Mode 4
combinations 1000, 1010 and 1100; its extended-height combinations are invalid.

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
transparent dots and pixels outside the 256×192 active field cannot collide.
Background priority hides sprite colour but does not suppress occupancy.
These are tests of the selected active-area model, not hardware timing
measurements. A status read catches up through the dot at `tick / 2`,
observes flags, then clears all flags and the control latch and recomputes
the frame IRQ level. Bits 4–0 still read as ones. Re-reading that same dot
cannot reassert; a later real overlap can. Sprite flags alone do not drive
IRQ. Sources: SMS-VDP sections 2.6 and 3.5–3.8, and
[SMS Power collision detection](https://www.smspower.org/Development/CollisionDetection).

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
