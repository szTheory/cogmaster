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

`libretro_saves` remains false and the core remains experimental in its `.info`:
memory export and in-process continuation do not prove the deferred hosted save-file
round trip or interoperability with another emulator.

## Testing saves

`runner.save.continuation.normal`, `.term` and `.int` verify fresh-process numeric
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
count toward the hosted RetroArch code budget. They do not establish the deferred
hosted save-file or cross-emulator interoperability claims.
