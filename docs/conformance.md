# Conformance

How the core is checked against public test data, assembled test programs and generated inputs. This page covers
the Z80 vectors, the assembled fixtures and their mailbox, and fuzzing; testing in general is in
[testing.md](testing.md).

| Label or preset | Command | What runs |
|---|---|---|
| `vectors` | `ctest --test-dir build/ci -L vectors` | The sample's seven shards, the converter's fixtures and the harness's negative controls |
| `z80` | `ctest --test-dir build/ci -L z80` | The interrupt, NMI, HALT, prefix-run and ED-hole cases |
| `fixtures` | `ctest --test-dir build/ci -L fixtures` | The assembled images, their mailbox records judged, and the judge's and the pin's negative controls |
| `fuzz` | `ctest --test-dir build/ci -L fuzz` | Every seed replayed through the two fuzz entries |
| `vectors-full` | `cmake --workflow --preset vectors-full` | Every vector, nightly; fetch the archive first |
| `fuzz` preset | `cmake --workflow --preset fuzz` | 60 s of libFuzzer per entry, nightly, with clang-21 |

## Z80 vectors

### The sample

`tests/vectors/z80/sst-sample.bin` holds the first 100 tests of each of the 1,604 files of the SingleStepTests
z80 set (`v1`), 160,400 tests in all. Each test is one instruction: an initial state, the final state, the RAM
and port contents, and the bus on every T-state. The set covers every documented and undocumented instruction,
including MEMPTR (`wz`) and the Q latch; it has no interrupts, no wait states and no prefix runs.

- **Pin:** https://github.com/SingleStepTests/z80 at `ebe1875d48f374bcfd4b505d8eb8ee751568b5f7`, the `sst-z80`
  row of `pins.txt`, which also holds the archive's SHA-256.
- **Licence:** MIT. The notice is in `THIRD-PARTY-NOTICES.md`; the file's SHA-256 is in `tests/roms/manifest.txt`.

### The access form

The upstream JSON lists the bus on every T-state. The sample stores, for each test, the bus cycles those T-states
are built from instead: an opcode fetch, a memory read or write, or an I/O read or write, each with its address,
its data, its start T-state when it does not follow the previous cycle directly, and for a fetch the refresh
address. The final state is stored as a mask of the fields that changed plus their values. A test takes about
81 bytes; the sample is about 13 MB. `tests/z80/sst_stream.h` documents the layout byte by byte.

### The stream templates

One function, `sst_stream_build` in `tests/z80/sst_stream.c`, expands bus cycles to T-states. The converter
uses it to check that every upstream test rebuilds exactly, and the harness uses it for both the expected and
the actual stream, so the two can never disagree about a template. Each entry is (address, data, pins), with
`-` for no data:

| Cycle | T-states |
|---|---|
| Opcode fetch | `(pc,-,----) (pc,-,r-m-) (refresh,opcode,----) (refresh,-,----)` |
| Memory read | `(a,-,----) (a,-,r-m-) (a,d,----)` |
| Memory write | `(a,-,----) (a,d,-wm-) (a,-,----)` |
| I/O read | `(p,-,----) (p,-,----) (p,-,r--i) (p,d,----)` |
| I/O write | `(p,-,----) (p,-,----) (p,d,-w-i) (p,-,----)` |

Any other T-state is idle: it repeats the previous address with no data and no pins. The refresh address is
`I << 8 | R`, with R taken before the fetch increments it.

### Regenerating the sample

```sh
sh scripts/fetch-pin.sh sst-z80 build/pins/sst-z80
cmake --workflow --preset ci
build/ci/tests/cogmaster-sst2bin --dir build/pins/sst-z80/v1 --count 100 --out tests/vectors/z80/sst-sample.bin
```

`fetch-pin.sh` checks the archive's digest before unpacking it. The converter knows the file names, so it needs no
directory listing; `--files "00,dd 34"` limits it to some files. It reads the fixed schema only: a test missing a
key fails naming the key, an unknown key is skipped, and a test whose bus does not rebuild from its cycles fails
naming the file and the test. It never drops a test. After regenerating, update the SHA-256 in
`tests/roms/manifest.txt`.

### Running the harness

```sh
build/ci/tests/cogmaster-test-sst --sample tests/vectors/z80/sst-sample.bin --only "00,dd 34"
```

