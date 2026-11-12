---
phase: 10-bios-boot
reviewed: 2026-11-11T00:00:00Z
depth: standard
files_reviewed: 10
files_reviewed_list:
  - include/cogmaster.h
  - src/bus.c
  - src/instance.c
  - src/boot.c
  - src/state.c
  - src/io.c
  - src/internal.h
  - tests/api/v14.c
  - tests/bios/test_bios.c
  - tests/fuzz/fuzz_bios.c
findings:
  critical: 0
  warning: 2
  info: 6
  total: 8
  necessity: 1
status: issues_found
---

# Phase 10: Code Review Report (core half)

**Reviewed:** 2026-11-11
**Depth:** standard, with cross-file reasoning on the bus, state and instance interplay
**Files Reviewed:** 10
**Status:** issues_found

## Summary

Scope was `git diff origin/main...HEAD -- include src` at fb40676 plus `tests/api/v14.c`, `tests/bios/test_bios.c` and `tests/fuzz/fuzz_bios.c`. `10-CONTEXT.md` decisions were read first. No BIOS bytes, no library and no `.env.local` were opened. No source file was modified. Two throwaway mutants were built from copies of `src/` in the scratchpad (not in the repo) to measure how strong the tests are.

The core is sound against the failure classes the brief asked about. I found no Critical issue. What I traced, and what held:

- **Stale `mem_slow`.** `bios.size` changes only in `cogmaster_load` (`instance.c:269-274`). `$3E` changes only at `io.c:170` (`io_reset`), `io.c:502` (port write), `boot.c:55` (`boot_power_on`) and the state APPLY pass. Each of these is followed by `cogmaster__bus_decode`. `machine` is never an input to the flag; `mem_slow_driven` reads it directly. A state load of a different boot mode is refused by the header, so only a same-mode state with a different `$3E` reaches APPLY, and `state.c:900` re-derives the flag there. Mutant check: deleting that line makes `bios.slow_flag` fail.
- **Load rollback and half-attached BIOS.** Every refusal (pair rule, size, no image, GG, probe, profile, allocation, `cart_copy`) returns before `cogmaster__cart_install` (`instance.c:266`). From there to the end of `cogmaster_load` there is no failure path, so the BIOS is attached atomically with the cartridge. A load without a BIOS clears the previous one.
- **Ownership and aliasing.** The caller's buffer is read once, by a constant-length `memcpy` of 8192 bytes, after all validation. `SIZE_MAX` and `NULL` with a nonzero size are refused by the pair rule and the `!= 8192` compare, with no multiplication or allocation involved.
- **Tag-size handling.** `tagged_bios` requires the tag to cover the end of `bios_size`. Both fields are read through `memcpy`, so there is no alignment or aliasing hazard. Struct offsets are pinned on 64-bit and 32-bit by `_Static_assert` (`internal.h:495-503`). I recomputed the layout (64-bit: `reserved3` 52, `bios` 56, `bios_size` 64, size 72; 32-bit: 32, 36, 40); it is consistent.
- **Peek against read.** Both go through `mem_slow_driven`. The only intended difference is the counter: a read counts `open_bus_reads` and may fault, and peek does not. The `bios.arbitration` test asserts peek equals read for 16 `$3E` combinations by 4 addresses by 2 machines.
- **State header.** Boot byte at 53, CRC-32 at 54-57, reserved 58-63, 6 bytes; `H_RESERVED + H_RESERVED_SIZE == 64` is asserted. A wrong boot mode or BIOS CRC-32 is refused by the VALIDATE pass before any write.
- **Hot path.** `mem_read` now tests one byte instead of a mask and compare, so it is cheaper than before. The BIOS-attached, cartridge-and-BIOS-both-on case is slow, and it exists only while a BIOS runs.
- **MSVC and C17.** Mixed declarations and statements are C99. There are no VLAs and no designated-initializer problems in the core. `memcpy` is used for the unaligned and pointer reads.

`cogmaster__state_load` has no caller in `src/` or the public header in this branch; only tests reach it. The state-binding logic is therefore exercised by tests alone at this point.

