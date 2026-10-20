---
title: "Master System and Game Gear cartridges, mappers, controllers, saves and hostile inputs"
summary: "The TMR SEGA header, every mapper with its library share from the CC0 MAME softlists, pin-level controller and peripheral protocols, battery RAM and 93C46 save interop, ROM-hack support and parser bounds for cogmaster."
read_when: "Before designing or changing the cartridge loader and probe, any mapper module, the controller ports, Light Phaser, 3-D glasses, saves (.srm), patch handling, or the loader fuzz targets."
updated: 2026-10-18
---
# Master System and Game Gear cartridges and peripherals (research CART)

| Question | Section |
|---|---|
| What is in the 16-byte header, how is the checksum computed, which images have none? | 1.1–1.3 |
| In what order does the loader identify an image? | 1.4 |
| What does each mapper do, how is it detected, how much of the library uses it? | 2.1–2.3 |
| How do bank numbers wrap for 1 MB and odd-sized ROMs? | 2.4 |
| Which bits of $DC/$DD/$3F are which pins; what do Pause and the GG Start button do? | 3.1–3.2 |
| How do the Light Phaser, 3-D glasses, Sports Pad, Paddle, MD pad, keyboard and link cable work? | 3.3–3.9 |
| What is a lag frame; when is input applied? | 3.10 |
| Which titles have battery RAM or EEPROM; what file layout interoperates? | 4 |
| IPS/BPS, hack identity, what ROM hackers want | 5 |
| Which parsers face untrusted bytes and with what bounds? | 6 |
| Which milestone gets each board and peripheral? | 7 |
| Open questions and the phase that settles each | 8 |

## 1. The ROM header
### 1.1 Fields at $7FF0 (also legal at $1FF0 and $3FF0; only $7FF0 is seen in known software) [CART.01]
| Off | Len | Field | Facts |
|---|---|---|---|
| $7FF0 | 8 | `TMR SEGA` | Required by export SMS BIOSes and later Game Gear BIOSes. MAME's `verify_cart` only warns when absent [CART.01][CART.10] |
| $7FF8 | 2 | reserved | Seen as $00 $00, $FF $FF or $20 $20 (Sega docs merged them into "TMR SEGA  ") [CART.01] |
| $7FFA | 2 | checksum, LE | Export SMS BIOS compares it; GG and Japanese releases often carry a wrong value [CART.01] |
| $7FFC | 2.5 | product code | Two BCD bytes give the low four digits (26 70 = 7026); the high nibble of $7FFE adds leading digits in hex (26 70 2 = 27026, 26 70 a = 107026) [CART.01] |
| $7FFE | 0.5 | version | Low nibble; 0 for the first release [CART.01] |
| $7FFF | 0.5 | region/system | High nibble: 3 SMS Japan, 4 SMS Export, 5 GG Japan, 6 GG Export, 7 GG International. Only the export SMS BIOS checks it [CART.01] |
| $7FFF | 0.5 | ROM size | Low nibble, table 1.2. Often present without a valid checksum, and often smaller than the real size to shorten the boot check [CART.01] |

### 1.2 Size nibble and checksum range (tested against MAME's table [CART.10]; semantics from [CART.01])
| Nibble | Size | Checksum covers | Note |
|---|---|---|---|
| $A / $B | 8 KB / 16 KB | $1FF0 / $3FF0 bytes | unused |
| $C | 32 KB | $7FF0 | common |
| $D | 48 KB | $CFF0 | unused, BIOS checksum buggy |
| $E | 64 KB | $10000 | rare |
| $F | 128 KB | $20000 | common |
| $0 | 256 KB | $40000 | common |
| $1 | 512 KB | $80000 | rare |
| $2 | 1 MB | $100000 | unused, buggy |

