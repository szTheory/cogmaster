# cogmaster-run

`cogmaster-run` is the headless runner. It runs the emulator for a fixed number of frames with no window, sound
device or input device. It can print a frame hash, write PNG images and report frame times. Scripts and CI use it
to check that a build produces the expected picture. It can also describe a cartridge file without running it
(see [Probe](#probe)).

With no file, every run shows the built-in test card (see [Test card](#test-card)). This release has no input,
sound or video chip drawing yet, so a cartridge runs but its picture stays black.

## Synopsis

```text
cogmaster-run [--machine NAME] [--region NAME] --frames N [OPTION]... [FILE]
cogmaster-run [--machine NAME] [--region NAME] --bench N [OPTION]... [FILE]
cogmaster-run --probe FILE
cogmaster-run --help | --version
```

## Options

| Option | Meaning |
|---|---|
| `FILE`, `--rom FILE` | The cartridge image to run: a Master System or Game Gear ROM from 1,024 bytes to 1 MiB, optionally after a 512-byte copier header. Give one file. Without one, the test card shows. |
| `--machine NAME` | `sms` for the Master System, `gg` for the Game Gear. `auto`, the default, reads the cartridge's header: region codes 5 to 7 are the Game Gear and 3 or 4 the Master System. With no header, a file name ending in `.gg` is the Game Gear and anything else the Master System. With no file, `auto` is the Master System. |
| `--region NAME` | The console's region and TV standard together: `ntsc-u` is an export console on NTSC, `ntsc-j` a Japanese console on NTSC and `pal` an export console on PAL. `auto`, the default, is `ntsc-u`. |
| `--frames N` | Run N frames, N from 1 to 4294967295. Give either `--frames` or `--bench`, once. |
| `--bench N` | Run N frames, time each one and print the report described under [Bench report](#bench-report). N is from 1 to 1073741823. The runner keeps 4 bytes per frame, so a very large N can exit 1 because memory ran out. |
| `--hash` | After the last frame, print the SHA-256 of its picture. |
| `--png N:FILE` | After frame N, write its picture to FILE as a PNG. N is from 1 to the frame count. Repeat the option to write more frames or files. |
| `--trace first-fault` | After each frame, read the core's first-fault record; stop at the first fault and print it, described under [First fault](#first-fault). |
| `--probe` | Print the file's header fields and digests, described under [Probe](#probe), and run nothing. Give it only with a file. |
| `--help` | Print the usage text and the exit statuses. Give it alone. |
| `--version` | Print `cogmaster-run` and the version, for example `cogmaster-run 1.2.3`. Give it alone. |

You can give options in any order. Anything else on the command line is a usage error and nothing runs. That
includes a second file and any option this release does not have.

## Refusals

The runner exits 77 and runs nothing when it cannot run what was asked:

- a file larger than 64 MiB, refused before any of it is read;
- an image smaller than 1,024 bytes or larger than 1 MiB plus a 512-byte copier header;
- a cartridge whose board (its bank-switching hardware) is not supported: the core runs cartridges with no mapper,
  the Sega mapper and the Codemasters mapper;
- an 8 KiB image on the Game Gear, which is the X-Terminator cheat device rather than a game;
- a machine and region no console had: PAL with the Game Gear or with `ntsc-j`'s Japanese console;
- a file whose name ends in `.sg` or `.sc`, refused unread with `unsupported: SG-1000 (planned)`.

A file that cannot be read exits 1.

## Frame hash

`--hash` prints one line after the last frame:

```text
frame 60 sha256 e14d77ce564f0b1498f703134e82e6c1fc718a27c94e49b5a35b95a981857ae7
```

The hash is SHA-256 over the visible picture only: 256×192 on the Master System and 160×144 on the Game Gear. It
reads the rows from top to bottom. Each pixel is an RGB444 value (`0000BBBBGGGGRRRR`), stored as two bytes with
the low byte first. The hash is therefore the same on little-endian and big-endian hosts. The test card does not
change from frame to frame, so `--frames 1 --hash` and `--frames 60 --hash` print the same digest.

## PNG output

`--png N:FILE` writes the visible picture unscaled as an RGB PNG with 8 bits per channel. Each RGB444 channel is
multiplied by 0x11, so 0x0 becomes 0x00 and 0xF becomes 0xFF, and nothing is lost. The data is stored
uncompressed and the file carries no aspect-ratio metadata. The runner builds the whole file in memory after frame
N and writes it in one call. If the file cannot be written in full, it is removed and the runner exits 1.

This PNG is the reference for screenshot comparisons: it is the card exactly as the core produces it.

## Probe

`--probe FILE` prints one `key=value` line per key, always every key and always in this order. Later releases only
add keys at the end. Integers are decimal and digests lowercase hexadecimal.

| Key | Value |
|---|---|
| `size` | the file's size in bytes |
| `copier_header` | 1 when the size is 512 past a multiple of 16 KiB, so a copier wrote a 512-byte header in front, else 0 |
| `header_at` | where the ROM header sits in the image after any copier header: `$7FF0`, `$3FF0`, `$1FF0`, or `none` |
| `header_magic` | 1 when a ROM header was found, else 0 |
| `checksum` | the checksum word the header stores |
| `checksum_ok` | 1 when it equals the sum the export console's BIOS computes over the range the size code names, else 0 |
| `product_code` | the header's product code |
| `version` | the header's version number |
| `region_code` | 3 Master System Japan, 4 Master System export, 5 Game Gear Japan, 6 Game Gear export, 7 Game Gear international |
| `rom_size_code` | the header's size code, 0 to 15 |
| `machine_guess` | `sms` or `gg`: the machine `--machine auto` runs the file on |
| `region_guess` | `ntsc-j` for a Japanese region code, else `ntsc-u` |
| `mapper_guess` | `none` (no mapper, 48 KiB or less), `sega`, `codemasters`, or `unsupported` for an image the core will not load |
| `sha1`, `sha256`, `crc32`, `md5` | digests of the whole file |
| `sha1_headerless`, `sha256_headerless`, `crc32_headerless`, `md5_headerless` | the same digests of the file after its copier header, or `none` when it has none |

Each header field from `checksum` to `rom_size_code` is `none` when the image has no ROM header. `md5` is the
RetroAchievements hash for both machines: the MD5 of the whole file, copier header included. `crc32` is the value
RetroArch's playlist scanner matches.

`--probe` runs no emulation. It exits 0, or 77 after printing every key for an image the core will not load
because of its size or its board.

## First fault

`--trace first-fault` checks after each frame whether the guest has done something no hardware answers. At the
first such fault the run stops after that frame and prints one line:

```text
fault frame=0 line=192 dot=329 pc=$0002 op=$DB addr=$0000 width=8 dev=ctrl kind=unmapped_read
```

With no fault by the last frame it prints `fault none`. Either way the exit status is 0: the fault is data about
the guest, not a failure of the run. `--hash` and `--bench` output follows the line, for the frames that ran.

| Field | Value |
|---|---|
| `frame` | the frame the fault happened in, counted from 0 at load |
| `line`, `dot` | the beam position: line and dot within the frame |
| `pc` | the address of the instruction, including its first `$DD` or `$FD` prefix |
| `op` | the instruction's first byte after any prefix; for `ed_undefined`, the byte after `$ED` |
| `addr` | the full 16-bit bus address; for `IN` and `OUT` with an immediate port, A × 256 + the port |
| `width` | the access width in bits, 8 |
| `dev` | `none`, `cpu`, `cart`, `ram`, `mapper` or `ctrl` (the memory and I/O control ports `$3E` and `$3F`) |
| `kind` | `unmapped_read`, `unmapped_write`, `rom_write`, `mapper_range` or `ed_undefined` |

[Embedding](embedding.md) says when each kind is recorded and which guest behaviour is never a fault. Later
releases add kinds and devices; scripts should accept names they do not know.

## Bench report

`--bench N` prints nine `key value` lines after the run:

```text
frames 300
p50_us 71
p90_us 84
p99_us 95
p99.9_us 138
p99.99_us 138
p99.999_us 138
max_us 138
max_rss_kib 1584
```

| Key | Value |
|---|---|
| `frames` | the number of frames timed |
| `p50_us` … `p99.999_us` | microseconds per frame at the 50th, 90th, 99th, 99.9th, 99.99th and 99.999th percentile |
| `max_us` | the slowest frame, in microseconds |
| `max_rss_kib` | the runner's peak resident memory, in KiB |

Only the time spent running each frame is measured. Hashing and writing files are not. The runner reads the host's
monotonic clock; the core never reads a clock.

Each percentile is a nearest-rank sample with no interpolation. With n frames, the p-th percentile is the frame
time at rank ⌈n × p⌉ in ascending order, so every value is a time that was actually measured. With one frame,
every line shows that frame's time. With fewer than 100,000 frames, p99.999 is the maximum. Microseconds are
whole numbers, truncated.

`max_rss_kib` is in KiB on every system, rounded up. Linux reports KiB directly. macOS reports bytes, which the
runner divides by 1024. Windows reports the peak working set in bytes. WASI has no way to report memory, so there
the line is `max_rss_kib 0`.

## Stopping

SIGINT (Ctrl+C) or SIGTERM ends the run after the frame in progress. Output is never cut off part-way through a
report. That frame's PNG is still written. `--hash` prints the last frame that ran, and `--bench` reports the
frames that ran. Output is flushed and the exit status is 0. A signal that was already ignored when the runner
started, as under `nohup`, stays ignored.

## Exit status

`cogmaster-run --help` prints this table:

```text
  0  ok       every frame ran or the probe printed, or a signal stopped the run after a whole frame
  1  failure  a file could not be read or written, memory ran out or the core failed
  2  usage    the command line is invalid, so nothing ran
  77  refused  the cartridge, its size or the machine and region is not supported, so nothing ran
```

Errors and usage messages go to standard error. Standard output holds only the results described above.

## Test card

With no cartridge loaded, every frame shows the same test card. Its elements are chosen so that a host showing
the picture wrongly fails visibly. The core draws the card from a fixed rule for each pixel, never through the
video chip model, so the card's hash stays the same as the emulation grows.

### Colours

All colours are RGB444 values, `0000BBBBGGGGRRRR`:

| Name | Value |
|---|---|
| white | `0x0FFF` |
| black | `0x0000` (also the backdrop) |
| red | `0x000F` |
| green | `0x00F0` |
| blue | `0x0F00` |
| magenta | `0x0F0F` (Game Gear surround only) |

### Master System card

The picture is 256×192, with x from 0 to 255 left to right and y from 0 to 191 top to bottom. For each pixel the
first rule that matches decides the colour:

1. **Corner block.** x < 8 and y < 8: white. An 8×8 block in the top-left corner only.
2. **Frame.** x = 0, x = 255, y = 0 or y = 191: white. A one-pixel border on every edge of the picture.
3. **Top band.** y < 32: red.
4. **Bottom band.** y ≥ 161: green.
5. **Checker.** x < 64: a one-pixel checkerboard, white where x + y is odd and black where it is even.
6. **Right band.** x ≥ 193: blue.
7. **Colour chart.** The rest, x 64 to 192 and y 32 to 160. Black one-pixel grid lines run every 16 pixels, at
   x = 64, 80, …, 192 and y = 32, 48, …, 160. The 8×8 cells between them are 15×15 swatches. The swatch in
   column c (0 to 7, left to right) and row r (0 to 7, top to bottom) has red level c mod 4, green level r mod 4,
   and blue level 2 × (r div 4) + (c div 4). Each level from 0 to 3 is widened to four bits by repeating it, so the
   levels are 0x0, 0x5, 0xA and 0xF. These are the console's 64 colours, each shown once.

### Game Gear card

The 160×144 picture sits at x 48 to 207 and y 24 to 167 of a 256×192 field. Coordinates below are inside the
picture: u = x − 48 from 0 to 159 and v = y − 24 from 0 to 143. The first rule that matches decides the colour:

0. **Surround.** Any field pixel outside the picture: magenta. A correct host never shows it.
1. **Corner block.** u < 8 and v < 8: white.
2. **Frame.** u = 0, u = 159, v = 0 or v = 143: white.
3. **Top band.** v < 8: red.
4. **Bottom band.** v ≥ 136: green.
5. **Checker.** u < 16: a one-pixel checkerboard, white where u + v is odd.
6. **Right band.** u ≥ 144: blue.
7. **Colour chart.** The rest, u 16 to 143 and v 8 to 135, is 64×64 cells of 2×2 pixels. With cell column
   cx = (u − 16) div 2 and cell row cy = (v − 8) div 2, the cell's red is cx mod 16 and its green is cy mod 16.
   Its blue is 4 × (cy div 16) + (cx div 16). Red rises along each row and green down each column. Blue selects
   one of 4×4 blocks of 16×16 cells. All 4,096 RGB444 colours appear, each once.

### What each element diagnoses

| Element | What a wrong picture shows | Fault |
|---|---|---|
| Corner block | The block appears in another corner | The picture is mirrored or flipped |
| Red top, blue right | Red at the bottom, or blue on the left | A flip, as above. Blue on top with red on the right is a red/blue channel swap instead |
| White frame | An edge of the border is missing or doubled | The host crops the picture or adds a line |
| Magenta surround | Magenta appears at an edge (Game Gear) | The crop origin, width or height is wrong |
| Edges and grid lines | Straight lines lean or step from row to row | The row pitch (bytes per row) is wrong |
| Checker | Grey, soft or uneven pixels | Filtering, or scaling by a factor that is not a whole number |
| Colour charts | Swatches or cells with the wrong or repeated colours | The bytes of each pixel are swapped, or a channel is truncated. Every value of every four-bit channel appears, so any byte-order mistake changes some colour |
