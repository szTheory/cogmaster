# Embedding the library

The library is one static archive and one header, `cogmaster.h`, written in C17. It allocates memory only through
the allocator you pass, keeps all state in the instance, starts no threads, opens no files, reads no clock and uses
no floating point. It needs nothing from the C library beyond `memcpy`, `memmove`, `memset` and `memcmp`.

This release loads a cartridge image and runs its CPU, but draws no picture from it yet: the frame of a loaded
image stays black. With no image it runs the built-in test card.

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

**Output structs** (`cogmaster_info`, `cogmaster_versions`, `cogmaster_run_result`, `cogmaster_fault`):

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
media.image = rom;                        /* the image bytes, or NULL for the test card */
media.image_size = rom_size;              /* 0 with a NULL image */
cogmaster_load(m, &media);

cogmaster_run_frame(m, NULL);             /* one frame; the result struct is optional */
```

`cogmaster_media.image` with `image_size` 0 and a NULL `image` loads the built-in test card. Any other image must
be 1,024 to 1,048,576 + 512 bytes; any other size returns `COGMASTER_STATUS_ERR_UNSUPPORTED_SIZE`, and a NULL
`image` with a non-zero size returns `COGMASTER_STATUS_ERR_ARGUMENT`. When the size is 512 bytes past a multiple
of 16 KiB the first 512 bytes are a copier's header and are skipped. The library copies the rest into one buffer
from your allocator, padded with `$FF` to a multiple of 16 KiB, so you may free your copy once `load` returns.
This release maps every image as a cartridge without a mapper: the first 48 KiB at `$0000` and 8 KiB of work RAM
at `$C000`. The CPU starts as the console's BIOS leaves it (fast boot; no BIOS code runs): stack pointer `$DFF0`,
`$AB` at `$C000` on the Master System, program counter 0. `machine` selects the profile: `COGMASTER_MACHINE_AUTO` (the default, a Master
System II), `COGMASTER_MACHINE_SMS2` or `COGMASTER_MACHINE_GG`. `region` selects the console's region:
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

By default `cogmaster_get_frame` copies the machine's crop: the whole 256 × 192 field on the Master System, the
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

- `COGMASTER_RESET_SOFT` (0) is the console's reset: memory the machine keeps across a reset, such as work RAM,
  stays as it was.
- `COGMASTER_RESET_POWER` (1) is a power cycle: it also zeroes the internal counters and, on machines that have
  them, work RAM, video RAM and colour RAM.

The test card has no RAM, video RAM or colour RAM and its counters stay zero, so the two kinds have the same effect
on it. Any other `kind` returns `COGMASTER_STATUS_ERR_ARGUMENT`; a reset before a successful
`cogmaster_load` returns `COGMASTER_STATUS_ERR_NO_CONTENT`.

## Faults

`cogmaster_get_fault` fills a `cogmaster_fault` with the first fault the core recorded since the last load or
reset: an access to an unmapped address or port, an illegal opcode, a mapper write out of range, or a video write
faster than the hardware accepts. Only the first is kept; later ones are ignored until the next load or reset.
The record is part of the instance's saved state.

| Field | Meaning |
|---|---|
| `present` | 1 when a fault was recorded, 0 otherwise; every other field is 0 when it is 0 |
| `frame`, `line`, `dot` | where the beam was: the frame count, the line (0 to 312) and the dot (0 to 341) |
| `pc` | the CPU's program counter at the faulting instruction |
| `addr`, `width` | the full 16-bit address or port accessed and the access width in bits (8) |
| `opcode` | the opcode byte being executed |
| `device`, `kind` | which device faulted and how, as the codes below |

`kind`: 0 none, 1 unmapped read, 2 unmapped write, 3 ROM write, 4 mapper range, 5 undefined ED opcode. `device`:
0 none, 1 CPU, 2 cartridge, 3 work RAM, 4 mapper, 5 memory and I/O control. Both lists only grow. This release
records kind 5: the CPU ran an opcode after `$ED` that the Z80 does not define, as an 8 T-state no-operation;
`opcode` is that second byte, `pc` the address of the `$ED`. The test card runs no CPU, so it never records a
fault. The call works before a load too, and then reports `present` 0.

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
| `COGMASTER_STATUS_ERR_ARGUMENT` | 1 | a required pointer is NULL; the allocator lacks `alloc` or `free`; `image` is NULL while `image_size` is not 0; `flags` has an unknown bit; `pitch_bytes` is above 65536; the reset `kind` is unknown |
| `COGMASTER_STATUS_ERR_STRUCT_SIZE` | 2 | a size tag breaks the rules in [Structs and sizes](#structs-and-sizes) |
| `COGMASTER_STATUS_ERR_NO_MEMORY` | 3 | `alloc` returned NULL in `cogmaster_create` (`*out` is left as it was) or in `cogmaster_load` |
| `COGMASTER_STATUS_ERR_BUFFER_TOO_SMALL` | 4 | `pitch_bytes` is below `w × 2` for the rectangle requested |
| `COGMASTER_STATUS_ERR_NO_CONTENT` | 5 | `cogmaster_run_frame`, `cogmaster_get_frame`, `cogmaster_get_info` or `cogmaster_reset` before a successful `cogmaster_load` |
| `COGMASTER_STATUS_ERR_PROFILE_REJECTED` | 6 | `cogmaster_media.machine`, `region` or `tv` is not one of its `COGMASTER_MACHINE_*`, `COGMASTER_REGION_*` or `COGMASTER_TV_*` values, or the pair was never made: `COGMASTER_TV_PAL` with `COGMASTER_MACHINE_GG` or with `COGMASTER_REGION_JAPAN` |
| `COGMASTER_STATUS_ERR_UNSUPPORTED_SIZE` | 7 | an image is below 1,024 or above 1,048,576 + 512 bytes; checked before every other load rule |

| Function | Can return |
|---|---|
| `cogmaster_get_versions(out)` | OK, ERR_ARGUMENT, ERR_STRUCT_SIZE |
| `cogmaster_create(cfg, out)` | OK, ERR_ARGUMENT, ERR_STRUCT_SIZE, ERR_NO_MEMORY |
| `cogmaster_destroy(m)` | nothing; NULL is accepted |
| `cogmaster_load(m, media)` | OK, ERR_ARGUMENT, ERR_STRUCT_SIZE, ERR_NO_MEMORY, ERR_PROFILE_REJECTED, ERR_UNSUPPORTED_SIZE |
| `cogmaster_run_frame(m, out)` | OK, ERR_ARGUMENT, ERR_STRUCT_SIZE, ERR_NO_CONTENT |
| `cogmaster_get_frame(m, io)` | OK, ERR_ARGUMENT, ERR_STRUCT_SIZE, ERR_NO_CONTENT, ERR_BUFFER_TOO_SMALL |
| `cogmaster_get_info(m, out)` | OK, ERR_ARGUMENT, ERR_STRUCT_SIZE, ERR_NO_CONTENT |
| `cogmaster_reset(m, kind)` | OK, ERR_ARGUMENT, ERR_NO_CONTENT |
| `cogmaster_get_fault(m, out)` | OK, ERR_ARGUMENT, ERR_STRUCT_SIZE |

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
`line`, `dot`, `pc`, `addr`, `opcode`, `width`, `device`, `kind`); the functions `cogmaster_get_versions`,
`cogmaster_create`, `cogmaster_destroy`, `cogmaster_load`, `cogmaster_run_frame`, `cogmaster_get_frame`,
`cogmaster_get_info`, `cogmaster_reset` and `cogmaster_get_fault`; the constants `COGMASTER_MACHINE_AUTO`,
`COGMASTER_MACHINE_SMS2`, `COGMASTER_MACHINE_GG`, `COGMASTER_REGION_AUTO`, `COGMASTER_REGION_EXPORT`,
`COGMASTER_REGION_JAPAN`, `COGMASTER_TV_AUTO`, `COGMASTER_TV_NTSC`, `COGMASTER_TV_PAL`, `COGMASTER_STOP_FRAME`, `COGMASTER_FRAME_FULL_FIELD`,
`COGMASTER_RESET_SOFT` and `COGMASTER_RESET_POWER`, the eight `COGMASTER_STATUS_*` values above and the
version macros above.
