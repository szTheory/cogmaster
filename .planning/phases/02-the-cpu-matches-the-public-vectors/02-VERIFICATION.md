---
phase: 02-the-cpu-matches-the-public-vectors
verified: 2026-10-20T06:13:27Z
status: passed
score: 5/5 must-haves verified
covered_files:
  - ".github/workflows/ci.yml"
  - ".github/workflows/nightly.yml"
  - ".planning/phases/02-the-cpu-matches-the-public-vectors/02-01-PLAN.md"
  - ".planning/phases/02-the-cpu-matches-the-public-vectors/02-01-SUMMARY.md"
  - ".planning/phases/02-the-cpu-matches-the-public-vectors/02-02-PLAN.md"
  - ".planning/phases/02-the-cpu-matches-the-public-vectors/02-02-SUMMARY.md"
  - ".planning/phases/02-the-cpu-matches-the-public-vectors/02-03-PLAN.md"
  - ".planning/phases/02-the-cpu-matches-the-public-vectors/02-03-SUMMARY.md"
  - ".planning/phases/02-the-cpu-matches-the-public-vectors/02-04-PLAN.md"
  - ".planning/phases/02-the-cpu-matches-the-public-vectors/02-04-SUMMARY.md"
  - ".planning/phases/02-the-cpu-matches-the-public-vectors/02-05-PLAN.md"
  - ".planning/phases/02-the-cpu-matches-the-public-vectors/02-05-SUMMARY.md"
  - ".planning/phases/02-the-cpu-matches-the-public-vectors/02-06-PLAN.md"
  - ".planning/phases/02-the-cpu-matches-the-public-vectors/02-06-SUMMARY.md"
  - ".planning/phases/02-the-cpu-matches-the-public-vectors/02-07-PLAN.md"
  - ".planning/phases/02-the-cpu-matches-the-public-vectors/02-07-SUMMARY.md"
  - ".planning/phases/02-the-cpu-matches-the-public-vectors/02-08-PLAN.md"
  - ".planning/phases/02-the-cpu-matches-the-public-vectors/02-08-SUMMARY.md"
  - ".planning/phases/02-the-cpu-matches-the-public-vectors/02-09-PLAN.md"
  - ".planning/phases/02-the-cpu-matches-the-public-vectors/02-09-SUMMARY.md"
  - ".planning/phases/02-the-cpu-matches-the-public-vectors/02-10-PLAN.md"
  - ".planning/phases/02-the-cpu-matches-the-public-vectors/02-10-SUMMARY.md"
  - ".planning/phases/02-the-cpu-matches-the-public-vectors/02-11-PLAN.md"
  - ".planning/phases/02-the-cpu-matches-the-public-vectors/02-11-SUMMARY.md"
  - ".planning/phases/02-the-cpu-matches-the-public-vectors/02-12-PLAN.md"
  - ".planning/phases/02-the-cpu-matches-the-public-vectors/02-12-SUMMARY.md"
  - ".planning/phases/02-the-cpu-matches-the-public-vectors/02-13-PLAN.md"
  - ".planning/phases/02-the-cpu-matches-the-public-vectors/02-13-SUMMARY.md"
  - ".planning/phases/02-the-cpu-matches-the-public-vectors/02-14-PLAN.md"
  - ".planning/phases/02-the-cpu-matches-the-public-vectors/02-14-SUMMARY.md"
  - ".planning/phases/02-the-cpu-matches-the-public-vectors/02-15-PLAN.md"
  - ".planning/phases/02-the-cpu-matches-the-public-vectors/02-15-SUMMARY.md"
  - ".planning/phases/02-the-cpu-matches-the-public-vectors/02-16-PLAN.md"
  - ".planning/phases/02-the-cpu-matches-the-public-vectors/02-16-SUMMARY.md"
  - ".planning/phases/02-the-cpu-matches-the-public-vectors/02-17-PLAN.md"
  - ".planning/phases/02-the-cpu-matches-the-public-vectors/02-17-SUMMARY.md"
  - "CMakeLists.txt"
  - "CMakePresets.json"
  - "cmake/WlaDx.cmake"
  - "docs/conformance.md"
  - "docs/embedding.md"
  - "docs/maintaining.md"
  - "docs/runner.md"
  - "docs/testing.md"
  - "include/cogmaster.h"
  - "pins.txt"
  - "runner/args.c"
  - "runner/args.h"
  - "runner/exit_status.h"
  - "runner/mailbox.c"
  - "runner/mailbox.h"
  - "runner/main.c"
  - "runner/md5.c"
  - "runner/md5.h"
  - "runner/probe.c"
  - "runner/probe.h"
  - "runner/sha1.c"
  - "runner/sha1.h"
  - "src/boot.c"
  - "src/bus.c"
  - "src/cart.c"
  - "src/frame.c"
  - "src/instance.c"
  - "src/internal.h"
  - "src/io.c"
  - "src/mapper.c"
  - "src/state.c"
  - "src/vdp.c"
  - "src/z80.c"
  - "src/z80.h"
  - "src/z80_cb.c"
  - "src/z80_cycle.h"
  - "src/z80_ed.c"
  - "src/z80_x3.c"
  - "tests/abi/tests.cmake"
  - "tests/api/v1.c"
  - "tests/api/v2.c"
  - "tests/built/expected.txt"
  - "tests/bus/test_bus.c"
  - "tests/bus/test_vdp.c"
  - "tests/bus/tests.cmake"
  - "tests/cart/test_cart.c"
  - "tests/cart/tests.cmake"
  - "tests/cmake/abi.cmake"
  - "tests/cmake/budgets.cmake"
  - "tests/cmake/hashes.cmake"
  - "tests/cmake/mailbox.cmake"
  - "tests/core/test_core.c"
  - "tests/core/test_state.c"
  - "tests/core/tests.cmake"
  - "tests/diag/expected.csv"
  - "tests/fixtures/tests.cmake"
  - "tests/fuzz/fuzz.h"
  - "tests/fuzz/fuzz_loader.c"
  - "tests/fuzz/fuzz_run.c"
  - "tests/fuzz/replay.c"
  - "tests/fuzz/seeds.c"
  - "tests/fuzz/tests.cmake"
  - "tests/header/header.c"
  - "tests/header/header.cpp"
  - "tests/install/tests.cmake"
  - "tests/inventory/ci.txt"
  - "tests/inventory/fuzz.txt"
  - "tests/inventory/nofp.txt"
  - "tests/inventory/vectors-full.txt"
  - "tests/inventory/wasm32.txt"
  - "tests/io/test_io.c"
  - "tests/io/tests.cmake"
  - "tests/mapper/test_mapper.c"
  - "tests/mapper/tests.cmake"
  - "tests/roms/manifest.txt"
  - "tests/roms/src/header.inc"
  - "tests/roms/src/hello.asm"
  - "tests/roms/src/hello.link"
  - "tests/roms/src/mailbox.inc"
  - "tests/roms/src/mapper_codemasters.asm"
  - "tests/roms/src/mapper_codemasters.link"
  - "tests/roms/src/mapper_sega.asm"
  - "tests/roms/src/mapper_sega.link"
  - "tests/roms/src/ports.asm"
  - "tests/roms/src/ports.link"
  - "tests/roms/src/ports_gg.asm"
  - "tests/roms/src/ports_gg.link"
  - "tests/runner/test_runner.c"
  - "tests/runner/tests.cmake"
  - "tests/tests.cmake"
  - "tests/vectors/z80/allow.txt"
  - "tests/vectors/z80/sst-sample.bin"
  - "tests/z80/fixtures/accept/00.json"
  - "tests/z80/fixtures/bad_stream/00.json"
  - "tests/z80/fixtures/missing_key/00.json"
  - "tests/z80/fixtures/unknown_key/00.json"
  - "tests/z80/int_tests.c"
  - "tests/z80/sst2bin.c"
  - "tests/z80/sst_bus.c"
  - "tests/z80/sst_bus.h"
  - "tests/z80/sst_run.c"
  - "tests/z80/sst_stream.c"
  - "tests/z80/sst_stream.h"
  - "tests/z80/tests.cmake"
