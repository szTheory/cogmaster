# cogmaster-run

`cogmaster-run` is the headless runner. It runs the emulator for a fixed number of frames with no window and no
input device, and plays no sound. It can print a frame hash, print hashes of the sound, write PNG images and report
frame times. Scripts and CI use it
to check that a build produces the expected picture. It can also describe a cartridge file without running it
(see [Probe](#probe)).

With no file, every run shows the built-in test card (see [Test card](#test-card)). Content uses the Mode 4
background and sprite renderer, and the sound chips run and are heard through `--hash-audio` (see [Sound
hashes](#sound-hashes)).

`--machine sms` keeps the SMS2 profile; `--machine sms1` selects SMS1.
`--video-mode auto|sms|gg` selects video independently of the machine.
AUTO uses native video on GG and SMS video on SMS. GG video on either SMS
profile exits 77; unknown mode names exit 2. The existing sms/gg defaults
and no-content card hashes are unchanged.

Native GG output is the 160×144 crop at (48,24), or (48,40) at 224 lines.
Its 240-line LCD freeze holds the last valid crop until POWER reset while
the full field continues rendering. Explicit SMS video on GG exposes the
full SMS field with NTSC PAR 8:7 and immediate six-bit palette writes;
physical LCD resampling is a host presentation choice. In that mode Start
press edges act as Pause. Native GG Start is the port-00 level. Embedders
can inspect the resolved machine and video mode through `cogmaster_get_info`.

`--input FILE` reads exactly one absolute record per requested frame, in this order:
`p1=00 p2=00 pause=0 reset=0 start=0`. Masks have two hexadecimal digits; bits 0–5 are Up, Down, Left,
Right, button 1 and button 2. Console values are 0 or 1. LF, CRLF and an optional final newline work.
Blank lines, comments, unknown or repeated fields, high mask bits, overlong lines, missing and surplus
records exit 2 before running. File errors exit 1. The reader uses fixed memory and validates the whole
body before execution. Without `--input`, all levels are released. `--port1` and `--port2` take `auto`, `pad`, `none`, `paddle`,
`sports` or `lphaser`; `none` releases that port's pins. Opposing directions remain independent.
With `--input`, a successful run appends `lag_frames=N` to standard output,
counting completed frames without a decoded guest controller read. Host polling, peeks, disabled and
undriven port reads do not remove lag. Early stops count only the frames actually completed.

A record may instead carry the four axis deltas of that frame, 75 bytes: the 35 bytes, then
` x1=+00005 y1=-00003 x2=+00000 y2=+00000`. Each field has its sign and exactly five digits, from -32767 to
+32767, and the order is fixed. A line is 35 or 75 bytes and nothing between; the 35-byte form means all four
axes are 0. Port 1 takes `x1` and `y1`, port 2 takes `x2` and `y2`. A Sports Pad takes both axes and a Paddle
takes `x` and ignores `y`. A Light Phaser takes an aim: `x` 1 to 256 and `y` 1 to 240 are the
field dot the gun points at, `0 0` or the 35-byte form is no aim, and bit 4 of `p1` (or `p2`) is the trigger. The
runner draws no crosshair and maps nothing. A value out of range, a malformed field, or a non-zero axis for a port whose
controller takes no motion (a pad or `none`), or an aim a phaser cannot have (a negative axis, `x` above 256, `y` above 240, or
only one of the pair 0), exits 2 before frame 1; the second case names the port and the
controller. Deltas apply once per frame, so the same digits in two records are two moves. The script's bytes
are in the digest the `--expect` line prints, so a delta script is told apart from the same buttons without
motion.

Record 1 applies before frame 1. For example, released, `p1=10`, `p1=10`, released means the first press
is visible to CPU reads in frame 2, remains held in frame 3, and releases in frame 4. All five fields are
required on every record. Pause is edge-triggered by the core; Reset and native Game Gear Start are levels.

## Synopsis

```text
cogmaster-run [--machine NAME] [--region NAME] --frames N [OPTION]... [FILE]
cogmaster-run [--machine NAME] [--region NAME] --bench N [OPTION]... [FILE]
cogmaster-run --diagnose [--machine NAME] [--region NAME] [OPTION]... FILE
cogmaster-run --probe FILE
cogmaster-run --help | --version
```

## Options

| Option | Meaning |
|---|---|
| `FILE`, `--rom FILE` | The cartridge image to run: a Master System or Game Gear ROM from 1,024 bytes to 1 MiB, optionally after a 512-byte copier header; a body above 1 MiB is refused. Give one file. Without one, the test card shows. |
| `--machine NAME` | `sms` selects SMS2, `sms1` selects SMS1, `gg` selects Game Gear. `auto` uses the public probe's hash metadata/header facts, then the `.gg` filename fallback. With no file, `auto` selects SMS2. |
| `--region NAME` | Region and TV standard together: `ntsc-u` is export NTSC, `ntsc-j` Japanese NTSC and `pal` export PAL. `auto` uses probe metadata/header region when present, otherwise export NTSC. |
| `--fm NAME` | `auto` (the default), `on` or `off`, given once, spelt exactly. `auto` turns the FM sound unit on for a Japanese Master System and off elsewhere. `on` with `--machine gg` exits 77, and a run option is a usage error with `--probe`. |
| `--video-mode NAME` | `auto`, `sms` or `gg`; video selection is independent of the machine profile. |
| `--boot NAME` | `fast` (the default) starts the cartridge as the BIOS would have left the console, with no BIOS. `bios` starts from the Master System BIOS named by `--bios`, runs it, and lets it start the cartridge. Give it once. It needs a cartridge file and is a usage error with `--probe` or `--expect`. A BIOS run starts with the BIOS's own time, so frame N of a BIOS run is not frame N of a fast run, and an `--input` script written for fast boot misaligns. Most real BIOS images check the cartridge's header, so a cartridge that runs under `fast` can lock up under `bios`. |
| `--bios FILE` | The BIOS dump to start from: exactly 8,192 bytes, from a Master System. Give it once, only with `--boot bios` (with `--boot fast` or alone it is a usage error, so adding a path never changes a hash). The runner reads at most 8,193 bytes of it. It prints one line on standard error: `BIOS recognised: NAME` when the file's SHA-1 and size are a known dump, else `BIOS not recognised (sha1 DIGEST); running it as given`. A dump of the right size that is not recognised still runs, so a patched dump works. Recognition names the file; it selects no machine, region or FM sound. The path and the bytes are never printed. |
| `--input FILE` | Exactly one absolute input record per requested frame, applied before that frame. |
| `--port1 NAME`, `--port2 NAME` | `auto` (the default), `pad`, `none`, `paddle`, `sports` or `lphaser`. `auto` takes the controller the loaded cartridge was made for, a pad for any other content. A `paddle` needs `--region ntsc-j` on a Master System, and `sports` needs a Master System. |
| `--frames N` | Run N frames, N from 1 to 4294967295. Give either `--frames` or `--bench`, once. |
| `--bench N` | Run N frames, time each one and print the report described under [Bench report](#bench-report). N is from 1 to 1073741823. The runner keeps 4 bytes per frame, so a very large N can exit 1 because memory ran out. |
| `--hash` | After the last frame, print the SHA-256 of its picture. |
| `--hash-audio` | After the last frame, print two lines hashing the sound's level changes and its samples, described under [Sound hashes](#sound-hashes). It does not need `--hash` and `--hash` does not need it. It is a usage error with `--bench` or `--probe`. |
| `--save-dir DIR` | Import and persist raw battery data in an existing directory. Requires content. |
| `--save-interval N` | Also write the save after every Nth frame, counted in frames and never in time. N is from 0 to 4294967295; 0 or no option is off. It needs `--save-dir`; a malformed N is a usage error. |
| `--png N:FILE` | After frame N, write its picture to FILE as a PNG. N is from 1 to the frame count. Repeat the option to write more frames or files. If the run stops before frame N, FILE is not written and a line on standard error names it. |
| `--trace first-fault` | After each frame, read the core's first-fault record; stop at the first fault and print it, described under [First fault](#first-fault). |
| `--eyes` | After the last frame, print one line `eyes frames=F none=N left=L right=R flips=X last=left\|right\|none`: how many frames reported each 3-D glasses eye, how many frames showed a non-zero eye different from the last non-zero one, and the last eye shown. It only counts: `--hash`, `--png` and `--expect` stay the plain picture and nothing is composed. It is a usage error with `--probe`. |
| `--mailbox` | After each frame, read the RAM mailbox a diagnostic ROM fills; stop at the first frame whose status reads done after reading running, and print the mailbox, described under [Mailbox](#mailbox). |
| `--diagnose` | Run 1800 frames and print one line of boot facts, described under [Diagnose](#diagnose). Give it only with a file and the profile options `--machine`, `--region`, `--fm`, `--video-mode`, `--port1` and `--port2`. |
| `--probe` | Print the file's header fields and digests, described under [Probe](#probe), and run nothing. Give it only with a file. |
| `--expect FILE` | Judge accepted bounded frame, RAM, mailbox and boot observations. |
| `--help` | Print the usage text and the exit statuses. Give it alone. |
| `--version` | Print `cogmaster-run` and the version, for example `cogmaster-run 1.2.3`. Give it alone. |

You can give options in any order. Anything else on the command line is a usage error and nothing runs. That
includes a second file and any option this release does not have.

## Refusals

The runner exits 77 and runs nothing when it cannot run what was asked:

- a file larger than 64 MiB, refused before any of it is read;
- an image smaller than 1,024 bytes, or whose body after any 512-byte copier header is larger than 1 MiB;
- a cartridge whose board (its bank-switching hardware) is not supported: the core runs cartridges with no mapper,
  the Sega mapper and the Codemasters mapper;
- an 8 KiB image on the Game Gear, which is the X-Terminator cheat device rather than a game;
- a machine and region no console had: PAL with the Game Gear or with `ntsc-j`'s Japanese console;
- a controller the machine and region cannot take: `--port1 paddle` without `--region ntsc-j`, or any `paddle`, `sports` or `lphaser` choice on the Game Gear; the message names the fix;
- a file whose name ends in `.sg` or `.sc`, refused unread with `unsupported: SG-1000 (planned)`;
- a BIOS asked for with `--boot bios` that is absent (no `--bios`, or no file at that path), whose size is not 8,192
  bytes (the message gives the size and, for a known dump of another size, its name), or that would start a Game Gear
  cartridge (`--machine gg`, or a `.gg` image under `auto`). The messages never print the path.

A file that cannot be read exits 1, a `--bios` path that is a directory or unreadable included; a `--bios` path that
does not exist is a refusal and exits 77. A missing cartridge is still exit 1.

## Frame hash

`--hash` prints one line after the last frame:

```text
frame 60 sha256 e14d77ce564f0b1498f703134e82e6c1fc718a27c94e49b5a35b95a981857ae7
```

The hash is SHA-256 over the visible picture only: 256×192 on the Master System and 160×144 on the Game Gear. It
reads the rows from top to bottom. Each pixel is an RGB444 value (`0000BBBBGGGGRRRR`), stored as two bytes with
the low byte first. The hash is therefore the same on little-endian and big-endian hosts. The test card does not
change from frame to frame, so `--frames 1 --hash` and `--frames 60 --hash` print the same digest.

## Sound hashes

`--hash-audio` runs the sound path as a host would and prints two lines after the last frame (after the `--hash`
line, if both are given):

```text
audio-transitions 60 count 878 sha256 <64 hex digits>
audio-pcm 60 rate 48000 channels 1 samples 48061 peak 0 clipped 0 sha256 <64 hex digits>
```

The machine and console come from `--machine`, `--region` and `--video-mode` as for any run, so a Game Gear prints
`channels 2` and a Master System `channels 1`. The first number on each line is the frames that ran.

- **`audio-transitions`** hashes every change of a sound source's level, in time order. A source is one tone or
  noise channel, the Game Gear stereo byte, or a Japanese Master System's FM unit. Each frame the runner takes the
  core's log of changes, orders it by time and then by source, keeps only the last level a source was given at one
  instant, and drops a level the source already held. Each kept change is hashed as 11 bytes: the time in master
  clock ticks (8 bytes), the source (1) and the level (2), low byte first. `count` is how many were kept. Power-on
  levels count as changes at tick 0. A level is the chip's register state, not a volume in samples, so changing
  how loud a level sounds moves `audio-pcm` and leaves this line alone.
- **`audio-pcm`** hashes every sample of the run as 16-bit values, low byte first, left then right on a Game Gear.
  `samples` is the sample count per channel, which is `floor(frames x 3,504,512 / 4,375)` on NTSC and
  `floor(frames x 685,094,400 / 709,379)` on PAL. `peak` is the
  largest magnitude (32,768 for the lowest value) and `clipped` counts samples at the extreme values.

The run exits 1 and prints no `audio-` line when frame f does not hold exactly
`floor(f x N / D) - floor((f - 1) x N / D)` samples, N / D being the fraction above. NTSC frames hold 801 or
802 samples and PAL frames 965 or 966, in a fixed pattern, PAL's first five being 965, 966, 966, 966 and 965. It also exits 1
when the core dropped samples or log records, or when the core logs a source's changes out of time order. The
check is made after each frame, so a hostile ROM cannot make the runner hold more than one frame of sound.

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
| `mapper_guess` | the board: `none` (no mapper, 48 KiB or less), `sega`, `codemasters`, `korean`, `korean_nb`, `hicom`, `zemina`, `nemesis`, `hes_4pak`, or `unsupported` for an image the core will not load |
| `sha1`, `sha256`, `crc32`, `md5` | digests of the whole file |
| `sha1_headerless`, `sha256_headerless`, `crc32_headerless`, `md5_headerless` | the same digests of the file after its copier header, or `none` when it has none |
| `identified`, `public_identity` | whole-body metadata match, and whether that match is public CC0 data (1) rather than an owned fixture (0) |
| `persistence` | 0 unknown, 1 known nonbattery/volatile, 2 known battery |
| `persistent_size` | declared persistent byte count, independently of mapper allocation |
| `persistence_geometry` | 0 no battery geometry, 1 supported power-of-two layout, 2 unsupported persistence geometry |
| `video_mode` | verified metadata mode: 0 unspecified, 1 SMS, 2 native GG |
| `identity_sha1`, `title` | body identity, empty on a miss; `title` is always empty since titles left the table |
| `board_source` | where the board came from: `db` (a database row), `header` (a Codemasters or valid ROM header), `heuristic` (the store-site scan), `default` (a size or an empty scan decided it) or `none` |
| `board_candidate` | the board the scan pointed at, or `none` |
| `scan_sega`, `scan_candidate`, `scan_runner_up` | store sites counted for the Sega board, for the leading other family and for the runner-up |
| `unsupported_id` | the number of the named unsupported board a database row or the scan matched, 0 for none |
| `unsupported_reason` | that board's name and the reason, empty for none |
| `port1_default`, `port2_default` | the controller each port's table row names: `pad`, `paddle`, `sports` or `lphaser` |

Each header field from `checksum` to `rom_size_code` is `none` when the image has no ROM header. `md5` is the
RetroAchievements hash for both machines: the MD5 of the whole file, copier header included. `crc32` is the value
RetroArch's playlist scanner matches.

`--probe` runs no emulation. It exits 0, or 77 after printing every key for an image the core will not load
because of its size or its board. A run of such an image also exits 77 and its message names the board and the
reason: the unsupported board's name and reason, or the scan's candidate and its counts when it was not sure.

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
| `dev` | `none`, `cpu`, `cart`, `ram`, `mapper`, `ctrl` (the memory and I/O control ports `$3E` and `$3F`) or `vdp` |
| `kind` | `unmapped_read`, `unmapped_write`, `rom_write`, `mapper_range`, `ed_undefined` or `unsupported_video_mode` |

[Embedding](embedding.md) says when each kind is recorded and which guest behaviour is never a fault. Later
releases add kinds and devices; scripts should accept names they do not know.

## Mailbox

The diagnostic ROMs in `tests/roms/src` measure the machine and write what they measured into work RAM at
`$C100`-`$C1FF`:

| Address | Content |
|---|---|
| `$C100`-`$C103` | the magic `CMDG` |
| `$C104` | the protocol version, 1 |
| `$C105` | the status: `$80` running, `$00` done |
| `$C106`, `$C107` | the record count and the ROM's id |
| `$C108`-`$C109` | the CRC-16 of `$C110`-`$C1FF`, low byte first |
| `$C110` + 4 × n | record n: its test id, its flags (bit 0 valid), its value low byte first; up to 60 records |

The CRC-16 is CCITT's: polynomial `$1021`, initial value `$FFFF`, no reflection and no final XOR, so the nine
bytes `123456789` give `$29B1`.

`--mailbox` reads the mailbox after each frame without disturbing the run (the library's `cogmaster_peek`). The run
stops after the first frame whose status reads `$00` with the magic in place, once an earlier frame has ended with
the status reading `$80`; otherwise it runs all `--frames N` frames. Work RAM reads `$00` from power-on, so a ROM
that has not yet set up its mailbox, or a ROM that has none, is not taken as done. A ROM that finishes within its
first frame runs to the frame count. Then the runner prints, in hexadecimal:

```text
status=00
rom=01
magic=ok
crc=ok
01=F1F0 flags=1
02=F1F0 flags=1
```

`status` and `rom` are the bytes at `$C105` and `$C107`. `magic` is `ok` when `$C100` holds `CMDG`, else `bad`;
`crc` is `ok` when the stored CRC matches the records, else `bad`. Then one `NN=VVVV flags=F` line per record, in
order: the test id, the value and the flags. A status other than `00` means the ROM had not finished by the last
frame. The runner only measures: it judges nothing, and the exit status is 0 whatever the mailbox holds.
`--hash` and `--bench` output follows the mailbox.

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
report. That frame's PNG is still written; a `--png` for a later frame is not, and a line on standard error names
it, as after a `--trace` or `--mailbox` stop. `--hash` prints the last frame that ran, and `--bench` reports the
frames that ran. Output is flushed and the exit status is 0. A signal that was already ignored when the runner
started, as under `nohup`, stays ignored.

## Battery files

Native POSIX and WASI create temporary files exclusively with `open`, then replace
within the same directory with `rename`; native POSIX also synchronizes the file and the directory.
Windows uses wide paths and `MoveFileExW`. WASI filesystem rights and durable
storage remain properties of the host's preopened directory.

`--save-dir DIR` reads `<ROM stem>.srm` before emulation and saves changed battery data on exit.
Only the last extension is removed: `round.trip.sms` uses `round.trip.srm`. Missing files start fresh
with FF bytes; for a battery save, empty and short files pad with FF. An EEPROM save must be 0 or 128
bytes. An oversized file, or an EEPROM file of any other length, exits 4 before running with a message
that names the length wanted, and stays unchanged. Other file errors exit 1. Unknown persistence and unsupported geometry refuse with distinct
messages and exit 77 before running. Classified nonbattery cartridges need no file. Without this option,
the runner does no battery file I/O.

Only a changed generation writes a file. The runner exclusively creates a unique temporary beside the
target, checks the complete write, flush, native synchronization where supported and close, then replaces
the target with POSIX rename or Windows MoveFileExW. It acknowledges the generation only on success and
cleans only its own failed temporary. The previous target is never predeleted or truncated. SIGINT and
SIGTERM finish the bounded frame, then flush in ordinary control flow; a failed flush exits 1.

Replacement provides old-or-complete-new visibility on native local filesystems with atomic rename;
it is not a universal power-loss guarantee. On POSIX other than WASI the runner then opens the directory,
calls `fsync` and closes it, so the new name is durable; a filesystem that answers EINVAL or ENOTSUP for a
directory is accepted, as is a directory that refuses to open for reading (EACCES or EPERM) while still
taking the rename; any other error fails the flush and the generation stays unacknowledged. macOS
`fsync` does not flush the drive cache and `F_FULLFSYNC` is not used. Windows has no directory-sync
analogue; `MoveFileExW` writes through. Filesystem, device and network behavior can still limit crash
durability. Temporaries are named `<target>.tmp-<i>` for i from 0 to 1023 on every platform. At load the
runner deletes exactly those 1024 names of its own target, listing no directory, so a killed run leaves
nothing the next run trips on; another save's temporaries and any other file stay. A temporary created
after the load is never overwritten: a flush takes the next free index. Simultaneous writers to one target
are unsupported. Same-stem ROMs share a save name: use separate directories to avoid collisions. Raw files
have no game identity check, so exact-size wrong-game data cannot be detected. Windows uses Unicode paths;
POSIX keeps path bytes unchanged.

Recurring save tests force create, short-write, flush, sync, close, replacement and directory-sync failures for both
first creation and overwrite. They check the old bytes, dirty retry, exclusive same-directory temporary
cleanup and preservation of unrelated temporaries. Signal and abrupt-interruption runners are test-only
builds; the installed runner has no fault injection. One such build is stopped at its first flush with the
temporary complete and the target not yet replaced: the previous save stays byte for byte and the next run
goes on from it.

`--save-interval N` runs the same flush after the outputs of every frame whose number is a multiple of N,
so a killed run loses at most N frames of progress. It counts frames, never time, and `--frames` runs get
no special case: an N above the frame count writes only at exit. A periodic flush that fails ends the run
at once with exit 1, with no later frame and no hash line. A SIGUSR1 or SIGBREAK flush remains deferred.

## Accepted expectations

`--frames N --expect FILE` reads an LF-terminated ASCII file before running. It accepts
1–64 records, each at most 255 bytes; every requested frame is within `1..N`, and N is
at most 1,000,000. Each record is one of:

```text
frame@14=e14d77ce564f0b1498f703134e82e6c1fc718a27c94e49b5a35b95a981857ae7
ram@14:$C200=02
mailbox@20
boots@20
audio@300=<64 lowercase hex digits>
```

The frame value is an accepted full lowercase SHA-256 of the completed native crop,
serialized RGB444 low byte first. RAM compares a byte at `$C000..$FFFF` without side
effects. Mailbox checks magic, protocol version, done status, bounded nonempty distinct
records with valid/not-timed-out flags, and CRC; it does not validate record values.
Use independent RAM/frame expectations or the diagnostic judge for numeric claims.

`audio@F=H` judges the sound's changes of level. H is the SHA-256 of the merged transition stream of frames 1 to F,
the digest of the `audio-transitions` line of `--frames F --hash-audio`; F is from 1 to N, and one run can hold
several. The hash holds no machine, region or FM byte, so one judged under another profile fails. PCM is not a
kind: the mix is provisional. The line reads `predicate kind=4 frame=F address=0000 pass
observed_audio=<hash> transitions=<count>`. A mismatch exits 3, a malformed record 2, and a lost log record, a
source going back in time or a frame of the wrong length 1. A script's `reset=1` is the first console's Reset button
and restarts nothing, so an audio case judges that run like any other.

`boots@F` is strict smoke: a display-enabled latch was observed after a run call, at
least two distinct native crop hashes occurred by F, and no first fault was recorded.
Distinct counting saturates at two. An interrupt-driven HALT loop is healthy and is
never rejected for HALT. A static display may fail this heuristic. Neither pass nor
fail establishes gameplay, completion or sound; an audio record only locks our own output and proves nothing about how it sounds.

Each observation prints its frame, normalized kind/address and observed hash alongside
display/distinct/fault facts. Expected values stay in the accepted file. The execution
line reports resolved machine/region/TV/FM (`fm=1` off, `fm=2` on)/video/board, both devices by name (`port1=sports`), cap, behaviour revision
and SHA-256 of input bytes (the empty digest without a script). The first fault remains
a diagnostic and does not stop the capped run. `--trace`, `--mailbox`, `--bench` and
`--probe` conflict with `--expect`. A signal before an expected frame is infrastructure
failure; mismatches exit 3, unsupported capabilities 77, invalid syntax 2 and I/O errors 1.

## Diagnose

`--diagnose FILE` runs exactly 1800 frames and prints one line, then exits 0. It holds only numbers, enum words
and words from fixed tables, never a digest, byte, image size, name or path. It does print the first-event call
numbers and the fault pc; the fault's own frame is bucketed. Frames are run calls counted from 1; a field is `none`
when its event never happened.

```text
diagnose 1 verdict600=pass first_display=2 first_nonblank=2 first_change=2 distinct600=3 distinct1800=3 ram=still fault=none dev=none pc=none pcslot=none where=none fault_frame=none order=none mapper=none source=header
```

| Field | Value |
|---|---|
| `verdict600` | `pass` or `fail`: the `boots@F` predicate at call 600, computed from the same state, so it equals the status of `--frames 600 --expect` with `boots@600` (0 is pass, 3 is fail) |
| `first_display` | the first call whose frame had the display enabled |
| `first_nonblank` | the first call after which the native crop holds two or more distinct RGB444 values (the first non-blank picture) |
| `first_change` | the first call whose crop differs from call 1's, the event that makes `boots@F` see two distinct pictures |
| `distinct600`, `distinct1800` | distinct crops seen by call 600 and by call 1800, counted up to 8 |
| `ram` | `live` when the 8 KiB of work RAM at `$C000..$DFFF` differs between calls 1740 and 1800, else `still` |
| `fault`, `dev`, `pc` | the first fault's kind, device and address as under [First fault](#first-fault), `pc` as four uppercase hex digits |
| `pcslot` | where `pc` sat: 0 for `$0000..$00FF`, 1 for `$0100..$3FFF`, 2 for `$4000..$7FFF`, 3 for `$8000..$BFFF`, 4 for `$C000..$FFFF` |
| `where` | the kind of address the fault names, from its device: `rom` and `slot2` for the cartridge below and from `$8000`, `ram`, `mapper`, `port` (the control ports), `cpu`, `vdp` or `unknown` |
| `fault_frame` | the bucket of the fault's frame, counted from 0: `0`, `1-9`, `10-99`, `100-599` or `600+`; a fault below 600 is one `verdict600` sees |
| `order` | `before` when the fault was recorded during or before the first non-blank call, `after` when later, `nopicture` when no non-blank picture showed, `none` without a fault |
| `mapper`, `source` | the board the core chose and how, as under [Probe](#probe) |

The 1800 frames always run: a signal before the last one exits 1 with one line on standard error and prints no
fact. A usage error, exit 2, is any run option beyond the ones named above or a missing file; a cartridge the
core will not load exits 77 as for any run.

## Exit status

`cogmaster-run --help` prints this table:

```text
  0  ok       every frame ran or the probe printed, or a signal stopped the run after a whole frame
  1  failure  a file could not be read or written, memory ran out or the core failed; a missing --bios path is refused (77), not a failure; a missing cartridge is still 1
  2  usage    the command line or input script is invalid, so nothing ran
  3  mismatch  an accepted expectation did not match the observed predicate
  4  save     the battery file is oversized, so nothing ran
  77  refused  the cartridge, its size, the machine and region, or a controller is not supported, or the BIOS asked for is absent or not supported, so nothing ran
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
