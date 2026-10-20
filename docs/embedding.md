# Embedding the library

The library is one static archive and one header, `cogmaster.h`, written in C17. It allocates memory only through
the allocator you pass, keeps all state in the instance, starts no threads, opens no files, reads no clock and uses
no floating point. It needs nothing from the C library beyond `memcpy`, `memmove`, `memset` and `memcmp`.

This release loads a cartridge image and draws its Mode 4 background. With no image it runs the built-in test card.

`cogmaster_set_input` accepts a size-tagged `cogmaster_input` with `size`, `p1`, `p2`, `pause`, `reset`,
`start`, `port1` and `port2`. It validates every field before replacing pending input. The two pad masks
contain six independent bits (Up, Down, Left, Right, button 1, button 2). Console levels are binary.
`COGMASTER_DEVICE_PAD` is zero, the default; `COGMASTER_DEVICE_NONE` is one and releases that port's pins.
The native Game Gear has one local pad. Input latches before the next frame runs; identical submissions
remain held levels. Pause queues an NMI only on a rising edge. A disconnected port releases its pins;
reconnecting with Pause held requires release before another press can queue an NMI. Reset drives a
level only on export SMS1; native GG Start drives its active-low level on `$00`. Opposing directions
pass through independently. TR output directions override pad levels, with Japanese output echo low.
The first input struct size is 32 bytes. Oversized zero tails follow the general size-tag rule.

The appended `cogmaster_run_result.lagged` is one when no decoded controller read occurred in that frame.
Legacy 12-byte callers receive only their original fields. The runner submits input before each frame and
receives this result. Host peeks never earn lag credit.

The libretro adapter accepts `game.data` and `game.size` in memory, advertises
`sms|gg|bin|rom`, and keeps `need_fullpath` false. One input poll precedes each
frame; D-pad/B/A map to the six pad bits. Port-1 Start maps to SMS Pause or native
GG Start, and Select drives SMS Reset. Pad/none and button descriptors are sent
to the frontend. Missing callbacks and none devices submit released levels.
Their reconnection follows the same Pause arming rule as the library. Port 2 never supplies console
controls. RetroArch Restart calls SOFT on SMS and POWER on GG; Select changes only the supported physical
Reset level and cannot restart the CPU. Both restart kinds preserve cartridge RAM.
The experimental flag remains enabled while the background tracer is expanded.
API version 4 appends SMS1 and unsupported-video fault constants; all earlier
values and struct layouts are retained. Behaviour revision 10 adds classified battery write generations.
The input model includes pin-output mixing, export-SMS1-only Reset and held-reconnect Pause suppression. The renderer includes independent line
IRQs, six mode/TV V-counter sequences, the translated H coordinate and TH capture
to the sprite/background semantics. State format 10 saves battery generations and native LCD and palette-latch state,
completed pixels/height/backdrop alongside the line counter
and pending source, rolling NTSC-240 phase and held H value alongside the sprite
slots, dot cursor, scroll/display latches and last driven bus byte, so older state
bytes are refused. API version 6 appends the battery query; existing ABI layouts remain unchanged.

## Getting it

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
`cogmaster_cart_info`):

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
media.image = rom;                        /* the image bytes, or NULL for the test card */
media.image_size = rom_size;              /* 0 with a NULL image */
cogmaster_load(m, &media);