covered_digest: "v3:sha256:ee7e0ed371dcb6a1e1dc18ff30f2d4cfcfcb30e60da6813a4cc3c429dc450072"
behavior_unverified: 0
overrides_applied: 0
---

# Phase 2: The CPU matches the public vectors. Verification Report

**Phase Goal:** The Z80, the tick-lattice bus, the three v1.0 boards, the ports and the Pause NMI are proven against public vectors and own fixtures before any game runs, and every later stall is diagnosable from the first-fault trace.
**Verified:** 2026-10-20T06:13:27Z
**Status:** passed
**Re-verification:** No. This is the initial verification, run after code-review fix round 1.

## Goal Achievement

### Observable Truths (ROADMAP success criteria)

| # | Truth | Status | Evidence |
|---|-------|--------|----------|
| 1 | `ctest -L vectors` passes on the first 100 tests of each of the 1,604 files at `ebe1875d48f3`, matching final state (including MEMPTR, Q, R, IFF1/2, IM) and every bus cycle, and the nightly lane runs the full set (CPU-01) | VERIFIED | `ctest -L vectors -N` lists 13 tests. Running `cogmaster-test-sst` directly on each group printed `failed 0` with executed counts 25200/25600/25200/25200/25600/8000/25600, which sum to 160,400 = 1,604 x 100. `tests/z80/sst_run.c` compares all 25 fields (a..l, i, r, shadow pairs, ix, iy, pc, sp, wz, ei, iff1, iff2, im, p, q) and the final RAM. It checks that every access is stamped on a T-state boundary and compares the per-T-state stream (addr, data, pins), rebuilding both sides through the one `sst_stream_build`. `allow.txt` holds only its format comment, so nothing is excused. The negative controls `vectors.self_test` and `vectors.allow.self_test` pass. Running `cmake --workflow --preset vectors-full` locally at HEAD exited 0: 8/8 tests passed and the base shard printed `executed 252000`. The nightly run 39377102899 (job `vectors`) succeeded at de88bb9, and the later commits do not touch `src/z80*.c`. |
| 2 | A test shows every bus access stamped at its data T-state on the 3-tick, 684-tick-per-line lattice, and `run_frame` returns within one frame plus at most 69 ticks for a HALT/DI loop and for a hostile write loop (CPU-02) | VERIFIED | `tests/bus/test_bus.c`: `bus.stamps` checks VDP access ticks = start + 3 x data T-state for OUT (n),A, OUT (C),A, OUTI and IN A,(n), and the instruction lengths of memory instructions. The vector harness checks the stamp of every memory and I/O access across all vectors. `bus.halt_bound.di` and `.di_pal` (F3 76) and `bus.hostile_vdp_loop` (OUT $BF/$BE, JP loop, NTSC and PAL) assert `overshoot_ticks <= 69` on each of 10 frames and exact totals (179,208 and 214,092 per frame). All passed in the local `ci` workflow run. |
| 3 | The Sega, no-mapper and Codemasters boards pass register-level tests, and the `mapper_sega` and `mapper_codemasters` fixtures, built in CI with pinned WLA-DX v10.7, report expected values through the RAM mailbox, with 16 KB padding and bank wrap (CPU-03) | VERIFIED | The `mapper.sega.{banks,wrap,ram,mirror,shift}`, `mapper.codemasters.{banks,ram,wrap}`, `mapper.none.layout`, `mapper.pad_wrap` and `mapper.slot_off` tests pass. `pins.txt` holds the row `wla-dx 10.7 ... 38296a96...925a`, and `cmake/WlaDx.cmake` uses `file(DOWNLOAD ... EXPECTED_HASH)`. Running `cogmaster-run build/ci/fixtures/mapper_sega.sms --mailbox` printed status=00, magic=ok, crc=ok and records 01-0A, each equal to its `tests/diag/expected.csv` row, including 05 bank_wrap=1F03. `mapper_codemasters` printed 01-07, all equal to the CSV, including reset banks 0010 and wrap 0703. The `fixtures.judge.{wrong_value,missing_row,hang}` negative controls and `fixtures.wla_pin.self_test` pass. |
| 4 | The `ports` fixture reports the profile's values for $3E, $3F, $7E, $7F, $BE, $BF, $DC, $DD and GG $00-$06 per machine profile and region, and a test shows the Pause NMI firing at the start of the last line (CPU-04) | VERIFIED | I ran the `ports` fixture under sms ntsc-u, ntsc-j and pal, and `ports_gg` under gg. Every record matched expected.csv for its profile: region_echo 00C0/0000/00C0/C0C0, tv_loop 0787 (NTSC) and 08FD (PAL), tv_verdict 0/0/1, vc_lines 0106/0139, and GG 0D-10 = 00C0, FF7F, FF00, FF00. The four `fixtures.ports.*` CTests pass. `vdp.pause_nmi` latches Pause on line 100 and asserts that `nmi_pending` rises within the step that crosses tick 611 of line `lines-2`, which is /HSYNC starting the last line (`VDP_NMI_TICK 611` in src/vdp.c). The test checks that $0066 is taken, that the handler's `IN A,($7E)` stores $FF, and that one press gives one NMI. It runs for NTSC and PAL, plus a GG negative case. |
| 5 | `cogmaster-run --probe` prints header fields, region, size, mapper guess, SHA-1, SHA-256, CRC-32 and RA MD5; `--trace first-fault` prints PC, opcode, address, width, device, frame, line and dot for a ROM touching an unmapped device; the bounded `fuzz.run` regression replays the loader seeds per PR (CPU-05) | VERIFIED | `--probe mapper_sega.sms` printed all 21 keys. sha1, sha256, md5 and crc32 each equal what `shasum -a1`, `shasum -a256`, `md5` and Python `zlib.crc32` give for the same file. On `size-16k-512.sms`, `md5` is the whole-file MD5 and `md5_headerless` equals the MD5 of `tail -c +513`. `unsupported-board.sms` gave mapper_guess=unsupported and exit 77, and the Codemasters seed gave codemasters. `--trace first-fault` on `in-a-00.sms` printed `fault frame=0 line=192 dot=329 pc=$0002 op=$DB addr=$0000 width=8 dev=ctrl kind=unmapped_read`, and `di-halt.sms` printed `fault none` (exit 0). `fuzz.regress.loader` and `fuzz.regress.run` (label `portable`) pass in `ci`. The nightly `fuzz` job in run 39377102899 succeeded. |