## Critical Issues

None.

## Warnings

### WR-01: The BIOS-attached silent-slot branch has no test for RAM off (a mutant survives)

**File:** `src/bus.c:180` (guard `addr < 0xC000u`), test side `tests/bios/test_bios.c:28,141-143`
**Issue:** `mem_read_slow` returns open bus with no fault when a BIOS is attached and `$3E` b5 or b7 is clear. The guard `addr < 0xC000u` is what keeps work RAM reads (b4 set, RAM off) on the fault path. No test reads work RAM with RAM off while a BIOS is attached and a slot is enabled:
- `bios.arbitration` sweeps only b7, b6, b5 and b3; b4 is always 0, so RAM is always on (`test_bios.c:141-142`).
- The only other b4 coverage is `flag_matches` and its callers, which compare the flag and never read memory.

I verified this with a mutant: removing `addr < 0xC000u &&` from line 180, rebuilt with `src/*.c` and `tests/bios/test_bios.c`, passes all five core cases (`arbitration`, `slow_flag`, `load`, `state`, `hostile`) with exit 0.

Failure scenario: a later edit drops or loosens the guard. A game booted through a BIOS that writes `$3E` with b4 set and b5 or b7 clear, then reads `$C000`, silently gets open bus. The first-fault record that the no-BIOS path keeps is lost, and no test notices.

**Fix:** add `$C000`/`$FFFF` probes to the arbitration table for the combinations with b4 set, with the expectation that they fault (value `bus.data | pullup`, fault count incremented) whatever b5 and b7 say. In `arbitration`, also make b4 a fifth axis, or add a small RAM-off sweep after the existing one. The existing `must_fail` pattern makes this cheap.

### WR-02: The `mem_slow` invariant is held by convention, and the comment at `bus.c:134-136` overclaims

**File:** `src/bus.c:134-136`, `src/bus.c:188-194`, `src/internal.h:396-398`, `tests/bios/test_bios.c:141-143,522-523`
**Issue:** The comment on `mem_slow_driven` says it "reads $3E and the BIOS size directly and never the derived flag, so a register set from outside bus_decode still decodes truthfully". That holds for `cogmaster_peek` and the slow path. It does not hold for the dispatch in `mem_read`, which gates on `m->mem_slow` alone. A register written without `cogmaster__bus_decode` makes peek and read disagree:
- With `mem_slow == 0`, a read goes to `mem_driven` and returns the cartridge or RAM byte.
- Peek, meanwhile, decodes the new `mem_ctrl` and reports BIOS or open bus.

Today every writer is paired with `bus_decode`, including the tests, which call it by hand after setting `m->io.mem_ctrl` (`test_bios.c:141-143`, `522-523`). That makes direct writes a normalised pattern and each new writer an unchecked obligation. A future debugger or poke API, a rewind path that copies `io` wholesale, or a new reset path could silently skip it.

**Fix:** correct the comment to say that only peek and the cold path are independent of the flag, and that the flag must be refreshed by every `mem_ctrl` or `bios.size` writer. Better, route all writers through one setter, `static inline void cogmaster__set_mem_ctrl(cogmaster *m, uint8_t v)` in `internal.h`. That makes the pairing structural and lets a test assert there is no other `io.mem_ctrl =` in `src/` (the grep above found exactly three plus the state field).

## Info

### IN-01: Silent empty-slot reads are keyed on a BIOS being attached, not on it being enabled

**File:** `src/bus.c:180`
**Issue:** The no-fault, no-count rule applies whenever `m->bios.size != 0`, even with b3 set (BIOS off) and long after the BIOS has handed off to the game. The stated reason in the comment (the BIOS probes the slots in turn) applies only while the BIOS is enabled. A game that writes `$3E` with b5 or b7 clear and the cartridge off therefore records no fault under a BIOS boot but does under fast boot, so the first-fault record differs between two boots of the same game. The CONTEXT decision says empty slots open bus without fault, and the comment calls this a model row, so this is a judgement call and not a defect.
**Fix:** consider also requiring `(mem_ctrl & BUS_BIOS_OFF) == 0` in the condition, or note in `docs/bios.md` that fault accounting depends on whether a BIOS is attached.