`--only` takes comma-separated upstream file names; `--group base|cb|dd|ddcb|ed|fd|fdcb` runs one prefix group,
and `--expect-count N` fails unless exactly N tests ran. For each failing test the harness prints one line, for
example `mismatch "00" test 0 a: expected 111, actual 110`, naming the file, the zero-based test index and the first
field, RAM cell (`ram[4096]`) or T-state (`T3`) that differs. It ends with `failed N` and `executed N`, and exits 1
on any failure.

### The negative control

The `vectors.self_test` test shows the harness can fail. At build time the harness writes
`build/<preset>/tests/vectors/sst-corrupt.bin`, a copy of the sample in which test 0 of file `00` expects a
different final `a`:

```sh
build/ci/tests/cogmaster-test-sst --sample tests/vectors/z80/sst-sample.bin \
  --corrupt-copy build/ci/tests/vectors/sst-corrupt.bin 00 0 a
```

The test then requires exit 1 and the mismatch line for that file, test and field. The committed sample is never
changed. The `vectors.sst2bin.*` tests run the converter on four hand-written one-test files in
`tests/z80/fixtures`: one that converts and passes, one missing a key, one with an extra key, and one whose
opcode data sits on the wrong T-state.

### The allow list

A known upstream inaccuracy is excused by a line in `tests/vectors/z80/allow.txt`, never by skipping a test:

```text
2026-10-19 "dd 34" 17 f https://github.com/SingleStepTests/z80/issues/NN why the upstream value is wrong
```

The fields are the date the line was added, the file name in double quotes, the test index, the field as the
mismatch line names it, the upstream issue URL (`https://`), and a reason. Blank lines and lines starting with
`#` are ignored. Pass the file with `--allow tests/vectors/z80/allow.txt`. A matching mismatch prints as
`allowed` and does not count as a failure. A malformed line fails the run, and so does a line whose file ran
but which matched no failure, so a line is removed once the core or upstream is fixed. `vectors.allow.self_test`
checks that rule. The list is empty today.

### The shards

`ctest -L vectors` runs the sample in shards, one per prefix group, each with `--allow` and the group's exact
test count. `vectors.base` runs the main page (252 files, 25,200 tests), `vectors.cb` the CB page (256 files),
`vectors.dd` and `vectors.fd` the indexed pages (252 files each) and `vectors.ddcb` and `vectors.fdcb` the indexed
bit operations (256 files each), and `vectors.ed` the ED page (80 files, 8,000 tests). A DD or FD byte is a step
of its own, so the harness steps until no prefix is pending and compares the whole instruction. A repeating block
instruction (LDIR, CPIR, INIR, OTIR and their decrementing forms) runs one iteration per step and moves PC back
to itself, so each vector is one iteration.

### The nightly full run

The sample holds a tenth of the set. The nightly `vectors` job runs all of it:

```sh
scripts/fetch-pin.sh sst-z80 build/pins/sst-z80
cmake --workflow --preset vectors-full
```

`COGMASTER_VECTORS_FULL` makes the build convert all 1,000 tests of every file into
`build/vectors-full/sst-full.bin` (about 130 MB), and the first 100 again into `build/vectors-full/sst-sample.bin`.
Configure fails, naming the fetch command, when `build/pins/sst-z80/v1` is missing, so a night cannot pass on
nothing. The lane registers eight tests (`tests/inventory/vectors-full.txt`): `vectors.full.base`, `.cb`, `.dd`,
`.ddcb`, `.ed`, `.fd` and `.fdcb`, each with `--allow` and ten times its sample shard's count (252,000, 256,000,
252,000, 256,000, 80,000, 252,000 and 256,000), and `vectors.sample_equal`, which requires the regenerated sample
to equal the committed one byte for byte. A failure there means the converter or the pin changed without the
sample being regenerated. The whole run takes a few seconds on a recent Mac.

### What the vectors do not cover

The set has no interrupts, no NMI, no prefix runs and no undefined ED opcodes. `cogmaster-test-z80int` scripts
those cases in a flat 64 KiB memory, one case per test (`ctest -L z80`):

- `z80.int.im0`, `z80.int.im1`, `z80.int.im2`: acceptance in each interrupt mode, with $FF on the data bus as
  on the SMS; 13 T-states for IM 0 and IM 1, 19 for IM 2.
- `z80.int.ei_delay`, `z80.int.di`: no acceptance until the instruction after the last EI; none after DI.
- `z80.int.nmi`, `z80.int.retn`: an 11 T-state NMI to $0066, even after DI, that keeps IFF2; RETN restores IFF1.
- `z80.int.ld_a_i`: an interrupt right after LD A,I clears P/V, as on NMOS parts.
- `z80.int.halt_exit`, `z80.int.block_op`: the pushed address is the one after HALT, or the block instruction's
  own address between iterations.