**Score:** 5/5 truths verified (0 present, behavior-unverified)

Every truth here is behavior-dependent (state transitions, the frame bound, the NMI timing). Each was upgraded to VERIFIED only by a named passing test or by a runner command I ran and whose output I checked against the expected values, not by the presence of symbols.

### Plan-level must-haves (spot-checked beyond the roadmap SCs)

| Plan truth | Evidence |
|-----------|----------|
| Undefined ED opcode records first fault kind ed_undefined (02-06) | `z80.ed.holes` passes; seam `cogmaster__bus_ed_undefined` defined in both harness and library bus |
| No function-pointer table; mappers dispatch through a switch on board (02-03, 02-09) | grep finds no function-pointer declarators in `src/z80*.c`, `mapper.c`, `bus.c` or `io.c`; `mapper.c` has three `switch (m->cart.board)` |
| Core never reads a wall clock; no allocation after load (02-06/07) | no `time(`, `clock(` or `gettimeofday` in `src/`; no `malloc` or `realloc` in `src/` (instance allocator only) |
| Probe/core compute no digest other than CRC-32 (02-09/11) | no sha1/sha256/md5 in `src/*.c`; digests live in `runner/` |
| No planning identifiers in code, test names or docs | grep for `CPU-0n`, `D-nn`, `WR-0n` and `IN-0n` over src, runner, include, tests and docs finds nothing |
| Interrupt/NMI/EI/HALT/prefix-run semantics (02-05) | `z80.int.{im0,im1,im2,ei_delay,di,nmi,retn,ld_a_i,halt_exit,block_op,prefix_run,r_prefixes}` all pass |
| PAL 313 lines, profile rejection, region/TV state round-trip (02-07) | `bus.halt_bound.di_pal` and the state tests pass; the ports fixture under pal reports vc_lines 0139 (313) |
| Peek has no side effects; mailbox judge (02-13) | `runner.mailbox.crc` and `runner.mailbox.stop` pass, and the judge negative controls pass |

