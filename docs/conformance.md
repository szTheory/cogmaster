# Conformance

How the core is checked against public test data and assembled test programs. This page covers the Z80 vectors
and the assembled fixtures; testing in general is in [testing.md](testing.md).

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
