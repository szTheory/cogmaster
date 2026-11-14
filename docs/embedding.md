# Embedding the library

The library is one static archive and one header, `cogmaster.h`, written in C17. It allocates memory only through
the allocator you pass, keeps all state in the instance, starts no threads, opens no files, reads no clock and uses
no floating point. It needs nothing from the C library beyond `memcpy`, `memmove`, `memset` and `memcmp`.

This release loads a cartridge image and draws its Mode 4 background. With no image it runs the built-in test card.

`cogmaster_set_input` accepts a size-tagged `cogmaster_input` with `size`, `p1`, `p2`, `pause`, `reset`,
`start`, `port1`, `port2` and the signed axes `x1`, `y1`, `x2` and `y2`. It validates every field before storing
anything. Buttons and devices replace what was pending; axes add to it, clamped, until a frame takes them, and a
device change drops that port's waiting motion (see [controllers.md](controllers.md) "Input"). The call copies the
smaller of the tag and its own size, whole fields only, and zeroes the rest. The two pad masks
contain six independent bits (Up, Down, Left, Right, button 1, button 2). Console levels are binary.
`COGMASTER_DEVICE_PAD` is zero, the default; `COGMASTER_DEVICE_NONE` is one and releases that port's pins;
`COGMASTER_DEVICE_PADDLE` and `COGMASTER_DEVICE_SPORTS_PAD` read the axes, and a device the machine and region
cannot take returns `COGMASTER_STATUS_ERR_PROFILE_REJECTED`.
The native Game Gear has one local pad. Input latches before the next frame runs; identical submissions
remain held levels. Pause queues an NMI only on a rising edge. A disconnected port releases its pins;
reconnecting with Pause held requires release before another press can queue an NMI. Reset drives a
level only on export SMS1; native GG Start drives its active-low level on `$00`. Opposing directions
pass through independently. TR output directions override pad levels, with Japanese output echo low.
The first input struct size is 32 bytes; the axes make it 48. Oversized zero tails follow the general size-tag rule.

The appended `cogmaster_run_result.lagged` is one when no decoded controller read occurred in that frame.
Legacy 12-byte callers receive only their original fields. The runner submits input before each frame and
receives this result. Host peeks never earn lag credit.
API version 8 appends `cogmaster_run_result.display_enabled`: the VDP display-enable latch
observed at the end of this run call, zero for the test card. It is a sampled observation;
it does not prove that every line was enabled. Older 12-byte and 16-byte callers keep their layouts.

