# Phase 2: The CPU matches the public vectors - Research

**Researched:** 2026-10-19
**Domain:** Z80 core verified per bus cycle, a tick-lattice bus, SMS/GG cartridge boards and ports, WLA-DX fixtures, runner diagnostics, fuzzing (C17, CMake 4, GitHub Actions)
**Confidence:** HIGH for upstream formats, pins and in-repo integration points (all checked against live sources this session); MEDIUM for cost estimates under qemu/wasmtime (not measured yet)

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions

#### Carried and corrected
- **D-01:** Codemasters lands in phase 2 (REQUIREMENTS CPU-03, ROADMAP). ARCHITECTURE §3/§11 and the CONFORMANCE §2
  fixture matrix still say P3 for `mapper_codemasters` and `ports`; the plan corrects those docs.
- **D-02:** The vector label is `vectors` (`ctest -L vectors`, as ROADMAP says), not `z80` (ARCHITECTURE §11).
- **D-03:** Pause /NMI is asserted at the /HSYNC rise, tick 611 of line **260** NTSC / **311** PAL, which is the
  start of the last frame line 261/312 (SP.047, Charles MacDonald's logic analyser: "the /HSYNC edge 611 mclk into
  line 260"). ARCHITECTURE §2, §5 and SMS-SYSTEM C8 say "tick 611 of line 261/312", one line late; the plan
  fixes them. A unit test asserts the NMI tick and the V counter value read at `$0066` entry (`$FF`/`$00`
  boundary distinguishes the two readings).

#### Z80 core structure
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

#### Per-cycle vector harness
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

#### VDP boundary in phase 2
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

#### Fixtures and the mailbox
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

#### Public API, profile and runner surface
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

#### First fault, loader and fuzz
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

### Deferred Ideas (OUT OF SCOPE)
- `run_loop.inc` traced/untraced dual compile → with hooks (v5); not needed for the vectors (D-12).
- `nmi_count` fixture record and Pause movies → phase 3 with `--input`.
- H-counter TH latch records and edge → phase 3.
- Codemasters true checksum assertion → when a tool can compute it.
- Machine `SMS1` profile → phase 3; `fm` → phase 4; `set_input`, `poke`, `get_memory`, `get_z80`, `get_counters` → when a host consumes them.
- `rom_write` noise check on the owner's local dumps (`library` preset) → drop the kind if several titles record it in their first frame.

#### Reviewed Todos (not folded)
- `post-merge-ci-follow-ups.md` (arm the perf gate, first nightly issue, Dependabot, wasm32 promotion) — stays its own
  `ci:` pull request because it needs dispatches on `main`; ideally merged before phase 2 execution so the Z80's
  Ir/frame is gated.
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| CPU-01 | `ctest -L vectors` runs the committed sample (first 100 tests of each of 1,604 files at `ebe1875d48f3`); every test matches final state (incl. MEMPTR, Q, R, IFF1/2, IM) and every bus cycle; nightly runs the full set | §"SingleStepTests at the pin" (format, file-name rule, cycle templates, `p`/`ei`/`q`/R semantics all re-checked this session); Pattern 2 (link seam); Pattern 3 (shared stream builder); Pitfalls 1–6 |
| CPU-02 | Bus driven by the Z80 on the 3-ticks/T-state, 684-ticks/line lattice; every access stamped at its data T-state; `run_frame` within one frame + ≤ 69 ticks for HALT/DI and a hostile write loop | Pattern 1 (cycle primitives); Pattern 4 (incremental line/t counters, no 64-bit modulo); Open Question 1 (where the beam starts) |
| CPU-03 | Sega mapper, no-mapper 32/48 KB, Codemasters: register-level tests and `mapper_sega`/`mapper_codemasters` fixtures built in CI with pinned WLA-DX v10.7; images padded to 16 KB multiples; banks wrap | §"WLA-DX v10.7" (pin re-checked: tarball digest and size match, build-script facts); Pattern 6 (`cmake/WlaDx.cmake`); Pitfalls 9–12 |
| CPU-04 | Ports `$3E $3F $7E $7F $BE $BF $DC $DD` and GG `$00–$06` per profile and region; Pause NMI at the start of the last line; `ports` fixture through the mailbox | SMS-SYSTEM §4/§5 facts collected in "Port and profile facts"; D-03 V-counter arithmetic in Code Examples; mailbox CRC check value computed |
| CPU-05 | `--probe` (header fields, region, size, mapper guess, SHA-1, SHA-256, CRC-32, RetroAchievements MD5); `--trace first-fault`; loader fuzz nightly with bounded `fuzz.run` replay per PR | rcheevos rule re-checked at `dfdc64ea035c` (whole file, cap 64 MiB); RFC check vectors; Pattern 7 (fuzz entry shared by libFuzzer and replay) |
</phase_requirements>

## Project Constraints (from CLAUDE.md / AGENTS.md)

These carry the same weight as locked decisions. The planner must check every plan against them.

- **Never GPL.** MIT/BSD/ISC/zlib/CC0 code may seed a module with its notice; GPL, LGPL, non-commercial and unlicensed emulator source is never opened and never kept in the workspace. WLA-DX is GPL-2.0-or-later: it runs as a *tool* built from the pinned tarball in the build tree, and its source is never copied, vendored or read for design (PROVENANCE §1).
- **Three deliverables only.** Library, runner, libretro core. No window, audio or input-device code.
- **Clean tree.** No ROM, BIOS or save bytes except files listed in `tests/roms/manifest.txt` (the file does not exist yet: phase 2 creates it). No personal paths, e-mail addresses or names. Noreply commit identity. `scripts/hygiene.sh` runs on every commit and push.
- **Deterministic, bounded core.** Integer-only, all state in the instance, same inputs → same hashes on every platform, every call bounded in emulated time even for a hostile ROM, no wall clock.
- **Tested and documented together.** Every behaviour change lands with a test that `cmake --workflow --preset ci` runs. README user-facing and short; testing/CI/maintainer docs in `docs/`; the public header stays terse (≤ 24 KB; currently 3,588 B).
- **Automation-first verification.** The owner verifies nothing by default; never ask for UAT.
- **Proportional gates.** Repository-check code is CMake `-P` or POSIX sh (from phase 3 under a tenth of `src/` lines). Every gate names the fault it prevents. At most two review fix rounds and one gap round.
- **No invented ceremony**; a failing harness assertion is a test bug to triage.
- **One GSD step per session**; never chain; never set `COGMASTER_ALLOW_CHAIN` or `COGMASTER_ALLOW_MAIN_COMMIT`. Record runtime and model in STATE.md.
- **Branch, then PR.** Work is on `phase/02-the-cpu-matches-the-public-vectors`.
- **Evidence is a command, an exit status and a run URL**, never a committed log; run from the repository root with no `cd` chains.
- **Pins are public**: verified with `gh api repos/<o>/<r>/commits/<sha>` (done for both pins this session, see Sources).
- **Plan SUMMARY format**: `requirements-completed:` plus a block-style `coverage:` list with `id`, `description`, `requirement`, `verification` (kind/ref/status), `human_judgment`; no flow style, no inline `#`; acceptance on exit status and tolerant regexes, never exact tool text.
- **Style**: small modules, plain control flow, fixed-width integers; public `cogmaster_`, internal `cogmaster__<module>_`; hardware comments cite a source; **no planning IDs (CPU-01, D-12, SP.047…) in code, test names or user docs**; files < 800 lines, functions ≤ 60 lines unless a dispatch switch of one-line cases (ENGINEERING §10, line 238).

## Summary

Phase 2 is a large but well-specified phase: CONTEXT.md already locks nearly every design choice, and this research re-checked the external facts those choices rest on. The SingleStepTests pin `ebe1875d48f3` is the head of the upstream default branch (2026-04-10, "Re-generate the tests."), MIT licensed, with exactly 1,604 files totalling 1,374,635,394 bytes. They split into the seven groups D-16 names (252/256/252/256/80/252/256), and the file names follow a closed rule, so the C converter can generate all 1,604 names itself and needs no directory listing. The per-T-state templates in D-12 match six sampled opcodes byte for byte (memory write, I/O read, I/O write, DD-indexed read-modify-write, LDIR repeat, HALT). The samples also settled two facts the preparation pack had slightly wrong. First, `p` is 1 in the final state after `LD A,I` (all 1,000 `ed 57` cases), not "never changes"; it is 0 after other instructions. Second, the refresh address in an M1 is `I<<8 | R` using R *before* its increment, and R's bit 7 survives the 7-bit wrap.

WLA-DX v10.7's tag tarball still hashes to the pinned SHA-256 (1,402,817 bytes, tag commit `91c52b1f4ef3`, latest release). Its build script has five properties `cmake/WlaDx.cmake` must handle:
- it caches `CMAKE_BUILD_TYPE` as RelWithDebInfo unless one is passed;
- it writes binaries to `<build>/binaries/`;
- it builds and *runs* a per-architecture table generator (`gen-z80`) at build time, so it must be built natively, never cross-compiled;
- it forces `/Za /W3 /WX` under MSVC;
- `GEN_DOC` defaults off when sphinx is absent.

The RetroAchievements rule is confirmed at rcheevos `dfdc64ea035c`: SMS and GG use the generic whole-file MD5, with no header stripping, capped at the first 64 MiB.

The existing phase 1 code gives clean seams. The instance struct and `cogmaster__fault_record` exist (no call sites yet). The 5-mode state visitor is the place to add Z80, bus, mapper and VDP fields. Test registration is one `tests/<area>/tests.cmake` per subsystem with an exact inventory, and *every new test must be listed in both `tests/inventory/ci.txt` and `tests/inventory/wasm32.txt`*. `fetch-pin.sh` already unpacks `.tar.gz` with the top directory stripped. The biggest integration risks are in the build and tooling, not the CPU:
- static-archive link order for the D-11 seam;
- `abi.undefined` on 32-bit targets if the core uses 64-bit `%`;
- `docs.api_names` and `docs.budgets` (docs/testing.md is already at 22.7 of 40 KB);
- `runner/main.c` (441 lines) crossing 800 lines;
- `hygiene.sh` rules that a 14 MB `.bin` and the new manifest must satisfy.

**Primary recommendation:**
- Build the vector pipeline first: a shared stream builder, then `sst2bin`, then the sample, then the harness with its negative control. Then grow the Z80 shard by shard against it.
- Compile the Z80 sources directly into the harness executable instead of linking `libcogmaster.a`, so the D-11 seam cannot collide with `bus.c`.
- Keep `line` and `t` as incremental `uint32_t` counters so the core never does 64-bit division.

## Architectural Responsibility Map

The usual web tiers do not apply. These are the tiers of an embeddable emulator.

| Capability | Primary Tier | Secondary Tier | Rationale |
|------------|-------------|----------------|-----------|
| Z80 execution, flags, WZ/Q/R, interrupt acceptance | Core (`src/z80*.c`) | — | All state in the instance; deterministic |
| Time lattice, dispatcher, decode, open bus | Core (`src/bus.c`) | Core (`src/vdp.c` events) | ARCH §2: bus is the only place time advances |
| Mapper boards, cart RAM, padding/wrap | Core (`src/mapper.c`, `src/cart.c`) | — | `switch (board)` dispatch (DEC.37) |
| Ports `$3E $3F $DC $DD`, GG `$00–$06`, region/TV readback | Core (`src/io.c`) | Core (profile in instance) | Profile fields per DEC.42 |
| V/H counters, status, control/data port, NMI tick | Core (`src/vdp.c` timing half) | — | D-18; no renderer |
| Header probe, board guess | Core (`cogmaster_probe`, pure) | Runner (prints) | Pure function, fuzzed |
| SHA-1, SHA-256, MD5, CRC-32 of the file | Runner (`runner/*.c`) | — | D-29: core stays digest-free except CRC-32 |
| Mailbox read and print | Runner (`--mailbox` via `cogmaster_peek`) | Test judge (`tests/cmake/mailbox.cmake`) | Runner measures, CSV judges (D-24) |
| First fault capture | Core (records) | Runner (`--trace first-fault` prints) | XFER-055 |
| Vector conversion and comparison | Test tooling (`tests/z80/`) | CI (nightly full set) | Host C tool, never in the library |
| WLA-DX build, fixture assembly | Build (CMake configure + custom commands) | CI legs | Tool output only; never vendored |

## Standard Stack

No package registry is involved. The stack is the existing toolchain plus two pinned downloads.

### Core
| Tool / source | Version / pin | Purpose | Why standard |
|---|---|---|---|
| C17 compiler (clang 21, GCC 15, MSVC) | as phase 1 | core, runner, harness, converter | DEC.52; already on all legs |
| CMake | ≥ 4.3 presets (local 4.4.3) | build, `-P` checks, `file(DOWNLOAD EXPECTED_HASH)`, `file(ARCHIVE_EXTRACT)` | presets file requires 4.3.0 [VERIFIED: CMakePresets.json `cmakeMinimumRequired` 4.3.0] |
| SingleStepTests `z80` v1 data | `ebe1875d48f374bcfd4b505d8eb8ee751568b5f7` (MIT) | per-cycle CPU vectors | [VERIFIED: gh api commits → 200, HEAD of default branch, 2026-04-10] |
| WLA-DX `wla-z80` + `wlalink` | v10.7, tarball SHA-256 `38296a96…925a`, 1,402,817 B, tag commit `91c52b1f4ef3` | assembles own fixtures | [VERIFIED: downloaded `https://github.com/vhelin/wla-dx/archive/refs/tags/v10.7.tar.gz` and `shasum -a 256` this session; `gh api releases/latest` = v10.7, 2026-06-28] |

### Supporting
| Tool | Version | Purpose | When |
|---|---|---|---|
| clang-21 libFuzzer + ASan/UBSan | Ubuntu 26.04 apt | `fuzz.loader`, `fuzz.run` nightly | nightly only (D-36) |
| qemu-ppc (be32 lane) | existing | vector shards on big-endian ILP32 | per PR |
| wasmtime 48.0.7 LTS | pinned in `pins.txt` | wasm32 lane | per PR, report-only |

### Alternatives Considered (all rejected by locked decisions; listed so no plan reopens them)
| Instead of | Could use | Why not |
|---|---|---|
| C converter `sst2bin.c` | Python converter | D-13: no Python dependency |
| Archive tarball nightly | sparse partial clone | D-15: only if the archive digest changes twice |
| Link seam | runtime hook / traced build | D-12 |

**Installation:** nothing to install from a registry. Both pins go in `pins.txt` rows (`name version url sha256`):
```
sst-z80 ebe1875d48f3 https://github.com/SingleStepTests/z80/archive/ebe1875d48f374bcfd4b505d8eb8ee751568b5f7.tar.gz f44683f0edf1c1c37f98861ccd1bfda3b9bce65b5b258d4817a0f1d2012076a7
wla-dx 10.7 https://github.com/vhelin/wla-dx/archive/refs/tags/v10.7.tar.gz 38296a96bc20be873d17e0e88be0c5b20a15ef2ec4da5279600e56af30ad925a
```
The row format comes from `pins.txt` line 1 ("one row each: name version url sha256") [VERIFIED: read pins.txt]. The `sst-z80` digest is from CONTEXT D-15 and was not re-downloaded this session. The archive URL itself answers `HTTP/2 200` with `content-disposition: attachment; filename=z80-ebe1875d48f374bcfd4b505d8eb8ee751568b5f7.tar.gz` [VERIFIED: `curl -sIL`].

## Package Legitimacy Audit

This phase installs **no** npm/PyPI/crates packages. It downloads two pinned source archives, each checked by SHA-256 before use.

| Artifact | Registry | Age | Source repo | Verdict | Disposition |
|---|---|---|---|---|---|
| WLA-DX v10.7 tag tarball | GitHub archive (not a package registry) | project since 1998; v10.7 released 2026-06-28 | github.com/vhelin/wla-dx | digest re-verified this session | Approved (tool; GPL-2.0-or-later, never vendored) |
| SingleStepTests z80 archive at `ebe1875d48f3` | GitHub archive | repo since 2024; pin 2026-04-10 | github.com/SingleStepTests/z80 | URL resolves; digest from CONTEXT | Approved (MIT data; sample committed with notice) |

**Packages removed due to [SLOP] verdict:** none. **Packages flagged [SUS]:** none. `gsd-tools package-legitimacy check` was not run because no registry package is proposed.

## SingleStepTests at the pin (facts the converter and harness rely on)

All facts below come from tool calls this session.

- **Files and groups.** 1,604 files, 1,374,635,394 bytes, `git/trees` not truncated [VERIFIED: gh api]. Name rule, which the converter generates with no directory listing [VERIFIED: computed from the tree]:
  - `base`: `%02x` for every byte except `cb dd ed fd` → 252
  - `cb %02x` → 256
  - `dd %02x` except `cb dd ed fd` → 252; `fd %02x` likewise → 252
  - `dd cb __ %02x` → 256; `fd cb __ %02x` → 256
  - `ed %02x` for `40–7f` plus `a0–a3 a8–ab b0–b3 b8–bb` → 80

  Smallest file `v1/00.json` 606,165 B; largest `v1/dd cb __ fd.json` 1,086,995 B. **File names contain spaces**: quote them in CMake and the shell.
- **Keys** in `initial` and `final` (identical sets): `a af_ b bc_ c d de_ e ei f h hl_ i iff1 iff2 im ix iy l p pc q r ram sp wz` [VERIFIED: `jq keys` on `v1/02.json`]. `cycles` is `[[addr, data|null, "rwmi"], …]`. `ports` is `[[addr16, value, "r"|"w"]]` or `null` [VERIFIED: README.MD at pin; `02.json`, `d3.json`].
- **Cycle templates** (test 0 of each file, verbatim) [VERIFIED]:
  - `02` LD (BC),A, 7 T: `[17818,null,"----"],[17818,null,"r-m-"],[57103,2,"----"],[57103,null,"----"],[35358,null,"----"],[35358,162,"-wm-"],[35358,null,"----"]`. M1 has opcode data at T3 on the refresh address; the write has data at T2.
  - `d3` OUT (n),A, 11 T: I/O write `[26271,null,"----"],[26271,null,"----"],[26271,102,"-w-i"],[26271,null,"----"]`, so the data sits with `-w-i` at T3.
  - `ed a2` INI, 16 T: one idle T at the M1 refresh address `[7251,null,"----"]`, then I/O read `…,[26210,null,"r--i"],[26210,91,"----"]` (data at T4), then the memory write.
  - `dd 34` INC (IX+d), 23 T: five idle T on the last address `44696` after reading d.
  - `ed b0` LDIR (repeating), 21 T: after the write, the write's T3 plus 7 idle T on the write address.
  - `76` HALT, 4 T: `final.pc = initial.pc + 1`, R + 1.
- **R and refresh.** The M1 T3 address is `I<<8 | R` using **R before the increment** (test 0 of `02`: i=223, r=15, refresh 57103 = $DF0F, final r=16). R's low 7 bits wrap and bit 7 is kept (r=127 → 0) [VERIFIED].
- **`p`.** Final `p` is 1 in all 1,000 `ed 57` (LD A,I) cases and 0 in every `02` case; initial `p` is random 0/1 [VERIFIED]. SMSPOWER-EXTRACTS SP.003's "never changes a shipped final state" is wrong for `LD A,I/R`; compare `p` exactly.
- **`ei`.** Final `ei` is 1 after `fb` (EI) and 0 after `02`, even when initial `ei` was 1 [VERIFIED].
- **`q`.** Initial `q` takes 217 distinct values in `02` (random or a copy of F). Upstream defines Q as "a copy of the final value of F if the instruction modifies the flags; otherwise, Q is 0". Instructions that *can* affect flags always copy F, even when F is unchanged [CITED: SingleStepTests/z80 issue 11, redcode 2025-11-03 and 2026-03-18, issue still open for `qff`].
- **Repeating block instructions.** In `ed b0` test 0 (PC $2D25, repeating), final F = $2C: bits 5 and 3 match PC high byte $2D [VERIFIED for one test]. This matches SP.018's rule (interrupted/repeating LDxR takes YF/XF from PC bits 13/11). INxR/OTxR also adjust P/V and H [CITED: SMSPOWER-EXTRACTS SP.018]. The `ed b*` shards will expose any deviation.
- **Not in the set.** ED holes (`ed 00–3f`, `80–9f` except the block ops, `c0–ff`), prefix runs (`dd dd`, `dd fd`, `dd ed`), interrupts and NMIs. These need `int_tests.c` and unit tests (D-17), including the `ed_undefined` fault and the 8-T NOP.

## Architecture Patterns

### System Architecture Diagram

```
                      cogmaster_load(media)                     cogmaster_probe(bytes) ──► cart_info (pure)
                               │                                        ▲
                               ▼                                        │ same parser
   image bytes ──► size check ─► strip 512 ─► pad $FF to 16 KiB ─► board guess ─► profile resolve (machine, region, tv)
                                                                        │                    │ reject PAL+GG / PAL+JAPAN
                                                                        ▼                    ▼
                                                                 fast-boot hand-off (DEC.43) ──► instance ready
   cogmaster_run_frame ──► loop: z80_step()  (≤ 23 T per step)
                               │  each primitive: ticks += 3 per T; at data T-state call bus_*(addr, data)
                               ▼
                        bus_fetch / bus_read / bus_write / bus_in / bus_out
                               │ 1. if tick ≥ next_deadline: fire dispatcher events (604 V step, 607 frame-INT on
                               │    line 192, 611 Pause NMI on line 260/311, line wrap at 684, frame end)
                               │ 2. decode: A15/A14 → RAM / mapper slots ; ports → A7/A6/A0 (partial) or full decode
                               ▼
         ┌─────────────┬──────────────┬───────────────┬──────────────┬─────────────────┐
         │ mapper.c    │ RAM $C000–   │ io.c          │ vdp.c timing │ open bus +      │
         │ none/sega/  │ $DFFF (+mirror│ $3E $3F $DC  │ $7E/$7F/$BE/ │ first fault     │
         │ codemasters │ $FFFC–F → reg)│ $DD, GG $00–6│ $BF, /INT,NMI│ (cold arms)     │
         └─────────────┴──────────────┴───────────────┴──────────────┴─────────────────┘
                               │
                               ▼  frame end tick reached at an instruction boundary → stop FRAME, overshoot ≤ 69
                        run_result {stop, overshoot_ticks}
   runner: --probe (no emulation; digests in runner/) | --mailbox (peek $C100–$C1FF each frame) | --trace first-fault
   tests:  sst harness links z80*.c + its own bus_*  (no bus.c, no library)
```

### Recommended Project Structure
```
src/
├── z80.h  z80_cycle.h  z80.c  z80_cb.c  z80_ed.c   # D-04
├── bus.c              # ticks, dispatcher, decode, open bus, seam definitions (bus_* ); fault calls
├── vdp.c              # timing half only (D-18)
├── io.c               # $3E $3F $DC $DD, GG $00–$06, region/TV
├── mapper.c           # switch (board): none, sega, codemasters
├── cart.c             # size rules, copier strip, header and Codemasters probe, opcode scan, cogmaster_probe
├── boot.c             # fast-boot hand-off
└── (existing) instance.c frame.c state.c fault.c crc32.c testcard.c
runner/  main.c (split: args.c, probe.c, mailbox.c) sha1.c md5.c
tests/
├── z80/ tests.cmake  sst_stream.c sst_stream.h  sst2bin.c  sst_bus.c  sst_run.c  int_tests.c
├── vectors/z80/ sst-sample.bin  allow.txt
├── bus/ mapper/ io/ cart/ (one tests.cmake each, ≤ 400 lines)
├── roms/ manifest.txt  src/{mailbox.inc,header.inc,hello.asm,mapper_sega.asm,mapper_codemasters.asm,ports.asm,*.link}
├── built/expected.txt   diag/expected.csv
├── fuzz/ seeds.c replay.c fuzz_loader.c fuzz_run.c regress/
└── cmake/ mailbox.cmake (judge)
cmake/WlaDx.cmake
```

### Pattern 1: Cycle primitives that advance the lattice
**What:** static inline primitives in `z80_cycle.h`. Each one adds 3 ticks per T-state and calls the bus once, at the data T-state. **When:** every access in every opcode.
```c
/* Source: D-04/D-10; T-state layout from SingleStepTests cycle streams (M1 data at T3,
   memory read T3, memory write T2 upstream / stamped per D-10, I/O read T4). */
static inline uint8_t m1(struct cogmaster__z80 *z, cogmaster *m)
{
    uint16_t refresh = (uint16_t)((uint16_t)z->i << 8 | z->r);  /* R before the increment */
    z->ticks += 3u * 2u;                                        /* T1, T2 */
    uint8_t op = cogmaster__bus_fetch(m, z->pc, refresh, z->ticks + 3u); /* sample at T3 */
    z->ticks += 3u * 2u;                                        /* T3, T4 */
    z->pc = (uint16_t)(z->pc + 1u);
    z->r = (uint8_t)((z->r & 0x80u) | ((z->r + 1u) & 0x7Fu));
    return op;
}
```
(The exact tick argument convention is the executor's choice under D-10. The harness only needs the T-state index of each access within the instruction.)

### Pattern 2: The link seam without archive collisions
**What:** D-11 makes `bus_*` real external functions. **Do not** link the harness against `libcogmaster.a`. A static archive pulls in `bus.o` as soon as `z80.o` needs any other symbol defined there (for example an interrupt-line query). That gives duplicate-symbol errors on ELF and `LNK2005` on MSVC, depending on link order. **Recommendation:** build the harness as `add_executable(cogmaster-test-sst tests/z80/sst_run.c tests/z80/sst_bus.c tests/z80/sst_stream.c ${PROJECT_SOURCE_DIR}/src/z80.c ${PROJECT_SOURCE_DIR}/src/z80_cb.c ${PROJECT_SOURCE_DIR}/src/z80_ed.c)`, compiled with the core's own options (as `tests/abi/tests.cmake` copies `abi_options`). Every symbol `z80*.o` references outside itself must be in the seam set the harness defines: `cogmaster__bus_fetch/read/write/in/out`, plus any `cogmaster__bus_int_line/nmi` query.

### Pattern 3: One stream builder shared by converter and harness
**What:** `tests/z80/sst_stream.c` turns a list of accesses `{kind, t_index, addr, data, refresh}` plus a total T count into the per-T-state `(addr, data|null, pins)` stream, using the D-12 templates.
- `sst2bin` uses it to **reject** any JSON test whose stream does not rebuild from its extracted accesses (D-13).
- The harness uses it to build the **expected** stream from the stored accesses and the **actual** stream from the core's logged accesses.

A mismatch then prints the first differing T-state with both triples. One implementation means the converter and the harness cannot disagree about the templates.

### Pattern 4: Incremental `line` and `t`, never `ticks % 684`
**What:** keep `uint32_t line, t` in the instance and advance them in the dispatcher; keep `uint64_t ticks` for totals only. **Why:** on ppc32, `uint64_t % 684` compiles to `__umoddi3`. That is allowed on that lane only [VERIFIED: tests/cmake/abi.cmake helper list `"^Linux/(ppc|powerpc)$:__udivdi3 __umoddi3 __ashldi3 __lshrdi3"`]. Any other 32-bit target (armhf nightly later: `__aeabi_uldivmod`) would fail `abi.undefined`, and it is slower everywhere.

### Pattern 5: Bounded run loop
```c
/* run_frame: deadline = frame_start + frame_len; step until ticks >= deadline at an
   instruction boundary. Every step (instruction, prefix byte, HALT M1, INT/NMI
   acceptance) costs <= 23 T = 69 ticks, so overshoot <= 69 (D-09). */
while (m->ticks < deadline) {
    cogmaster__z80_step(m);
}
overshoot = (uint32_t)(m->ticks - deadline);
```
The bound holds only if *no* step loops internally: no prefix loop, no block-op loop, and no HALT spin inside a step.

### Pattern 6: `cmake/WlaDx.cmake` (configure-time native tool build)
Facts from the v10.7 build script and docs [VERIFIED: read the build script and `doc/*.rst` in the downloaded tarball this session; the extracted tree was then deleted under PROVENANCE §1]:
- `cmake_minimum_required(VERSION 2.8.12...3.5 FATAL_ERROR)`, which CMake 4 accepts because the range maximum is 3.5.
- `set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING …)`, so pass `-DCMAKE_BUILD_TYPE=Release` explicitly.
- `set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/binaries/")`: tools land in `<wla-build>/binaries/wla-z80[.exe]` and `wlalink[.exe]`.
- Each `wla-<arch>` is built from a generated table produced by running `gen-<arch>` at build time. When cross-compiling it needs `CMAKE_CROSSCOMPILING_EMULATOR` or `IMPORT_GENERATORS`. So the child build must use the **host** compiler with no toolchain file, never the be32/wasm32 one.
- MSVC: `set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /Za /W3 /WX")`. This is the `/WX` D-21 mentions. The script appends `/WX` to `CMAKE_C_FLAGS`, so a `/WX-` passed in `CMAKE_C_FLAGS` would come *before* it. An override must land later on the command line, for example through `CMAKE_C_FLAGS_RELEASE` [ASSUMED: flag order not tested; try it only if the Windows arm64 leg actually fails].
- `option(GEN_DOC …)` defaults OFF when `sphinx-build` is not found; pass `-DGEN_DOC=OFF` anyway.
- `add_subdirectory(byte_tester)` and `add_subdirectory(doc)` exist. Build with `--target wla-z80 wlalink` so nothing else builds.
- Linking: `wlalink [OPTIONS] <LINK FILE> <OUTPUT FILE>`, `-r` for a ROM image, link file sections `[objects]` … [CITED: WLA-DX doc/linking.rst v10.7].
- `.COMPUTESMSCHECKSUM` sums bytes `0x0000–0x7FEF` (or `0x3FEF`/`0x1FEF` for 16/8 KB ROMs) into `$7FFA/$7FFB`. `.SMSHEADER` has `PRODUCTCODE`, `VERSION`, `REGIONCODE 3–7`, `RESERVEDSPACE`, `ROMSIZE` (auto if omitted), `CHECKSUMSIZE`, `FORCECHECKSUM`, `BASEADDRESS`. `.EMPTYFILL` defaults to `$00` [CITED: WLA-DX doc/asmdiv.rst v10.7].

Skeleton:
```cmake
# Fault prevented: fixtures assembled by an unpinned or tampered assembler.
set(wla_dir ${PROJECT_BINARY_DIR}/wla-dx)
file(DOWNLOAD ${COGMASTER_WLA_URL} ${wla_dir}/wla-dx.tar.gz
     EXPECTED_HASH SHA256=${COGMASTER_WLA_SHA256} STATUS st)          # values read from pins.txt
file(ARCHIVE_EXTRACT INPUT ${wla_dir}/wla-dx.tar.gz DESTINATION ${wla_dir}/src)  # top dir wla-dx-10.7/
execute_process(COMMAND ${CMAKE_COMMAND} -S ${wla_dir}/src/wla-dx-10.7 -B ${wla_dir}/build -G Ninja
                  -DCMAKE_BUILD_TYPE=Release -DGEN_DOC=OFF
                  -UCMAKE_TOOLCHAIN_FILE   # child is native: unset env CC/CFLAGS cross values too
                COMMAND_ERROR_IS_FATAL ANY)
execute_process(COMMAND ${CMAKE_COMMAND} --build ${wla_dir}/build --target wla-z80 wlalink
                COMMAND_ERROR_IS_FATAL ANY)
```
The archive's top directory name (`wla-dx-10.7`) is [ASSUMED] from GitHub's tag-archive convention. Resolve it with `file(GLOB … LIST_DIRECTORIES true)` instead of hard-coding it. `file(DOWNLOAD … EXPECTED_HASH)` fails with a hash-mismatch message, which the `file://` negative control can match with a tolerant regex.

### Pattern 7: One fuzz entry, two drivers
`tests/fuzz/fuzz_loader.c` defines `int LLVMFuzzerTestOneInput(const uint8_t *d, size_t n)`. `replay.c` is a plain `main` that reads each seed or regression file given on its command line and calls the same function. The nightly libFuzzer build links the target with `-fsanitize=fuzzer`; the PR build links it with `replay.c`. The seeds generator writes into the build tree, and the replay test globs that directory at test time, not configure time.

### Anti-Patterns to Avoid
- **A prefix or block loop inside one step**: it breaks the 69-tick bound and the "ROM of DD bytes" test (D-07).
- **Function-pointer opcode tables**: they land in `.data.rel.ro` under PIC and fail `abi.globals` (XFER-071, DEC.37).
- **Clearing 64 KiB of harness RAM per test**: 160,400 × 64 KiB ≈ 10.5 GB of `memset` per PR run, far more under qemu. Restore only the addresses in the test's initial RAM list and those the core wrote.
- **`PASS_REGULAR_EXPRESSION` for mailbox judging**: any one pattern passes (D-24).
- **Planning IDs in test names** (`cpu01.*`): forbidden. Use `vectors.base`, `z80.int.ei_delay`, `bus.halt_bound`, `mapper.sega.wrap`, `fixtures.mapper_sega`.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---|---|---|---|
| Pinned download + verify | a new curl/hash script | existing `scripts/fetch-pin.sh` (CI) and CMake `file(DOWNLOAD EXPECTED_HASH)` (configure) | fetch-pin already strips the top dir for `.tar.gz` [VERIFIED: scripts/fetch-pin.sh:49 `*.tar.xz | *.tar.gz) tar -xf "$file" -C "$dest" --strip-components=1 ;;`] |
| Hash file across lanes | a new compare script | `tests/cmake/hashes.cmake` write/compare and its `file(CONFIGURE … NEWLINE_STYLE UNIX)` [VERIFIED: hashes.cmake:23] | CRLF on Windows (XFER-061) |
| Negative controls | `WILL_FAIL` | `tests/cmake/expect_fail.cmake` (exact code + regex, LSan off) | XFER-013, XFER-079 |
| Test registration | raw `add_test` | `cogmaster_add_test(NAME … COMMAND … LABELS … TIMEOUT …)` | timeout scaling and exact inventory |
| SHA-256 / CRC-32 in runner | new code | `runner/sha256.c`, `runner/crc32.c` | already tested |
| SHA-1, MD5 | a library dependency | RFC 3174 / RFC 1321 reference algorithms rewritten small in `runner/` | D-29; "copy beats dependency" |
| State fields | ad-hoc serialisation | `cogmaster__sio_u8/u16/u32/u64` visitor (`src/state.h`) | VALIDATE/APPLY/RECORD sweep for free |
| Property tests | `theft` | in-repo seeded splitmix64 harness | XFER-073 |
| JSON parsing | a JSON library | a fixed-schema tokenizer in `sst2bin.c` (numbers, `null`, short strings, arrays, objects; fail on missing key, skip unknown) | D-13; the schema is closed and known |

**Key insight:** the only new external code is data (the vectors) and a tool (WLA-DX). Everything else reuses phase-1 machinery, so most of the risk sits in how the new files register with the existing gates.

## Runtime State Inventory

Not a rename or migration phase. One note on stored state: the phase adds state-visitor fields (Z80, bus/io, mapper, VDP timing, cart RAM). A phase-1 state no longer loads after them. That is expected (the format freezes in v3.0), but `core.state.*` tests must be updated in the same plan as each field group, and `COGMASTER_STATE_FORMAT_VERSION` should be bumped once.

## Common Pitfalls

### Pitfall 1: `p` treated as constant
**What goes wrong:** every `ed 57`/`ed 5f` vector fails on `p`. **Cause:** SP.003's paraphrase. **Avoid:** set `p=1` in LD A,I/R and clear it at the start of every other step. **Warning sign:** exactly 2,000 failures in the ED shard on field `p` (nightly), 200 on PR.

### Pitfall 2: Refresh address after the R increment
**What goes wrong:** every M1's T3 address is off by one. **Avoid:** form `I<<8|R` before incrementing (verified above).

### Pitfall 3: One "instruction" per harness call versus prefix steps
**What goes wrong:** under D-07 a DD/FD prefix is its own step, but an SST test is the whole prefixed instruction. **Avoid:** the harness steps until the core reports an instruction boundary (no pending `idx`), then compares. For `dd 00`-style tests the prefix step and the following NOP make one test.

### Pitfall 4: Q across prefixes and EX AF,AF'/POP AF
Instructions that *can* affect flags set Q = F even when F is unchanged. EX AF,AF', POP AF, a prefix byte and interrupt acceptance leave Q = 0 [CITED: issue 11; SP.010, SP.012].

### Pitfall 5: The sample and hygiene
- `.bin` matches hygiene's `ROM_NAME='\.(sms|gg|sg|sc|mv|bin|rom|sav|srm|state|eeprom)$'` [VERIFIED: scripts/hygiene.sh:40].
- A NUL byte makes the file "binary not in manifest".
- `tests/roms/manifest.txt` does not exist yet [VERIFIED: `ls tests/roms` fails]. Its format is tab-separated `path source pin licence sha256 notice` [CITED: ASSET_POLICY §2]. Create it in the same commit as the sample, or the pre-commit hook refuses the commit.
- The sample is MIT data, so add a `THIRD-PARTY-NOTICES.md` section (with `**Applies to:**` and `**Licence:** MIT`). `policy.notices` checks that every named path exists, and the libretro `.info` licence list must stay exactly equal to the notices' licences (MIT is already present).

### Pitfall 6: Shard runtime under qemu and wasmtime
The be32 scale is 8 and wasm32 is 5 [VERIFIED: CMakePresets.json comments]. Measure each shard's native time first, then set `TIMEOUT` so that timeout × scale covers it (D-16). Read the 14 MB sample with one `fread` into a heap buffer, not byte-wise `fgetc`. Under wasm32 the sample must be inside the preopened `--dir=${sourceDir}`, which `tests/vectors/` is.

### Pitfall 7: Exact inventory in two files
Every new test goes into `tests/inventory/ci.txt` **and** `tests/inventory/wasm32.txt` (the wasm32 list is ci minus the six libretro tests) [VERIFIED: diff of the two files]. Lines are sorted, as `name<TAB>labels` with labels sorted. The nightly vectors preset needs its own inventory file and a preset option that registers only those tests (the pattern `COGMASTER_HYGIENE`/`COGMASTER_NOFP` use in `tests/tests.cmake`). With `parallelization: false` and no worktrees in `.planning/config.json`, plans execute sequentially anyway, so every plan touching `ci.txt` is safe in order. "Waves" only express dependencies.

### Pitfall 8: Header growth gates
- `abi.api_version` requires every header name in some `tests/api/vK.c` with K ≤ `COGMASTER_API_VERSION`. Add `tests/api/v2.c` and bump to 2 in the first header plan [VERIFIED: tests/abi/tests.cmake self-test text "in no tests/api/vK.c with K <= 1"].
- `abi.symbols` requires defined external `cogmaster_*` names to equal the header's functions, and every other external name to start `cogmaster__`.
- `docs.api_names` requires every new `cogmaster_`/`COGMASTER_` header name to appear in `docs/embedding.md`, and every such name the guide writes to be declared, so `COGMASTER_FIXTURES` in that guide fails unless it is passed via `-DOTHER`.
- `internal.h` pins every public struct's offsets with `_Static_assert` and a `FIRST_SIZE` constant. Appended fields (`media.region`, `media.tv`, `info.region`, `info.tv`) add asserts. `FIRST_SIZE_MEDIA`/`INFO` stay at the first released sizes (4P and 48), since the size-tag rule accepts the old size.

### Pitfall 9: Docs budgets
`docs/*.md` < 40,960 B each; `docs/testing.md` is already 22,735 B [VERIFIED: wc -c]. Put the vectors, fixtures and mailbox docs in a new `docs/conformance.md` (or similar) instead of growing testing.md past budget. `docs/runner.md` (8,690 B) gains `--probe` keys, `--mailbox`, `--trace`, `--region` and exit 77.

### Pitfall 10: `runner/main.c` past 800 lines
It is 441 lines now. Adding file loading, four flags, probe printing and mailbox decoding will cross the ENGINEERING §10 cap. Split into `runner/probe.c` (digests and key=value printing), `runner/mailbox.c` (peek, CRC-16, records) and possibly `runner/args.c`. Add them to `COGMASTER_RUNNER_SOURCES`.

### Pitfall 11: WLA-DX checksum range versus the probe's
`.COMPUTESMSCHECKSUM` sums only `0x0000–0x7FEF` for images ≥ 32 KB [CITED: asmdiv.rst]. The BIOS-style range depends on the size nibble ($F = 128 KB, $1 = 512 KB) [CITED: SMS-CARTRIDGES §1.2]. So a 512 KB `mapper_sega` with an auto ROMSIZE nibble would print `checksum_ok=0` from our probe. That only matters if a test asserts it (D-23 asserts it for `hello` only, a 32 KB image with nibble $C, where both agree). Set `ROMSIZE` explicitly in `header.inc` so the image is deliberate.

### Pitfall 12: Line endings and `-text`
`/tests/roms/** -text` disables EOL normalisation [VERIFIED: .gitattributes line 3]. D-22's `/tests/roms/src/** text eol=lf` line must come **after** it (later lines win in gitattributes).

### Pitfall 13: GPL text in code paths
`hygiene.sh` flags `GNU General Public License` text in `cmake/`, `tests/`, `src/` and so on. `cmake/WlaDx.cmake` comments may say "GPL-2.0-or-later" (no full phrase) but must not paste licence text. The extracted tarball lives under `build/`, which is untracked.

### Pitfall 14: The frame origin and power-on beam (see Open Question 1)
D-18 makes (line 192, t 607) the frame origin. ARCHITECTURE §9 and the current `cogmaster_reset` doc say the beam starts at line 0 tick 0. D-20 expects exactly 10 × 179,208 ticks over 10 frames. Those three cannot all hold unless the beam starts at the origin.

### Pitfall 15: GG has no `$3E` in GG mode, and Pause differs
The GG profile decodes ports fully. Start is `$00` bit 7; it is an NMI only in SMS mode [CITED: SMS-SYSTEM §4–§5]. `region=JAPAN` on GG gives `$00` bit 6 = 0. PAL with GG is `PROFILE_REJECTED`.

## Code Examples

### Pause NMI tick and the V counter at `$0066` (D-03 arithmetic)
NTSC-192 V counter table: lines 0–218 read `$00–$DA`, lines 219–261 read `$D5–$FF` [CITED: SMS-SYSTEM §2, SP.052]. The CPU-visible value steps at t = 604 [CITED: SP.046]. The NMI is asserted at line 260, t 611. By then the counter already shows line 261's value, `$FF`. Worst-case handler entry: 611 + 69 (the current instruction finishes) + 33 (NMI acceptance, 11 T) = 713 ticks, so the handler starts no later than line 261, t 29. An `IN A,($7E)` there still reads `$FF`, because the step to line 0's `$00` happens at line 261 t 604. An NMI one line late (the stale reading) would read `$00`. That is the decisive bit.

### Mailbox CRC-16 (CCITT, poly `$1021`, init `$FFFF`)
```c
/* CONFORMANCE §4: CRC-16 over $C110–$C1FF. Check value for "123456789" = $29B1. */
static uint16_t crc16_ccitt(const uint8_t *p, size_t n)
{
    uint16_t c = 0xFFFFu;
    for (size_t i = 0; i < n; i++) {
        c ^= (uint16_t)(p[i] << 8);
        for (int b = 0; b < 8; b++) {
            c = (c & 0x8000u) ? (uint16_t)((c << 1) ^ 0x1021u) : (uint16_t)(c << 1);
        }
    }
    return c;
}
```
Check value `0x29b1` [VERIFIED: computed this session]. The Z80 side (`mailbox.inc`) must compute the same CRC; a unit test of the runner function against `0x29B1` pins it.

### Digest check vectors for `runner/sha1.c` and `runner/md5.c`
MD5("abc") = `900150983cd24fb0d6963f7d28e17f72`; SHA-1("abc") = `a9993e364706816aba3e25717850c26c9cd0d89d` [VERIFIED: `md5`, `shasum -a 1` locally; these are the RFC 1321 / RFC 3174 test vectors]. Add the empty string and a ≥ 64-byte input to catch padding bugs.

### RetroAchievements rule (rcheevos `dfdc64ea035c`, MIT)
`src/hash/rc_hash.c` lists `case RC_CONSOLE_GAME_GEAR:` and `case RC_CONSOLE_MASTER_SYSTEM:` in `rc_hash_from_file` under the comment `/* generic whole-file hash - don't buffer */` → `return rc_hash_whole_file(hash, iterator);` (lines 822–826), and in `rc_hash_from_buffer` → `return rc_hash_buffer(hash, iterator->buffer, iterator->buffer_size, iterator);` (lines 533–537). `MAX_BUFFER_SIZE 64 * 1024 * 1024` (`rc_hash_internal.h:26`) caps the hashed length [VERIFIED: fetched with gh api at the pin]. So `md5=` is MD5 of the whole file *including* any 512-byte copier header. The runner's 64 MB refusal (D-27) means the cap never applies.

### `fault` line (D-34) built from the existing record
```c
/* fields per include/cogmaster.h cogmaster_fault: frame (u64), line, dot, pc, addr, opcode, width, device, kind */
printf("fault frame=%llu line=%lu dot=%lu pc=$%04X op=$%02X addr=$%04X width=%u dev=%s kind=%s\n", ...);
```

## State of the Art

| Old approach | Current approach | When changed | Impact |
|---|---|---|---|
| Q as a boolean "flags changed" | Q = copy of final F when the instruction *can* affect flags, else 0 | upstream regeneration 2025-11 and 2026-04 | compare `q` byte-exactly |
| A DD/FD prefix transparent to Q | a prefix zeroes Q | SST regenerated 2026-04-10 (SP.010) | prefix is its own step (D-07) |
| CONFORMANCE §1.1 Python converter, per-T-state form | C converter, access form (≈ 86 B/test) | CONTEXT D-13/D-14 | no Python; 14 MB sample |
| Pause NMI "tick 611 of line 261" | tick 611 of line 260 | CONTEXT D-03 | fix ARCH §2, §5, SMS-SYSTEM C8 |
| `ctest -L z80` | `ctest -L vectors` | D-02 | fix ARCH §11 |

**Deprecated or outdated in the prep pack (the plan corrects these docs):** ARCH §3/§11 and the CONFORMANCE §2 matrix (Codemasters and `ports` in P3), ARCH §11 label `z80`, CONFORMANCE §1.1 `tools/sst2bin.py` and its 126 B/test layout, SMSPOWER-EXTRACTS SP.003's "never changes a shipped final state", HOSTS §4 fault vocabulary `kind=<read|write|exec>`, and ENGINEERING §3 `-max_len=524800`.

## Assumptions Log

| # | Claim | Section | Risk if wrong |
|---|---|---|---|
| A1 | The GitHub tag archive's top directory is `wla-dx-10.7/` | Pattern 6 | configure fails to find the sources; mitigated by globbing the extracted top directory |
| A2 | Native vector cost ≈ 5 µs/test, so ≈ 0.8 s per PR run; qemu ×8, wasmtime ×5 | Pitfall 6 | timeouts too tight; D-16 already requires measuring first |
| A3 | WLA-DX v10.7 builds on Windows arm64 MSVC with `/Za /WX` | Pattern 6 | that leg's configure fails; D-21 fallback `/WX-` or a shared Linux-built copy |
| A4 | Starting the beam at the frame origin (Open Question 1) breaks no phase-1 pinned hash | Pitfall 14 | test-card hashes are frame-content only and the no-content path keeps `testcard.c` (phase 1 D-15), so low risk |
| A5 | The archive SHA-256 in D-15 is still current (the URL was re-checked, the 288 MB body was not re-downloaded) | Standard Stack | the nightly fetch fails loudly with a digest mismatch; D-15 says what to do |
| A6 | `ed b*` repeat X/Y comes from the PC high byte for all repeating cases (one vector observed) | SST facts | ED shard failures; the vectors decide, not this note |

## Open Questions (RESOLVED)

1. **Where does the beam start after load and reset?**
   - What we know: D-18 makes (line 192, t 607) the frame origin; D-20 asserts 10 frames = exactly 10 × 179,208 ticks plus overshoot; ARCH §9, SMS-SYSTEM C7 and `docs/embedding.md` say "line 0, tick 0".
   - What's unclear: with the beam at line 0 tick 0, the first frame ends at line 192 t 607 after 131,935 ticks, which contradicts D-20's total.
   - Recommendation (Claude's discretion, "fast-boot hand-off details beyond DEC.43"): start `ticks = 0` at the frame origin (line 192, t 607, with that tick's frame-pending event treated as already fired and cleared). Every frame is then exactly 179,208/214,092 ticks, and update ARCH §9, SMS-SYSTEM C7 and `docs/embedding.md` in the same plan. The alternative is a short first frame, with the D-20 test counting from the end of frame 1.
   - RESOLVED: the recommendation is adopted in plan 02-06 (objective "Recorded choice"; frame origin in its own constants `BUS_ORIGIN_LINE`/`BUS_ORIGIN_TICK`, `bus.frame_origin`, and the ARCH, SMS-SYSTEM and `docs/embedding.md` corrections in its Task 3).
2. **Nightly vectors preset and inventory name.** ENGINEERING §2 names a `vectors-full` preset. Recommendation: preset `vectors-full`, option `COGMASTER_VECTORS_FULL`, inventory `tests/inventory/vectors-full.txt` (7 shards + `vectors.sample_equal`), run from `nightly.yml` after `scripts/fetch-pin.sh sst-z80 build/pins/sst-z80`.
   - RESOLVED: adopted as recommended in plan 02-17 (objective "Recorded choices", Task 1).
3. **Perf gate input.** The perf job currently benches the test card only. Once the Z80 runs, a ROM-driven perf case (for example the `mapper_sega` fixture, which is built in CI) is the natural Ir/frame subject. Whether that lands in phase 2 depends on the post-merge `ci:` PR (CONTEXT, Reviewed Todos).
   - RESOLVED: no ROM-driven perf case lands in phase 2, recorded in plan 02-17 (objective "Recorded choices"; ARCHITECTURE section 12 note in its Task 2): the fixtures stop in a HALT loop once measured, so they describe no real workload; phase 3's `boot_to_play` is the first representative subject.

## Environment Availability

| Dependency | Required by | Available (local) | Version | Fallback |
|---|---|---|---|---|
| CMake | everything | ✓ | 4.4.3 | — |
| Ninja | presets, WLA child build | ✓ | 1.13.2 | — |
| Apple clang | local builds | ✓ | 21.0.0 | — |
| Homebrew llvm@21 | hygiene lane | ✓ | 21 | — |
| gh | pins, fetch of vectors for sample generation | ✓ | 2.101.0 | — |
| jq | hygiene lane | ✓ | 1.7.1 | — |
| curl, tar | `fetch-pin.sh` | ✓ (system) | — | — |
| Network at configure | `file(DOWNLOAD)` of WLA-DX | ✓ (checked: GitHub reachable) | — | none: configure fails naming the URL |
| libFuzzer runtime | nightly fuzz | ✗ locally (Apple clang) | — | nightly Ubuntu clang-21 only (D-36); PR legs use `replay.c` |
| qemu-ppc | be32 | ✗ locally | — | CI lane only |
| WLA-DX | fixtures | built from pin at configure | 10.7 | — |

**Missing with no fallback:** none for the plan. **Missing with fallback:** libFuzzer and qemu are CI-only by design.

Generating the committed sample needs the 288 MB archive once on the executor's machine (`scripts/fetch-pin.sh sst-z80 build/pins/sst-z80`), then `sst2bin` over 1.37 GB. Keep that tree under `build/` (untracked) and never commit it.

## Validation Architecture

(`workflow.nyquist_validation` is `false` in `.planning/config.json`; this section is included because the orchestrator asked for it.)

### Test Framework
| Property | Value |
|---|---|
| Framework | CTest via `cogmaster_add_test`; C test programs using `tests/check.h` (`CHECK`, `CHECK_EQ_U64`, `CHECK_EQ_HEX`); CMake `-P` scripts for runner and judge checks |
| Config file | `cmake/CogmasterTests.cmake`, `tests/tests.cmake`, `tests/inventory/{ci,wasm32}.txt` (+ new `vectors-full.txt`) |
| Quick run command | `ctest --preset ci -L vectors` (after `cmake --build --preset ci`); per area `ctest --preset ci -R '^z80\.'` |
| Full suite command | `cmake --workflow --preset ci` |

### Phase Requirements → Test Map
| Req | Behaviour | Type | Automated command | File exists? |
|---|---|---|---|---|
| CPU-01 | 7 shards × (files × 100) match state and cycles; executed count printed and asserted | unit (data-driven) | `ctest --preset ci -L vectors` | ❌ Wave 0: `tests/z80/tests.cmake`, `sst_*.c`, `tests/vectors/z80/sst-sample.bin` |
| CPU-01 | negative control: corrupted copy → specific mismatch line | unit | `ctest --preset ci -R vectors.self_test` | ❌ |
| CPU-01 | allow-list line matching no failure fails | unit | `ctest --preset ci -R vectors.allow` | ❌ |
| CPU-01 | INT/NMI acceptance, EI delay, prefix runs, HALT exit, R across prefixes, ED holes as 8-T NOP | unit | `ctest --preset ci -R '^z80\.int'` | ❌ `tests/z80/int_tests.c` |
| CPU-01 (nightly) | full 1,604,000 + sample byte-equality | unit | `cmake --workflow --preset vectors-full` | ❌ preset + inventory |
| CPU-02 | every access stamped at its data T-state on the lattice | unit | `ctest --preset ci -R bus.stamps` | ❌ `tests/bus/` |
| CPU-02 | DI;HALT and EI;HALT 10 frames, overshoot ≤ 69, exact totals; IE0 positive control +1/frame; hostile `$BE/$BF` loop | unit | `ctest --preset ci -R 'bus\.(halt|hostile)'` | ❌ |
| CPU-03 | Sega / none / Codemasters register-level, padding, wrap, cart RAM, `$DFFC` mirror | unit | `ctest --preset ci -R '^mapper\.'` | ❌ `tests/mapper/` |
| CPU-03 | `fixtures.mapper_sega`, `fixtures.mapper_codemasters` through `--mailbox` + `mailbox.cmake` judge; `fixtures.hello` digest + `checksum_ok=1`; `fixtures.wla_pin.self_test` | integration | `ctest --preset ci -L fixtures` | ❌ |
| CPU-04 | port decode per profile/region; V/H counter tables; status read clears; Pause NMI at line 260 t 611 and V=$FF at `$0066` | unit | `ctest --preset ci -R '^(io|vdp)\.'` | ❌ |
| CPU-04 | `fixtures.ports.{sms-export,sms-japan,sms-pal,gg}` judged by `expected.csv`; judge negative controls (wrong value, missing row, hang) | integration | `ctest --preset ci -R fixtures.ports` | ❌ |
| CPU-05 | `--probe` key order and values on synthetic images; MD5/SHA-1 vectors; exit 77 cases | integration | `ctest --preset ci -R '^runner\.(probe|refuse)'` | ❌ |
| CPU-05 | `--trace first-fault` prints `kind=unmapped_read dev=ctrl addr=$0000`; control prints `fault none` | integration | `ctest --preset ci -R runner.trace` | ❌ |
| CPU-05 | `fuzz.regress.loader`, `fuzz.regress.run` on all legs; libFuzzer nightly | unit / nightly | `ctest --preset ci -L fuzz` | ❌ `tests/fuzz/` |
| CPU-05 | `cogmaster_probe` purity property; `peek` leaves state hash unchanged; rejected load leaves instance byte-identical | unit (property) | `ctest --preset ci -R '^cart\.'` | ❌ |

### Sampling Rate
- **Per task commit:** the area's `ctest --preset ci -R '<area>'` (< 30 s native).
- **Per plan (wave) merge:** `cmake --workflow --preset ci`.
- **Phase gate:** `cmake --workflow --preset ci` green locally; CI green on all legs, plus be32 and wasm32 report.

### Wave 0 Gaps
- [ ] `tests/z80/sst_stream.{c,h}`, `sst2bin.c`, `sst_bus.c`, `sst_run.c`, `tests/z80/tests.cmake`
- [ ] `tests/vectors/z80/sst-sample.bin` + `allow.txt` + `tests/roms/manifest.txt` + THIRD-PARTY-NOTICES section
- [ ] `tests/{bus,mapper,io,cart,fuzz}/tests.cmake` included from `tests/tests.cmake`
- [ ] `cmake/WlaDx.cmake`, `tests/roms/src/*`, `tests/built/expected.txt`, `tests/diag/expected.csv`, `tests/cmake/mailbox.cmake`
- [ ] `CMakePresets.json` `vectors-full` (and `fuzz`) presets; `tests/inventory/vectors-full.txt`

## Security Domain

`security_enforcement` is `false` in `.planning/config.json`, so no ASVS mapping. The relevant hardening is already locked as hostile-input bounds: D-27's 64 MB runner refusal, D-35's size and loader rules, D-36/D-37 fuzzing, and 69-tick bounded calls.

## Sources

### Primary (HIGH confidence)
- SingleStepTests/z80 at `ebe1875d48f374bcfd4b505d8eb8ee751568b5f7`: `gh api …/commits/<sha>` (200, 2026-04-10), `git/trees?recursive=1` (1,604 files, sizes), `README.MD`, `LICENSE` (MIT, 2024), raw `v1/02.json`, `d3.json`, `dd 34.json`, `ed a2.json`, `ed b0.json`, `ed 57.json`, `fb.json`, `76.json`; issue 11 comments.
- Archive URL `…/archive/ebe1875d….tar.gz`: `curl -sIL` → 200 attachment.
- vhelin/wla-dx v10.7: tarball downloaded, SHA-256 `38296a96…925a`, 1,402,817 B; `git/ref/tags/v10.7`; `commits/v10.7` = `91c52b1f4ef3` (2026-06-27); `releases/latest` = v10.7; build script and `doc/asmdiv.rst`, `doc/linking.rst` read; extracted tree deleted after reading.
- RetroAchievements/rcheevos at `dfdc64ea035c34aa168cf13bbda72b71ca4eba1e` (MIT): `src/hash/rc_hash.c` lines 502–560, 805–845; `rc_hash_internal.h:26`.
- This repository: `include/cogmaster.h`, `src/internal.h`, `src/fault.c`, `src/instance.c`, `src/frame.c`, `src/state.h`, `CMakeLists.txt`, `CMakePresets.json`, `cmake/CogmasterTests.cmake`, `tests/tests.cmake`, `tests/core/tests.cmake`, `tests/abi/tests.cmake`, `tests/cmake/{abi,expect_fail,hashes,api_names,notices,budgets,timeouts}.cmake`, `tests/inventory/*.txt`, `scripts/{fetch-pin,hygiene}.sh`, `pins.txt`, `.gitattributes`, `runner/main.c`, `runner/exit_status.h`, `.github/workflows/{ci,nightly}.yml`, `THIRD-PARTY-NOTICES.md`, `PROVENANCE.md`, `ASSET_POLICY.md`.
- Preparation pack: ARCHITECTURE, CONFORMANCE, SMS-SYSTEM, SMS-CARTRIDGES-AND-PERIPHERALS §1–2, SMSPOWER-EXTRACTS §0–1.4, 2.1, 4, 5, 7, DECISIONS, LESSONS, ENGINEERING §3, §10.

### Secondary (MEDIUM confidence)
- MEMPTR rules summary (gist drhelius/8497817, boo_boo et al., 2006) via WebFetch paraphrase. The vectors are the oracle; the summary is only a guide.

### Tertiary (LOW confidence)
- Per-test cost and qemu/wasmtime multipliers (A2), until measured.

## Metadata

**Confidence breakdown:**
- Standard stack and pins: HIGH. Both pins checked live; the WLA digest was recomputed.
- Vector format and semantics: HIGH. Six cycle templates and the `p`/`ei`/R semantics were checked on real files.
- Architecture and integration: HIGH. Based on reading the phase-1 code and gates.
- Pitfalls: HIGH for gate interactions (read from the gate scripts); MEDIUM for cost and timeouts.

**Research date:** 2026-10-19
**Valid until:** 2026-11-18 for the external pins (SingleStepTests issue 11 may still add `qff`; the converter ignores unknown keys).
