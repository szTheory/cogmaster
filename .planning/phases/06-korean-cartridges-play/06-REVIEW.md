---
phase: 06-korean-cartridges-play
reviewed: 2026-11-10T00:00:00Z
depth: standard
files_reviewed: 21
files_reviewed_list:
  - include/cogmaster.h
  - src/cart.c
  - src/cart_scan.c
  - src/cartdb.c
  - src/cartdb.h
  - src/internal.h
  - src/mapper.c
  - src/mapper_ext.c
  - src/state.c
  - runner/main.c
  - runner/probe.c
  - runner/probe.h
  - tools/cartdb.c
  - tests/mapper/test_mapper_ext.c
  - tests/cart/test_cart.c
  - tests/cart/test_cartdb.c
  - tests/api/v10.c
  - tests/roms/src/mapper_korean.asm
  - scripts/compat-common.sh
  - scripts/compat-update.sh
findings:
  critical: 0
  warning: 3
  info: 3
  total: 6
  necessity: 1
status: issues_found
---

# Phase 6: Code Review Report

**Reviewed:** 2026-11-10
**Depth:** standard
**Files Reviewed:** 21 (the list above names 20 files; `.planning/.../06-CONTEXT.md` was read as context)
**Status:** issues_found

## Summary

The review covered the diff against `8c97f8e1`, with the focus areas the task named. MAME's `rom.cpp` was not opened, so the register semantics of the new boards (4PAK, Zemina, Nemesis, Hicom, Korean) were checked against the project's own notes in `.planning/preparation/SMS-CARTRIDGES-AND-PERIPHERALS.md` (rows for each board) and not against the MAME source. They agree with those notes.

No Critical findings. Memory safety holds in every place the focus list named:

- **Bank index arithmetic (`src/mapper_ext.c`).** `ext_bank` reduces onto `2 * pages`, which is at most 128 because the body is capped at 1 MiB, so the result fits `uint8_t`. The largest intermediate index is `4 * 255 + 3` for Hicom, and `0x30 + 255` doubled for 4PAK, both well inside `uint32_t`. `cart.pages` is at least 1 for any loaded board: the body is at least 1 KiB and is padded to a 16 KiB multiple. `rom[(bank8 << 13) | (addr & 0x1FFF)]` therefore always lands inside `pages * 16 KiB`. Odd page counts are covered by `ext_check` plus the modulo. The 8 KiB-multiple Zemina bodies (padded up to a whole page) read `$FF` from the padded half bank.
- **State visitor (`src/state.c`).** `bank8` is bounded to `[0, 2 * pages - 1]` and `raw` is free (it only feeds a recompute that is reduced again). The `sizeof(struct cogmaster__mapper) == 12` tripwire matches `ctrl + bank[3] + bank8[6] + raw[2]`. The ext fields are visited only for `board >= KOREAN`, and the board itself is fixed to the instance's cartridge, so a hostile state cannot reach a `bank8` that points outside the image.
- **Scan (`src/cart_scan.c`).** I checked the 256-entry `op_len` table row by row against the Z80 instruction set. The prefix lengths are right: `CB` is 2, `ED` is 4 for `(next & 0xC7) == 0x43` and 2 otherwise, `DD`/`FD` is `1 + op_len + (HL-memory ? 1 : 0)`, and `DD CB d op` is 4. `HALT` is excluded from the displacement set. Reads are guarded (`i + 3 <= end` for the operand, `i + 1 < end` before a prefix looks ahead), counters are bounded by 10,922 sites, and there is no read past `end = min(body, 32 KiB)`.
- **Probe and DB (`src/cart.c`, `src/cartdb.c`, `src/cartdb.h`, `runner/probe.c`).** Row layout is pinned by static asserts (36 bytes, offsets), and the hex-string sort order the generator uses equals the `memcmp` order the search uses. `unsupported_reason` is zeroed by `memset` and terminated at index `k <= 127`. The size-tagged copy is covered by `tests/api/v10.c`. The refusal message passes catalog text as `%s` arguments only, and the exit status is 77.
- **Generator (`tools/cartdb.c`).** An unmapped slot or controller value exits 1. Records are NUL-terminated at `</software>` before `scan_features` runs, so the feature scan cannot cross record boundaries.

The three Warnings are about heuristic quality and a test gap. The Infos are minor.

## Warnings

### WR-01: Nemesis is recognised from a single byte, so a Zemina image can be mistaken for it

**File:** `src/cart.c:230-233`
**Issue:** An image is promoted from Zemina to Nemesis when `body == 128 KiB`, `b[body - 8 KiB] == 0xF3` and `b[0] != 0xF3`. The preparation notes (SMS-CARTRIDGES-AND-PERIPHERALS.md, scan step 5) describe the signature as a 128 KiB body with `$00 $00 $00` at 0 and `DI; IM 1` (`F3 ED 56`) at `$1E000`. The code checks one byte of the second part and a weaker form of the first. Any 128 KiB Zemina-family game whose last bank happens to start with `$F3`, and whose first byte is not `$F3` (an image that opens with `LD SP,nn`, `JP` or `IM 1` instead of `DI`), is played as Nemesis. The consequence is different power-up banks: bank 0 at `$0000` becomes the last 8 KiB, so the game fails to boot. The test (`tests/cart/test_cart.c:330-337`) only exercises the single-byte form, so this is not caught.
**Fix:**
```c
if (f == COGMASTER__SCAN_ZEMINA && body == 131072u &&
    b[body - CART_BANK] == 0xF3u && b[body - CART_BANK + 1u] == 0xEDu &&
    b[body - CART_BANK + 2u] == 0x56u && b[0] == 0u && b[1] == 0u && b[2] == 0u) {
```
Then add a test with `F3` at `0x1E000` and no `ED 56`.