- `z80.int.prefix_run`, `z80.int.r_prefixes`: 65,536 DD bytes run in 4 T-state steps with no acceptance between
  them, and R counts one per opcode fetch with bit 7 kept.
- `z80.ed.holes`: each of the 176 undefined ED opcodes is an 8 T-state no-operation that adds 2 to R.

## Assembled fixtures

Test programs for the whole machine are written in Z80 assembly under `tests/roms/src`, one `<name>.asm` and one
`<name>.link` each, sharing `header.inc` (the memory map and the Sega header, with the ROM size set explicitly and
the header checksum computed by the assembler). Git checks these sources out with LF line endings on every host,
so every lane assembles the same bytes.

### The assembler pin

The fixtures are built with WLA-DX v10.7, the `wla-dx` row of `pins.txt`. At configure time `cmake/WlaDx.cmake`
downloads that tarball into `build/<preset>/wla-dx`, refuses it unless its SHA-256 matches the row, and builds
only `wla-z80` and `wlalink` with the host compiler: Release, no documentation, no toolchain file and no cross or
sanitizer flags, since the tools run on the build machine. A later configure reuses the two programs while the pin
they were built from is unchanged. WLA-DX is GPL-2.0-or-later; it runs only as a tool, and its source stays in the
build tree. `fixtures.wla_pin.self_test` checks a local `file://` tarball against the pinned digest and passes
only when the download fails with a hash mismatch, so it needs no network.

### The images

The `cogmaster-fixtures` target assembles each source into `build/<preset>/fixtures/<name>.sms`. The images are
never committed and never packed into a release archive: the source and the pinned assembler define them, and
`tests/built/expected.txt` pins each image's SHA-256. `fixtures.hello` assembles the smallest one, a valid export
header and a jump to itself, checks its digest and that `cogmaster-run --probe` reports `checksum_ok=1`.
`runner.hashes_file` adds each image's digest to `hashes.txt`, so the `hash-equality` job also shows every lane
assembled the same bytes.

### Measurements and verdicts

A fixture measures and never judges. It reports through the RAM mailbox, and `tests/diag/expected.csv` holds the
verdicts.

### The mailbox protocol

The mailbox is 256 bytes of work RAM at `$C100`-`$C1FF`, away from the stack and from the mapper registers' copy at
`$DFFC`-`$DFFF`. The byte layout is the table in [runner.md](runner.md#mailbox): the magic `CMDG`, the protocol
version 1, the status, the record count, the ROM's id, the CRC, then up to 60 four-byte records (test id, flags
with bit 0 valid, value low byte first). A fixture includes `tests/roms/src/mailbox.inc` in bank 0's code and
calls its three routines in order:

1. `mb_init` with A = the ROM's id clears the mailbox, writes the header and sets the status to `$80`, running.
2. `mb_record` with A = the test id and HL = the value appends one record; past the 60th it writes nothing.
3. `mb_done` stores the CRC-16 of `$C110`-`$C1FF` (CCITT: polynomial `$1021`, initial value `$FFFF`, no reflection,
   no final XOR, low byte first) and only then sets the status to `$00`, done.

A fixture waits for the second frame before `mb_done`, so a run cut after one frame still reads `$80`.
`cogmaster-run --mailbox` stops at the first frame whose status reads `$00`, or at `--frames N`, and prints the
mailbox as `status=`, `rom=`, `magic=`, `crc=` and one `NN=VVVV flags=F` line per record.

### The judge

`tests/cmake/mailbox.cmake` judges a runner's output against the CSV rows for one ROM and one profile:

```sh
cmake -DOUT=build/ci/tests/mailbox/mapper_sega.txt -DCSV=tests/diag/expected.csv -DROM=mapper_sega \
  -DPROFILE=sms-export -P tests/cmake/mailbox.cmake
```

It passes only when every record is inside its row and every row has its record. Otherwise it fails with one of
five messages:

| Message | Cause |
|---|---|
| `bad CRC or magic` | no `CMDG` magic (the ROM never wrote it), or records that fail their CRC (they changed after `mb_done`) |
| `the status is not 00 at the frame cap (hang)` | the ROM did not reach `mb_done` within `--frames` |
| `record NN=VVVV is outside its row` | a measurement differs from the expected value or range |
| `record NN has no row` | the ROM reported a record the CSV does not list |
| `row ... has no record` | the CSV lists a record the ROM did not report |

