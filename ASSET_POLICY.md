# Asset policy

Which bytes may be in this repository, its history, its CI artifacts and its release archives: game ROMs, BIOS and
other firmware, saves, states, screenshots and test fixtures. Code provenance is in [PROVENANCE.md](PROVENANCE.md);
this file covers data. Written on 2026-10-18, before any such file exists. `[AP.NN]` IDs resolve in §8.

| Question | Section |
|---|---|
| Can this file be committed? How is it listed? | 1, 2 |
| What is built in CI instead of committed? | 3 |
| How do I test with my own dumps and BIOS? | 4 |
| What enforces this? | 5 |

## 1. Never
- Bytes of a commercial game in any amount or form: ROM images with or without a `TMR SEGA` header, with or
  without a 512-byte copier header, patches that embed them, disassemblies, and extracted graphics, sound or text.
- Firmware: every Master System BIOS (US/European 1.3, 2.0, 2.4 Hang-On and Safari Hunt, 3.4 Hang-On, 4.4
  Missile Defense 3-D, Alex Kidd, Sonic, the prototypes, the Japanese 2.1, the Korean Gam*Boy), the Store
  Display unit ROM, the Game Gear Majesco BIOS, and the games built into them. This includes copies inside
  permissively licensed repositories and bytes retyped into source code.
- Battery saves, EEPROM images, save states, movies, screenshots, recordings, traces or logs made while running a
  commercial game or a real BIOS.
