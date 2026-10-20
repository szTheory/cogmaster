# Video

Content uses one production Mode 4 background renderer. In the 192-line
path, the name table selects four-plane 8×8 tiles; its palette and flip bits
are decoded before SMS two-bit channels expand to RGB444 by multiplication
by five. Display-off pixels use CRAM entry 16 plus R7's low nibble.
The fixed field remains bounded by 256×240 pixels.

Each completed line advances a monotonic cursor. Stamped writes catch up
before changing VRAM, CRAM or registers. A line touched during its active
part uses its pre-write sources for the whole line; later raster plans
widen the timing model. The cursor and emitted pixels participate in state.
The frame interrupt completes the field and opens the next cursor.
Sources: SMS-VDP sections 2, 3, 6 and 9 in the preparation index.

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
