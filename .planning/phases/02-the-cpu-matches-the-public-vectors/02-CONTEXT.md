# Phase 2: The CPU matches the public vectors - Context

**Gathered:** 2026-10-19
**Status:** Ready for planning

<domain>
## Phase Boundary

An own Z80 matches SingleStepTests `z80/v1` (pin `ebe1875d48f3`) on final state and on every bus cycle; the
Z80-driven bus runs on the 3-ticks-per-T-state, 684-ticks-per-line lattice; the no-mapper, Sega and Codemasters
boards, ports `$3E $3F $7E $7F $BE $BF $DC $DD` and GG `$00`–`$06`, and the Pause NMI are proven by unit tests and
own WLA-DX fixtures judged through the RAM mailbox; `cogmaster-run` gains `--probe`, `--trace first-fault` and
`--mailbox`; the loader fuzz target runs nightly with a bounded per-PR replay. Requirements CPU-01..CPU-05.

Not in this phase: mode-4 rendering, sprites, line interrupts, 224/240-line modes, the H-counter TH latch, pads
and `set_input`, the CC0 hash table, battery saves, sound (phases 3 and 4).

Method: every area below was fanned out to a research agent (architecture, performance, portability, test, CI,
API, security and adversarial lenses; primary and permissive sources only), then reconciled here; the owner's
standing instruction for this step was to follow the synthesised recommendations without a question round.

</domain>

<decisions>
## Implementation Decisions

### Carried and corrected
- **D-01:** Codemasters lands in phase 2 (REQUIREMENTS CPU-03, ROADMAP). ARCHITECTURE §3/§11 and the CONFORMANCE §2
  fixture matrix still say P3 for `mapper_codemasters` and `ports`; the plan corrects those docs.
