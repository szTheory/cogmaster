---
phase: 07-saves-survive-a-restart
reviewed: 2026-11-11T02:40:00Z
depth: standard
files_reviewed: 54
files_reviewed_list:
  - .github/workflows/ci.yml
  - CMakeLists.txt
  - docs/boards.md
  - docs/embedding.md
  - docs/hosted.md
  - docs/perf-fuzz.md
  - docs/runner.md
  - docs/saves.md
  - include/cogmaster.h
  - libretro/cogmaster_libretro.info.in
  - quality.json
  - runner/args.c
  - runner/args.h
  - runner/main.c
  - runner/probe.c
  - runner/save.c
  - runner/save.h
  - src/battery.c
  - src/cart.c
  - src/cartdb.inc
  - src/eeprom.c
  - src/internal.h
  - src/mapper_ext.c
  - src/state.c
  - tests/abi/tests.cmake
  - tests/api/v10.c
  - tests/api/v11.c
  - tests/built/expected.txt
  - tests/cart/fixture.cmake
  - tests/cart/fixture_facts.txt
  - tests/cart/generator.cmake
  - tests/cart/test_cartdb.c
  - tests/cmake/repository.sh
  - tests/compat/catalog.tsv
  - tests/fixtures/tests.cmake
  - tests/fuzz/fuzz_eeprom.c
  - tests/fuzz/seeds.c
  - tests/fuzz/tests.cmake
  - tests/inventory/ci.txt
  - tests/inventory/fuzz.txt
  - tests/inventory/nofp.txt
  - tests/inventory/wasm32.txt
  - tests/libretro/host.c
  - tests/libretro/tests.cmake
  - tests/mapper/test_battery.c
  - tests/mapper/test_eeprom.c
  - tests/mapper/tests.cmake
  - tests/retroarch/run_retroarch.cmake
  - tests/roms/manifest.txt
  - tests/roms/src/eeprom.asm
  - tests/roms/src/eeprom.link
  - tests/runner/test_save.c
  - tests/runner/tests.cmake
  - tools/cartdb.c
findings:
  critical: 0
  warning: 2
  info: 2
  total: 4
status: issues_found
---

# Phase 07: Code Review Report

**Reviewed:** 2026-11-11T02:40:00Z
**Depth:** standard
**Files Reviewed:** 54
**Status:** issues_found

## Summary

The 93C46 model in `src/eeprom.c` was traced against the Microchip 93C46 instruction set (start bit, 2 opcode
bits, 6 address bits, EWEN 00-11, EWDS 00-00, WRAL 00-01, ERAL 00-10, READ with a dummy 0 then D15 to D0,
sequential read wrapping 63 to 0, write-type commands refused until EWEN, programming at the chip-select fall).
No protocol defect was found. The generation counter moves only from `ee_store` reporting a changed byte, once
per completed command, and `cogmaster_import_battery` stays generation-neutral. The work per write is O(1),
or 64 words for WRAL and ERAL. There is no wall clock and no floating point. `cart_copy`, the `cartdb.inc`
rows (all 8 board-10 rows carry 128 bytes, persistence 2 and geometry 1; no row keeps persistence 3) and the
`state.c` range checks agree with each other. The runner's atomic save, sweep (only `<target>.tmp-0` to
`<target>.tmp-1023`, so no save name can match), `--save-interval` parsing (digit loop caps at 32 bits before the
next multiply) and the libretro span guard hold up.

No Critical issues. Two Warnings: a state-load invariant gap in the EEPROM chip state, and a directory-sync open
failure that is reported as a failed save after the save is already in place. Two Info items on tooling and
argument handling.

## Warnings

### WR-01: `eeprom_io` does not cross-check the loaded chip fields against each other

**File:** `src/state.c:304-318`
**Issue:** Each field is range-checked, and three relations are checked (count against phase, count at most 7 in a
command, count 0 whenever chip select is low). Several relations that every reachable state satisfies are not:

- flags b4 (`PENDING`) is accepted in any phase. `tests/fuzz/fuzz_eeprom.c:100-102` asserts `PENDING` implies phase
  4 and `DATA_LOW` implies phase 3 on reachable states, so the invariants exist but the loader does not enforce them.
- `cmd` is not tied to `phase`: phase 3 (read) is accepted with a WRITE or ERASE opcode in `cmd`, and phase 2
  (data in) with a READ opcode.
- `PENDING` is accepted while `WRITE_ON` is clear.

