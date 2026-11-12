# Saves

How battery-backed cartridge RAM is exposed to a host and how the saves are tested. The calls and structs are in [embedding.md](embedding.md).

## Battery media

`cogmaster_get_battery` fills `cogmaster_battery` (`size`, `data`, `length`, `knowledge`,
`generation`). Set `size` to `sizeof(cogmaster_battery)`; a smaller tag is refused without
changing the output or instance. Larger outputs retain their unknown tail. Before load it
returns `COGMASTER_STATUS_ERR_NO_CONTENT`.

`COGMASTER_BATTERY_KNOWN` exposes exactly the classified persistent bytes from load, before
RAM is enabled. Its writable pointer and length stay fixed through frames, SOFT and POWER
resets until the next successful load or destruction. Fresh bytes are FF. `COGMASTER_BATTERY_NONE`
means known nonbattery content (including the test card); `COGMASTER_BATTERY_UNKNOWN` means
unidentified persistence; `COGMASTER_BATTERY_UNSUPPORTED` means known battery with unsupported
geometry. These three expose NULL, zero length and zero generation, never guessed storage.

The uint64 generation starts at zero and increments once modulo 2^64 per CPU-mapped persistent
write, including a same-value write. Reads, mapper register writes, disabled cartridge writes,
volatile writes and direct host writes do not increment it. Both resets preserve bytes and
generation. Internal states include and validate the generation, allowing only zero without
supported persistence. Hosts compare generations with their last successful flush snapshot;
acknowledge only after replacement succeeds. File persistence belongs to the host.

API version 7 adds `cogmaster_import_battery(m, data, length)`. With supported media,
lengths from zero through the exact span succeed: copy the supplied prefix and pad the
suffix with FF. NULL is accepted only for an empty import. Overlapping sources inside
the exposed span are supported. Import preserves generation, including repeated identical
imports. Snapshot generation after loading a save, before running. Oversized input returns
`COGMASTER_STATUS_ERR_SAVE_SIZE` before reading any source byte; NULL with a nonzero
accepted length returns `COGMASTER_STATUS_ERR_ARGUMENT`. Before load, or for known-none
media, import returns `COGMASTER_STATUS_ERR_NO_CONTENT`; unknown or unsupported persistence
returns `COGMASTER_STATUS_ERR_UNSUPPORTED_BOARD`. Every refusal preserves all instance state.

On the 93C46 board an import takes either no bytes or exactly 128: any other length, even a
shorter prefix, returns `COGMASTER_STATUS_ERR_SAVE_SIZE` before a source byte is read, because
a prefix would split a two-byte word. Export is never padded, trimmed or converted, so a host
sees the 128 bytes in the order the chip keeps them.

The core accepts exact-size wrong-game data: raw bytes carry no identity. Hosts must bound
file reads to expected length plus one and keep save namespaces separate. For a missing
file, leave the fresh FF bytes or import NULL with length zero. Neither operation marks
the cartridge dirty.

The libretro adapter exposes this same writable pointer and exact length through
`retro_get_memory_data(RETRO_MEMORY_SAVE_RAM)` and `retro_get_memory_size` immediately
after content load, before guest RAM enable. The frontend restores raw bytes through
that pointer and owns file naming, reading and writing; direct restoration does not
increment CPU-write generation. Frames and frontend Restart preserve the span; SMS
Restart uses SOFT reset and Game Gear Restart uses POWER reset. Unload, failed reload
and deinitialization make callbacks return NULL/0. A later successful load creates
fresh FF storage; hosts must discard prior pointers even if the address is reused.
Other memory IDs, known nonbattery, unknown persistence and unsupported geometry
return NULL/0. The adapter performs no filesystem I/O.

The `.info` declares `libretro_saves`, and `libretro.span.battery`, `.eeprom` and `.plain`
guard the claim: for the 32 KiB battery fixture, the 128-byte EEPROM fixture and a plain
cartridge, the span the module exports equals the library's battery length (32768, 128 and
NULL with zero). RetroArch decides how a differently sized `.srm` loads; only the runner
refuses a layout that is not its own.

## 93C46 EEPROM

A Game Gear cartridge on the Sega EEPROM board (`COGMASTER_BOARD_SEGA_EEPROM`, named `sega_eeprom`) carries a
93C46 serial EEPROM of 64 words of 16 bits. The 128 bytes are the cartridge RAM, so a host saves them through
the battery calls above with a span of 128. Each word is two bytes, little-endian (the low byte first), and a
fresh chip is all FF.

The board pages 16 KiB banks through `$FFFD`-`$FFFF` as the Sega board does, and the first 1 KiB stays fixed.
In `$FFFC`, bit 3 opens a window at exactly `$8000` and bit 7 resets the chip: chip select falls, a command in
progress is dropped and writes are locked again. Through the open window a write drives data in from bit 0, the
clock from bit 1 and chip select from bit 2, applied in that order, and a read returns the bits last written
with the low two cleared, bit 1 set and Data Out in bit 0. A byte is taken on a rising clock edge with chip
select high. The chip powers up write-disabled: WRITE, ERASE, WRAL and ERAL do nothing until EWEN, and again
after EWDS; READ always works and continues from word 63 to word 0 while the clock runs. Ready is immediate:
Data Out is high at all times except while a READ shifts data, so a game that polls for ready sees it on its
first look. There is no busy time.

The generation counter moves once per completed command that changed a byte, when chip select falls, never
per clock edge and never per WRAL byte, and not for a command that wrote the value already stored. This is a
rule of its own next to the same-value rule for battery RAM above. Importing a save does not move it.

The wiring and the chip model follow the Microchip 93C46 data sheet and the MAME `sega8_eeprom` board. They are
not verified on real hardware.

## Testing saves

The EEPROM board has its own cases: `eeprom.*` drive the chip bit by bit and the board through the bus, `fuzz.regress.eeprom`
(and the nightly libFuzzer entry) clock it with hostile streams, and `runner.save.continuation.eeprom` continues the owned fixture's
128-byte save across two fresh processes. `runner.save.continuation.normal`, `.term` and `.int` verify fresh-process numeric
continuation, missing and exact-size wrong-save controls, full save bytes and unchanged
oversize rejection; see [conformance](conformance.md). `libretro.battery.sms2` and
`.gg` restore the real classified battery fixture through the frontend's SAVE_RAM
pointer, continue 7 to 8, Restart to 9, unload/reload and restore again. They check
pre-enable and post-frame span stability, fresh FF bytes, invalidation, unknown and
no-content NULL/0 controls. `battery.continuation` independently checks both SOFT
and POWER resets with real media on SMS2 and GG: pointer, size, all bytes and generation
are unchanged, direct host writes and API imports are generation-neutral, and the
next guest write increments generation once. `battery.excluded` covers known-none,
unknown and unsupported geometry at the public battery-query boundary; mapper tests
keep volatile RAM separate. In-process tests load the production module and do not
count toward the hosted RetroArch code budget. They do not establish the hosted
save-file claim; the hosted RetroArch restart steps do. Interoperability with another emulator's save file is claimed only
for layouts that cogmaster wrote or that were measured once with released binaries
(2026-11-11, under the pinned RetroArch with the null drivers). Released Gearsystem
(option `EEPROM 93C46`) wrote the owned EEPROM fixture's save as 128 bytes starting `34 12`,
the little-endian word order cogmaster uses. Released Genesis Plus GX wrote the owned
battery fixture's save as a 3-byte file `a5 5a 01`, the first bytes cogmaster holds, not
the 32768-byte span; what it leaves beyond them is not measured. Nothing else is claimed.
