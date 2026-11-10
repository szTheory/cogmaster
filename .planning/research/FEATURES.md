# Feature Research

**Domain:** Sega Master System / Game Gear emulator core, milestone v2.0 "The whole library" (additions to the shipped v1.0 core)
**Researched:** 2026-11-09
**Confidence:** HIGH for library counts, titles and board names (parsed today from the CC0 MAME lists at pin `1d7a454ed79e`); MEDIUM for peripheral behaviour (MAME BSD-3 and SMS Power facts already in the preparation files); LOW for other emulators' `.srm` byte layouts (docs state names, not sizes; the owning sources are GPL or non-commercial and were not opened).

Scope: only what v2.0 adds. Z80, Sega/Codemasters/no-mapper boards, VDP mode 4, two pads, battery RAM, PSG + YM2413, runner, libretro core and the four-fixture ledger are built and are treated here only as dependencies.

## What the CC0 lists say (counted today, `gh`/raw fetch at the pin)

| Fact | Count | Why it matters |
|---|---|---|
| SMS entries / GG entries | 653 / 823 (235 and 411 are clones, so 418 and 412 parent dumps) | The ledger has 1,476 rows to seed; clone rows are separate dumps and stay separate rows |
| Non-Sega, non-Codemasters SMS boards | 33 (hicom 9, zemina 8, korean 6, korean_nb 5, and one each of 4pak, korean_188in1, janggun, nemesis, seojin) | The whole mapper phase; 5.1 % of SMS |
| GG `eeprom` slot | 7 entries (4 base games) | The whole EEPROM feature; 0.9 % of GG |
| Battery entries | 27 SMS + 37 GG (v1.0 covers 8/16/32 KiB; Moldorian 24 KiB stays refused) | Already built; EEPROM is the missing save kind |
| `supported="no"` / `"partial"` in MAME | SMS: Action Replay, Excellent Dizzy Collection (prototype), 188-in-1, Sonic Chaos prototype. GG: Pro Action Replay, Wonder Kid prototype, Micro Machines x3 (partial) | Ledger rows that start as `unsupported` or `skipped` with a reason, not `fail` |
| `ctrl1_default`/`ctrl2_default` shared features (new finding) | lphaser 13 (+1 on port 2), paddle 4 (+1), sports 3 (both ports), graphic board 1 | The CC0 list already carries a per-dump controller default; the hash table can set the auto port device with no code-side title list |
| XML `Notes:` comments (new finding) | FM support 107 entries (58 parents); 3-D glasses 13 entries; "optional SK-1100" about 90; "extra-height video mode absent on the 315-5124" 4; "Ys relies on the 315-5124 tilemap mirroring bug" 2 | Gives the FM, 3-D and VDP-revision rows for the profile ledger without hand curation |
| "(Korea" in title | 74 SMS entries; "Brazil" 222; Europe-only-ish about 93 | Sizes the Korean and PAL/Brazil profile work |

## Feature Landscape

### Table Stakes (Users Expect These)