Algorithm (MAME's reimplementation of the BIOS): a 16-bit sum of every byte from 0 to the range end, skipping $7FF0–$7FFF, compared little-endian with $7FFA [CART.10]. A range larger than the file is clamped to the file.

**Codemasters header at $7FE0** (their games have no TMR SEGA): $7FE0 = 16 KB page count; $7FE1–$7FE5 = day, month, year, hour, minute in BCD; $7FE6 = 16-bit LE sum of all 16-bit words outside $7FF0–$7FFF; $7FE8 = $10000 minus that sum (derived from MAME's log routine and detection test: page count ≤ 9 in the low nibble, year 93/94/95, $7FEF = 0) [CART.10]. An **SDSC header** for homebrew also lives at $7FE0; the SMS Power page exists but was not fetched, so its layout is (unverified) [CART.01].

### 1.3 Images without a header
All Game Gear and export SMS titles have one; only some Japanese SMS titles do; the Japanese BIOS does not check [CART.01]. Korean titles, Codemasters titles and every SG-1000 image lack it (the SG-1000 list has no `TMR SEGA` entries; MAME marks SG-1000 and Korean carts by `slot` feature instead) [CART.11][CART.13][CART.10]. **512-byte copier header:** MAME skips 512 bytes when `size % 16384 == 512`, then rounds the length up to a 16 KB multiple [CART.10].

### 1.4 Probe order
**Recommendation:** (1) strip a 512-byte prefix when `size % 16 KiB == 512`; (2) SHA-1 of the body against a build-time table generated from the three CC0 softlists (slot, RAM size, battery, `pin_42` SMS-mode, peripheral); (3) `TMR SEGA` at $7FF0, then $3FF0, then $1FF0: region nibble, size nibble, checksum pass/fail recorded as lint only; (4) size rules: ≤ 48 KB and no mapper writes → no mapper, 8 KB on GG → X-Terminator (not supported, `UNSUPPORTED_DEVICE`); (5) MAME's opcode scan over the first 32 KB counting `LD (nn),A` targets ($FFFF vs $0002–$0004 Zemina/MSX, $8000 Codemasters, $A000 Korean, $3FFE 4PAK, all of $4000/$6000/$8000/$A000 Janggun; Nemesis when 128 KB with $00 $00 $00 at 0 and `DI; IM 1` at $1E000) [CART.10]; (6) Sega mapper. A hash match overrides steps 3–6 for board, RAM and battery, never the host's controller choice. **Strongest alternative:** header first, as the BIOS does. **Would change if:** the hash table misclassifies a dump the owner holds; then the opcode scan outranks the table for that CRC.

## 2. Mappers
### 2.1 Sega mapper (315-5208, 315-5235, 315-5365 and ROM-integrated revisions) [CART.02][CART.10][CART.20]
| Register | Function |
|---|---|
| $FFFC b7 | "ROM write" enable (development boards with RAM in place of ROM); no software uses it |
| $FFFC b4 | on-cart RAM over $C000–$FFFF; no known software uses it (needs port $3E to disable console RAM) |
| $FFFC b3 | on-cart RAM in slot 2 ($8000–$BFFF), overriding $FFFF |
| $FFFC b2 | RAM bank: 0 = first 16 KB, 1 = second 16 KB (slot 2 only). 8 KB carts mirror the same 8 KB |
| $FFFC b1–0 | bank shift added to every later $FFFD–$FFFF write: 00 → +0, 01 → +$18, 10 → +$10, 11 → +$08 (one 512 KB chip; no software uses it) |
| $FFFD/E/F | 16 KB ROM bank for slot 0/1/2. Slot 0 keeps its first 1 KB fixed (interrupt vectors) |
| Reset | 315-5235 powers up as $FFFC=0, $FFFD=0, $FFFE=1, $FFFF=2; 315-5208 with 831001/832011 ROMs is undefined |

The registers are write-only; the value is readable only from the RAM mirror at $DFFC–$DFFF [CART.02]. Bank width varies by revision (3, 5 or 6 bits; 1 MB is the largest known) [CART.02]. MAME uses `data % page_count`; ares applies the shift as a 2-bit upper field (`shift << 17`) and models b7 [CART.10][CART.20]. **No-mapper ROMs:** 8/16/32/48 KB images simply decode linearly; the BIOS assumes the Sega mapper above 48 KB [CART.02].

**Paging chips (S8-Dev document by Eric Quinn, Zoop, Mike G. and Charles MacDonald) [CART.08]:**
| Chip | Pages | Notes | Seen in |
|---|---|---|---|
| 315-5208 | 8 × 16 KB (1 Mbit) | pairs with the 28-pin 831000 ROM; registers undefined at reset [CART.02] | Action Fighter, Alex Kidd in Miracle World, Astro Warrior, Gangster Town, Ghostbusters, The Ninja, World Grand Prix |
| 315-5235 | 32 × 16 KB | battery RAM logic; some boards split two ROMs on CE1/CE2; defined reset state | After Burner, Golden Axe, Monopoly, Penguin Land, Phantasy Star, R-Type, Space Gun, Spellcaster |
| 315-5365 | 32 × 16 KB | pages slots 0 ($400–$3FFF), 1 and 2; SRAM support unknown | Bubble Bobble, Casino Games, Chase HQ, Operation Wolf, Wonder Boy III |

Emulating the full register set everywhere is safe: the revisions only remove features [CART.02].

### 2.2 Every other mapper [CART.02][CART.10][CART.20]
| Mapper (MAME slot) | Registers and semantics | Detection | Titles |
|---|---|---|---|
| Codemasters (`codemasters`) | $0000 → slot 0, $4000 → slot 1, $8000 → slot 2; whole-slot mirrors in the wiki, exact addresses in MAME. First 1 KB is **not** fixed. $4000 with b7 set maps on-cart RAM over $A000–$BFFF (MAME: RAM page = low 3 bits, 8 KB). Power-on 0, 1, 0 (ares "verified") | Codemasters header (1.2) or `$8000` write count | SMS 5 (Cosmic Spacehead, Dinobasher, Excellent Dizzy, Fantastic Dizzy, Micro Machines); GG 17; Ernie Els Golf has 8 KB RAM (softlist; MAME comment says 64 KB when loaded raw) |
| Korean $A000 (`korean`) | slots 0/1 fixed to banks 0/1; a write to $A000 selects slot 2 | `$A000` count | Dodgeball King, Hong Kil Dong, Jang Pung II/3, Sangokushi 3, Super Boy I, Dallyeora Pigu-Wang (432 KB) |
| Korean no-bank (`korean_nb`) | 32/48 KB, no registers (Bubble Bobble YM Soft, C_So!, FA Tetris, Flashpoint, Xyzolog) | softlist only | 5 |
| MSX/Zemina (`zemina`) | 8 KB banks: $0000 → $8000, $0001 → $A000, $0002 → $4000, $0003 → $6000; first 16 KB fixed | `$0002–$0004` count > `$FFFF` count | Cyborg Z, F-1 Spirit, Knightmare II, Nemesis 2, Penguin Adventure, Street Master, Super Boy 3, Wonsiin |
| Nemesis (`nemesis`) | as MSX, but $0000–$1FFF = last 8 KB page, $2000–$3FFF = page 1 | 128 KB + signature | Nemesis |
| Janggun (`janggun`) | $4000/$6000/$8000/$A000 select 8 KB banks; $FFFE/$FFFF select 16 KB pairs; a set high bit reverses the bits of each byte read (sprite flipping for free). Wiki says bit 6, MAME tests ≥ $80 and masks 6 bits: settle at phase | all four register targets present | Janggun-ui Adeul (512 KB) |
| Hicom (`hicom`) | $FFFF selects a 32 KB bank at $0000–$7FFF; $8000–$BFFF mirrors the first 16 KB | softlist | 9 "Best Game Collection" multicarts |
| 4PAK All Action (`4pak`) | $3FFE → slot 0, $7FFF → slot 1, $BFFF → slot 2; slot 2 = (reg0 & $30) + reg2 | `$3FFE` count | 1 (1 MB, Australia) |
| 188-in-1 (`korean_188in1`) | any write with (addr & $6000) == $2000 sets an 8 KB-granular base (value ^ $1F) for $4000–$BFFF; $0000–$3FFF fixed; menu entries 6 and 13 fail in both MAME and ares | softlist | 1 (1 MB) |
| Seo Jin 11-in-1 (`seojin`) | Sega mapper plus game select at $FFF0 (written through RAM); 32 KB per game; data read XOR $80 | softlist | 1 (512 KB) |
| 2 Hap in 1 (ares `Hap2000`) | $2000 selects a 32 KB bank; $0000–$3FFF fixed | ares only | David-2 + Moai (not in MAME list) |
| 93C46 EEPROM (`eeprom`) | Sega mapper; $FFFC b3 opens a window at $8000, b7 resets the chip. Read $8000: b0 = DO, b1 = 1, rest = last write. Write $8000: b0 DI, b1 CLK, b2 CS. 64 × 16-bit = 128 bytes | softlist | GG 7: Majors Pro Baseball, World Series Baseball v0/v1, WSB '95, Pro Yakyuu GG League, GG League '94, Nomo's WSB |
| Terebi Oekaki (`terebi`) | SG-1000. $6000 W: b0 axis (0 = X, 1 = Y); $8000 R: b0 pen (0 = touching), b7 busy; $A000 R: 8-bit position, X 0–255, Y 0–191. MAME clamps X to 4–251 and adds $20 to Y | string `annakmn` at $13B3 | 1 |
| The Castle (`castle`) | SG-1000, 8 KB RAM at $8000–$9FFF | `ASCII 1986` at $1CC3 | 1 (Taiwanese version is Dahjee B) |
| Othello (`othello`) | SG-1000, 2 KB RAM at $8000–$9FFF | softlist | 2 |
| BASIC Level 3 (`level3`) | 32 KB RAM at $8000–$FFFF | string at $6A20 | 2 |
| Dahjee A / B (`dahjee_typea/b`) | Taiwanese RAM pass-through: A = 8 KB at $2000–$3FFF plus 1 KB at $C000; B = 8 KB over $C000–$FFFF | $2000–$3FFF all $FF and > 10 stores to $2000/$3000 (A) or $D000–$F000 (B) | 11 + 5 |
| SC-3000 Survivors Multicart / Megacart | 2 MB / 4 MB images, 32 KB block via an I/O write, 16 KB RAM at $8000 | exact size | later seed |
| X-Terminator, Master Gear, Card Catcher | cheat device / converter / card adapter | size 8 KB (X-T) | out of scope |

### 2.3 Library share (tested: the three CC0 softlists at MAME pin `1d7a454ed79e`, licence header `license:CC0-1.0` confirmed in each file [CART.11][CART.12][CART.13])
Count command, run from a MAME checkout at that pin: `for f in hash/sms.xml hash/gamegear.xml hash/sg1000.xml; do echo "$f $(grep -c '<software ' $f)"; grep -oE '<feature name="slot" value="[^"]*"' $f | sort | uniq -c | sort -rn; grep -c '<feature name="batt' $f; done`. Entries without a `slot` feature use MAME's default (Sega mapper or none).

| List (entries) | Sega / none | Codemasters | Korean family | EEPROM | SG-1000 boards | Other | Battery |
|---|---|---|---|---|---|---|---|
| sms (653) | 615 (94.2%) | 5 (0.8%) | 33 (5.1%): hicom 9, zemina 8, korean 6, korean_nb 5, 4pak, 188in1, janggun, nemesis, seojin | 0 | 0 | 0 | 27 |
| gamegear (823) | 795 (96.6%) | 17 (2.1%) | 0 | 7 (0.9%) | 0 | xterminator 2, mgear 2 | 37 `battery="yes"` (+3 marked `batt` blank or `??`; recounted 2026-10-18) |
| sg1000 (214) | 191 (89.3%) | 0 | 0 | 0 | 23 (10.7%): dahjee A 11, B 5, othello 2, level3 2, castle, terebi | cardcatcher 1 | 0 |

Sizes: sms 32 KB 95, 48 KB 6, 64 KB 7, 128 KB 180, 256 KB 264, 512 KB 92, 1 MB 7 (plus one 432 KB and one 800 KB); gamegear 256 KB 290, 512 KB 362, 1 MB 10, and odd homebrew sizes 368/502/510/511 KB; sg1000 8–48 KB only. So v1.0's three boards cover 620/653 SMS (94.9%) and 812/823 GG (98.7%) entries; SMS+GG together 1432/1476 (97.0%).

### 2.4 Bank masks and odd sizes
Power-of-two ROMs mirror, so over-range bank numbers are harmless [CART.02]. MAME reduces `value % page_count`; ares masks to the chip's bit width; neither pads [CART.10][CART.20]. Non-power-of-two images exist (48 KB, 432 KB, 800 KB, GG homebrew). **Recommendation:** round the image up to a 16 KB multiple with $FF, then `bank % pages` (equal to a mask when pages is a power of two). Slot 0's first 1 KB always reads ROM offset 0–$3FF. **Strongest alternative:** pad to the next power of two and mask (simpler, matches hardware mirroring for real chips). **Would change if:** a 432 KB or 800 KB title shows a mirror dependence under `%`.

## 3. Controllers and peripherals
### 3.1 Port bit map (0 = pressed or driven low) [CART.10][CART.14]
| Port | b7 | b6 | b5 | b4 | b3 | b2 | b1 | b0 |
|---|---|---|---|---|---|---|---|---|
| $DC read | P2 Down | P2 Up | P1 TR (button 2) | P1 TL (button 1) | P1 Right | P1 Left | P1 Down | P1 Up |
| $DD read | P2 TH | P1 TH | 1 | Reset button (SMS1, 0 = pressed) | P2 TR | P2 TL | P2 Right | P2 Left |
| $3F write | P2 TH level | P2 TR level | P1 TH level | P1 TR level | P2 TH dir | P2 TR dir | P1 TH dir | P1 TR dir |

Pins: 1 Up, 2 Down, 3 Left, 4 Right, 5 +5 V, 6 TL, 7 TH, 8 GND, 9 TR [CART.14]. $3F direction bits: 0 = output, 1 = input (pull-up) [CART.10]. When TH or TR is an output, an export console reads back the driven level in $DC/$DD; a Japanese console returns 0 there, which is how games detect the region (`m_ioctrl_region_is_japan`) [CART.10]. Port $3E bit 2 ("I/O chip disable") makes $DC/$DD read $FF on SMS; not on GG [CART.10]. The Korean SMS2 decodes I/O addresses more strictly [CART.10].

**Recommendation:** model the two ports at pin level: each device gets the port's output levels and directions from $3F and returns 7 pin levels; the console mixes in $3E, the region rule and the TH latch. **Strongest alternative:** per-device register shortcuts (MAME's `in_r`/`out_w` are already pin-shaped, so the cost is the same). **Would change if:** never; pin-level is also what the Sports Pad and MD pad need.

### 3.2 Pause, Reset, Game Gear Start
Pause is wired through the I/O controller to the Z80 NMI, so a press jumps to $0066 unconditionally; games set a RAM flag and `retn` [CART.06]. There is no readable pause bit (MAME `gg_pause_callback` asserts `INPUT_LINE_NMI`) [CART.10]. Reset on an SMS1 is readable at $DD b4 [CART.10]. The stock Control Pad (3020) and Control Stick (3060) connect Up/Down/Left/Right/TL/TR only; TH is unconnected and button 1 is the "START" button on the console's label [CART.07]. Game Gear: port $00 b7 = Start (0 = pressed), b6 = NJAP (0 domestic, 1 overseas), b5 = NNTS (0 NTSC); in SMS-mode carts Start acts as Pause/NMI [CART.10]. GG ports $DC/$DD mirror at $C0/$C1 and return $70 | pad bits for $DD [CART.10].

**Game Gear SMS-mode carts:** 52 gamegear.xml entries carry `pin_42 = sms_mode` (tested: `grep -c 'pin_42" value="sms_mode"' hash/gamegear.xml`) (Taiwanese reissues, Codemasters, Castle of Illusion, Prince of Persia, Olympic Gold, Out Run Europa, Super Kick Off, WWF Steel Cage, R.C. Grand Prix, Chase H.Q. and others); cartridge pin 42 puts the GG into SMS video and I/O mode, which the core needs as a per-image flag from the hash table [CART.12][CART.10]. The Master Gear converter does the same for any SMS cart. **Recommendation:** the hash table carries the flag; without a match, a GG image with region nibble 3 or 4 is lint-flagged and run in GG mode; the host may force SMS mode.

### 3.3 Light Phaser (model 3050; works on SMS1/SMS2 and GG via adapter, not on Mark III) [CART.03][CART.15][CART.10]
Wiring: TL = trigger, TH = light sensor, both inputs; sensor low while it sees light [CART.03]. Software: on trigger, draw the next frame bright, busy-poll TH in $DD b6/b7; the VDP latches the H counter when TH is active, so the game reads V counter + latched H counter for the left edge of the sensor's circle, then averages [CART.03]. MAME's model: sensor radius 6 px, polled every 4 px along the beam, lit when the output pixel's luma (0.299 R + 0.587 G + 0.114 B) ≥ $7F; the H latch fires after a per-game X offset (Space Gun, Gangster Town 9; Shooting Gallery 19; Rescue Mission 15; Laser Ghost 13; Assault City 22; Missile Defense 3-D 21; others −1) keyed by header bytes; a sensor-on flag is kept until $DD is read or the frame ends because "Color & Switch Test" reads TH only after VINT and "Hang-On & Safari Hunt" only reads the H counter [CART.15][CART.10]. Latch edge: SMS Power says latch while TH is 0; MAME latches on the 0→1 transition and sets the flag on 1→0; settled in §8.

Titles with a MAME offset entry or a Light Phaser version in sms.xml: Assault City (Light Phaser version), Gangster Town, Laser Ghost, Missile Defense 3-D, Rescue Mission, Shooting Gallery, Space Gun; others known to the community (Marksman Shooting/Trap Shooting/Safari Hunt, Operation Wolf, Rambo III, Wanted, Bank Panic 3-in-1) are (unverified) until the title list is cross-checked against the softlist in the peripheral phase [CART.11][CART.10].

**Recommendation (v2.0):** the host supplies cursor X/Y in screen pixels; the core computes the sensor from the pixel it actually emitted inside a radius-6 window with MAME's luma threshold and the per-game offsets; the trigger is TL. **Strongest alternative:** geometry only (ares: trigger only, no sensor) [CART.20]. **Would change if:** a hardware capture of the phaser's delay appears.

### 3.4 SegaScope 3-D glasses [CART.04][CART.10]
Write to $FFF8 (mirrors $FFF8–$FFFB): b0 = 0 right shutter open, 1 left open. Readable only through the RAM mirror, so presence is undetectable. The jack is on the Japanese SMS; elsewhere the adapter sits in the SMS1 card slot and snoops the bus, so SMS2 and GG cannot use it [CART.04]. Shutter transition takes about 10 scanlines (MacDonald, via MAME) [CART.10]. Titles: Maze Hunter 3-D, Missile Defense 3-D, Out Run 3-D, Poseidon Wars 3-D, Space Harrier 3-D, Zaxxon 3-D, Blade Eagle (required for Blade Eagle, Maze Hunter, Missile Defense; the rest have 2-D modes) [CART.11][CART.16]. **Recommendation:** the core reports the eye flag per frame; the host chooses left-only, right-only, alternate or anaglyph. Frame hashes stay per rendered frame.

### 3.5 Sports Pad (US 3040; JP SP-500) [CART.17][CART.18][CART.20]
TH is an output. Sequence: TH = 0, wait 80 µs, read X high nibble on D0–3; TH = 1, wait 40 µs, X low; TH = 0, 40 µs, Y high; TH = 1, 40 µs, Y low. TL/TR are the buttons. MAME resets the phase after a timeout of 3.58 MHz / 512 ≈ 143 µs ("not verified"); ares resets after TH held low 60 µs and clamps the delta. The Japanese Sports Pad Soccer polls TL/TR instead when it does not see a Japanese console (Mark III lacks TH) [CART.18]. Titles: Great Ice Hockey, Sports Pad Football, Sports Pad Soccer [CART.11][CART.17].

### 3.6 Paddle Control (HPD-200, Japan only) [CART.05][CART.19][CART.20]
8-bit position on D0–3 in two nibbles; TR = 0 low nibble, TR = 1 high nibble, toggled by the controller's own clock (ares: 18 kHz "as measured on real hardware"); button on TL. Games detect it by sampling TR 256 times and expecting about 50% ones. On an export console the games switch to a TH-driven nibble select (Out Run 3-D), which only a Sports Pad can answer [CART.05][CART.17]. Titles: BMX Trial Alex Kidd (required), Out Run, Out Run 3-D (optional) [CART.19].

### 3.7 Mega Drive pad on an SMS port [CART.20][CART.10]
TH = 1 (default input, pulled up): D0–3 = directions, TL = B, TR = C, so a stock game sees B/C as buttons 1/2. TH driven 0: D0–1 = Up/Down, D2–3 read 0, TL = A, TR = Start. ares and MAME both model it; a 6-button pad adds the 3-pulse protocol.

### 3.8 SK-1100 keyboard (SG-1000/SC-3000/Mark III) [CART.21][CART.22]
An 8255 PPI at $DC–$DF (A, B, C, control). PC0–2 select a row 0–7; row 7 means "keyboard disabled" and the pads appear on $DC/$DD. The printer port and the 2-player link cable (F-16 Fighting Falcon) hang off PC5–PC7 and PB5–PB6 [CART.22]. Later seed (SG-1000 compatibility mode).

### 3.9 Gear-to-Gear cable (v3.0 seed) [CART.23][CART.16]
Ports $01 data (7 pins, TL/TR double as TXD/RXD), $02 direction, $03 TxDATA, $04 RxDATA, $05 serial control: b5 SIN, b4 SOUT, b3 RINT, b2 RERR, b1 RRDY, b0 TFUL, b7–6 bit rate. The port's interrupt (PC6 falling or RRDY with RINT) drives the Z80 NMI. MAME still lists the base clock and overrun behaviour as TODO. 13 titles use it (Sega Retro list, e.g. Streets of Rage 2, Outrun, G-LOC) [CART.16].

### 3.10 Lag frames and input timing
**Recommendation (derived):** input is latched when `cogmaster_run_frame` starts (VBlank boundary); a lag frame is one in which the CPU never read $DC, $DD, $C0, $C1 or (GG) $00. The runner counts them and the libretro core exposes the count for TAS tools. Mid-frame changes are not modelled in v1.0.

## 4. Saves
### 4.1 Battery RAM [CART.11][CART.12][CART.02]
All 27 SMS and 37 GG `battery="yes"` entries use the Sega mapper's slot-2 window (§2.1). Sizes: 8 KB (Phantasy Star ×6, Ys ×2, Golden Axe Warrior, Miracle Warriors, Monopoly, Penguin Land, Doki Doki Penguin Land, Golfamania, Haja no Fuuin, Ultima IV, most GG RPGs), 16 KB (Shining Force Gaiden), 24 KB (Moldorian), 32 KB (Shining Force II and Final Conflict, Flight of Pigarus homebrew). Non-battery RAM: Desert Speedtrap, The Flash (8 KB), The Terminator (32 KB), Ernie Els Golf (8 KB, Codemasters). MAME loaded raw allocates 32 KB and saves only if the game ever set $FFFC b3 [CART.10].

### 4.2 File conventions
| Emulator | File | Facts |
|---|---|---|
| RetroArch with Genesis Plus GX | `<rom>.srm` | "MS/GG/MD/Pico/SG-1000 cartridge backup save"; the core needs full paths, so no soft-patching [CART.24][CART.26] |
| RetroArch with Gearsystem | `<rom>.srm`, `.srm2` for screen 2 in link mode | "battery RAM or EEPROM save" in one file [CART.25][CART.26] |
| MAME | nvram | raw RAM of the softlist `ram` size; the 93C46 is a separate 128-byte eeprom region [CART.10][CART.27] |
| ares | `save.ram` | raw bytes of the board's RAM [CART.20] |
| Emulicious, MEKA | not documented in the pages read; MEKA's docs describe only save states and its ROM patch file [CART.28][CART.29] |

Neither libretro page states the byte count written; GPGX's and Gearsystem's `.srm` sizes for an 8 KB game are (unverified) and are measured in §8 from released binaries.

**Implemented scope:** the classified library battery query and raw import expose supported 8/16/32 KiB
lengths without changing physical mapper allocation. Moldorian retains its pinned 24 KiB declaration but
unsupported geometry; no guessed 24 KiB mask or modulo decode is installed. Unknown persistence is refused
when saving is requested, known volatile RAM stays outside SAVE_RAM, and EEPROM remains deferred. Raw short
imports pad with $FF, oversize rejects transactionally, and runner `.srm` replacement is atomic on POSIX and
Windows. Exact-size wrong-game raw files cannot be identified; continuation controls test their effect.

**Recommendation:** `RETRO_MEMORY_SAVE_RAM` is exactly the on-cart RAM (8/16/24/32 KB, raw, no header); for an EEPROM title it is the 128-byte 93C46 image stored as 64 little-endian 16-bit words; a title with both never exists in the lists. The library exports a span plus a generation counter that increments on every write while $FFFC b3 (or the EEPROM window) is enabled; the host persists when the counter changes. The runner writes `<rom>.srm` in the same bytes. **Strongest alternative:** MAME-style 32 KB always. **Would change if:** GPGX's `.srm` for an 8 KB game turns out to be 32 KB and users ask to swap files; then pad on import, never on export.

## 5. ROM hacks and patches
- **IPS:** `PATCH`, records {u24 BE offset, u16 BE size, data}, size 0 = RLE {u16 count, u8 value}, `EOF` [CART.30]. **BPS:** `BPS1`, varint sizes, four actions, three CRC32s [CART.31]. Both are host concerns: RetroArch soft-patches `rom.ips/.bps/.ups` (and `.ips1`…) when the core sets `need_fullpath = false` [CART.26].
- **Identity:** rcheevos hashes the whole file for SMS, GG and SG-1000 (`rc_hash_whole_file`), so a 512-byte copier header changes the RetroAchievements identity; the probe reports both the file hash and the body hash [CART.32].
- **Tools and what they want:** Emulicious (closed, Java) ships a debugger with source-level view, watchpoints, profiler, tracer, RAM watch and remote debugging from VS Code [CART.28]; WLA-DX is the assembler and emits symbol files Emulicious reads [CART.33]; devkitSMS (SDCC, C) [CART.34]; BMP2Tile (MIT) converts images to tile data [CART.35]; PSGTalk (furrtek) makes SN76489 register streams from speech [CART.36].
- **Recommendation:** no patching in the core; `cogmaster-run --patch` applies IPS/BPS through a fuzzed `patch.c`. The library offers: bus read/write at any address, address → ROM offset through the current banks, a watch callback on read/write/execute ranges, a label file loader (WLA-DX `.sym` "bank:addr name" format, (unverified) layout, settled in §8) and deterministic rerun from a state plus an input log. **Strongest alternative:** a Lua or GDB stub (host work, later seed). **Would change if:** Playstead wants the stub in the library.

## 6. Hostile-input bounds
Fuzz targets: `cogmaster_probe`, each mapper with random bus traffic, the 93C46 bit-banger, the state loader, the save loader, IPS/BPS, the movie loader.

| Parser | Bounds | Invariants |
|---|---|---|
| ROM image | 1 KB ≤ size ≤ 1 MiB + 512 in v1.0 (largest known cart is 1 MB; 4 MB SC-3000 Megacart is a later seed); else `UNSUPPORTED_SIZE` | strip 512 only when `size % 16 KiB == 512`; pad to a 16 KB multiple; every bank index reduced `% pages`; RAM index `& (size − 1)` |
| Header | header read only when size ≥ $8000 (or $4000/$2000 for the lower slots) | region nibble outside 3–7 → unknown, lint; size nibble not in table → checksum skipped |
| Hash table | built at build time from the CC0 XML, sorted, validated by a test | lookup is a binary search on 20 bytes; no parsing at run time |
| Mapper writes | registers masked to the ROM's page count; Codemasters RAM page `& 7` then `% size` | a write never indexes past ROM or RAM |
| 93C46 | 128 bytes exactly; opcode state machine bounded to 6 address bits | unknown opcode → ignore |
| Save file | exactly the RAM size (or 128 for EEPROM); shorter → pad $FF; longer → reject | contents are copied, never parsed |
| State file | versioned header, fixed-size fields, size checked before any copy | bank numbers re-masked on load; RAM sizes must match the loaded board |
| IPS | target ≤ cap; every offset + size ≤ cap; RLE count ≥ 1; `EOF` required | offset $454F46 ambiguity: treat as `EOF` only at end of file |
| BPS | varints < 2⁶⁴ with overflow check; source size and CRC must match | actions stay inside both buffers; output fully written; patch CRC checked |
| Movie | header + fixed-size input records; count ≤ 2³¹ | ROM SHA-256 pinned (full file and body) |

## 7. Milestone placement
**Recommendation:** **v1.0** = no-mapper, Sega mapper (RAM, bank shift, b7 modelled), Codemasters (with Ernie Els RAM); two pads, Pause/NMI, GG Start and port $00; battery RAM export. Covers 94.9% of SMS and 98.7% of GG entries (§2.3). **v2.0** = Korean family (5.1% of SMS), 93C46 EEPROM (0.9% of GG), Light Phaser, 3-D glasses, Sports Pad, Paddle, MD pad, SG-1000 boards (Castle, Othello, Terebi Oekaki, Dahjee, BASIC L3) with the SG-1000 profile. **v3.0** = Gear-to-Gear cable, SK-1100 keyboard and link, SC-3000 multicarts. **Strongest alternative:** pull Korean $A000 and MSX into v1.0 (about 60 lines, 14 titles). **Would change if:** phase slack appears, or the owner's must-play list names a Korean title.

## 8. Open questions settled by phases
| Question | Settled by |
|---|---|
| H-counter latch edge: TH active-low (SMS Power) vs 0→1 edge (MAME) | VDP phase: test with the "Color & Switch Test" ROM and Hang-On & Safari Hunt under both rules |
| Janggun reverse-bit flag: bit 6 or bit 7 of the bank value | Korean mapper phase: trace the game's writes |
| Sports Pad phase timeout (143 µs MAME vs 60 µs ares) | Peripheral phase: both games must track the ball |
| `.srm` size GPGX and Gearsystem write for an 8 KB game and for a 93C46 title; EEPROM word order | Save phase: run the released libretro binaries once, record the sizes and bytes |
| WLA-DX `.sym` layout for the label loader | Debug-API phase: read the WLA-DX manual at its pin |
| Whether the GG serial bit-rate field maps to 4800/2400/1200/300 | Link-cable phase (v3.0): MAME lists it as TODO; measure from a title |
| SMS Power pages not fetched today (PortDC/DD, HCounter, GearToGear, TerebiOekaki, SK-1100 returned 503 under rate limiting; Port3F, Checksum, SportsPad are not the page names) | Next revision of this file: fetch via the wiki index and cite, then re-tier the T3 claims they confirm |
| Light Phaser title list beyond the seven MAME keys | Peripheral phase: cross-check the softlist descriptions and SMS Power's game tags |

## Sources
| ID | Tier | Source | Pin or access date | Supports |
|---|---|---|---|---|
| CART.01 | T2 | smspower.org/Development/ROMHeader | accessed 2026-10-18 | header fields, region and size nibbles, which images have one |
| CART.02 | T2 | smspower.org/Development/Mappers | accessed 2026-10-18 | Sega, Codemasters, Korean, MSX/Nemesis, Janggun mappers |
| CART.03 | T2 | smspower.org/Development/LightPhaser | accessed 2026-10-18 | wiring, TH latch, polling method |
| CART.04 | T2 | smspower.org/Development/3DGlasses | accessed 2026-10-18 | $FFF8 protocol, card-slot adapter |
| CART.05 | T2 | smspower.org/Development/Paddle | accessed 2026-10-18 | nibble protocol, detection, export mode |
| CART.06 | T2 | smspower.org/Development/PauseButton | accessed 2026-10-18 | NMI, $0066 handler |
| CART.07 | T2 | smspower.org/Development/Joypad | accessed 2026-10-18 | pad wiring, TH unconnected |
| CART.08 | T2 | smspower.org/Development/SMSPagingChips (Eric Quinn, rev 0.03.00) | accessed 2026-10-18 | 315-5208/5235/5365 capabilities and sightings |
| CART.10 | T3 | github.com/mamedev/mame src/devices/bus/sega8/{sega8_slot,rom}.cpp, src/mame/sega/{sms_m,sms}.cpp (BSD-3 headers checked) | 1d7a454ed79e | detection scan, checksum, every mapper, ports, Pause, GG port $00, light-phaser offsets, 3-D timing |
| CART.11 | T3 | mame hash/sms.xml (CC0-1.0 header checked) | 1d7a454ed79e | SMS counts, battery titles, 3-D titles |
| CART.12 | T3 | mame hash/gamegear.xml (CC0-1.0) | 1d7a454ed79e | GG counts, EEPROM titles, pin_42 SMS-mode entries |
| CART.13 | T3 | mame hash/sg1000.xml (CC0-1.0) | 1d7a454ed79e | SG-1000 counts and boards |
| CART.14 | T3 | mame src/devices/bus/sms_ctrl/smsctrl.h (BSD-3) | 1d7a454ed79e | pin numbering |
| CART.15 | T3 | mame src/devices/bus/sms_ctrl/lphaser.cpp (BSD-3) | 1d7a454ed79e | sensor radius, luma threshold, flag rules |
| CART.16 | T2 | segaretro.org 3-D_Glasses, Gear-to-Gear_Cable (CC BY 4.0) | accessed 2026-10-18 | required-glasses titles, link-cable titles |
| CART.17 | T3 | mame src/devices/bus/sms_ctrl/sports.cpp (BSD-3) | 1d7a454ed79e | TH sequence and timeout, paddle export mode |
| CART.18 | T3 | mame src/devices/bus/sms_ctrl/sportsjp.cpp (BSD-3) | 1d7a454ed79e | Mark III TL/TR mode |
| CART.19 | T2 | segaretro.org Paddle_Control, Sports_Pad | accessed 2026-10-18 | model codes, supported titles |
| CART.20 | T3 | github.com/ares-emulator/ares ares/ms/cartridge/board/*.cpp, ares/ms/controller/* (ISC, LICENSE checked) | 9408cb43d494 | bank shift model, power-on banks, Hap2000, 188-in-1, paddle 18 kHz, sports pad 60 µs, MD pad, light phaser |
| CART.21 | T3 | mame src/devices/bus/sg1000_exp/sk1100.cpp (BSD-3) | 1d7a454ed79e | PPI rows, row 7 disables keyboard |
| CART.22 | T3 | mame src/devices/bus/sg1000_exp/kblink.cpp (BSD-3) | 1d7a454ed79e | link cable pins |
| CART.23 | T3 | mame src/devices/machine/sega_md_ioport.{cpp,h} (BSD-3) | 1d7a454ed79e | GG ports $01–$05, S_CTRL bits, NMI |
| CART.24 | T1 | docs.libretro.com/library/genesis_plus_gx/ | accessed 2026-10-18 | `.srm`, light gun, paddle, sports pad options |
| CART.25 | T1 | docs.libretro.com/library/gearsystem/ | accessed 2026-10-18 | `.srm`/`.srm2`, mapper list |
| CART.26 | T1 | docs.libretro.com/guides/softpatching/; libretro-super `*_libretro.info` for GPGX (`needs_fullpath = true`) and Gearsystem (`false`) | accessed 2026-10-18 | soft-patching rule |
| CART.27 | T3 | mame src/devices/machine/eepromser.cpp (BSD-3) | 1d7a454ed79e | 93C46 = 64 × 16 bits |
| CART.28 | T1 | emulicious.net; smspower.org/Development/Emulicious | accessed 2026-10-18 | debugger features, WLA-DX output |
| CART.29 | T1 | github.com/ocornut/meka meka/meka.txt and tech.txt (docs only) | accessed 2026-10-18 | patch file, no save-format statement |
| CART.30 | T2 | zerosoft.zophar.net/ips.php | accessed 2026-10-18 | IPS format |
| CART.31 | T2 | romhacking.net/documents/746 (BPS; page shell only, body not rendered) | accessed 2026-10-18 | BPS existence; layout from the muhtari pack's reading (unverified here) |
| CART.32 | T1 | github.com/RetroAchievements/rcheevos src/hash/rc_hash.c, rc_hash_rom.c (MIT) | dfdc64ea035c | whole-file hash for SMS/GG/SG-1000 |
| CART.33 | T1 | github.com/vhelin/wla-dx (repo metadata) | accessed 2026-10-18 | assembler |
| CART.34 | T1 | github.com/sverx/devkitSMS (repo metadata) | accessed 2026-10-18 | C toolchain |
| CART.35 | T1 | github.com/maxim-zhao/bmp2tile (MIT) | accessed 2026-10-18 | tile converter |
| CART.36 | T1 | github.com/furrtek/PSGTalk | accessed 2026-10-18 | PSG speech tool |