The libretro adapter accepts `game.data` and `game.size` in memory, advertises
`sms|gg|bin|rom`, and keeps `need_fullpath` false. One input poll precedes each
frame; D-pad/B/A map to the six pad bits, except that on a Sports Pad or Paddle port the D-pad, left stick and
mouse become axis deltas. Port-1 Start maps to SMS Pause or native
GG Start, and Select drives SMS Reset. Pad/none and button descriptors are sent
to the frontend. Missing callbacks and none devices submit released levels.
Their reconnection follows the same Pause arming rule as the library. Port 2 never supplies console
controls. RetroArch Restart calls SOFT on SMS and POWER on GG; Select changes only the supported physical
Reset level and cannot restart the CPU. Both restart kinds preserve cartridge RAM.
API version 4 appends SMS1 and unsupported-video fault constants; all earlier
values and struct layouts are retained. Behaviour revision 10 adds classified battery write generations.
The input model includes pin-output mixing, export-SMS1-only Reset and held-reconnect Pause suppression. The renderer includes independent line
IRQs, six mode/TV V-counter sequences, the translated H coordinate and TH capture
to the sprite/background semantics. State format 10 saves battery generations and native LCD and palette-latch state,
completed pixels/height/backdrop alongside the line counter
and pending source, rolling NTSC-240 phase and held H value alongside the sprite
slots, dot cursor, scroll/display latches and last driven bus byte, so older state
bytes are refused. API version 6 appends the battery query; existing ABI layouts remain unchanged.
API version 9 appends the audio drain `cogmaster_get_audio`. Behaviour revision 11 and state format 11
cover the whole of the sound work, so older state bytes are refused. See [Audio](#audio).
API version 10 appends the board constants `COGMASTER_BOARD_KOREAN`, `COGMASTER_BOARD_KOREAN_NB`, `COGMASTER_BOARD_HICOM`,
`COGMASTER_BOARD_ZEMINA`, `COGMASTER_BOARD_NEMESIS` and `COGMASTER_BOARD_4PAK`, the board source constants
`COGMASTER_BOARD_SOURCE_DB`, `COGMASTER_BOARD_SOURCE_HEADER`, `COGMASTER_BOARD_SOURCE_HEURISTIC` and `COGMASTER_BOARD_SOURCE_DEFAULT`, and the
`cogmaster_cart_info` fields from `board_source` to `unsupported_reason`, with behaviour revision 12 and state format 12; [Boards](boards.md) says what they mean.
API version 11 appends the board constant `COGMASTER_BOARD_SEGA_EEPROM`, the Sega board with a 93C46 save chip, with behaviour revision 13 and state format 13, so older state bytes are refused; [Saves](saves.md) says what the chip and its window do.
API version 12 appends the axis fields `x1`, `y1`, `x2` and `y2` to `cogmaster_input` (48 bytes), the devices `COGMASTER_DEVICE_PADDLE` and `COGMASTER_DEVICE_SPORTS_PAD`, the eye constants `COGMASTER_GLASSES_NONE`, `COGMASTER_GLASSES_RIGHT` and `COGMASTER_GLASSES_LEFT`, `glasses` in `cogmaster_run_result` and `default_port1` and `default_port2` in `cogmaster_info` and `cogmaster_cart_info`, with behaviour revision 14 and state format 14. Axis values are deltas per call. A caller built against the 32-byte input stays valid; a zeroed result or info means no eye and a pad. The controllers guide has the rest.
API version 13 appends `COGMASTER_DEVICE_LIGHT_PHASER`, with behaviour revision 15 and state format 15, so older state bytes are refused. A phaser port reads its aim from `x1` `y1` (port 1) or `x2` `y2` (port 2) as 1-based field dots; the last call wins and `0 0` means no aim.
API version 14 appends `bios` and `bios_size` to `cogmaster_media` (72 bytes), with behaviour revision 16 and state format 16, so older state bytes are refused.
Behaviour revision 17 refuses the Sega Graphic Board dump, which read as a plain pad before, so state bytes saved at revision 16 are refused.
Behaviour revision 18 makes the unsupported-video-mode fault record the mode select nibble in `addr` and stops recording writes to the unused bits of `$FFFC`, writes to a cartridge address that is no register, reads of ports `$00`–`$3F` nothing drives, and writes to ports nothing decodes, as faults, so state bytes saved at revision 17 are refused.

## Getting it

The H-counter origin's 32-tick translation is a coordinate inference from sourced timing discussions.
The tick-600 display/scroll/height latches and native GG 240-line LCD freeze are selected sourced models;
tests establish deterministic implementation, not hardware measurements. The measured IRQ edges remain
607/608. Moldorian's metadata declares 24 KiB persistent media, but physical bank decoding remains unresolved;
its persistence query/import is refused rather than guessing a mask or modulo mapping.

**Source tree.** `add_subdirectory` or `FetchContent` gives the `cogmaster::cogmaster` target. Only the C compiler
is needed: an embedding build gets the library alone, without the runner, the tests or this project's warning
flags (`COGMASTER_BUILD_RUNNER`, `COGMASTER_BUILD_LIBRETRO` and `COGMASTER_BUILD_TESTS` default to off when
cogmaster is not the top-level project).

```cmake
add_subdirectory(third_party/cogmaster)
target_link_libraries(app PRIVATE cogmaster::cogmaster)
```

`cogmaster.h` includes `cogmaster_version.h`, which the build generates from `version.txt`; the target's include
path covers both, and `cmake --install` puts them side by side.

**Release archive.** `cogmaster-library-<os>-<arch>.zip` from a release holds `include/`, the static library in
`lib/`, a CMake package in `lib/cmake/cogmaster/` and `lib/pkgconfig/cogmaster.pc`. Unzip it anywhere; both
files find the rest from their own location. The CMake package accepts a request from the same major and minor
version only, because before 1.0 a minor release may change the API.

```cmake
# configure with -DCMAKE_PREFIX_PATH=<unzipped directory>
find_package(cogmaster CONFIG REQUIRED)
target_link_libraries(app PRIVATE cogmaster::cogmaster)
```

```sh
PKG_CONFIG_PATH=<unzipped directory>/lib/pkgconfig cc app.c $(pkg-config --cflags --libs cogmaster)
```

The header compiles as C and as C++; a C++ program includes it directly.

## Structs and sizes

Every struct you pass starts with `uint32_t size`. Zero the whole struct with `memset`, then set `size`:

```c
cogmaster_config cfg;
memset(&cfg, 0, sizeof cfg);
cfg.size = (uint32_t)sizeof cfg;
```

There is no init macro, and a brace initialiser or a struct copy does not zero padding. The size tag lets a
program built against one header run with a library built from another.

**Input structs** (`cogmaster_config`, `cogmaster_media`, and `cogmaster_frame` for its tag):

- A size below the struct's first released size returns `COGMASTER_STATUS_ERR_STRUCT_SIZE`.
- A size between the first released size and the library's own: the fields the caller's header lacks read as
  zero, and a field added later keeps the old behaviour when it is zero. A size equal to the first released size
  reads every later field as zero, even one that lies in that struct's tail padding (`cogmaster_media.region` on
  a 64-bit target).
- A larger size from a newer header is accepted only if every byte past the library's struct is zero; otherwise
  the call returns `COGMASTER_STATUS_ERR_STRUCT_SIZE`.
- A size above 4096 bytes returns `COGMASTER_STATUS_ERR_STRUCT_SIZE` whatever the bytes hold. Checking that the
  extra bytes are zero reads them, so without this cap a garbage size tag would make the library read an
  unbounded distance past your object.

**Output structs** (`cogmaster_info`, `cogmaster_versions`, `cogmaster_run_result`, `cogmaster_fault`,
`cogmaster_cart_info`, `cogmaster_audio`, `cogmaster_audio_trace`):