cogmaster_run_frame(m, NULL);             /* one frame; the result struct is optional */
```

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
be 1,024 to 1,048,576 + 512 bytes; any other size returns `COGMASTER_STATUS_ERR_UNSUPPORTED_SIZE`, and a NULL
`image` with a non-zero size returns `COGMASTER_STATUS_ERR_ARGUMENT`. When the size is 512 bytes past a multiple
of 16 KiB the first 512 bytes are a copier's header and are skipped. The library copies the rest into one buffer
from your allocator, padded with `$FF` to a multiple of 16 KiB, so you may free your copy once `load` returns.
The library guesses the cartridge's board from the image (see [Probing an image](#probing-an-image)) and maps it
at `$0000`-`$BFFF`, with 8 KiB of work RAM at `$C000`; an image on no supported board returns
`COGMASTER_STATUS_ERR_UNSUPPORTED_BOARD`. The CPU starts as the console's BIOS leaves it (fast boot; no BIOS code runs): stack pointer `$DFF0`,
`$AB` at `$C000` on the Master System, program counter 0. `machine` selects the profile: `COGMASTER_MACHINE_AUTO` (the default, a Master
System II), `COGMASTER_MACHINE_SMS2`, `COGMASTER_MACHINE_GG` (2), or
`COGMASTER_MACHINE_SMS1` (3, the 315-5124 VDP). SMS1 uses the console boot and
port rules with address masks described in [video](hardware/video.md).
`region` selects the console's region:
`COGMASTER_REGION_AUTO` (the default, export), `COGMASTER_REGION_EXPORT` or `COGMASTER_REGION_JAPAN`. `tv` selects
the video standard: `COGMASTER_TV_AUTO` (the default, NTSC), `COGMASTER_TV_NTSC` or `COGMASTER_TV_PAL`. No Game
Gear and no Japanese console was made for PAL, so `COGMASTER_TV_PAL` with either returns
`COGMASTER_STATUS_ERR_PROFILE_REJECTED`. Each
`load` restarts the frame count and clears the fault record.

`cogmaster_run_frame` runs one frame: 262 lines, 179,208 ticks of the 10,738,635 Hz video clock, on NTSC, and
313 lines, 214,092 ticks of the 10,640,684.8 Hz (53,203,424 / 5) clock, on PAL. The CPU runs whole instructions,
so a frame stops at the first instruction boundary at or after its end, at most 69 ticks late, and the next frame
ends one frame length after this one should have: the frames never drift. If `out`, a
`cogmaster_run_result`, is not NULL it receives `stop`, always `COGMASTER_STOP_FRAME` in this release, and
`overshoot_ticks`, the ticks run past the frame's end (always 0 for the test card).

The Master System's Pause button reaches the CPU as on the console: a press raises the CPU's non-maskable interrupt
at the start of the frame's last line, tick 611 of line 260 on NTSC and of line 311 on PAL, wherever in the frame it
was pressed. The Game Gear has no Pause button.

## Probing an image

`cogmaster_probe(image, n, out)` reads an image the way `load` does, without an instance: it fills a
`cogmaster_cart_info` and returns the status `load` would give for the image alone. Appended fields distinguish
`COGMASTER_PERSISTENCE_UNKNOWN`, `COGMASTER_PERSISTENCE_NONE` and `COGMASTER_PERSISTENCE_BATTERY`.
`persistent_size` is the declared extent, independent of mapper decoding. `persistence_geometry` is
`COGMASTER_GEOMETRY_NONE`, `COGMASTER_GEOMETRY_SUPPORTED` or `COGMASTER_GEOMETRY_UNSUPPORTED`.
Unknown/unsupported geometry provides no persistent span. `identified` means a whole-body SHA-1 match;
`public_identity` distinguishes public CC0 identities from owned fixture facts. `sha1` and `title` are
zero-terminated identity strings, empty on a miss. `video_mode` carries a verified GG SMS-mode fact.
Old 52-byte output tags leave the appended fields untouched. AUTO machine/region use table/header facts
before defaults; explicit choices win. These facts are independent of compatibility-ledger membership.
The probe reads only the `n` bytes
at `image`, never changes them, and gives the same answer for the same bytes. It returns
`COGMASTER_STATUS_ERR_ARGUMENT` for a NULL `image` or `out`, `COGMASTER_STATUS_ERR_UNSUPPORTED_SIZE` outside the
load bounds (with only `image_size` filled), and `COGMASTER_STATUS_ERR_UNSUPPORTED_BOARD` with `board` 0 and
every other field filled when no supported board fits.

| Field | Meaning |
|---|---|
| `image_size` | `n` |
| `body_offset` | 512 after a copier header, else 0 |
| `header_at` | where the ROM header was found: `0x7FF0`, `0x3FF0`, `0x1FF0`, or 0 for none |
| `checksum`, `checksum_ok` | the header's checksum, and 1 when it matches the sum over the range its size code names |
| `product_code`, `version` | the product number (decimal) and the version nibble |
| `region_code`, `rom_size_code` | the header's region and size nibbles |
| `machine_guess`, `region_guess` | the machine and region the region code names (`COGMASTER_MACHINE_*`, `COGMASTER_REGION_*`), or 0 |
| `board` | `COGMASTER_BOARD_ROM_ONLY` (1), `COGMASTER_BOARD_SEGA` (2) or `COGMASTER_BOARD_CODEMASTERS` (3); 0 for none |

Every header field is 0 when no header was found. The board is chosen in this order:

1. When the size is 512 bytes past a multiple of 16 KiB, the first 512 bytes are a copier's header and are skipped.
2. The ROM header is looked for at `$7FF0`, then `$3FF0`, then `$1FF0`. Its region code names the machine and
   region; its checksum is checked but a wrong one refuses nothing.
3. A Codemasters header at `$7FE0` (page count, BCD date, checksum and its complement, `$7FEF` zero) gives the
   Codemasters board.
4. An 8 KiB image whose header names the Game Gear is refused, and so is any 8 KiB image loaded as a Game Gear.
5. An image of at most 48 KiB has no mapper.
6. Otherwise the first 32 KiB are scanned for `LD (nn),A` stores. When stores to another board's registers
   (`$0002`-`$0004`, `$3FFE`, `$6000`, `$A000`) outnumber stores to the Sega registers (`$FFFC`-`$FFFF`), the
   image is refused; else it has the Sega mapper.

The boards:

- **No mapper.** The image at `$0000`, padded with `$FF` up to `$BFFF`. Writes to it change nothing.
- **Sega.** Writes to `$FFFD`, `$FFFE` and `$FFFF` select the 16 KiB bank in `$0000`, `$4000` and `$8000`; the
  first 1 KiB stays on bank 0. Bit 3 of `$FFFC` maps 16 KiB of cartridge RAM at `$8000` (bit 2 picks which of
  two banks); bits 1-0 add `$18`, `$10` or `$08` to later bank writes. Every register write also lands in work
  RAM, so `$DFFC`-`$DFFF` read them back. Fast boot leaves the banks at 0, 1 and 2.
- **Codemasters.** Writes to `$0000`, `$4000` and `$8000` select the bank in those slots; they start at 0, 1 and
  0. Bit 7 of the `$4000` write maps 8 KiB of cartridge RAM at `$A000`-`$BFFF`.

The image is padded with `$FF` to whole 16 KiB banks, and a bank number past the end selects the bank number
modulo the bank count. Cartridge RAM starts zeroed at load, is kept by both resets and is part of the saved state.

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

## Reset

`cogmaster_reset(m, kind)` restarts the loaded content without loading it again. Both kinds set the frame count
and the tick count to zero, put the beam at the frame origin (line 192, tick 607, where a frame ends) as `load`
does, start the CPU again from the fast-boot hand-off and clear the fault record. Every frame, the first after a
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
reset: an access to an unmapped address or port, a write to ROM, a mapper value out of range, or an undefined
opcode. Only the first is kept; later ones are ignored until the next load or reset.
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
| 1 | `COGMASTER_FAULT_KIND_UNMAPPED_READ` | a read nothing drives returns open bus: a port in `$00`–`$3F` other than the Game Gear's `$00`–`$06`, the cartridge slots with the cartridge disabled by port `$3E`, or work RAM disabled by port `$3E` |
| 2 | `COGMASTER_FAULT_KIND_UNMAPPED_WRITE` | a write nothing takes: on the Japanese console or the Game Gear, which decode every port address, a port in `$00`–`$3F` or `$80`–`$BF` that nothing latches, such as `$3E` on the Game Gear; the cartridge slots with the cartridge disabled by port `$3E`, which also hides `$FFFC`–`$FFFF` from its mapper; or work RAM disabled by port `$3E`, except a write to `$FFFC`–`$FFFF` the enabled cartridge takes |
| 3 | `COGMASTER_FAULT_KIND_ROM_WRITE` | a write to `$0000`–`$BFFF` that is neither a register of the cartridge's mapper nor cartridge RAM |
| 4 | `COGMASTER_FAULT_KIND_MAPPER_RANGE` | a bank number at or past the end of an image whose page count is not a power of two, or a write to `$FFFC` that sets bit 7, bit 4 or bits 1–0 |
| 5 | `COGMASTER_FAULT_KIND_ED_UNDEFINED` | the CPU ran an opcode after `$ED` that the Z80 does not define, as an 8 T-state no-operation; `opcode` is that second byte and `pc` the address of the `$ED` |
| 6 | `COGMASTER_FAULT_KIND_UNSUPPORTED_VIDEO_MODE` | an enabled display selects an unsupported TMS or invalid text mode; output is backdrop and frame delivery stays bounded |

| `device` | Constant | Meaning |
|---|---|---|
| 0 | `COGMASTER_FAULT_DEVICE_NONE` | no device answers the address |
| 1 | `COGMASTER_FAULT_DEVICE_CPU` | the CPU itself |
| 2 | `COGMASTER_FAULT_DEVICE_CART` | the cartridge |
| 3 | `COGMASTER_FAULT_DEVICE_RAM` | work RAM |
| 4 | `COGMASTER_FAULT_DEVICE_MAPPER` | the cartridge's mapper |
| 5 | `COGMASTER_FAULT_DEVICE_CTRL` | the memory and I/O control ports `$3E` and `$3F` |
| 6 | `COGMASTER_FAULT_DEVICE_VDP` | the video display processor |

`addr` is the full 16-bit bus address; for `IN A,(n)` and `OUT (n),A` that is A × 256 + n. Outside kind 5, `pc`
is the address of the instruction, its first `$DD` or `$FD` prefix included, and `opcode` its first byte after
those prefixes.

Things shipped games do are never faults, so the record stays free for the real one: reads of `$C0`–`$FF` while
port `$3E` has the I/O chip disabled, ports `$F0`–`$F2`, `$DE` and `$DF`, writes to the I/O chip's ports, the Game
Gear's link ports `$01`–`$05`, a bank number past the end of an image whose page count is a power of two (it
wraps), toggling port `$3E`, and undocumented CPU instructions. An undriven read returns `$FF` and counts in the
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
| `COGMASTER_STATUS_ERR_NO_CONTENT` | 5 | `cogmaster_run_frame`, `cogmaster_get_frame`, `cogmaster_get_info` or `cogmaster_reset` before a successful `cogmaster_load` |
| `COGMASTER_STATUS_ERR_PROFILE_REJECTED` | 6 | `cogmaster_media.machine`, `region` or `tv` is not one of its `COGMASTER_MACHINE_*`, `COGMASTER_REGION_*` or `COGMASTER_TV_*` values, or the pair was never made: `COGMASTER_TV_PAL` with `COGMASTER_MACHINE_GG` or with `COGMASTER_REGION_JAPAN` |
| `COGMASTER_STATUS_ERR_UNSUPPORTED_SIZE` | 7 | an image is below 1,024 or above 1,048,576 + 512 bytes; checked before every other load rule |
| `COGMASTER_STATUS_ERR_SAVE_SIZE` | 9 | raw save exceeds the classified persistent length |
| `COGMASTER_STATUS_ERR_UNSUPPORTED_BOARD` | 8 | the image is on a cartridge board the library does not emulate, or is an 8 KiB Game Gear image (the X-Terminator cheat device) |

| Function | Can return |
|---|---|
| `cogmaster_get_versions(out)` | OK, ERR_ARGUMENT, ERR_STRUCT_SIZE |
| `cogmaster_create(cfg, out)` | OK, ERR_ARGUMENT, ERR_STRUCT_SIZE, ERR_NO_MEMORY |
| `cogmaster_destroy(m)` | nothing; NULL is accepted |
| `cogmaster_load(m, media)` | OK, ERR_ARGUMENT, ERR_STRUCT_SIZE, ERR_NO_MEMORY, ERR_PROFILE_REJECTED, ERR_UNSUPPORTED_SIZE, ERR_UNSUPPORTED_BOARD |
| `cogmaster_run_frame(m, out)` | OK, ERR_ARGUMENT, ERR_STRUCT_SIZE, ERR_NO_CONTENT |
| `cogmaster_get_frame(m, io)` | OK, ERR_ARGUMENT, ERR_STRUCT_SIZE, ERR_NO_CONTENT, ERR_BUFFER_TOO_SMALL |
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
`cogmaster_allocator` (`alloc`, `free`, `user`), `cogmaster_config` (`size`, `allocator`), `cogmaster_versions`
(`size`, `library_major`, `library_minor`, `library_patch`, `api`, `abi`, `behaviour`, `state`),
`cogmaster_media` (`size`, `image`, `image_size`, `machine`, `region`, `tv`), `cogmaster_info` (`size`, `machine`,
`crop_w`, `crop_h`, `field_w`, `field_h`, `max_w`, `max_h`, `fps_num`, `fps_den`, `par_num`, `par_den`, `region`,
`tv`), `cogmaster_frame`
(`size`, `pixels`, `pitch_bytes`, `flags`, `x`, `y`, `w`, `h`, `field_h`, `backdrop_rgb444`) and
`cogmaster_run_result` (`size`, `stop`, `overshoot_ticks`) and `cogmaster_fault` (`size`, `present`, `frame`,
`line`, `dot`, `pc`, `addr`, `opcode`, `width`, `device`, `kind`) and `cogmaster_cart_info` (`size`,
`image_size`, `body_offset`, `header_at`, `checksum`, `checksum_ok`, `product_code`, `version`, `region_code`,
`rom_size_code`, `machine_guess`, `region_guess`, `board`, `identified`, `public_identity`, `persistence`,
`persistent_size`, `persistence_geometry`, `video_mode`, `sha1`, `title`); the functions `cogmaster_get_versions`,
`cogmaster_create`, `cogmaster_destroy`, `cogmaster_load`, `cogmaster_run_frame`, `cogmaster_get_frame`,
`cogmaster_get_info`, `cogmaster_reset`, `cogmaster_get_fault`, `cogmaster_probe` and `cogmaster_peek`; the constants `COGMASTER_MACHINE_AUTO`,
`COGMASTER_MACHINE_SMS2`, `COGMASTER_MACHINE_GG`, `COGMASTER_REGION_AUTO`, `COGMASTER_REGION_EXPORT`,
`COGMASTER_REGION_JAPAN`, `COGMASTER_TV_AUTO`, `COGMASTER_TV_NTSC`, `COGMASTER_TV_PAL`, `COGMASTER_STOP_FRAME`, `COGMASTER_FRAME_FULL_FIELD`,
`COGMASTER_RESET_SOFT`, `COGMASTER_RESET_POWER`, `COGMASTER_BOARD_ROM_ONLY`, `COGMASTER_BOARD_SEGA`,
`COGMASTER_BOARD_CODEMASTERS`, `COGMASTER_SPACE_BUS`, the five `COGMASTER_FAULT_KIND_*` and six `COGMASTER_FAULT_DEVICE_*` values
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
reload. SMS PAL uses PAR 2950000:2128137 and exact rational silence pacing (965 or
966 stereo frames at 48 kHz); NTSC delivers 801 or 802.

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
