# Video

Content uses one production Mode 4 background renderer. In the 192-line
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