### WR-02: The scan's `$A000`, `$BFFF` and `$7FFF` families collide with legitimate Sega cartridge-RAM stores, so a headerless Sega image can be played on the wrong board

**File:** `src/cart_scan.c:51-58` (consumed at `src/cart.c:222-235`)
**Issue:** On the Sega board, `$8000-$BFFF` is cartridge RAM when `$FFFC` bit 3 is set (`src/mapper.c:36-38, 83-85`). `LD ($A000),A` and `LD ($BFFF),A` are ordinary RAM stores there, and the scan counts them as Korean and 4PAK register writes. Header-valid and database images never reach the scan, but a headerless or bad-checksum Sega image of 64 KiB or more that saves to RAM through absolute stores, and reaches its mapper through `LD (HL),A` or `LD ($FFFC)` stores (so few Sega sites are counted), satisfies `x >= 3 && x >= 2s + 2 && geometry_fits` (any 16 KiB multiple). It is then installed as Korean or 4PAK and plays wrongly instead of being refused. The comment in `scan_board` says a doubtful image never plays; this is a case where a confident but wrong answer is possible. MAME accepts the same risk (the scan is its heuristic), so this is a deliberate trade-off, but nothing here records it or mitigates it.
**Fix:** The cheapest mitigation is to count a `$A000` or `$BFFF` store only when no earlier instruction in the sweep stored a Sega register value with bit 3 set. A smaller step is to document the limit in the `scan_board` comment, and add a test of a Sega image with three `LD ($A000),A` and no mapper writes that asserts a refusal. Raising the threshold for `$BFFF`/`$A000` from 3 should wait for real-dump measurements, as the existing comment says.

### WR-03: Test gap: instruction-length decoding for the prefix families has no test

**File:** `tests/cart/test_cart.c:265-392`
**Issue:** `src/cart_scan.c`'s only reason to decode lengths is that an operand byte `$32` must not be counted. The only operand test is the unprefixed `3E 32 00 A0` run (`"operand decoy"`), and the prefix tests are `0xDD` and `0xED` fills. No test places `32` as the operand of `DD 36 d n`, `DD 21 nn nn`, `DD CB d op`, `ED 43 nn nn`, `FD 34 d` and `CB` forms, so an off-by-one in `length_at` or `uses_hl_memory` (the part of the scan with the most branches) would pass the suite and silently shift every later site in a real image. Project rule 7 says findings about test code become seeds, so file it as one.
**Fix:** Seed a test that builds one image per prefix form (for example `DD 36 05 32 00 A0 ...` repeated) and asserts `expect_counts(..., 0, 0, 0)`, plus a positive control with a real `LD ($A000),A` following each form that asserts it is counted.

## Info

### IN-01: A `DD` or `FD` prefix in front of `$32` hides a real `LD (nn),A`

**File:** `src/cart_scan.c:84-88`
**Issue:** For `DD`/`FD` followed by a non-prefix opcode the length is `1 + op_len[next] + hl`, and the loop skips the whole span, so `DD 32 nn nn` (the prefix is ignored by the Z80 and the instruction is `LD (nn),A`) is not counted. The sweep length is right; only the count is missed. Real code does not emit this, and an undercount only pushes an image toward refusal, so it is minor. `ED ED 43 ...` has the same shape (two bytes consumed, then the `43` decoded as `INC BC`).
**Fix:** Either count `next == 0x32` in the `DD`/`FD` branch, or note in the function comment that a redundant prefix before a store hides it.

### IN-02: Impossible-state guards on `pages == 0` for ext boards

**File:** `src/mapper_ext.c:23-24`, `src/state.c:313`
**Category:** necessity
**Issue:** `ext_bank` tests `n != 0u` and `mapper_io` tests `m->cart.pages != 0u` before building the ext `bank8` limit. Neither state can occur for a board of 4 or more. `cogmaster__cart_copy` rounds the body up to a 16 KiB multiple (`src/cart.c:331`) and the probe refuses a body under 1 KiB (`src/cart.c:308`), so `pages >= 1` whenever a board is installed; boards of 4 and up are never present on the test card (`rom == NULL`, board 0). The guards also mirror the existing `bank_of` and `bank_max` pattern in `src/mapper.c:34` and `src/state.c:312`, which is why this rests on judgment (Info) rather than being strictly dead.
**Fix:** Keep them for consistency with the three existing boards, or drop both and state the `pages >= 1` invariant in a comment where `cogmaster__cart_copy` sets `pages`.

### IN-03: Generator slot table has two unchecked limits

**File:** `tools/cartdb.c:185-188` and `tools/cartdb.c:120-131`
**Issue:** `counted & 1u << s->id` and `counted |= 1u << s->id` assume a slot id below 32; there are 20 slots now, but nothing enforces it, so adding a 32nd slot is undefined behaviour. `check_table` also does not check `strlen(name) < sizeof ((struct cogmaster__cart_slot *)0)->name` (16): a 16-character name would compile in C as an unterminated array and `cogmaster__cartdb_slot_named` is the only reader that bounds it. The longest name today is `korean_188in1` (13 characters).
**Fix:** In `check_table`, add `strlen(s->name) > 15u` and `s->id > 31u` to the `fail("slot table row is not consistent")` condition.

---

_Reviewed: 2026-11-10T00:00:00Z_
_Reviewer: Claude (gsd-code-reviewer)_
_Depth: standard_