- A size below the struct's first released size returns `COGMASTER_STATUS_ERR_STRUCT_SIZE` and leaves the struct
  as it was.
- Any larger size, with no upper bound, is accepted: the library writes the smaller of your size and its own and
  leaves every byte past its own struct as you set it. The `size` field keeps your value.

First released sizes, where P is the pointer size (4 or 8):

| Struct | First released size |
|---|---|
| `cogmaster_config` | 4 × P |
| `cogmaster_media` | 4 × P |
| `cogmaster_frame` | 2 × P + 32 |
| `cogmaster_info` | 48 |
| `cogmaster_versions` | 32 |
| `cogmaster_run_result` | 12 |
| `cogmaster_fault` | 32 |
| `cogmaster_cart_info` | 52 |
| `cogmaster_audio` (output) | 2 × P + 16 |
| `cogmaster_audio_trace` (output) | 32 on 64-bit, 20 on 32-bit |

Whole structs: `cogmaster_media` is 72 bytes (40 on 32-bit, `fm` at 28) and `cogmaster_info` is 68.

Fields are fixed-width integers and pointers; enumerations travel as `uint32_t` and zero is the default of every
input field. Statuses, constants and fields are only ever appended.

## The allocator

`cogmaster_config.allocator`, a `cogmaster_allocator`, needs both `alloc` and `free`; either one missing returns
`COGMASTER_STATUS_ERR_ARGUMENT`. There is no default: the library links no allocator of its own. `alloc` must
return memory aligned for `max_align_t`, as `malloc` does. `user` is passed back unchanged. A host with a C library
can pass this wrapper:

```c
static void *host_alloc(void *user, size_t size) {
    (void)user; return malloc(size);
}
static void host_free(void *user, void *ptr, size_t size) {
    (void)user; (void)size; free(ptr);
}
```

The library calls `alloc` only in `cogmaster_create` and `cogmaster_load`, and `free` receives the size that was
allocated, so a fixed arena or pool needs no header of its own. Running frames allocates nothing.
`cogmaster_destroy` frees everything; it accepts NULL and does nothing.

## Loading and running

```c
cogmaster *m = NULL;
cfg.allocator.alloc = host_alloc;
cfg.allocator.free = host_free;
if (cogmaster_create(&cfg, &m) != COGMASTER_STATUS_OK) { /* handle the error */ }

cogmaster_media media;
memset(&media, 0, sizeof media);
media.size = (uint32_t)sizeof media;
media.machine = COGMASTER_MACHINE_AUTO;   /* or COGMASTER_MACHINE_SMS2, COGMASTER_MACHINE_GG */
media.region = COGMASTER_REGION_AUTO;     /* or COGMASTER_REGION_EXPORT, COGMASTER_REGION_JAPAN */
media.tv = COGMASTER_TV_AUTO;             /* or COGMASTER_TV_NTSC, COGMASTER_TV_PAL */
media.video_mode = COGMASTER_VIDEO_AUTO;  /* or COGMASTER_VIDEO_SMS, COGMASTER_VIDEO_GG */
media.fm = COGMASTER_FM_AUTO;             /* or COGMASTER_FM_OFF, COGMASTER_FM_ON */
media.image = rom;                        /* the image bytes, or NULL for the test card */
media.image_size = rom_size;              /* 0 with a NULL image */
cogmaster_load(m, &media);

cogmaster_run_frame(m, NULL);             /* one frame; the result struct is optional */
```

`fm` picks the YM2413 FM unit: AUTO attaches it only for a Japanese Master System, OFF never, ON on any Master
System; a value above ON, or ON with a Game Gear, returns `COGMASTER_STATUS_ERR_PROFILE_REJECTED`. A size tag
from before `fm` reads AUTO. See [audio](audio.md).

`video_mode` defaults to native GG on a Game Gear and SMS on either Master System.
Explicit GG video requires a Game Gear. Explicit SMS video on a Game Gear retains
the handheld machine and port decode, uses immediate SMS colours, and exposes the
full SMS field with NTSC PAR 8:7. Physical LCD resampling belongs to the host.
`cogmaster_info.video_mode` reports the resolved value. Media tags from before
this extension (40 bytes on 64-bit, 24 on 32-bit) read video mode as AUTO,
including the old tail padding. Zero the reserved media word on 64-bit systems.

Native GG crops (48,24,160,144) at 192 lines and (48,40,160,144) at 224 lines.
The selected 240-line LCD model holds the last native image and its descriptor
until POWER reset; SOFT reset retains the freeze. FULL_FIELD continues to expose
the current field independently. Both the held image and palette latch are state.
GG controller reads decode C0/C1 and DC/DD; native Start is active low at port 00.
In explicit SMS video mode Start instead raises the edge-detected Pause NMI.