### Required Artifacts

| Artifact | Status | Details |
|----------|--------|---------|
| `src/z80.c`, `z80_cb.c`, `z80_ed.c`, `z80_x3.c`, `z80_cycle.h` | VERIFIED | Full decode; vectors pass; compiled into both library and harness |
| `src/bus.c`, `vdp.c`, `io.c`, `mapper.c`, `cart.c`, `boot.c`, `fault.c` | VERIFIED | Wired through `cogmaster_run_frame` / `cogmaster_load`; exercised by bus/vdp/io/mapper/cart tests and fixtures |
| `tests/z80/sst_run.c`, `sst_stream.c`, `sst_bus.c`, `sst2bin.c` | VERIFIED | Harness and converter share the stream builder; refusals tested |
| `tests/vectors/z80/sst-sample.bin` (13,017,674 bytes) | VERIFIED | Below 16 MiB; `vectors.sample_equal` (nightly and local vectors-full) proves it equals regeneration from the pin |
| `tests/vectors/z80/allow.txt` | VERIFIED | Format comment only; no excused failures |
| `cmake/WlaDx.cmake`, `tests/roms/src/*.asm`, `tests/diag/expected.csv`, `tests/cmake/mailbox.cmake` | VERIFIED | Fixtures built under `build/ci/fixtures/`; judged per profile |
| `runner/probe.c`, `sha1.c`, `md5.c`, `sha256.c`, `crc32.c`, `mailbox.c` | VERIFIED | Outputs cross-checked against system tools |
| `tests/fuzz/fuzz_loader.c`, `fuzz_run.c`, `replay.c`, `seeds.c` | VERIFIED | Replay runs per PR; libFuzzer build is nightly |
| `tests/inventory/vectors-full.txt` | VERIFIED | Nightly inventory, checked by `check_junit.cmake` in the nightly job |