### Adding a row

Each row of `tests/diag/expected.csv` is one record of one ROM:

```text
rom,test,profile,expected,source,citation
mapper_sega,01:bank_sum_slot1,*,F1F0,doc,SMS-CARTRIDGES section 2.1
```

- `rom` is the fixture's name, `tests/roms/src/<rom>.asm`.
- `test` is the record's id in two hex digits, a colon and a short name.
- `profile` is the machine profile the row applies to, or `*` for every profile.
- `expected` is a hex value, or `lo-hi` with both ends inclusive for a measurement that varies.
- `source` is `hw` (measured on hardware), `doc` (a document says so), `oracle` (a reference emulator's output)
  or `derived` (computed from other rows), and `citation` names the document and section, or the oracle and its
  pin.

To add a record, call `mb_record` with a new id in the fixture, add its row, rebuild, and update the image's
SHA-256 line in `tests/built/expected.txt` to the digest `fixtures.<rom>` reports. The value comes from the
cited source, never from the core's own output.

### The mapper fixtures

`fixtures.mapper_sega` runs a 512 KiB Sega-mapper image of 32 banks, each stamped with its number at `$0400`. Its
records:

- the sum of every bank's stamp through slot 1, and through slot 2;
- slot 0 after selecting bank 5: the first 1 KiB stays on bank 0 while `$0400` reads bank 5;
- wrap: `$FFFE = 35` selects bank 3 and `$FF` selects bank 31;
- `$DFFF` reads back the value written to `$FFFF`;
- cartridge RAM through `$FFFC` bit 3 (the written pattern, then ROM with the bit clear, then the pattern again),
  its second 16 KiB bank through bit 2, and the bank shift in bits 1-0, which adds `$18`, `$10` or `$08` to a
  later bank write.

`fixtures.mapper_codemasters` runs a 128 KiB Codemasters image of 8 stamped banks with the Codemasters header at
`$7FE0` and no `TMR SEGA`, so the test passes `--machine sms`. Its records: the banks at reset (0, 1, 0), the sum
of every bank's stamp through `$0000`, `$4000` and `$8000` (slot 0 is paged by a loop copied to work RAM, with
interrupts off), wrap, and 8 KiB of cartridge RAM at `$A000` through `$4000` bit 7 (the pattern, ROM with the bit
clear, the pattern again). The header's checksum word is not the true sum; the probe does not check it.

`fixtures.judge.wrong_value`, `fixtures.judge.missing_row` and `fixtures.judge.hang` are the judge's negative
controls: a CSV copy with one value changed, a copy missing one row, and a run cut after one frame, each of which
must fail with its message.

### The ports fixture

`tests/roms/src/ports.asm` reads the I/O chip's and the VDP's ports, and runs once per machine profile:
`fixtures.ports.sms-export` (`--machine sms --region ntsc-u`), `fixtures.ports.sms-japan` (`--region ntsc-j`),
`fixtures.ports.sms-pal` (`--region pal`) and `fixtures.ports.gg` (`--machine gg`). The Game Gear runs
`ports_gg.asm`, the same source under a Game Gear export header, and its records are judged against the `ports`
rows with profile `gg`. The records:

- the region echo: `$3F = $F5` then `$55`, reading `$DD` bits 7-6 each time; an export console reads back its
  output TH levels, a Japanese one reads 0;
- `$DD` bits 4-5, and `$DC` and `$DD` with every button up;
- the open-bus forms: a read of `$3E`, a read of `$C0`, `$DC` and `$DD` with the I/O chip off through `$3E` bit 2,
  and work RAM off through `$3E` bit 4 (the Game Gear has no `$3E`, so its RAM stays on);
- the TV standard: the count of a 31-cycle loop polling `$BF` bit 7 for one frame, judged by a range (59,736 cycles
  a frame on NTSC, 71,364 on PAL), and whether that count reaches 2,048, the PAL verdict games use;
- the lines in a frame: the V counter's changes from one line-0 reading to the next, 262 on NTSC and 313 on PAL;
- two bytes written to VRAM and read back through `$BE`, the status byte's low bits after the flags clear, and the
  H counter's step between two `$7F` reads 15 cycles apart;
- on the Game Gear, port `$00` (Start up, the region bit, the NTSC bit) and ports `$01`-`$06` at their power-on
  values.