`cogmaster_media.image` with `image_size` 0 and a NULL `image` loads the built-in test card. Any other image must
be 1,024 bytes to a 1 MiB body, with an optional 512-byte copier header in front; any other size, or a body above
1 MiB whatever the image's total size, returns `COGMASTER_STATUS_ERR_UNSUPPORTED_SIZE`, and a NULL
`image` with a non-zero size returns `COGMASTER_STATUS_ERR_ARGUMENT`. When the size is 512 bytes past a multiple
of 16 KiB the first 512 bytes are a copier's header and are skipped. The library copies the rest into one buffer
from your allocator, padded with `$FF` to a multiple of 16 KiB, so you may free your copy once `load` returns.
The library guesses the cartridge's board from the image (see [Probing an image](boards.md#probing-an-image)) and maps it
at `$0000`-`$BFFF`, with 8 KiB of work RAM at `$C000`; an image on no supported board returns
`COGMASTER_STATUS_ERR_UNSUPPORTED_BOARD`. The CPU starts as the console's BIOS leaves it (fast boot; no BIOS code runs): stack pointer `$DFF0`,
`$AB` at `$C000` on the Master System, program counter 0. A BIOS in `cogmaster_media` boots from it instead: `bios` and `bios_size` take an 8,192-byte Master System image, are read only when the size tag covers `bios_size`, and are both set or both zero (`COGMASTER_STATUS_ERR_ARGUMENT`). Another size returns `COGMASTER_STATUS_ERR_UNSUPPORTED_SIZE`; the test card returns `COGMASTER_STATUS_ERR_NO_CONTENT` and a Game Gear `COGMASTER_STATUS_ERR_PROFILE_REJECTED`. The library keeps its own copy, a load without a BIOS drops it, and a state saved under one boot mode or BIOS is refused by another. See the BIOS guide, `docs/bios.md`. `machine` selects the profile: `COGMASTER_MACHINE_AUTO` (the default, a Master
System II), `COGMASTER_MACHINE_SMS2`, `COGMASTER_MACHINE_GG` (2), or
`COGMASTER_MACHINE_SMS1` (3, the 315-5124 VDP). SMS1 uses the console boot and
port rules with address masks described in [video](hardware/video.md).
`region` selects the console's region:
`COGMASTER_REGION_AUTO` (the default, export), `COGMASTER_REGION_EXPORT` or `COGMASTER_REGION_JAPAN`. `tv` selects
the video standard: `COGMASTER_TV_AUTO` (the default, NTSC), `COGMASTER_TV_NTSC` or `COGMASTER_TV_PAL`. No Game
Gear and no Japanese console was made for PAL, so `COGMASTER_TV_PAL` with either returns
`COGMASTER_STATUS_ERR_PROFILE_REJECTED`. Each
`load` restarts the frame count and clears the fault record.

`cogmaster_run_frame` runs one frame: 262 lines, 179,208 ticks of the 315/88 MHz × 3 (about 10,738,636 Hz) tick clock, on NTSC, and
313 lines, 214,092 ticks of the 10,640,684.8 Hz (53,203,424 / 5) clock, on PAL. The CPU runs whole instructions,
so a frame stops at the first instruction boundary at or after its end, at most 69 ticks late, and the next frame
ends one frame length after this one should have: the frames never drift. If `out`, a
`cogmaster_run_result`, is not NULL it receives `stop`, always `COGMASTER_STOP_FRAME` in this release, and
`overshoot_ticks`, the ticks run past the frame's end (always 0 for the test card).

The Master System's Pause button reaches the CPU as on the console: a press raises the CPU's non-maskable interrupt
at the start of the frame's last line, tick 611 of line 260 on NTSC and of line 311 on PAL, wherever in the frame it
was pressed. The Game Gear has no Pause button.

Probing an image without an instance, and the cartridge boards each image gets, are in [boards.md](boards.md).

## Frames

The library keeps a field 256 dots wide; the test card and the 192-line mode use 192 of its lines. Each dot is
a `uint16_t` in RGB444, `0000BBBBGGGGRRRR`: red in the low nibble, blue in the third. Master System colours are
two bits per channel, widened to four as `v | v << 2`, so they live in the same space as Game Gear colours.

```c
static uint16_t pixels[256 * 240];        /* the largest field */
cogmaster_frame io;
memset(&io, 0, sizeof io);
io.size = (uint32_t)sizeof io;
io.pixels = pixels;
io.pitch_bytes = 256 * 2;                 /* bytes from one row to the next */
cogmaster_get_frame(m, &io);              /* io.x, io.y, io.w, io.h: what was copied */
```

By default `cogmaster_get_frame` copies the machine's crop: the whole completed 256 × H field on the Master System (H = 192, 224 or 240), the
160 × 144 window at (48, 24) on the Game Gear. With `flags` set to `COGMASTER_FRAME_FULL_FIELD` it copies the
whole 256-dot field instead; outside the Game Gear window the test card is magenta (`0x0F0F`). Any other flag bit
returns `COGMASTER_STATUS_ERR_ARGUMENT`. The library fills `x`, `y`, `w` and `h` with the rectangle copied, in
field coordinates, `field_h` with the field's height and `backdrop_rgb444` with the backdrop colour (black for
the test card). Rows are written `pitch_bytes` apart and the bytes between the end of a row and the next row are
left alone. `pitch_bytes` below `w × 2` returns `COGMASTER_STATUS_ERR_BUFFER_TOO_SMALL` and writes nothing;
above 65536 returns `COGMASTER_STATUS_ERR_ARGUMENT`, which keeps every row offset inside a 32-bit `size_t`.