### Key Link Verification

| From | To | Via | Status |
|------|----|-----|--------|
| `tests/z80/tests.cmake` | `src/z80.c` (+cb, ed, x3) | object library compiled into the harness, never linking libcogmaster | WIRED |
| `sst_run.c` | `sst_stream.c` | `sst_stream_build` for both the expected and the actual stream | WIRED |
| `src/z80.c` | `src/bus.c` → `vdp.c` / `io.c` / `mapper.c` | seam functions; the dispatcher fires VDP line events | WIRED (fixtures show it end to end) |
| `src/vdp.c` | `z80.nmi_pending` / `int_line` | set at tick 611 and tick 607 | WIRED (`vdp.pause_nmi`, `bus.halt_bound.ie_positive`) |
| `runner/main.c` | `cogmaster_get_fault` | `--trace first-fault` after each frame | WIRED (observed output) |
| `runner/mailbox.c` | `cogmaster_peek` | reads the mailbox records | WIRED (observed output) |
| `.github/workflows/nightly.yml` | `pins.txt` / `vectors-full` | `scripts/fetch-pin.sh sst-z80` then `cmake --workflow --preset vectors-full` (lines 142-143); `fuzz` job runs `--preset fuzz` | WIRED (nightly run success) |

### Data-Flow Trace (Level 4)

| Artifact | Data | Source | Real data | Status |
|----------|------|--------|-----------|--------|
| `--mailbox` output | records $C110-$C1FF | fixture ROM run on the core, read via `cogmaster_peek`, CRC checked | Yes: profile-dependent values differ across ntsc-u, ntsc-j, pal and gg | FLOWING |
| `--trace first-fault` | fault record | `cogmaster__fault_record` in cold default arms during emulation | Yes: line, dot and pc vary with the image | FLOWING |
| `--probe` digests | file bytes | runner digests over the file and the body | Yes: equal to system tools | FLOWING |

### Behavioral Spot-Checks

| Behavior | Command | Result | Status |
|----------|---------|--------|--------|
| Whole PR suite | `cmake --workflow --preset ci` (from repository root) | exit 0; 183/183 + 4/4 tests passed | PASS |
| Hygiene lane | `cmake --workflow --preset hygiene` | exit 0 | PASS |
| Full vector set | `cmake --workflow --preset vectors-full` (pins already fetched under `build/pins/sst-z80`) | exit 0; 8/8 passed | PASS |
| Vector shards | `build/ci/tests/cogmaster-test-sst --sample tests/vectors/z80/sst-sample.bin --group G --allow tests/vectors/z80/allow.txt` for each of 7 groups | exit 0, failed 0, counts sum 160,400 | PASS |
| Mapper fixtures | `build/ci/cogmaster-run build/ci/fixtures/mapper_{sega,codemasters}.sms --mailbox --frames 600` | exit 0; records equal expected.csv | PASS |
| Ports fixture per profile | `build/ci/cogmaster-run --machine sms --region {ntsc-u,ntsc-j,pal} build/ci/fixtures/ports.sms --mailbox --frames 600`, and `--machine gg ports_gg.sms` | records equal expected.csv for each profile | PASS |
| Probe digests | `build/ci/cogmaster-run --probe build/ci/fixtures/mapper_sega.sms`, compared with `shasum`, `md5` and `zlib.crc32` | all four equal | PASS |
| First-fault trace | `build/ci/cogmaster-run build/ci/tests/seeds/in-a-00.sms --frames 5 --trace first-fault` | the fault line carries all 8 fields | PASS |