The H counter latch, the Pause NMI counter and the Pause movies need input and are not in this fixture yet.

## Video geometry and timing fixture

`vdp_lines` is assembled three times from the same owned source, as
`vdp_lines_192.sms`, `vdp_lines_224.sms` and `vdp_lines_240.sms`. Only their
SHA-256 digests are committed; all image bytes stay under the build directory.
Each runs on NTSC and PAL, with eight numeric mailbox records judged against
`tests/diag/expected.csv`: line IRQ count, frame IRQ count, entry H/V and two
status reads plus V samples either side of the selected $3F/$40 boundary. The [video notes](hardware/video.md) derive the counts, handler
range, rolling phase and feature coordinates.

`fixtures.vdp_lines.pixels.*` checks every coordinate against an independent
diagonal/solid-colour construction, including both sprite sizes, zoom, SAT
order, flips, palette, priority, mask, scroll locks, IRQ scroll bands and the
last active row. `fixtures.vdp_lines.mailbox.*` also pins each assembled image.
Named wrong-value, missing-row and one-frame-hang controls must return their
expected nonzero result. The lane hash file contains all fixture image
digests plus actual playing-frame hashes for both TV standards; comparison
across portable lanes detects host-dependent rendering. The separate
`boot_to_play` retains the same pixel oracle. Its SMS mailbox values now hold independent pad-1 and
pad-2 edge counters in the high and low bytes. Separate and overlapping pad schedules exercise no input,
press, hold, release and repeated press; the shared visible counter, CRAM colour and scroll still drive
the independently checked picture. Setup frames 1–3 and mailbox-sealing frame 12 are lagged; every other
frame executes a decoded controller read. The runner reports their sum with `--input`.

## Frontend play evidence

The owned `boot_to_play.gg` is assembled from a thin GG-header wrapper
over the SMS fixture. Its own diamond/bar glyphs cover the LCD window and
its 64-byte palette supplies the independently widened SMS colour sequence
as native RGB444. The fixture polls native Start once per VBlank; press
edges change CRAM 15 and scroll, while held and released levels do not.
The first eight polls record both counter/scroll and raw Start/region
bytes, sealed with the mailbox CRC. Baseline and scripted rows are judged
against `tests/diag/expected.csv`; every LCD coordinate and the runner hash
are checked against the explicit diamond table and colour arithmetic.
The same SMS source now polls button 1 on either pad, with separate p1/p2
scripts proving the same first-boundary, held and repeated-edge sequence.
The image digests are in `tests/built/expected.txt`; no built ROM is committed.

`vdp.gg` tests exhaust all 4096 native colours and 64 SMS colours, shared
even-byte latch reuse across address changes/wrap, raster immutability,
192/224 crop identity, held LCD versus live full-field output, SOFT/POWER
reset, matching-mode state restore and specific controller aliases. These
are checks of the selected sourced model; LCD freeze behavior and remaining
full-decode uncertainty still need hardware measurement for stronger claims.

The hosted RetroArch job preserves the no-content card comparison and adds the owned SMS game with frontend
replay input. Its baseline and press/hold/release/repeated-press screenshots must equal runner pixels and the
independent coordinate oracle at counter/scroll 0 and 2. A one-pixel mutation must fail. The replay-only module
adds existing internal state visitor forwarding for the frontend's initial checkpoint, while compiling the
same input and video adapter source; it is excluded from installation and archives. The in-process host runs
the richer sequence against both production and replay modules. This proves the SMS frontend path; two-pad,
GG Start, region and lag expansions remain separately tested as they land.

## Fuzzing

The loader and a two-frame run are fuzzed from generated seeds. Every pull request replays the seeds and the
committed regressions (`ctest -L fuzz`); the nightly `fuzz` preset runs libFuzzer with the address and
undefined-behaviour sanitizers. A finding is triaged as a harness or core bug: fix it, then commit the minimised
input under `tests/fuzz/regress/` with its `tests/roms/manifest.txt` row so every lane replays it.
[testing.md](testing.md#fuzzing) has the commands and limits.

Game Gear hosted proof uses the native 160×144 LCD crop with the default 6:5 pixel
aspect. Baseline and Start-scripted screenshots must equal the runner crop and
the independent glyph oracle at field origin (48,24). Full-field and square-pixel
options are presentation only, with in-process completed-geometry and PAL pacing
coverage. Frontend agreement checks integration; it is not a hardware measurement.