A frame hash is SHA-256 over the crop rows, top to bottom, each pixel as two bytes with the low byte first. Hash
those bytes, not the `uint16_t` array, or a big-endian host gets a different digest.

## Audio

The host contract is in [audio](audio.md). `cogmaster_run_frame` makes one frame's samples at 48 kHz and `cogmaster_get_audio(m, io)` hands them over:
it copies the smaller of `capacity` and what is pending, in sample frames, into `samples` and sets `written`,
`channels` and `dropped`. A sample frame is one `int16_t` when `channels` is 1 (the Master System) and a left
and right pair when it is 2 (the Game Gear, in either video mode). Set `size` to `sizeof(cogmaster_audio)`.

Every NTSC frame holds 801 or 802 sample frames and every PAL frame 965 or 966, never padded. After F frames the
total is floor(F × 3,504,512 / 4,375) on NTSC and floor(F × 685,094,400 / 709,379) on PAL. Frames you do not
take before the next `cogmaster_run_frame` are lost and counted in `dropped`, the total since load or POWER
reset. Before the first frame after a load, `written` is 0.

`capacity` 0 with a NULL `samples` is OK and reports `channels`; a NULL `samples` with `capacity` above 0
returns `COGMASTER_STATUS_ERR_ARGUMENT`. A load and a POWER reset restart the cadence at 801; a SOFT reset keeps
it and any samples not yet taken. The test card is silent but follows the same cadence.

### The audio trace

`cogmaster_config.trace_records` asks for a log of that many `cogmaster_audio_record` values (`tick`, `source`,
`level`; 16 bytes each): 0, the default, keeps none, and above 1,048,576 `cogmaster_create` returns
`COGMASTER_STATUS_ERR_ARGUMENT`. The first successful `cogmaster_load` allocates the log through your allocator,
once; later loads reuse it and nothing is allocated after a load. `cogmaster_get_audio_trace(m, io)` copies up to
`capacity` records, oldest first, into `records`, sets `written` and `dropped` and empties the log. `dropped`
counts the records the full log refused plus those that did not fit `capacity`. An instance made with no log
returns `COGMASTER_STATUS_ERR_ARGUMENT`, as does a NULL `records` with `capacity` above 0. A load and both
resets empty the log, since ticks restart there.

## Reset

`cogmaster_reset(m, kind)` restarts the loaded content without loading it again. Both kinds set the frame count
and the tick count to zero, put the beam at the frame origin (line 192, tick 607, where a frame ends) as `load`
does, start the CPU again from the fast-boot hand-off (at address 0 with a BIOS) and clear the fault record. Every frame, the first after a
load or reset included, is then exactly 179,208 ticks on NTSC and 214,092 on PAL:

- `COGMASTER_RESET_SOFT` (0) reapplies the fast-boot hand-off: memory kept across this restart, such as work RAM,
  stays as it was.
- `COGMASTER_RESET_POWER` (1) is a power cycle: it also zeroes the internal counters and, on machines that have
  them, work RAM, video RAM and colour RAM.

The test card has no RAM, video RAM or colour RAM and its counters stay zero, so the two kinds have the same effect
on it. Any other `kind` returns `COGMASTER_STATUS_ERR_ARGUMENT`; a reset before a successful
`cogmaster_load` returns `COGMASTER_STATUS_ERR_NO_CONTENT`.

## Faults

`cogmaster_get_fault` fills a `cogmaster_fault` with the first fault the core recorded since the last load or
reset: a cartridge or RAM address that port `$3E` took off the bus, a mapper value out of range, an undefined
opcode, or an unsupported video mode. Only the first is kept; later ones are ignored until the next load or reset.
The record is part of the instance's saved state.

| Field | Meaning |
|---|---|
| `present` | 1 when a fault was recorded, 0 otherwise; every other field is 0 when it is 0 |
| `frame`, `line`, `dot` | where the beam was: the frame count, the line (0 to 312) and the dot (0 to 341) |
| `pc` | the CPU's program counter at the faulting instruction |
| `addr`, `width` | the full 16-bit address or port accessed and the access width in bits (8) |
| `opcode` | the opcode byte being executed |
| `device`, `kind` | which device faulted and how, as the codes below |

`kind` and `device` take these values; both lists only grow, and 0 means none.

| `kind` | Constant | Recorded when |
|---|---|---|
| 1 | `COGMASTER_FAULT_KIND_UNMAPPED_READ` | a read nothing drives returns open bus: the cartridge slots with the cartridge disabled by port `$3E` and no BIOS answering (and, with a BIOS attached, not an enabled empty card or expansion slot), or work RAM disabled by port `$3E` |
| 2 | `COGMASTER_FAULT_KIND_UNMAPPED_WRITE` | a write nothing takes: the cartridge slots with the cartridge disabled by port `$3E`, which also hides `$FFFC`–`$FFFF` from its mapper; or work RAM disabled by port `$3E`, except a write to `$FFFC`–`$FFFF` the enabled cartridge takes |
| 3 | `COGMASTER_FAULT_KIND_ROM_WRITE` | no longer recorded: a write to `$0000`–`$BFFF` that is no register and no enabled cartridge RAM changes nothing; the constant stays |
| 4 | `COGMASTER_FAULT_KIND_MAPPER_RANGE` | a bank number at or past the end of an image whose page count is not a power of two |
| 5 | `COGMASTER_FAULT_KIND_ED_UNDEFINED` | the CPU ran an opcode after `$ED` that the Z80 does not define, as an 8 T-state no-operation; `opcode` is that second byte and `pc` the address of the `$ED` |
| 6 | `COGMASTER_FAULT_KIND_UNSUPPORTED_VIDEO_MODE` | an enabled display selects an unsupported TMS or invalid text mode; output is backdrop and frame delivery stays bounded |