| Feature | Why Expected | Complexity | Notes |
|---|---|---|---|
| Korean boards detected and played with no setting: `zemina` (MSX-style, 8 titles: Cyborg Z, F-1 Spirit, Knightmare II, Nemesis 2, Penguin Adventure, Street Master, Super Boy 3, Wonsiin), `korean` $A000 (6: Hong Kil Dong, Jang Pung II and 3, Sangokushi 3, Super Boy I, Dallyeora Pigu-Wang 432 KB), `korean_nb` (5: Bubble Bobble YM Soft, C_So!, FA Tetris, Flashpoint, Xyzolog), `nemesis`, `4pak`, `hicom` multicarts (9) | Collectors with Korean carts expect them to boot; emulators that loop to a menu or hang (4 Pak looping, Nemesis 2 and Operation Wolf open issues on MiSTer) are named in user complaints (C.09) | LOW each (about 60 lines for korean + zemina; registers are 1 to 3 writes) | Registers in `SMS-CARTRIDGES-AND-PERIPHERALS.md` 2.2. Needs a new `board` enum value per mapper and the hash table's `slot` carried into `cartdb.inc` |
| Hash-database detection first, opcode scan as fallback | Korean dumps have no header; No-Intro lists about 1,057 SMS dumps against MAME's 653, so about 400 dumps the owner may hold are not in the CC0 list and only the scan can classify them | MEDIUM | `--probe` already prints `mapper_guess`; the scan counts `LD (nn),A` targets ($FFFF vs $0002-$0004, $A000, $3FFE, all four of $4000/$6000/$8000/$A000). A hash match overrides the scan for board, RAM and battery, never the controller choice |
| Janggun (`janggun`, 512 KB) and 188-in-1 | Named in the milestone brief and in user complaints; both have quirks | MEDIUM (Janggun), LOW (188-in-1) | Janggun reverse-bit flag: SMS Power says bit 6, MAME tests >= $80 and masks 6 bits; settle by tracing the game's writes. 188-in-1 is `supported="no"` in MAME and menu entries 6 and 13 fail in MAME and ares: implement the documented rule, ledger row records the known failing entries, never a hack |
| Seo Jin 11-in-1 (`seojin`, "Super Game 11-in-1") | In the list, one entry | LOW | Sega mapper plus a game-select register at $FFF0 and a data XOR $80; fold into the mapper phase only if it costs under about 30 lines, else a named `unsupported` row |
| 93C46 EEPROM for the four GG baseball games (The Majors Pro Baseball, World Series Baseball v0/v1, World Series Baseball '95, Pro Yakyuu GG League and '94, Nomo's WSB) | Their saves (team data, pennant mode) are the point of the cartridges; one competing FPGA core listed "none of the four boot" as an open issue until 2026 | MEDIUM | Sega mapper plus a window at $8000 opened by $FFFC bit 3, bit 7 resets the chip. Read: b0 = DO, b1 = 1, upper bits = last write. Write: b0 DI, b1 CLK, b2 CS. Bit-bang state machine over 64 x 16 bits; unwritten chip reads all $FF. Fuzz target already planned (CART 6) |
| EEPROM persists through the existing save path | Players expect the same behaviour as battery carts: automatic, no menu | MEDIUM | `RETRO_MEMORY_SAVE_RAM` = 128 bytes, 64 little-endian words (MAME stores 16-bit EEPROM words little-endian and starts erased to all ones: verified in BSD-3 `eeprom.cpp` at the pin). Extends `cogmaster_get_battery` / `cogmaster_import_battery`, a new persistence class in `cartdb`, the generation counter, runner `--save-dir` atomic replace and exit 4 on size mismatch. State format needs the chip's shift state |
| `.srm` that other emulators can open for the same game | Players move saves between RetroArch cores and standalone emulators and keep backups; "renaming a file should just work" | LOW if layout matches, MEDIUM if conversion is needed | Raw bytes, no header, no identity binding, exact size per board (8/16/32 KiB, 128 B). See the interop table below; measure released binaries once before fixing import rules |
| Atomic runner saves for EEPROM too | Shipped for battery RAM in v1.0 (POSIX rename, Windows MoveFileExW); an EEPROM that corrupts on a killed process breaks trust | LOW | Reuse; add a hosted/fresh-process continuation case for an EEPROM fixture |
| Light Phaser at pin level for the 13 flagged dumps (3D Gunner prototype, Assault City LP version, Gangster Town (both ports), Marksman Shooting & Trap Shooting (two dumps), Missile Defense 3-D (two), Operation Wolf, Rambo III, Rescue Mission, Shooting Gallery, Space Gun, Wanted) plus Laser Ghost and Hang-On & Safari Hunt (optional, not flagged in the list) | Gun games are a visible share of the library and the most requested peripheral; libretro "mouse as gun" has been an open request since 2015 in a leading core | HIGH | TL = trigger, TH = sensor (low while it sees light); latch the H counter on TH; the host supplies X/Y, the core samples the pixel it actually emitted. Not available on Mark III or GG profiles. Depends on the v1.0 VDP latch tick and per-line model (SEED-001 note) |
| Sports Pad (Great Ice Hockey, Sports Pad Football, Sports Pad Soccer; World Soccer optional) | Three games need it; two-port in the list | MEDIUM | TH output, four-phase nibble read, TL/TR buttons. Timeout 143 us (MAME, unverified) vs 60 us (ares): decide by whether both games track the ball. Japanese Sports Pad Soccer polls TL/TR when it does not see a Japanese console |
| Paddle Control (Alex Kidd BMX Trial, Galactic Protector, Megumi Rescue, Woody Pop required; Out Run, Out Run 3-D, Super Racing optional) | Japan-only controller, four required titles, three optional | MEDIUM | Two-nibble 8-bit position selected by TR, toggled by the paddle's own clock (ares 18 kHz); button on TL; games detect it by sampling TR 256 times. Only on Japan-region profiles; export consoles switch games to a TH nibble select that only a Sports Pad answers |
| 3-D glasses state exposed per frame (Blade Eagle, Maze Hunter 3-D, Maze Walker, Missile Defense 3-D, Out Run 3-D, Poseidon Wars 3-D, Space Harrier 3-D, Zaxxon 3-D, Line of Fire) | Games flash the shutter at $FFF8; without eye state the picture shakes (Zaxxon "not a bug" complaint, C.08) | LOW in core, MEDIUM in host | Write-only register, readable only via the RAM mirror; shutter transition about 10 lines. The core reports left/right per frame; host chooses both, left, right, anaglyph. Not on SMS2 or GG profiles |
| Controller type chosen automatically per game, overridable | "Laser Ghost needs a pad in port 2", mis-set ports are a recurring complaint | LOW | Hash table `ctrl1_default`/`ctrl2_default` set the port devices; user option wins. A hash hint selects a device, never behaviour |
| Per-profile device availability | A gun on a Mark III or GG, glasses on an SMS2, a paddle on an export console do not exist in hardware | LOW | Reject at profile load with `PROFILE_REJECTED`; document the matrix |
| BIOS boot with the user's own dump, off by default | Every popular core treats BIOS as optional (GPGX "System Boot ROM" default Off; Gearsystem optional; neither requires one) | MEDIUM | `--boot bios --bios FILE` in the runner, firmware rows in `.info` (`bios_E.sms`, `bios_U.sms`, `bios_J.sms`, `bios.gg`, `bios.sms`), recognise by digest only. The 8 KB export and Japanese BIOS and the 1 KB GG BIOS are the baseline; BIOS overlays $0000-$1FFF until $3E disables it. Boot must end in the C7 hand-off state the fast boot already reproduces |
| BIOS behaviours collectors notice | Export BIOS shows "SOFTWARE ERROR"/lock-up for a Japanese or bad-checksum cart; Snail Maze; GG BIOS lock-up without `TMR SEGA` | LOW once the BIOS runs | These are the BIOS's own code; the core only provides memory map, `$3E`, region nibble reading and timing. Test with a self-built stand-in BIOS in CI (no Sega bytes, rule 3) and the owner's dump in the local `library` run |
| PAL profile that is truthful over real games | Complaints: "forced at 60 fps", Lucky Dime Caper and Dizzy too fast, PAL list wrong for Brazil | MEDIUM | v1.0 already has the 313-line 49.701459 fps profile; v2.0 puts every Europe/Australia row under it with 224/240-line modes, PAL PSG and FM sample rates (about 965.77 samples per frame), and the Brazil-NTSC rule documented |
| Japanese FM profile played over real FM titles | 107 entries (58 parents) carry the FM note; FM is the reason Japanese collectors care | MEDIUM | YM2413 exists. v2.0 adds the machine profiles Mark III + FM unit and SMS-J built-in, auto-detect through $F2, and ledger rows whose predicate includes an audio hash (the ledger text says audio "awaits phase 4", which has now shipped) |
| VDP-revision rows for the games that need one | "Extra-height mode absent on the 315-5124": Cosmic Spacehead, Fantastic Dizzy, Micro Machines, Excellent Dizzy Collection need SMS2; Ys needs the SMS1 tilemap mirroring bug | LOW (data) | Ledger `machine` column already exists; seed rows with `sms1`/`sms2` from the list notes instead of guessing |
| Compatibility ledger over all 1,476 CC0 rows | The stated exit gate for `is_experimental`; users ask "does my cartridge work" and a generated page answers | MEDIUM | Seed every row from the hash table (mapper, machine, region, profile, controller, persistence); statuses `unknown` until the owner's local `library` run evaluates a real dump; `unsupported` (names the missing board or device, exit 77) for X-Terminator, Master Gear, Action Replay, Pro Action Replay and anything not built; `skipped` with a reason for MAME `supported="no"` rows |
| `.info` drops `is_experimental` on a stated, tested rule | The owner asked for it; RetroArch shows the flag to users | LOW | The HOSTS table already guards it with a ledger pass-count threshold; the threshold must be a number the owner can reach with their own cartridges and must not need a person other than the owner (rule 8) |

### Differentiators (Competitive Advantage)

| Feature | Value Proposition | Complexity | Notes |
|---|---|---|---|
| Light Phaser sensed from the exact beam position and the emitted pixel (radius-6 window, luma threshold from MAME's model) | Crosshair misalignment is a named complaint of an "instruction accurate" core; cogmaster has per-dot counters | HIGH | Evidence: own WLA-DX gun fixture with the RAM mailbox first; real titles in the owner-local library run. The H-latch edge question (TH active-low vs 0->1) is settled by running both rules against Color & Switch Test and Hang-On & Safari Hunt locally |
| Gun, paddle and Sports Pad as deterministic recorded input (X/Y in each frame record) | Movies, TAS and CI hashes for peripheral games, which nobody offers; BizHawk's own TODO says "HCounter manually set for light phaser" | MEDIUM | Runner `--port1/--port2 lightgun\|paddle\|sportspad` and a versioned input record; part of the replay identity. Depends on the single absolute record per frame the runner uses today |
| Anaglyph and left/right output composed outside the hashed frame | A leading core refused anaglyph for six years; only a few emulators offer eye modes | MEDIUM | Adapter or runner helper over the per-frame eye flag; frame hashes stay per rendered frame. Option `cogmaster_3d_glasses` |
| Crosshair drawn by the adapter, not in the core frame | Keeps frame hashes clean; other cores bake it in | LOW | Option `cogmaster_crosshair`; libretro lightgun device with screen X/Y, trigger, off-screen reload, and a pointer/touch variant that never fires at screen centre first (named complaint) |
| Manual mapper override (option and `--mapper`) | Wanted by users with unknown dumps (C.09); Gearsystem has a long list (about 15 Korean sub-variants, Jumbo Dahjee, Iratahack, EEPROM 93C46, Multi 4PAK) | LOW | Only for the mappers built; unknown names exit 2. Gearsystem's long tail shows there are real Korean variants beyond the CC0 lists; do not chase them in v2.0 |
| Probe report of mapper, persistence and controller default with the evidence that chose them (hash hit, scan counts) | Collectors can audit why a dump was classified; matches the "hash hints never behaviour hacks" rule | LOW | Extend `--probe` keys; one line each |
| Fast-boot vs real-BIOS equivalence check | No emulator publishes it: after the BIOS hands off, registers, VDP, RAM residue and mapper state equal the fast-boot hand-off table, or the table grows | MEDIUM | Owner-local only (the dump never enters the repo); records one command, exit status and a run URL for the synthetic stand-in in CI |
| Built-in-game BIOSes (SMS2 Alex Kidd in Miracle World 128 KB, Hang-On, Hang-On + Safari Hunt, Missile Defense 3-D, Sonic 256 KB) and Korean Alex Kidd BIOS | The most common SMS2 BIOS contains a game; collectors with SMS2 dumps will try it | MEDIUM to HIGH | Needs BIOS-side paging that the preparation files do not describe in detail (LOW confidence): research flag. Accept as a stretch behind the 8 KB baseline |
| Hosted save round trip on EEPROM and battery | Proves bytes survive a fresh process and a released RetroArch | MEDIUM | Existing fresh-process continuation machinery; add an EEPROM fixture |
| Ledger page with mapper, device and profile counts and a not-a-bug list | GPGX gives a blanket "100 %" claim; MEKA gives text; none gives per-dump status with evidence | LOW (generator exists) | Reuse `scripts/compat-common.sh` and `policy.compat_doc_fresh`; add columns only if a status needs them |
| Audio predicate in ledger rows for FM and PSG titles | A frame hash proves video only; a PCM or transition hash proves the FM unit is wired | LOW to MEDIUM | New predicate kind `audio@F=H` over the transition hash that is already identical on eight lanes |

### Anti-Features (Commonly Requested, Often Problematic)

| Feature | Why Requested | Why Problematic | Alternative |
|---|---|---|---|
| Per-game Light Phaser X offset table (MAME keys offsets by header bytes: Space Gun and Gangster Town 9, Shooting Gallery 19, Rescue Mission 15, Laser Ghost 13, Assault City 22, Missile Defense 3-D 21) | Makes MAME's approximate beam position line up | It is exactly the per-title behaviour hack the project forbids, and the offsets compensate MAME's coarse model | One hardware-derived latency constant on a per-dot model; if two or more titles still need different values, record each as a ledger observation and re-derive, never a code table |
| Accepting any save file and guessing its layout (swap words, trim 32 KB to 8 KB, divine a 24 KiB Moldorian size) | "My old save should just load" | Raw files carry no identity, so a wrong-game or wrong-order file loads silently; v1.0 already refuses unknown geometry | Strict exact-size import (short pads with $FF, long rejects with exit 4); after measuring other emulators' bytes, add at most one documented, tested pad-on-import rule |
| Writing a fixed 32 KB `.srm` for every game (MAME style) | One size for all frontends | Breaks the exact-span rule, hides 8 KB vs 32 KB differences, and an EEPROM title would be 128 B anyway | Export the exact board size; revisit only if the measured cores demand it |
| Bundling, downloading or shipping any BIOS, or making BIOS required | Players want the logo and region lock | Copyright, rule 3 and ASSET_POLICY; two reference cores keep it optional | User supplies the dump; digest recognition only; default fast boot |
| BIOS region lock on by default | Hardware-accurate | Korean and Japanese carts stop at the export BIOS, a top complaint (C.05) | Honour it only when `--boot bios` is chosen |
| FM forced on for every game ("FM-for-all", "always on") | Richer sound | Games that detect FM misbehave, costs speed, a rejected request in other cores | FM only on profiles that have the unit; `COGMASTER_FM_AUTO` stays |
| Crosshair or anaglyph baked into the hashed frame | Easy for RetroArch users | Frame hash would depend on host options; breaks cross-platform hashes | Adapter-side overlay and eye composition |
| Graphic Board, SK-1100 keyboard, Master Tap, Gear-to-Gear cable, SG-1000/SC-3000 boards (Castle, Othello, Terebi Oekaki, Dahjee), Mega Drive pad, cheat devices (X-Terminator, Pro Action Replay, Master Gear) | "Whole library" sounds like all of it | About 90 SMS titles carry only an optional keyboard note and the rest are one-title devices; SG-1000 is a dormant seed; cheat devices are not games | Named `unsupported` ledger rows with exit 77; Mega Drive pad only if it is an hour's work on the pin-level port model (not in the milestone brief) |
| Chasing Gearsystem's full Korean mapper list (Korean MD FFF5, FFF0, BFFC, Jumbo Dahjee, Iratahack) | Completeness | None is in the CC0 lists, so no ledger row exists to prove them; MAME itself lists 188-in-1 as unsupported | Build the 33 listed entries; add others when a dump in the owner's collection fails and the probe scan names a pattern |
| "100 % compatible" claims or `pass` seeded from the core's own output | Marketing; easy ledger fill | Rule 8 and the ratchet; passing rows must come from independent predicates and real dumps | Statuses from the local `library` run only; protected golden values |
| Uploading frames, saves or file names from the owner's library to make rows public | Public evidence | ASSET_POLICY: only the public SHA-1, CC0 title, predicate result and date may leave | Existing `compat-update.sh` rules |

## Feature Dependencies

```
Korean mappers (board enum + hash `slot`)
    └──requires──> hash table with slot/persistence/controller columns (cartdb.inc generator)
    └──requires──> opcode-scan probe fallback (for dumps not in the list)

93C46 EEPROM
    └──requires──> Sega mapper $FFFC b3/b7 window (shipped)
    └──requires──> new persistence class in cartdb + save-state field
    └──requires──> battery API extension (128 B span, generation counter)
          └──enhances──> runner --save-dir atomic replace (shipped)
          └──enhances──> libretro libretro_saves / hosted save round trip

.srm interop ──requires──> measured byte layouts from released GPGX/Gearsystem binaries
              ──requires──> EEPROM layout fixed (LE16 words)

Pin-level controller ports ($3F direction + levels, TH/TR/TL)
    └──requires──> v1.0 I/O ports and region rule (shipped)
          ├──> Paddle (TR clock, Japan only)
          ├──> Sports Pad (TH sequence, timeout)
          └──> Light Phaser ──requires──> VDP H-counter latch on TH + per-line pixel readback (shipped, timing-sensitive)
3-D glasses ──requires──> $FFF8 shutter register + per-frame eye flag (small, independent)
Device selection ──requires──> hash `ctrl*_default` + runner --port options + libretro SET_CONTROLLER_INFO + versioned input record

BIOS boot ──requires──> $3E memory-map overlay (shipped) + firmware digest table
          ──requires──> hand-off table (shipped as fast boot) for the equivalence check
          └──enhances──> ledger rows that need BIOS state (e.g. California Games II per GPGX)

PAL + Japanese FM profile rows ──require──> PAL 313-line model and YM2413 (shipped)
                              ──require──> ledger profile columns + audio predicate

Whole-list ledger ──requires──> every board and device above built or named `unsupported`
                  ──requires──> local `library` preset (shipped) with the owner's dumps
is_experimental drop ──requires──> ledger threshold met
```

### Dependency Notes

- **Light Phaser requires the VDP latch and per-line model:** SEED-001's own note says schedule it after the v1.0 line timing has held; it must be the last controller, not the first.
- **EEPROM requires Sega-mapper paging to be unchanged:** the chip rides on $FFFC; a mapper refactor for the Korean boards must not move that register logic without the fixture.
- **Device selection requires a versioned record format:** the runner today reads exactly one absolute pad record per frame; analog X/Y is a format bump, which also bumps state format 10 and the public API 8 (EEPROM and device state).
- **Profile device availability conflicts with "all devices everywhere":** the Light Phaser is not on Mark III or GG, 3-D glasses are not on SMS2 or GG, the paddle is Japan-only. Enforce in the profile, not in each device.
- **Whole-list ledger conflicts with committing any game bytes:** CI can only run owned fixtures; real-title status comes from the owner's local run merged through `compat-update.sh`.
- **BIOS equivalence conflicts with CI evidence:** CI uses a self-built stand-in BIOS; the real dump is local.

## Other emulators' `.srm` and save layouts (for interop decisions)

| Emulator | File | What is known | Tier / confidence |
|---|---|---|---|
| RetroArch + Genesis Plus GX | `<rom>.srm` in the save directory | Docs: "MS/GG/MD/Pico/SG-1000 cartridge backup save". Byte count for an 8 KB SMS or GG game and any GG EEPROM support are not stated; a tracker issue shows its Mega Drive SRAM files can be twice the needed size because it stores 16-bit | T1 docs; size LOW. Do not open the source |
| RetroArch + Gearsystem | `<rom>.srm`; `<rom>.srm2` for Screen 2 in link mode | Docs: "battery RAM or EEPROM" in one file; has an EEPROM 93C46 mapper and a Multi 4PAK mapper | T1 docs; bytes LOW. GPL: not opened |
| MAME | `nvram` raw RAM of the softlist `ram` size; separate 128-byte EEPROM region | 16-bit EEPROM words stored little-endian; erased value all ones; loaded raw it allocates 32 KB and saves only if the game set $FFFC bit 3 | T3 (BSD-3 `eeprom.cpp`, `rom.cpp` read at the pin) HIGH for EEPROM order; MEDIUM for nvram sizing |
| ares | `save.ram` | raw bytes of the board's RAM | T3 (ISC) MEDIUM |
| Emulicious, MEKA | not documented in pages read | | no claim |

**Recommendation:** export exactly the board's span (8/16/24-refused/32 KiB, or 128 B EEPROM as 64 LE words); import exact size, pad short with $FF, reject long. Before relaxing, run the released GPGX and Gearsystem libretro binaries once on one 8 KB battery game and one EEPROM game (rule 1 allows released binaries; record command, exit status and sizes) and decide whether a documented pad-on-import rule is needed. Never convert silently between word orders.

## MVP Definition

### Launch With (milestone v2.0)

- [ ] Korean boards in the CC0 list (zemina, korean, korean_nb, nemesis, 4pak, hicom, janggun) via hash then scan; 188-in-1 and seojin as tiny stretches with ledger rows that say what fails
- [ ] 93C46 EEPROM for the four GG baseball titles, saved as 128 B `.srm`, atomic in the runner, round-tripped in a fresh process
- [ ] Sports Pad and Paddle (simple, pin-level), 3-D eye flag, then the Light Phaser on the existing latch; hash-driven default devices; runner `--port1/--port2`; libretro device list
- [ ] BIOS boot path for the 8 KB and 1 KB BIOSes with a self-built stand-in in CI
- [ ] PAL and Japanese FM rows under the right profile, with audio predicates for FM titles
- [ ] Ledger seeded for all 1,476 rows; `unsupported`/`skipped` rows carry reasons; `is_experimental` removed when the threshold holds

### Add After Validation (v2.x)

- [ ] Built-in-game BIOSes (128 KB and 256 KB) if the owner's dump is one of them
- [ ] Manual mapper option for dumps outside the list that the scan misclassifies
- [ ] Mega Drive pad as a port device (cheap on pin-level ports)
- [ ] A documented pad-on-import rule if the measured cores demand it

### Future Consideration (v3.0 and seeds)

- [ ] Gear-to-Gear cable, SK-1100 keyboard, Graphic Board, SG-1000/SC-3000 boards (SEED-006), cheat devices
- [ ] Optional hardware-measured Light Phaser delay (v4.0 measured accuracy)

## Feature Prioritization Matrix

| Feature | User Value | Implementation Cost | Priority |
|---|---|---|---|
| Korean boards (33 SMS entries) + hash/scan detection | HIGH | LOW | P1 |
| 93C46 EEPROM + 128 B `.srm` | HIGH | MEDIUM | P1 |
| Runner `--save-dir` for EEPROM, hosted round trip | HIGH | LOW | P1 |
| Ledger seeded for all 1,476 rows | HIGH | MEDIUM | P1 |
| Paddle, Sports Pad (4 + 3 titles) | MEDIUM | MEDIUM | P1 |
| 3-D eye flag + host eye modes | MEDIUM | LOW | P1 |
| Light Phaser (14 titles) | HIGH | HIGH | P1, last of the controllers |
| PAL and Japanese FM profile rows with audio predicate | HIGH | MEDIUM | P1 |
| BIOS boot (8 KB / 1 KB) | MEDIUM | MEDIUM | P1 |
| `is_experimental` removal | HIGH | LOW | P1, last |
| Built-in-game BIOSes | LOW | MEDIUM-HIGH | P2 |
| Manual mapper override | MEDIUM | LOW | P2 |
| `.srm` pad-on-import rule | LOW-MEDIUM | LOW | P2, after measurement |
| Mega Drive pad | LOW | LOW | P3 |
| Cheat devices, keyboard, link cable, SG-1000 | LOW | HIGH | P3 / seeds |

**Priority key:** P1 must ship in v2.0; P2 when evidence appears; P3 deferred.

## Suggested Phase Order (for the roadmap)

1. Tune-up (SEED-007), unchanged by this research.
2. Mappers: lowest risk, widest ledger yield (33 SMS rows), and the hash table gains the `slot`, `persistence` and `ctrl*_default` columns every later phase reads.
3. Saves: EEPROM rides on mapper infrastructure; `.srm` measurement runs once here.
4. Controllers: Paddle and Sports Pad, then 3-D flag, then Light Phaser; BIOS boot can share this phase or stand alone, since both change ports and the memory map.
5. PAL and FM profiles over the ledger, then the whole-list ledger and `is_experimental`.

## Competitor Feature Analysis

| Feature | Genesis Plus GX (NC, closed to us) | Gearsystem (GPL, closed to us) | MAME / ares (readable) | Our Approach |
|---|---|---|---|---|
| Korean mappers | Claims 100 %; open issues for 188-in-1, Janggun, 4 Pak | Auto plus about 25 mapper options | MAME: eleven mapper slots, 188-in-1 unsupported; ares: Hap2000, 188-in-1 | CC0 hash list first, opcode scan second, optional override; behaviour from documented registers |
| GG EEPROM | Not documented | Has "EEPROM 93C46" mapper | MAME: 93C46 16-bit device | Own state machine, fuzzed; MAME as differential reference only |
| Light Phaser | Mouse/touch device, optional crosshair, aux buttons | Light-gun device with crosshair and offset X/Y | MAME: per-game offsets; ares: trigger only | Beam-exact sensor; no title table |
| Paddle / Sports Pad | Analog devices on both ports | Paddle port 1; Sports Pad ports 1-2 with sensitivity | MAME and ares model both | Pin-level devices; analog X/Y in the recorded input |
| 3-D glasses | Not implemented as eye modes | Both, left, right | MAME: shutter | Eye flag in core; composition in host |
| BIOS | Optional, per-region names, default Off | Optional `bios.sms`, `bios.gg` | MAME: all BIOS files | Optional, same file names, default fast boot, equivalence check |
| Compatibility claim | Blanket 100 % + requirement list | Test-ROM screenshots | ares status table | Per-dump ledger with evidence and a not-a-bug list |

## Sources

- `.planning/preparation/SMS-CARTRIDGES-AND-PERIPHERALS.md` sections 2-4, 7, 8 (mappers, controllers, saves, milestone table): HIGH for structure; individual facts tiered there.
- `.planning/preparation/SMS-ECOSYSTEM-AND-USERS.md` sections 1-3, 5 (complaints C.05-C.09, library sizes, personas): MEDIUM, tracker complaints are T4.
- `.planning/preparation/HOSTS-AND-INTEGRATION.md` sections 1 and 4 (`.srm` layout, device list, runner options, `.info` guards); `SMS-SYSTEM.md` sections 5-8 (regions, BIOS table, hand-off); `CONFORMANCE.md` section 5 (ledger columns and ratchet).
- MAME CC0 softlists `hash/sms.xml`, `hash/gamegear.xml` at `1d7a454ed79e` (raw fetch and XML parse, 2026-11-09): board slots, `supported` flags, `ctrl*_default` shared features, `Notes:` comments. HIGH.
- MAME BSD-3 `src/devices/machine/eeprom.cpp` and `src/devices/bus/sega8/rom.cpp` at the same pin (EEPROM word order, window protocol). HIGH.
- libretro docs for [Gearsystem](https://docs.libretro.com/library/gearsystem/) (saves, mapper list, devices, BIOS) and [Genesis Plus GX](https://docs.libretro.com/library/genesis_plus_gx/) (saves, gun, paddle, BIOS names, region). HIGH for what they state; silent on SMS/GG byte counts.
- Web search results (LOW, cross-check before use): [Sega Retro Light Phaser](https://www.segaretro.org/Light_Phaser), [Sega Fandom Light Phaser](https://sega.fandom.com/wiki/Light_Phaser), [MobyGames Light Phaser attribute](https://www.mobygames.com/attribute/sheet/attributeId,314) disagree on gun title lists; [GameTech wiki Game Gear page](https://emulation.gametechwiki.com/index.php/Special:MobileDiff/64401) and [Black Falcon](https://blackfalcongames.net/?p=147) on the 128-byte EEPROM in four baseball games; [SMS_MiSTer issues](https://github.com/MiSTer-devel/SMS_MiSTer/issues?q=EEPROM) (issue 130, PRs 199 and 221) for the EEPROM boot gap; [Genesis Plus GX tracker issue 389](https://bitbucket.org/eke/genesis-plus-gx/issues/389/genesis-md-sram-files-size) for 16-bit SRAM doubling on Mega Drive. SMS Power pages returned 503 or access-denied again today; archive.org is blocked from this tool.
- Licence note: GPGX, Gearsystem and other GPL or non-commercial emulator source was not opened; only docs, trackers and permissive or CC0 files.

---
*Feature research for: cogmaster v2.0 "The whole library"*
*Researched: 2026-11-09*