### IN-02: A state binds to a BIOS by CRC-32 alone

**File:** `src/state.c:140-141`
**Issue:** The BIOS identity in the header is a 32-bit CRC. CRC-32 is not collision-resistant, so a crafted 8 KiB image can be given any chosen CRC and a state saved under one such image loads against another. `cogmaster__state_load` has no public caller in this branch, and a state is not a trust boundary against the owner of the instance, so I rate this Info. The state is bound to the cartridge the same way (ROM CRC-32), so this is consistent with existing practice.
**Fix:** none required now. If the public state API ever treats a state as untrusted input, bind with a wider digest; `src/sha1.c` is already in the tree and the runner already computes SHA-1 for recognition.

### IN-03: A size tag strictly between the old and new struct size silently selects fast boot

**File:** `src/instance.c:93`
**Issue:** `tagged_bios` reads both fields only when `size >= offsetof(bios_size) + sizeof(size_t)`. A tag from the half-open range above the old size (56 on 64-bit, 32 on 32-bit) up to one below the full size is not a released size. It returns OK with the BIOS ignored and a fast boot, so the hashes differ from what the host asked for without any signal. `test_bios.c:488-499` pins this as intended and it matches how `tagged_field` already treats partial tags, so it is a deliberate convention and the cost is a silent downgrade for a malformed struct only.
**Fix:** none needed. If wanted, return `COGMASTER_STATUS_ERR_STRUCT_SIZE` for tags in that range, but it would diverge from the other tagged fields.

### IN-04: The public header carries the BIOS size only as a literal in a comment

**File:** `include/cogmaster.h:169`
**Issue:** Hosts must hard-code 8192 (`COGMASTER__BIOS_SIZE` is internal). The runner and libretro half each carry their own copy of the number, so a change to the supported size would have to be made in three places. CLAUDE.md asks the public header to stay terse, so a constant is a small addition.
**Fix:** add `#define COGMASTER_BIOS_SIZE 8192u` to the public header and define the internal macro from it.

### IN-05: Generality without instantiation: `bios.size` is only ever 0 or 8192

**File:** `src/internal.h:430-432`, `src/instance.c:272`
**Category:** necessity
**Issue:** `struct cogmaster__bios.size` can hold only 0 or `COGMASTER__BIOS_SIZE`. `instance.c:166` refuses every other length and `instance.c:272` is the only writer. Every reader uses `size != 0u` as a boolean (`bus.c:130,153,180`, `instance.c:292,365`, `state.c:140-141`), and `mem_slow_driven` indexes with the constant, never with the field. The field gives the impression of a variable-length image the code does not support. This rests on judgement, since it is a harmless one-word field and a future second BIOS size may reuse it, so Info.
**Fix:** either leave it with a one-line comment that it is a presence flag, or rename it `attached`.

### IN-06: An AUTO machine on a Japanese cartridge takes the SMS 2 AND arbitration

**File:** `src/instance.c:185` (AUTO resolves to SMS2), `src/bus.c:154`
**Issue:** With `machine = AUTO`, a Japanese-region cartridge resolves to `COGMASTER_MACHINE_SMS2`, so with a BIOS and the cartridge both enabled it gets the AND rule, not the SMS 1 rule where the cartridge wins. The Japanese Master System shares the first-generation board. The brief's decision names SMS 1 and SMS 2 only and AUTO has always defaulted to SMS 2, so this follows the settled choices. It is a place where a BIOS boot of a Japanese game may read differently from real hardware.
**Fix:** none now. Note the dependency in `docs/bios.md` or in the follow-up seed, or let `--boot bios` choose SMS 1 for a Japanese region.

---

_Reviewed: 2026-11-11_
_Reviewer: Claude (gsd-code-reviewer)_
_Depth: standard_