| `device` | Constant | Meaning |
|---|---|---|
| 0 | `COGMASTER_FAULT_DEVICE_NONE` | no device answers the address |
| 1 | `COGMASTER_FAULT_DEVICE_CPU` | the CPU itself |
| 2 | `COGMASTER_FAULT_DEVICE_CART` | the cartridge |
| 3 | `COGMASTER_FAULT_DEVICE_RAM` | work RAM |
| 4 | `COGMASTER_FAULT_DEVICE_MAPPER` | the cartridge's mapper |
| 5 | `COGMASTER_FAULT_DEVICE_CTRL` | no longer recorded since behaviour revision 18: nothing faults on the control ports `$3E` and `$3F`; the constant stays |
| 6 | `COGMASTER_FAULT_DEVICE_VDP` | the video display processor |

`addr` is the full 16-bit bus address; for `IN A,(n)` and `OUT (n),A` that is A × 256 + n. For kind 6 it holds
the video mode select nibble M4 M3 M2 M1 (0–15), and 0, Graphics I, is a valid value. Outside kind 5, `pc`
is the address of the instruction, its first `$DD` or `$FD` prefix included, and `opcode` its first byte after
those prefixes.

Things shipped games do are never faults, so the record stays free for the real one: reads of `$C0`–`$FF` while
port `$3E` has the I/O chip disabled, ports `$F0`–`$F2`, `$DE` and `$DF`, writes to the I/O chip's ports, the Game
Gear's link ports `$01`–`$05`, a bank number past the end of an image whose page count is a power of two (it
wraps), writes to the unused bits of `$FFFC`, writes to a cartridge address that is no register, reads of ports `$00`–`$3F` nothing drives, writes to ports nothing decodes, toggling port `$3E`, and undocumented CPU instructions. An undriven read returns `$FF` and counts in the
instance's open-bus counter instead. SMS1 undriven memory/port reads retain the
last byte driven by a CPU fetch, read or write; SMS2/GG pull it to `$FF`.
Peeks observe it without changing it or the counter. The selected model follows
preparation SMS-SYSTEM C5; instruction-specific electrical behavior remains unmeasured.
The test card runs no CPU, so it never records a fault. The call works before
a load too, and then reports `present` 0.

## Reading memory

`cogmaster_peek(m, COGMASTER_SPACE_BUS, addr)` returns the byte the CPU would read at `addr` (`$0000`-`$FFFF`)
right now: the cartridge through its mapper, work RAM and its mirror, and `$FF` where port `$3E` has taken the
cartridge or work RAM off the bus. It has no side effect: it moves no clock, records no fault, counts no open-bus
read and clears no status, so peeking every address leaves the saved state byte for byte the same. A peek with no
answer returns a value above 255: a NULL instance, no successful `cogmaster_load` yet, a `space` other than
`COGMASTER_SPACE_BUS` (0, the only space), or an `addr` above `$FFFF`. With the test card loaded the slots read
`$FF`.

Persistent cartridge RAM has its own calls; see [saves.md](saves.md).

## Information

`cogmaster_get_info` fills a `cogmaster_info` for the loaded machine, so a host need not hard-code it:

| Field | Master System | Game Gear |
|---|---|---|
| `machine` | `COGMASTER_MACHINE_SMS2` | `COGMASTER_MACHINE_GG` |
| `crop_w`, `crop_h` | 256, 192 | 160, 144 |
| `field_w`, `field_h` | 256, 192 | 256, 192 |
| `max_w`, `max_h` | 256, 240 | 256, 240 |
| `fps_num`, `fps_den` | 1,640,625 / 27,379 (about 59.92 Hz); on PAL 13,300,856 / 267,615 (about 49.70 Hz) | 1,640,625 / 27,379 |
| `par_num`, `par_den` | 8 / 7 | 6 / 5 |
| `region` | the loaded region: `COGMASTER_REGION_EXPORT` or `COGMASTER_REGION_JAPAN` | the same |
| `tv` | the loaded video standard: `COGMASTER_TV_NTSC` or `COGMASTER_TV_PAL` | `COGMASTER_TV_NTSC` |
| `audio_channels` | 1 | 2 |
| `fm` | `COGMASTER_FM_ON` or `COGMASTER_FM_OFF` | `COGMASTER_FM_OFF` |

`region` and `tv` are never AUTO: they report what `load` resolved. The pixel aspect ratio is reported, never
applied. `max_w` × `max_h` sizes a buffer for any frame.

## Functions and statuses