Failure scenario: a crafted state with `phase=0, count=0, cmd=0xC5` (ERASE word 5), `flags=0x14` (chip select plus
`PENDING`, writes disabled) loads without error. The next window write with chip select low takes the
`!cs && (flags & EE_CS)` branch, `ee_program` runs, word 5 becomes 0xFFFF while writes are disabled, and
`cart.generation` moves, which makes the runner write a save file for a command that never completed. A phase 2
state whose `cmd` has opcode READ and `(cmd >> 4) & 3 == 1` reaches the WRAL loop in `ee_program` and overwrites all
64 words once 16 data bits arrive. Work stays bounded and no memory is touched outside the 128 bytes, so this is a
robustness gap rather than memory corruption. The stated aim was that every loaded field is cross-checked, and a
state that no run can produce still resumes.

**Fix:** add the missing relations next to the existing checks:
```c
/* PENDING: only a complete write-type command with writes enabled and chip select high */
cogmaster__sio_check(io, (flags & 0x10u) == 0u ||
                         (phase == 4u && (flags & 0x02u) != 0u && (flags & 0x04u) != 0u));
cogmaster__sio_check(io, (flags & 0x01u) == 0u || phase == 3u);        /* Data Out low only in a read */
cogmaster__sio_check(io, phase != 3u || (e->cmd >> 6) == 2u);          /* a read holds a READ opcode */
cogmaster__sio_check(io, phase != 2u || (e->cmd >> 6) == 1u ||
                         (e->cmd >> 4) == 1u);                         /* WRITE, or WRAL (00 01) */
```
Optionally also require, when `PENDING` is set, that `cmd` is WRITE, ERASE, WRAL or ERAL. Add an
`eeprom.state.bounds` case that loads the crafted `PENDING` in phase 0 and expects a refusal.

### WR-02: A failed `open` of the directory in `save_sync_directory` reports a flush failure after the save is in place

**File:** `runner/save.c:272-278`, `runner/save.c:304-312`
**Issue:** `open(dir, O_RDONLY | O_DIRECTORY)` returning -1 makes `save_sync_directory` return 0, and `save_flush`
then returns "cannot write, synchronize, close or replace file" with the generation left unacknowledged. By then
`rename` has succeeded and the new bytes are the save. A directory that allows create and rename but not listing
(mode 0300, a drop-box style share, some restricted network mounts) opens for write and rename but not for
`O_RDONLY`. The failure scenario is `--save-dir` on such a directory with `--save-interval 1`: the first periodic
flush writes the save, reports failure, and the run ends at once with exit 1 (`runner/main.c:446-449`) although
the file is complete. The same condition makes every exit flush exit 1 and rewrite the same bytes each time. The
code tolerates `EINVAL` and `ENOTSUP` from `fsync` for the same reason, filesystems that cannot sync a directory,
but not the cousin case where the directory cannot be opened.
**Fix:** treat only a real durability error as failure. Accept `EACCES` and `EPERM` on the `open` the way `EINVAL` and
`ENOTSUP` are accepted on `fsync`, so that the best-effort sync is skipped and the save is acknowledged:
```c
if (fd < 0)
    return errno == EACCES || errno == EPERM;
```
Document the choice next to the existing `EINVAL` and `ENOTSUP` text in `docs/runner.md`.

## Info

### IN-01: `tools/cartdb.c` fixture loop checks the row cap before it knows a row exists, and the message is wrong

**File:** `tools/cartdb.c:468-470`
**Issue:** `if (count == CAP) fail("missing fixture facts")` runs at the top of every iteration, including the one that
would read EOF. With `count` at `CAP` after the last fixture row, a well-formed file fails with a message that
names a missing file. Only reachable at 4096 rows, so it is a message and ordering defect, not a live bug. Seed
candidate per rule 7 (tooling code).
**Fix:** read the row first, and test `count == CAP` only before storing it, with its own message ("too many facts").

### IN-02: `--save-interval` accepted without `--save-dir` when 0, and a repeated option is silently last-wins

**File:** `runner/args.c:188-189`, `runner/args.c:235-238`
**Issue:** `check_combination` refuses `--save-interval` without `--save-dir` only when the value is nonzero, while
`docs/runner.md` says "It needs `--save-dir`". `--save-interval 0` alone exits 0. `--save-interval 5 --save-interval 9`
takes 9, while `--expect` rejects a repeat (`o->expect != NULL`). No data is at risk. Seed candidate (CLI polish).
**Fix:** track presence with a flag, not the value, and reject a second occurrence as `--expect` does.

---

_Reviewed: 2026-11-11T02:40:00Z_
_Reviewer: Claude (gsd-code-reviewer)_
_Depth: standard_