- Bulk tables from databases that state no licence (No-Intro DATs, emulator header databases, ROM-pack headers,
  SMS Power!'s game lists). MAME's CC0 `hash/sms.xml`, `hash/gamegear.xml` and `hash/sg1000.xml` are the allowed
  databases [AP.01].
- Sega logos and artwork; for example, a frame dump of a BIOS splash screen or the Game Gear boot logo.
- Test ROMs or data whose authors state no licence, or a copyleft or non-commercial one (handled as in §3). Most
  downloads on SMS Power! state no licence; they stay in §3 unless their page or source says otherwise.

Owning the cartridge or the console changes nothing on this list. A player's own dumps stay on the player's machine.

## 2. Allowed, when listed in `tests/roms/manifest.txt`
- Third-party test ROMs and data under a licence from PROVENANCE.md §1, checked in both the LICENSE and the file's
  header at the exact pin.
- Our own builds: diagnostic and fixture ROMs assembled from this repository's MIT sources with WLA-DX, which
  links no runtime library, so every byte in the image is ours (a sibling learned that an SDK runtime, a font or
  a compiler library can carry its own licence into a built ROM [AP.07]); fuzz seeds and generated fixtures.
- Single facts about a game or board (mapper type, ROM size, a hash, a product code), and images or recordings
  made from committed fixtures or from the core's own test card.

The manifest has one line per file, with tab-separated fields; `#` starts a comment. It uses nesturbator's format
plus a `notice` field [AP.06]: `path source pin licence sha256 notice`. The fields are: path from the repository
root; source = upstream URL or `this-repo`; pin = 40-hex commit, or release tag plus asset name; licence = SPDX
expression; sha256 = 64 lowercase hex; notice = the `THIRD-PARTY-NOTICES.md` section name or `-`.
- **Recommendation:** the `notice` field. **Strongest alternative:** nesturbator's five fields. **Would change
  if:** no committed fixture ever needs a notice.

## 3. Built or fetched in CI, hashes committed
A file whose licence is unstated or restricts redistribution is built or fetched in CI at pinned commits. Only
its digests are committed: `tests/built/expected.txt` holds the name, tool and source pins, ROM SHA-256, and
frame and audio hashes. Tools are pinned by the SHA-256 of their archives, and every OS must produce
byte-identical files. Built files stay in the build directory; they never reach a release archive.

| Fixture | Built or fetched from (pin) | Why not committed |
|---|---|---|
| ZEXALL / ZEXDOC for the SMS | fetched at a pin from the SMS port's repository; SHA-256 checked | the exerciser is GPL-2 (SMSPOWER-EXTRACTS 6) |
| SingleStepTests `z80` full set | fetched nightly at `ebe1875d48f3` | size: ~1,600 files; a committed sample covers the per-PR tier [AP.04] |
| SMS Power! test ROMs with no stated licence | fetched at an access date with a recorded digest, local or nightly lanes only | no licence statement |
- **Recommendation:** fetch at pins, commit digests. **Strongest alternative:** commit permissively licensed
  test ROMs outright (allowed by §2 when the licence is clear). **Would change if:** an author grants a
  permissive licence on request; then the file moves to §2 with the grant cited.

## 4. Local only: your own dumps and BIOS
- `.env.local` (git-ignored; template `.env.example`) sets `COGMASTER_ROM_DIR` (your cartridge dumps) and
  `COGMASTER_BIOS_DIR` (your BIOS dumps, recognised by digest). Local runs read these, and everything they produce
  stays on that machine. The local `library` preset is never registered in the `ci` inventory.
- Firmware is identified by digest. The digests are facts and may be committed; the bytes may not. From MAME's
  BSD-3 driver source at `1d7a454ed79e` [AP.02]:

| Dump (MAME name) | Bytes | CRC32 | SHA-1 |
|---|---|---|---|
| US/European BIOS v1.3, 1986 (`mpr-10052.rom`) | 8,192 | `0072ed54` | `c315672807d8ddb8d91443729405c766dd95cae7` |
| European BIOS v2.0 (`mpr-10883.rom`) | 8,192 | `b3d854f8` | `fc7eb9141f38c92bf98d9134816f64b45e811112` |
| US/European v2.4 Hang-On and Safari Hunt (`mpr-11459a.rom`) | 131,072 | `91e93385` | `9e179392cd416af14024d8f31c981d9ee9a64517` |
| US/European v3.4 Hang-On (`mpr-11458.rom`) | 131,072 | `8edf7ac6` | `51fd6d7990f62cd9d18c9ecfc62ed7936169107e` |
| US/European v4.4 Missile Defense 3-D (`missiled.rom`) | 131,072 | `e79bb689` | `aa92ae576ca670b00855e278378d89e9f85e0351` |
| US/European Alex Kidd in Miracle World, 1990 (`mpr-12808.ic2`) | 131,072 | `cf4a09ea` | `3af7b66248d34eb26da40c92bf2fa4c73a46a051` |
| European/Brazilian Sonic the Hedgehog, 1991 (`sonbios.rom`) | 262,144 | `81c3476b` | `6aca0e3dffe461ba1cb11a86cd4caf5b97e1b8df` |
| US v1.0 prototype (`v1.0.bin`) | 8,192 | `72bec693` | `29091ff60ef4c22b1ee17aa21e0e75bac6b36474` |
| US prototype (`m404prot.rom`) | 8,192 | `1a15dfcc` | `4a06c8e66261611dce0305217c42138b71331701` |
| Japanese BIOS v2.1, 1987 (`mpr-11124.ic2`) | 8,192 | `48d44a13` | `a8c1b39a2e41137835eda6a5de6d46dd9fadbaf2` |
| Korean Gam*Boy II with Alex Kidd (`akbioskr.rom`) | 131,072 | `9c5bad91` | `2feafd8f1c40fdf1bd5668f8c5c02e5560945b17` |
| Store Display unit (`smssdisp.rom`) | 16,384 | `ee2c29ba` | `fc465122134d95363112eb51b9ab71db3576cefd` |
| Game Gear Majesco BIOS (`majbios.rom`) | 1,024 | `0ebea9d4` | `914aa165e3d879f060be77870d345b60cfeb4ede` |

No SHA-256 is recorded upstream; the first maintainer dump that matches both CRC32 and SHA-1 records one here.
The runner reports an unknown BIOS digest by name instead of guessing. libretro's BIOS file names for other cores
are in HOSTS-AND-INTEGRATION 1.

## 5. How this is checked
- `scripts/hygiene.sh` runs at pre-commit, at pre-push over all outgoing history, and in CI. Unless the manifest
  lists the file, it rejects ROM, BIOS and save extensions (`.sms .gg .sg .sc .mv .bin .rom .sav .srm .state
  .eeprom`, as in `.gitignore`), the `TMR SEGA` signature at `$7FF0`, `$3FF0` or `$1FF0` (compared as hex so the
  script never carries the signature as text [AP.05]), and any file containing a NUL byte. `policy.rom_hash_coverage`
  ties manifest rows to files.
- `release.firmware_scan` (planned, v1.0 phase 1) hashes every member of every release zip and every tracked
  binary (SHA-1, SHA-256): whole, and again without the first 512 bytes when a copier header is present. It fails
  on any §4 digest or any ROM SHA-1 in MAME's CC0 lists. A synthetic headered image is its negative control.
- `local.firmware_window_scan` (local only, planned) fails if any 32-byte window of a dump in `COGMASTER_BIOS_DIR`,
  with at least 8 distinct byte values, appears in a built core, runner or libretro module.

## 6. Names
"Sega", "Master System", "Mark III", "Game Gear", "SG-1000" and "SC-3000" are used only to say what the core
emulates. The project is not affiliated with or endorsed by Sega.

## 7. If something gets in
Remove it and rewrite the branch before pushing; the push hook scans every commit about to leave. If it was
already pushed, rewrite the history, force-push, and ask GitHub support to drop cached copies.

## 8. Sources
| ID | Tier | Source (URL) | Pin or access date | Supports |
|---|---|---|---|---|
| AP.01 | T1 | github.com/mamedev/mame `hash/sms.xml`, `hash/gamegear.xml`, `hash/sg1000.xml` (header `license:CC0-1.0`; 653, 823 and 214 `<software>` entries counted here (tested)) | `1d7a454ed79e` | allowed databases, library sizes |
| AP.02 | T1 | github.com/mamedev/mame `src/mame/sega/sms.cpp` L1020–1164 (`license:BSD-3-Clause`) | `1d7a454ed79e` | BIOS names, sizes, CRC32 and SHA-1 digests |
| AP.03 | T1 | github.com/szTheory/muhtari `ASSET_POLICY.md` | `952fce970117` | template and section structure |
| AP.04 | T1 | github.com/SingleStepTests/z80 `LICENSE` (MIT), `README.md` | `ebe1875d48f3` | vector set licence and size |
| AP.05 | T1 | `scripts/hygiene.sh` in this repository; muhtari's `ATARI7800` hex comparison | this repository | signature check design |
| AP.06 | T1 | github.com/szTheory/nesturbator `ASSET_POLICY.md`, `tests/roms/manifest.txt`, `scripts/hygiene.sh` | `9f9386c661f6` | manifest format |
| AP.07 | T1 | `.planning/LESSONS.md` XFER-019 (an unpublished Genesis sibling's phase 1) | this repository | runtime bytes carry licences |