`cogmaster_status` is a `uint32_t`. Every function checks its arguments before it changes anything, so an error
changes no state; a failed `cogmaster_load` keeps the content that was loaded before.

| Status | Value | Returned when |
|---|---|---|
| `COGMASTER_STATUS_OK` | 0 | the call did what it says |
| `COGMASTER_STATUS_ERR_ARGUMENT` | 1 | a required pointer is NULL (`cogmaster_probe`'s `image` included); the allocator lacks `alloc` or `free`; `image` is NULL while `image_size` is not 0; `flags` has an unknown bit; `pitch_bytes` is above 65536; the reset `kind` is unknown |
| `COGMASTER_STATUS_ERR_STRUCT_SIZE` | 2 | a size tag breaks the rules in [Structs and sizes](#structs-and-sizes) |
| `COGMASTER_STATUS_ERR_NO_MEMORY` | 3 | `alloc` returned NULL in `cogmaster_create` (`*out` is left as it was) or in `cogmaster_load` |
| `COGMASTER_STATUS_ERR_BUFFER_TOO_SMALL` | 4 | `pitch_bytes` is below `w × 2` for the rectangle requested |
| `COGMASTER_STATUS_ERR_NO_CONTENT` | 5 | `cogmaster_run_frame`, `cogmaster_get_frame`, `cogmaster_get_info`, `cogmaster_get_audio`, `cogmaster_get_audio_trace` or `cogmaster_reset` before a successful `cogmaster_load` |
| `COGMASTER_STATUS_ERR_PROFILE_REJECTED` | 6 | `cogmaster_media.machine`, `region` or `tv` is not one of its `COGMASTER_MACHINE_*`, `COGMASTER_REGION_*` or `COGMASTER_TV_*` values, or the pair was never made: `COGMASTER_TV_PAL` with `COGMASTER_MACHINE_GG` or with `COGMASTER_REGION_JAPAN` |
| `COGMASTER_STATUS_ERR_UNSUPPORTED_SIZE` | 7 | an image is below 1,024 bytes, or its body after any copier header is above 1 MiB; checked before every other load rule |
| `COGMASTER_STATUS_ERR_SAVE_SIZE` | 9 | raw save exceeds the classified persistent length |
| `COGMASTER_STATUS_ERR_UNSUPPORTED_BOARD` | 8 | the image is on a cartridge board the library does not emulate, or is an 8 KiB Game Gear image (the X-Terminator cheat device) |

| Function | Can return |
|---|---|
| `cogmaster_get_versions(out)` | OK, ERR_ARGUMENT, ERR_STRUCT_SIZE |
| `cogmaster_create(cfg, out)` | OK, ERR_ARGUMENT (`trace_records` above 1,048,576 included), ERR_STRUCT_SIZE, ERR_NO_MEMORY |
| `cogmaster_destroy(m)` | nothing; NULL is accepted |
| `cogmaster_load(m, media)` | OK, ERR_ARGUMENT, ERR_STRUCT_SIZE, ERR_NO_MEMORY, ERR_PROFILE_REJECTED, ERR_UNSUPPORTED_SIZE, ERR_UNSUPPORTED_BOARD |
| `cogmaster_run_frame(m, out)` | OK, ERR_ARGUMENT, ERR_STRUCT_SIZE, ERR_NO_CONTENT |
| `cogmaster_get_frame(m, io)` | OK, ERR_ARGUMENT, ERR_STRUCT_SIZE, ERR_NO_CONTENT, ERR_BUFFER_TOO_SMALL |
| `cogmaster_get_audio(m, io)` | OK, ERR_ARGUMENT, ERR_STRUCT_SIZE, ERR_NO_CONTENT |
| `cogmaster_get_audio_trace(m, io)` | OK, ERR_ARGUMENT, ERR_STRUCT_SIZE, ERR_NO_CONTENT |
| `cogmaster_get_info(m, out)` | OK, ERR_ARGUMENT, ERR_STRUCT_SIZE, ERR_NO_CONTENT |
| `cogmaster_reset(m, kind)` | OK, ERR_ARGUMENT, ERR_NO_CONTENT |
| `cogmaster_get_fault(m, out)` | OK, ERR_ARGUMENT, ERR_STRUCT_SIZE |
| `cogmaster_probe(image, n, out)` | OK, ERR_ARGUMENT, ERR_STRUCT_SIZE, ERR_UNSUPPORTED_SIZE, ERR_UNSUPPORTED_BOARD |
| `cogmaster_peek(m, space, addr)` | a byte, 0 to 255, or a value above 255 (no status) |

## Version numbers

`COGMASTER_VERSION_MAJOR`, `COGMASTER_VERSION_MINOR` and `COGMASTER_VERSION_PATCH` are the release the header
came from. `COGMASTER_API_VERSION`, `COGMASTER_ABI_VERSION`, `COGMASTER_BEHAVIOUR_REVISION` and
`COGMASTER_STATE_FORMAT_VERSION` are the numbers the program was compiled against. At run time,
`cogmaster_get_versions` fills a `cogmaster_versions` with the library's own:

| Field | Meaning |
|---|---|
| `library_major`, `library_minor`, `library_patch` | The release. `library_major` is 0 before version 1. |
| `api` | Rises with every addition to the header; never falls. Refuse a library whose `api` is below the `COGMASTER_API_VERSION` you compiled with. |
| `abi` | Changes only when a released signature, layout or value changes; appending to a size-tagged struct is not such a change. |
| `behaviour` | Changes when the same inputs give a different frame, audio or state hash. |
| `state` | The save-state byte layout. |

`api`, `abi`, `behaviour` and `state` start at 1, so zero means absent: a field that a library older than your
header does not know stays at the zero you wrote.

## Names

Every public name, for reference: the opaque instance `cogmaster`; the types `cogmaster_status`,
`cogmaster_allocator` (`alloc`, `free`, `user`), `cogmaster_config` (`size`, `allocator`, `trace_records`, `reserved`), `cogmaster_versions`
(`size`, `library_major`, `library_minor`, `library_patch`, `api`, `abi`, `behaviour`, `state`),
`cogmaster_media` (`size`, `image`, `image_size`, `machine`, `region`, `tv`, `video_mode`, `reserved2`, `fm`), `cogmaster_info` (`size`, `machine`,
`crop_w`, `crop_h`, `field_w`, `field_h`, `max_w`, `max_h`, `fps_num`, `fps_den`, `par_num`, `par_den`, `region`,
`tv`, `video_mode`, `audio_channels`, `fm`), `cogmaster_frame`
(`size`, `pixels`, `pitch_bytes`, `flags`, `x`, `y`, `w`, `h`, `field_h`, `backdrop_rgb444`),
`cogmaster_audio` (`size`, `samples`, `capacity`, `written`, `channels`, `dropped`),
`cogmaster_audio_record` (`tick`, `source`, `level`), `cogmaster_audio_trace` (`size`, `records`, `capacity`,
`written`, `dropped`) and `cogmaster_run_result` (`size`, `stop`, `overshoot_ticks`) and `cogmaster_fault` (`size`, `present`, `frame`,
`line`, `dot`, `pc`, `addr`, `opcode`, `width`, `device`, `kind`) and `cogmaster_cart_info` (`size`,
`image_size`, `body_offset`, `header_at`, `checksum`, `checksum_ok`, `product_code`, `version`, `region_code`,
`rom_size_code`, `machine_guess`, `region_guess`, `board`, `identified`, `public_identity`, `persistence`,
`persistent_size`, `persistence_geometry`, `video_mode`, `sha1`, `title`, `board_source`, `board_candidate`, `scan_sega`,
`scan_candidate`, `scan_runner_up`, `unsupported_id`, `unsupported_reason`); the functions `cogmaster_get_versions`,
`cogmaster_create`, `cogmaster_destroy`, `cogmaster_load`, `cogmaster_run_frame`, `cogmaster_get_frame`,
`cogmaster_get_info`, `cogmaster_get_audio`, `cogmaster_get_audio_trace`, `cogmaster_reset`, `cogmaster_get_fault`, `cogmaster_probe` and `cogmaster_peek`; the constants `COGMASTER_MACHINE_AUTO`,
`COGMASTER_MACHINE_SMS2`, `COGMASTER_MACHINE_GG`, `COGMASTER_REGION_AUTO`, `COGMASTER_REGION_EXPORT`,
`COGMASTER_REGION_JAPAN`, `COGMASTER_FM_AUTO`, `COGMASTER_FM_OFF`, `COGMASTER_FM_ON`, `COGMASTER_TV_AUTO`, `COGMASTER_TV_NTSC`, `COGMASTER_TV_PAL`, `COGMASTER_STOP_FRAME`, `COGMASTER_FRAME_FULL_FIELD`,
`COGMASTER_RESET_SOFT`, `COGMASTER_RESET_POWER`, `COGMASTER_BOARD_ROM_ONLY`, `COGMASTER_BOARD_SEGA`,
`COGMASTER_BOARD_CODEMASTERS`, `COGMASTER_SPACE_BUS`, the six `COGMASTER_FAULT_KIND_*` and seven `COGMASTER_FAULT_DEVICE_*` values
above, the nine `COGMASTER_STATUS_*` values above and the
version macros above.

Video queries describe the same completed image, with initial black 192-line
geometry before the first completion. Height writes latch at final-line tick
600 for the next field; writes at the tick miss that latch. The completed
image remains stable while a following raster is in progress. SMS PAL pixel
aspect is 2950000:2128137; NTSC is 8:7. See [video](hardware/video.md) for
the selected latch model, extended name tables and fixed host deadlines.

The libretro adapter reports the completed field geometry and notifies changes during
`retro_run`, continuing delivery if the frontend rejects the notification. Conversion
storage is fixed at load for 256×240. Game Gear defaults to the native 160×144 LCD
crop with 6:5 pixels (4:3 display); the square-pixel option gives 10:9.
The full-field option reveals normally hidden VDP content using the selected pixel
aspect without changing native emulation or its hashes. These options take effect
on the next frame. Machine, region/timing and Game Gear mode options require content
reload. SMS PAL uses PAR 2950000:2128137. The adapter drains `cogmaster_get_audio`
every frame before the video work and keeps no pacing state of its own.