### CI evidence (rule 11)

- `gh run view 39377100295`: CI, conclusion success, head de88bb9 (phase branch, before the review fixes).
- `gh run view 39377102899`: Nightly, conclusion success, head de88bb9. Its vectors, fuzz, bench and report jobs all succeeded, and the vectors job log shows `100% tests passed out of 8`.
- The 7 commits 775b29c..b8dad7a (review fixes and docs) are local and unpushed, so they have no CI run yet. I ran the `ci` and `vectors-full` workflows on them locally (exit 0 each). Under rules 9 and 10, CI on the head commit is confirmed at ship, so this is not a gap. The sanitizer lanes are evidenced only in CI, because clang-21 ASan hangs on this macOS host.

### Probe Execution

There are no `scripts/*/tests/probe-*.sh` files, and the plans declare no probe scripts. Here "probe" means `cogmaster-run --probe`, a runner feature I exercised above.

### Requirements Coverage

| Requirement | Source Plans | Status | Evidence |
|-------------|--------------|--------|----------|
| CPU-01 | 02-01, 02-02, 02-03, 02-04, 02-05, 02-17 | SATISFIED | Truth 1 |
| CPU-02 | 02-06, 02-07, 02-08, 02-17 | SATISFIED | Truth 2 |
| CPU-03 | 02-09, 02-12, 02-13, 02-14, 02-17 | SATISFIED | Truth 3 |
| CPU-04 | 02-08, 02-10, 02-15 | SATISFIED | Truth 4 |
| CPU-05 | 02-09, 02-10, 02-11, 02-16 | SATISFIED | Truth 5 |

REQUIREMENTS.md maps no other IDs to Phase 2, so no requirement is orphaned.

### Prohibitions

The plan prohibitions carry no `verification:` tier. The ones that can be checked in code were checked: allow.txt is empty, there is no function-pointer dispatch table, no wall clock, no allocation in `src/`, no digest in the core other than CRC-32, no planning IDs, and the negative control writes a build-tree copy. The provenance prohibitions cannot be proven from the tree: no GPL source opened, ares read for structure only, and WLA-DX source not read. My judgment is non-authoritative: I found no contrary evidence, PROVENANCE and hygiene are unchanged, and the hygiene lane exits 0. Under project rule 6 these do not become owner UAT items.

### Anti-Patterns Found

| File | Line | Pattern | Severity | Impact |
|------|------|---------|----------|--------|
| (106 phase-modified files outside .planning) | - | TBD/FIXME/XXX | none found | - |
| `src/`, `runner/` modified files | - | TODO/HACK/placeholder | none found | - |
| Review IN-01..IN-06 (deferred in 02-REVIEW-DISPOSITION.md) | - | info-level findings | Info | Diagnostic precision and seeds only. None blocks a success criterion. IN-01 (fault PC/opcode precision for prefixed ED holes and interrupt acceptance) is the one closest to "diagnosable from the first-fault trace". It concerns precision on rare paths, not the absence of a trace, and is deferred to the phase 3 trace work. |

### Human Verification Required

None. Under project rule 6 the owner verifies nothing by default. 02-REVIEW-FIX.md tags WR-02 and WR-03 with "requires human verification", but neither is a matter of identity-bound consent or of a run on the owner's hardware:
- WR-02 (the beam position is tied to `bus.at` on state load) is covered by the round-trip and refusal tests in `tests/core/test_state.c`, which pass.
- WR-03 (writes while `$3E` disables the cartridge) is a documented hardware choice where SMS-SYSTEM is silent. The `mapper.slot_off` test pins it, the reason is stated in `src/bus.c` and in the fault table of `docs/embedding.md`, and it is recorded in the review disposition. This is information, not a gap.

### Gaps Summary

None. All five roadmap success criteria hold in the codebase at HEAD (b8dad7a), and each is backed by a test or a runner command I ran myself. There are no orphaned requirements, no debt markers and no stub artifacts.

---

_Verified: 2026-10-20T06:13:27Z_
_Verifier: Claude (gsd-verifier)_