- **D-02:** The vector label is `vectors` (`ctest -L vectors`, as ROADMAP says), not `z80` (ARCHITECTURE §11).
- **D-03:** Pause /NMI is asserted at the /HSYNC rise, tick 611 of line **260** NTSC / **311** PAL, which is the
  start of the last frame line 261/312 (SP.047, Charles MacDonald's logic analyser: "the /HSYNC edge 611 mclk into
  line 260"). ARCHITECTURE §2, §5 and SMS-SYSTEM C8 say "tick 611 of line 261/312", one line late; the plan
  fixes them. A unit test asserts the NMI tick and the V counter value read at `$0066` entry (`$FF`/`$00`
  boundary distinguishes the two readings).

### Z80 core structure
- **D-04:** Hand-written, instruction-granular core with T-state-exact access stamps (ARCHITECTURE §2 Rec). Files:
  `src/z80.h` (state), `src/z80_cycle.h` (static inline cycle primitives `m1`, `mread`, `mwrite`, `ioread`,
  `iowrite`, `idle(addr, n)` and flag helpers), `src/z80.c` (step, interrupt/NMI acceptance, main page, prefixes),
  `src/z80_cb.c` (CB, DDCB/FDCB), `src/z80_ed.c` (ED, block ops; ED holes = 8-T NOP). Names `cogmaster__z80_*`.
  The module spans several files to stay under ENGINEERING §10's 800-line cap.
- **D-05:** Decode: one `switch` per page; main page `0x40–0xBF` decoded by x/y/z fields, the rest one-line cases
  calling helpers with constant arguments; CB page fully x/y/z. No function-pointer tables (DEC.37).
- **D-06:** Registers in `uint8_t r[]` with IXH/IXL/IYH/IYL at fixed indices; pairs built byte-wise; no unions.
- **D-07:** DD/FD: each prefix byte is its own bounded step (4-T M1, R+1, sets `idx`, blocks interrupt
  acceptance). Never a loop over prefixes: a ROM of DD bytes must still return within the frame bound. Under
  `idx`, H/L remap and `(HL)` becomes `(IX+d)` with WZ set; `LD H,(IX+d)`, `EX DE,HL`, EXX keep true HL; DDCB reads
  d and op as memory reads and copies the result to the register when z≠6. Verified: in 239 decisive `dd 37`
  vectors the prefix clears Q, so a prefix behaves as its own instruction.
- **D-08:** Flags computed inline (SZ53 from `v & 0xA8` plus zero, parity by XOR folds, H/V/C from `a^b^r` and
  carry), no tables. If profiling ever wants one, a pointer-free `static const uint8_t` table is allowed.
- **D-09:** Hidden state: `q` holds the F value written by the last instruction (one `setf()` writes `f` and
  `q`; each step saves then clears `q`; SCF/CCF X/Y from `((q_prev ^ f) | a) & 0x28`); WZ per Sean Young's MEMPTR
  table; R low 7 bits per M1, bit 7 kept; `ei` blocks acceptance after EI (runs of EI stay blocked); `p` set by
  `LD A,I/R` and an interrupt accepted right after clears P/V (NMOS); HALT: PC+1 then halted 4-T M1s at PC with
  R+1; IM 1 acceptance 13 T and NMI 11 T as their own steps, so no step exceeds 23 T and the overshoot stays ≤ 69
  ticks. `OUT (C),0` writes 0 (NMOS; ares' comment has it reversed).
- **D-10:** Stamps: each primitive advances `ticks` by 3 per T-state and calls the bus at the data T-state (M1 and
  memory reads at T3, I/O at the fourth T-state after the automatic wait). Upstream shows write data at T2; the
  stamp convention stays, the harness's per-kind offset absorbs it.
- **D-11:** `bus_fetch` (M1, carries the refresh address I<<8|R), `bus_read`, `bus_write`, `bus_in`, `bus_out`
  are real external functions (not header-inline) so the vector harness can supply its own at link time.

### Per-cycle vector harness
- **D-12:** Link seam, no hook and no traced build: `tests/z80/sst_bus.c` defines the `bus_*` functions, logs
  {kind, tick, addr, data, refresh}, and expands each access into the per-T-state stream with fixed templates
  (M1 `[pc,-,----][pc,-,r-m-][IR,op,----][IR,-,----]`; memory read data at T3; write data at T2 with `-wm-`; I/O
  4 T with `r--i`/`-w-i` at T3, read data at T4; idle cycles = previous address, null, `----`). Measured on 29,000
  upstream tests (incl. LDIR, INIR, RLD, EX (SP),HL, DDCB, HALT, I/O, RETN): every stream rebuilds exactly.
  The `run_loop.inc` dual compile (ARCHITECTURE §8/§10) is not needed for the vectors and waits for hooks (v5).
- **D-13:** Converter `tests/z80/sst2bin.c`, a host C tool built by CMake (replaces CONFORMANCE §1.1's
  `tools/sst2bin.py`: no Python dependency). Fixed-schema tokenizer; fails on a missing key, ignores unknown keys
  (upstream issue 11's possible `qff`), and rejects any test whose stream does not rebuild from its accesses.
- **D-14:** Committed sample `tests/vectors/z80/sst-sample.bin`: first 100 tests of each of the 1,604 files in
  the **access form** (≈ 86 B/test, ≈ 14 MB measured; the per-T-state form measured 126.4 B/test ≈ 20 MB),
  uncompressed (xz only reaches 0.55, git already deflates), little-endian, read byte-wise (be32). Listed in
  `tests/roms/manifest.txt` so hygiene accepts it.
- **D-15:** Nightly full set via the archive tarball through the existing `scripts/fetch-pin.sh` with a
  `pins.txt` row (`https://github.com/SingleStepTests/z80/archive/ebe1875d48f374bcfd4b505d8eb8ee751568b5f7.tar.gz`,
  SHA-256 `f44683f0edf1c1c37f98861ccd1bfda3b9bce65b5b258d4817a0f1d2012076a7`, 288,807,342 B, measured once;
  ≈ 1.37 GB unpacked). The nightly regenerates the sample and requires it byte-equal to the committed file. If
  the archive digest changes twice, switch to a sparse partial clone.
- **D-16:** Seven `vectors` shards by prefix group (base 252 files, cb 256, dd 252, ddcb 256, ed 80, fd 252,
  fdcb 256); each prints and asserts its executed count (100/file on PR, 1,000 nightly); one negative control
  corrupts a byte of a copy and asserts the specific mismatch line; `tests/vectors/z80/allow.txt` starts empty,
  lines `YYYY-MM-DD <file> <test> <field> <upstream issue URL> <reason>`, and a line that matches no failure
  fails. Inventories: shards + control in `ci.txt`, nightly shards + sample-equality in a new nightly inventory.
  Measure the be32 (qemu) cost per shard before setting timeouts.
- **D-17:** Interrupt and NMI acceptance, EI delay, prefix runs, HALT exit and R across prefixes are not in the
  vectors: `tests/z80/int_tests.c` scripts /INT and /NMI around EI, DI, RETN, `LD A,I`, HALT and block ops
  (CONFORMANCE §1.1a).

### VDP boundary in phase 2
- **D-18:** A VDP **timing half** lands in `src/vdp.c`; no renderer. In phase 2: line and `t = ticks mod 684`
  dispatcher events at 604 (V counter step), 607 on line 192 (frame-interrupt pending; also the frame origin of
  DEC.31) and the D-03 Pause tick; V counter reads from the NTSC-192 and PAL-192 tables only; `$7F` live H counter
  from the SMS-VDP §4.3 formula, unit-tested against `t`; control port two-byte latch, code, address; R0–R10
  stored; data port with auto-increment and read-ahead buffer; VRAM 16 KiB and CRAM writes stored; status bit 7
  frame-pending, bits 4–0 per SMS-VDP choice 2.A, read clears flags and releases /INT; /INT = pending AND R1 IE0;
  ports decoded per profile (A7/A6/A0 mirrors on export SMS, full decode on Japan and GG).
- **D-19:** Explicitly phase 3: rendering and catch-up, sprites, overflow and collision, line counter / R10 / line
  interrupts at 608, 224/240-line modes and their V counter tables (phase 2 stores the M bits and treats the mode
  as 192), latches at 600, the H-counter TH latch and its edge, the 607-vs-≈654 settlement, the GG window,
  access-speed counters, `cogmaster_get_vdp`. The interrupt tick stays one private named constant so phase 3 can
  move it without invalidating any phase-2 evidence (frame length, Pause tick 611, V step 604 and the overshoot
  bound do not depend on it).
- **D-20:** HALT/DI bound test (CPU-02), a unit test in `ci`: images `F3 76` (DI; HALT) and `FB 76` (EI; HALT,
  IE0 clear); 10 frames each, every call stops `FRAME`, `overshoot_ticks` ≤ 69, total ticks = 10 × 179,208 (PAL
  214,092) plus the final overshoot, CPU still halted. Positive control: EI; HALT with IE0 set and a `$0038`
  handler that reads `$BF` and increments RAM → exactly +1 per frame. A hostile `OUT ($BF)/OUT ($BE)` loop covers
  the second half of CPU-02. A further test shows every bus access stamped at its data T-state on the lattice.

### Fixtures and the mailbox
- **D-21:** `cmake/WlaDx.cmake` builds WLA-DX v10.7 at configure time on every leg: a `wla-dx` row in `pins.txt`
  (tarball SHA-256 `38296a96bc20be873d17e0e88be0c5b20a15ef2ec4da5279600e56af30ad925a`), `file(DOWNLOAD …
  EXPECTED_HASH)` + `file(ARCHIVE_EXTRACT)`, a child `cmake` build of only `wla-z80` and `wlalink` (Release,
  `GEN_DOC=OFF`, Ninja), not inheriting the be32/wasm32 toolchain files. `option(COGMASTER_FIXTURES)` ON in
  `ci`, `ci-msvc`, `asan`, `be32`, `wasm32`, OFF in `library`. Negative control `fixtures.wla_pin.self_test`
  (a `file://` tarball with a wrong digest fails with a digest-mismatch message, no network). Windows arm64 has
  no upstream CI and upstream forces `/WX`: if it breaks, pass `/WX-` last. Build-once-and-share is only the
  fallback for a leg whose `hello` digest mismatches. Measure the build time before adding a timeout or cache.
- **D-22:** `.gitattributes`: add `/tests/roms/src/** text eol=lf` after the `/tests/roms/** -text` line (which
  otherwise disables LF normalisation for `.asm`). Fixture image digests go into `hashes.txt` via the existing
  `file(CONFIGURE … NEWLINE_STYLE UNIX)` path so `hash-equality` covers them.
- **D-23:** First fixture plan: `hello.asm` (`.SMSHEADER`, `.COMPUTESMSCHECKSUM`, `jp $`), digest pinned in
  `tests/built/expected.txt`; `fixtures.hello` checks the digest and `--probe checksum_ok=1`. Stays permanently.
- **D-24:** `cogmaster-run --mailbox` measures and prints only: stops at the first frame with status `$00` or at
  `--frames N`, checks magic and CRC-16, prints `status=`, `rom=`, `crc=ok` and one `NN=VVVV flags=F` line per
  record; exit statuses unchanged (0/1). A `tests/cmake/mailbox.cmake -P` judge (≈ 60 lines) reads
  `tests/diag/expected.csv` (`rom,test,profile,expected,source,citation`) and fails with distinct messages on a
  record outside its row, a record with no row or a row with no record, status not `$00` (`hang`), bad CRC or
  magic. Never judge with CTest `PASS_REGULAR_EXPRESSION` (any one pattern passes). Negative controls via
  `expect_fail.cmake`: one wrong value, one missing row, one `--frames 1` run (hang). Phase 3's `--expect`
  (exit 3) absorbs the same CSV rows.
- **D-25:** Phase-2 fixture records (sources in `tests/roms/src/` with `mailbox.inc`, `header.inc`, one linkfile per
  image; images built into the build tree, never committed, excluded from release archives):
  - `mapper_sega` (512 KB, `.EMPTYFILL $FF`, 16-byte stamp per bank): signature sums over banks 0–31 via slot 1 and
    slot 2; slot 0 below `$0400` still bank 0 after `$FFFD=5` while `$0400` reads bank 5; wrap (`$FFFE=35` → bank
    3, `$FF` → 31); `$DFFF` reads back the `$FFFF` write; cart RAM via `$FFFC` b3 (pattern, ROM, pattern) and RAM
    bank b2; shift b1–0 (source `doc`, SP.094). Power-on `$DFFC` values wait for hardware.
  - `mapper_codemasters` (128 KB, 8 pages): header as literal bytes (page count, BCD year, checksum/complement
    pair, `$7FEF = 0`, MAME-derived detection rules); reset banks 0,1,0; sums through `$0000`, `$4000`, `$8000`
    (slot-0 switch from a RAM stub with interrupts off); wrap; 8 KB RAM at `$A000` via `$4000` b7. The true
    Codemasters checksum (WLA-DX cannot compute it) is not asserted.
  - `ports` under `sms-export`, `sms-japan`, `sms-pal`, `gg`: region echo (`$3F` = `$F5` then `$55`, read `$DD`
    b7–6, SP.111); `$DD` b4/b5 and idle `$DC`; `$3E` and open-bus forms; `tv_loop` (raw count of the 31-cycle
    frame-length loop polling `$BF` b7, judged by range ≈ 1,924 NTSC / ≈ 2,288 PAL, plus the 2,048 verdict);
    `vc_lines` (V counter values between two line-0 readings: 262/313); GG `$00` Start/NJAP/NNTS bits and
    `$01`–`$06` reset values (SP.111/112). No H-latch records (phase 3).
  - Pause NMI in phase 2 is proved by the tick-level unit test (D-03); the fixture's `nmi_count` record and the
    `pause@100+200` / `pause-held@100` movies wait for `--input` (phase 3, phase 1 D-06).
  - The no-mapper 32/48 KB layout gets C register-level tests only.
  - Fixtures measure and `expected.csv` judges; frame cap → `hang`; no glyphs or `frame@F` predicates until phase 3;
    every new test name goes into the inventory files.

### Public API, profile and runner surface
- **D-26:** Header additions in phase 2 (each with its consumer and a `ci` test, phase 1 D-09; `COGMASTER_API_VERSION`
  1 → 2 in the first header-adding plan):
  - `cogmaster_probe(const void *image, size_t n, cogmaster_cart_info *out)`: pure, bounded, no instance; property
    test (same bytes → same output, never reads past `n`, input unchanged); also the `fuzz.loader` entry.
  - `cogmaster_cart_info {size, body_offset (0|512), header_at (0|$1FF0|$3FF0|$7FF0), checksum, checksum_ok,
    product_code, version, region_code, rom_size_code, machine guess, region guess, board guess}`.
  - Board constants (output only): `ROM_ONLY 1`, `SEGA 2`, `CODEMASTERS 3`; 0 = no supported board (with a status).
  - `cogmaster_peek(const cogmaster *m, uint32_t space, uint32_t addr)` with only `COGMASTER_SPACE_BUS`; no side
    effects (no status clears, read-buffer refill, open-bus update, TH latch or lag credit); property test: peeking
    all 64 KiB leaves the state hash unchanged. Consumer: `--mailbox`.
  - `cogmaster_media` appends `region` (`AUTO 0`, `EXPORT`, `JAPAN`) and `tv` (`AUTO 0`, `NTSC`, `PAL`) as two
    fields (DEC.42 keeps them separate; a combined region enum was considered and rejected without new evidence);
    `cogmaster_info` appends the resolved region and tv; fps num/den becomes PAL-correct.
  - Statuses: `ERR_UNSUPPORTED_BOARD` (opcode scan finds Korean, MSX/Zemina, 4PAK, Janggun or similar, or an 8 KB GG
    image), `ERR_UNSUPPORTED_SIZE` (outside 1 KiB ≤ n ≤ 1 MiB + 512, ARCHITECTURE §6); `ERR_PROFILE_REJECTED` gets
    its first use for PAL+GG and PAL+JAPAN. No `INVALID_IMAGE` (no phase-2 behaviour).
  - Deferred: `set_input`, `poke`, `get_memory`, `get_z80`, `get_counters` (phase 3 or later, when a host consumes
    them; phase-2 tests reach internals through `src/internal.h`); machine `SMS1` (phase 3), `fm` (phase 4).
- **D-27:** Runner: positional `FILE` (or `--rom FILE`), `--probe` (no emulation), `--trace first-fault`, `--mailbox`,
  `--region auto|ntsc-u|ntsc-j|pal` (a shorthand setting region and tv). `--machine auto` resolves header → `.gg`
  extension → SMS (Codemasters GG carts have no `TMR SEGA`). Exit 77 enters the table for `UNSUPPORTED_BOARD`,
  `UNSUPPORTED_SIZE`, `PROFILE_REJECTED`, and for a `.sg`/`.sc` image with the message
  `unsupported: SG-1000 (planned)` (DEC.03; the library refuses by hash once the CC0 table lands). The runner refuses
  files over 64 MB before reading.
- **D-28:** `--probe` format: one `key=value` per line, fixed order, every key always printed, keys only appended,
  listed in `docs/runner.md`. Integers decimal, digests lowercase hex, addresses `$7FF0`, missing header `none`,
  guesses as words (`machine_guess=sms|gg`, `region_guess=ntsc-u|ntsc-j|pal`, `mapper_guess=none|sega|codemasters|unsupported`).
  Keys: `size, copier_header, header_at, header_magic, checksum, checksum_ok, product_code, version, region_code,
  rom_size_code, machine_guess, region_guess, mapper_guess, sha1, sha256, crc32, md5, sha1_headerless,
  sha256_headerless, crc32_headerless, md5_headerless`. Exit 0, 77 for an unsupported board (keys still printed),
  1 on I/O failure.
- **D-29:** Digests are computed in `runner/` over the whole file and again over the body after a 512-byte copier
  header: SHA-256 and CRC-32 reuse `runner/sha256.c` and `runner/crc32.c`; new `runner/sha1.c` (RFC 3174) and
  `runner/md5.c` (RFC 1321). The core stays digest-free apart from CRC-32 and links only `mem*`. The
  RetroAchievements hash for SMS and GG is the MD5 of the whole file with no header stripping (rcheevos
  `src/hash/rc_hash.c`, generic whole-file case), so `md5` is that value. SHA-1 moves into `src/` in phase 3 for
  the CC0 table.
- **D-30:** Mapper guess without the table: strip a copier header when `size % 16384 == 512`; `TMR SEGA` at `$7FF0`,
  then `$3FF0`, then `$1FF0` (nibbles and checksum are lint only); Codemasters header at `$7FE0` (word sum at `$7FE6`
  and complement at `$7FE8`, page count plausible, BCD year, `$7FEF = 0`) → `codemasters`; body ≤ 48 KB → `none`;
  MAME-style `LD (nn),A` target count over the first 32 KB (SMS-CARTRIDGES §1.4): targets other than `$FFFF`/`$8000`
  families → `unsupported`; otherwise `sega`. Each branch has a synthetic-image unit test and a fuzz seed.

### First fault, loader and fuzz
- **D-31:** A fault means "nothing drives or latches this, or the guest uses a feature no shipped software uses".
  Probes real games make are counters (`open_bus_reads`), never faults, so the first fault stays informative.
  Phase-2 kinds (u8, append-only, 0 = none): `unmapped_read` (open bus returned: port reads `$00–$3F`, non-`$3E/$3F`
  ports in `$00–$3F` on full-decode profiles, slot-area reads with no source enabled, work RAM disabled),
  `unmapped_write` (port writes nothing latches on full-decode profiles), `rom_write` (write to `$0000–$BFFF` that is
  neither a mapper register on the detected board nor cart RAM), `mapper_range` (bank ≥ pages on a
  non-power-of-two image, or a `$FFFC` write setting b7, b4 or b1–0), `ed_undefined` (an undefined ED opcode
  executed). Phase 3 appends `vdp_fast_write`.
- **D-32:** Not faults: `$C0–$FF` reads with `$3E` b2 set, `$F0–$F2` on non-FM machines, `$DE/$DF`, I/O-chip
  writes, GG link ports `$01–$05`, bank wrap on power-of-two images, `$3E` toggling, undocumented DD/FD/CB forms
  and SLL.
- **D-33:** Devices (u8, append-only): 0 `none`, 1 `cpu`, 2 `cart`, 3 `ram`, 4 `mapper`, 5 `ctrl` (`$3E/$3F`);
  later `vdp`, `psg`, `io`, `fm`. `addr` is the full 16-bit bus address (`IN A,(n)` → A<<8|n); `opcode` is the first
  non-prefix byte (second byte for `ed_undefined`); `width` 8. Kind and device names are documented in
  `docs/embedding.md`; the header gains constants only where a test covers them (phase 1 D-09). Record calls sit in
  already-cold default arms (one branch).
- **D-34:** `--trace first-fault`: the runner checks `cogmaster_get_fault` after each frame and stops at the first
  fault (no new stop reason in the header). One stdout line with stable keys:
  `fault frame=12 line=191 dot=402 pc=$1A3F op=$DB addr=$0004 width=8 dev=ctrl kind=unmapped_read`, or `fault none`.
  Exit 0 either way (the fault is data); 2 usage, 77 refusals. HOSTS-AND-INTEGRATION §4's `kind=<read|write|exec>`
  vocabulary is amended to D-31/D-33. Test: a generated image doing `ld a,0 / in a,($00)` prints
  `kind=unmapped_read dev=ctrl addr=$0000`; a negative-control image prints `fault none`.
- **D-35:** Loader order: reject `UNSUPPORTED_SIZE` unless 1,024 ≤ n ≤ 1,048,576 + 512; strip 512 when
  `n mod 16,384 = 512`; copy into an instance-allocator buffer padded with `$FF` to a 16 KiB multiple; bank =
  value mod pages; RAM index & (size − 1); read a header only where the slot exists. Property test: a rejected
  load leaves the instance byte-identical.
- **D-36:** Fuzz: one `LLVMFuzzerTestOneInput` per target (`fuzz.loader` over `cogmaster_probe` + `load`;
  `fuzz.run` = fast boot, 2 frames). A plain C `tests/fuzz/replay.c` main feeds the same entry points on all six
  legs per PR (`fuzz.regress.loader`, `fuzz.regress.run`, timeouts scaled by `COGMASTER_TIMEOUT_SCALE`); a failure
  there is a harness or core bug to triage (XFER-049). libFuzzer runs nightly only on Ubuntu with clang-21
  `-fsanitize=fuzzer,address,undefined`, 60 s per target, `-timeout=2`, `-max_len=1049089` (replaces ENGINEERING's
  524,800 so the size-reject path is reachable), `continue-on-error` until ten green runs (DEC.51). Apple clang
  has no libFuzzer runtime and MSVC's is experimental.
- **D-37:** Seeds are generated at build time by `tests/fuzz/seeds.c` into the build tree: boundary sizes (1,023 /
  1,024 / 16 KiB + 512 / 48 KiB / 1 MiB / 1 MiB + 512 / + 513), `$7FF0` headers per region and size nibble with
  good and bad checksums, a Codemasters `$7FE0` header, tiny programs (DI;HALT, `JR $`, `in a,($00)`, `$FFFC`
  writes, `ld ($8000),a`, ED `$00`, a `$BE` hammer). The header magic is written as hex bytes (XFER-076). Only
  minimised regressions are committed, under `tests/fuzz/regress/`, each listed in `tests/roms/manifest.txt`;
  `tests/fuzz/**` stays ≤ 512 KiB.

### Claude's Discretion
- Plan breakdown and waves: z80 main page, z80 CB/DDCB, z80 ED, bus, mapper, io/ports, vdp timing half, cart/probe,
  fixtures toolchain, runner flags and fuzz share few inventories and can run as parallel waves (XFER-066); plans
  that share a test inventory stay sequential.
- Exact file split of `bus.c`, `io.c`, `mapper.c`, `cart.c`, `boot.c` (ARCHITECTURE §3 names them).
- Fast-boot hand-off details beyond DEC.43, the open-bus profile field values (DEC.42) and their unit tests.
- Arming the Ir/frame perf gate after the Z80 lands (ARCHITECTURE §12 "armed by phase 2") if the post-merge CI PR
  has supplied the two zero-spread control runs.

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Project rules and settled choices
- `AGENTS.md` (via `CLAUDE.md`) — rules 1–13, style, GSD summary format.
- `.planning/preparation/DECISIONS.md` — DEC.23, DEC.25, DEC.28–DEC.32, DEC.37–DEC.43, DEC.45, DEC.50–DEC.51, DEC.64.
- `.planning/LESSONS.md` — XFER-005, 013, 017, 049, 055, 056, 058, 059, 061, 066, 071, 073, 076, 079.
- `.planning/phases/01-a-test-frame-in-retroarch/01-CONTEXT.md` — D-01..D-14 (header growth rule D-09, size tags D-11, runner surface D-03..D-08).
- `.planning/REQUIREMENTS.md` — CPU-01..CPU-05; `.planning/ROADMAP.md` — phase 2 success criteria.

### Hardware
- `.planning/preparation/SMS-SYSTEM.md` §1–§5, §7, §8 — clocks, per-line timeline, Z80 model and interrupt acceptance, memory map, ports, regions, reset/Pause, power-on.
- `.planning/preparation/SMS-VDP.md` §2, §4 — registers, control port, status, V/H counters and tables (timing half only).
- `.planning/preparation/SMS-CARTRIDGES-AND-PERIPHERALS.md` §1, §2, §3.1–§3.2, §6 — header, mappers, port bits, Pause, hostile bounds.
- `.planning/preparation/SMSPOWER-EXTRACTS.md` — SP.045–SP.047 (interrupt and Pause timing), SP.094, SP.111–SP.112, §7 (WLA-DX pin).

### Architecture and conformance
- `.planning/preparation/ARCHITECTURE.md` §2, §3, §4, §6, §8, §9, §11, §13 — time base, modules, API rules, profiles, observability, boot (stale items listed in D-01..D-03, D-12).
- `.planning/preparation/CONFORMANCE.md` §1, §1.1, §1.1a, §2, §4, §6 — vectors, fixtures, mailbox, CI tiers (stale items listed in D-01, D-13, D-14).
- `.planning/preparation/ENGINEERING.md` §3, §4, §10 — tests and fuzzing, CI, style (800-line cap).
- `.planning/preparation/HOSTS-AND-INTEGRATION.md` §2, §4 — RetroAchievements hash, runner and exit table (fault vocabulary amended by D-34).
- `PROVENANCE.md`, `ASSET_POLICY.md` — what may be read, seeded or committed (vectors sample and fuzz regressions in the manifest).

### External primary sources
- SingleStepTests z80 (MIT) at `ebe1875d48f374bcfd4b505d8eb8ee751568b5f7`: https://github.com/SingleStepTests/z80 and issue 11 (Q/`qff`).
- Zilog UM0080 Z80 user manual; Sean Young, "The Undocumented Z80 Documented" (MEMPTR table); Cristian Dinu, "Decoding Z80 opcodes" (http://www.z80.info/decoding.htm); Patrik Rak's Q/SCF/CCF research (raxoft z80test).
- ares `component/processor/z80` (ISC) — structure cross-check only, never copied.
- SMS Power! TV type detection, region detection, display timing thread (forums/8161).
- rcheevos `src/hash/rc_hash.c` (MIT) — SMS/GG whole-file MD5.
- WLA-DX v10.7 `CMakeLists.txt` and release tag (tool only, built from the pinned tarball, never vendored).

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `src/fault.c` + `cogmaster_fault` + `cogmaster_get_fault`: first-fault recorder exists with no call sites; phase 2 adds them.
- `src/state.c` visitor: extend with Z80, bus/io, mapper, VDP-timing fields (SIZE/WRITE/VALIDATE/APPLY/RECORD sweep).
- `src/crc32.c` (core) and `runner/crc32.c`, `runner/sha256.c`: digests for `--probe`; add `runner/sha1.c`, `runner/md5.c`.
- `scripts/fetch-pin.sh` + `pins.txt`: pinned, digest-checked fetch for the nightly vector tarball and the WLA-DX tarball.
- `tests/cmake/hashes.cmake` (`file(CONFIGURE … NEWLINE_STYLE UNIX)`) and `hash-equality`: fixture image digests.
- `expect_fail.cmake`, `tests/inventory/*.txt`, `cmake/CogmasterTests.cmake`: negative controls and exact inventory.

### Established Patterns
- One registration file per subsystem (`tests/<area>/tests.cmake`); acceptance on exit status and tolerant regexes.
- Size-tagged structs with the first-released-size rule; header grows only with behaviour plus a test.
- `switch` dispatch, no pointer tables (`abi.globals`); no floats, no globals, no allocation after load.
- `src/internal.h` already included by `tests/core`, so internal tests need no public API.

### Integration Points
- `src/instance.c` `run_frame`: replace the test-card-only path with the Z80/bus loop when content is loaded; the no-content path stays on `testcard.c` (phase 1 D-15 pinned hash).
- `runner/main.c` and `runner/exit_status.h`: new flags and exit 77.
- `.github/workflows/ci.yml` and `nightly.yml`: vectors shards, fixtures, fuzz replay per PR; full vectors and libFuzzer nightly.

</code_context>

<specifics>
## Specific Ideas

- The owner's standing method: fan out each decision across stakeholder lenses with primary sources and an
  adversarial pass, synthesise one coherent recommendation, and follow it; "another copy and paste beats another
  dependency" (hence the C converter instead of Python, runner-side hashes from RFCs, no fuzz framework on PR legs).
- Measurements already taken (keep, do not re-derive): access-form rebuild 29,000/29,000; sample ≈ 14 MB; archive
  288,807,342 B with the SHA-256 in D-15; compression ratios xz 0.55, gzip 0.66.

</specifics>

<deferred>
## Deferred Ideas

- `run_loop.inc` traced/untraced dual compile → with hooks (v5); not needed for the vectors (D-12).
- `nmi_count` fixture record and Pause movies → phase 3 with `--input`.
- H-counter TH latch records and edge → phase 3.
- Codemasters true checksum assertion → when a tool can compute it.
- Machine `SMS1` profile → phase 3; `fm` → phase 4; `set_input`, `poke`, `get_memory`, `get_z80`, `get_counters` → when a host consumes them.
- `rom_write` noise check on the owner's local dumps (`library` preset) → drop the kind if several titles record it in their first frame.

### Reviewed Todos (not folded)
- `post-merge-ci-follow-ups.md` (arm the perf gate, first nightly issue, Dependabot, wasm32 promotion) — stays its own
  `ci:` pull request because it needs dispatches on `main`; ideally merged before phase 2 execution so the Z80's
  Ir/frame is gated.

</deferred>

---

*Phase: 02-the-cpu-matches-the-public-vectors*
*Context gathered: 2026-10-19*
