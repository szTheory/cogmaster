---
phase: 09-light-phaser
reviewed: 2026-11-11T00:00:00Z
depth: standard
files_reviewed: 19
files_reviewed_list:
  - include/cogmaster.h
  - src/cart.c
  - src/frame.c
  - src/gun.c
  - src/internal.h
  - src/io.c
  - src/ports.c
  - src/state.c
  - src/vdp.c
  - src/vdp_render.c
  - tests/api/v12.c
  - tests/api/v13.c
  - tests/cart/test_cart_device.c
  - tests/fuzz/fuzz_pad.c
  - tests/fuzz/seeds.c
  - tests/ports/test_gun.c
  - tests/ports/test_state.c
  - tests/ports/tests.cmake
  - tests/roms/src/light_phaser.asm
findings:
  critical: 0
  warning: 1
  info: 4
  total: 5
  necessity: 1
status: issues_found
---

# Phase 9 (core half): Code Review Report

**Reviewed:** 2026-11-11
**Depth:** standard
**Files Reviewed:** 19
**Status:** issues_found

## Summary

Reviewed the sensor (`src/gun.c`), its five sync sites, the TH/latch refactor in `io.c`/`vdp.c`, the port, state and cart changes, and the C tests and fixture. Traced the render-ahead neutrality argument (every VDP mutation renders `< t/2` first, so an earlier render of dots `< t/2` cannot change a later pixel or flag; status reads render to `t/2 + 1`, a superset), the monotonic cursor, the two-gun merge and tie rule, the per-picture snapshot, the unsupported-mode guard, state bounds and cross-checks (all gun fields bounded; a hostile loaded cursor cannot index outside `field` and its work stays bounded), the canonicalisation of aim, ABI sizes (48/24/76/536 unchanged; DEVICE_LIGHT_PHASER 4; 13/15/15), and the constants against MAME `lphaser.cpp` at the pin (radius 6, luma `>= 0x7f`, release on the on-to-off transition, H latch on the rise, 19 dots = 38 ticks; `H = (x_off + 35) >> 1` re-derived from `vdp_hcount_at`). No wall clock, no float, no per-line cost on a gun-less run (a handful of compares at `$3F` writes, `$DD`/`$7F` reads, once per frame and once per wrap).

One real defect: the wrap finishes the old picture's scan with the new picture's height. The check was run, not only read: a scratch program against `src/*.c` (224 to 192 line change at the wrap, gun aimed at rows 203 to 222) produced 26 sensor edges with a `$DD` poll at every tick and 2 edges with no poll in that stretch.

## Warnings

### WR-01: the wrap finishes the old picture's scan using the NEW picture's height (schedule-dependent edges, stale rows read)

**File:** `src/vdp.c:203-205`, `src/gun.c:79-91` (`next_row`) and `src/gun.c:189-199` (`cogmaster__gun_wrap`)
**Issue:** `cogmaster__vdp_event` sets `m->vdp.height = vdp_pending_height(m)` and only then calls `cogmaster__gun_wrap`, which first runs `run(m, UINT32_MAX, 0u)` to finish the old picture. `look()` then calls `next_row`, which ends the scan at `row >= m->vdp.height`, now the new height. The CONTEXT decision 5(d) said the wrap runs BEFORE the height update; the code (and its comment, "After the height update, so the sensor arms for the height this picture has") does the opposite for the finish step.

Failure scenarios, both need a height change at a wrap (R1 mode bits changed between pictures) and an aim near the bottom of the old picture:
1. Old height 224 or 240, new height 192. A 224-line picture's window ends at line 192, so rows 192 to 223 are scanned in the next window and the frame-end sync does not complete them. If the guest makes no `$DD`/`$7F`/`$3F` access between line 224 and the wrap, the wrap scans the pending row and then `next_row` stops at `row >= 192`. Rows 193 to 222 of the disc are never sensed, so their TH falls, rises, pulse bits and H latches are lost. A guest that polled in that stretch sees them. Measured: 26 edges against 2.
2. Old height 224, new height 240. After the last sync the wrap continues into rows 224 to 239, which were never drawn this picture, so `look()` reads stale `m->field` rows (leftovers of an earlier 240-line picture or zeros) and can raise spurious edges and a spurious latch.

Old 192 to new 224 is not affected only because the frame-end `gun_sync` at line 192 already completes a 192-line scan. This breaks the stated invariant that no poll schedule can change what is seen (the one documented exception is the unsupported mode), and the saved sensor block and `hcounter` depend on guest access timing. The core stays deterministic for a given guest; the sensor result is wrong relative to the hardware.
**Fix:** Finish the old picture before the height changes and arm after it. In `vdp.c`, call a new `cogmaster__gun_finish(m)` (just `run(m, UINT32_MAX, 0u)`) before `m->vdp.height = ...`, and keep `cogmaster__gun_wrap` (drop pulses, `arm`) after it:
```c
if (line + 1u == cogmaster__frame_lines(m)) {
    if (cogmaster__gun_attached(m)) cogmaster__gun_finish(m);   /* old height still in force */
    m->vdp.vscroll = m->vdp.reg[9];
    ...
    m->vdp.height = (uint8_t)vdp_pending_height(m);
    ...
    if (cogmaster__gun_attached(m)) cogmaster__gun_wrap(m);      /* clear pulses, arm */
}
```
Add a case to `test_walk` (or a fuzz seed) that changes the height between pictures with the aim in rows at or past the new height and compares a polled twin against an unpolled one (`same_sensor_state`).

## Info

### IN-01: no test or fuzz op changes the picture height between pictures with a gun attached

**File:** `tests/ports/test_gun.c:1445-1486`, `tests/fuzz/fuzz_pad.c`
**Issue:** `walk_pair` and the fuzz twin's schedule-independence check only cover fixed-height pictures (and `OP_BAD_MODE`, which switches to an unsupported mode). The height change at the wrap, WR-01, is therefore unexercised, which is why it passed the schedule-independence tests.
**Fix:** Seed candidate (AGENTS.md rule 7: test code is a seed, not a requirement): a twin run with a 224 to 192 and a 224 to 240 change at the wrap, aim on the bottom rows.

### IN-02: `th_pins` documentation comment is orphaned above `sensor_low`

**File:** `src/io.c:234-242`
**Issue:** The original block comment of `th_pins` ("Effective TH pins, shared by readback and the rising-edge H latch ... low_mask ...") now sits directly above the new `sensor_low` comment and function, and `th_pins` itself (line 252) has no comment. A reader attaches the `low_mask` description to the wrong function.
**Fix:** Move that comment down to `th_pins` and keep only the `sensor_low` comment above `sensor_low`.

### IN-03: `cogmaster__vdp_latch_h` has no caller in `src/`, and `cogmaster__vdp_hcount_at` carries an unused parameter

**Category:** necessity
**File:** `src/vdp.c:280-283`, `src/vdp.c:262-268`, `src/internal.h:688-690`
**Issue:** After the refactor the only user of `cogmaster__vdp_latch_h` is `tests/ports/test_gun.c:92`; `th_edge` now calls `cogmaster__vdp_latch_h_at`. `cogmaster__vdp_hcount_at(const cogmaster *m, uint32_t tick)` takes `m` only to discard it with `(void)m` ("carried for a model that depends on the profile"), a generality with no instantiation (the formula is profile independent, proven by the function body). The evidence is judgment, not a defect, hence Info.
**Fix:** Keep `latch_h` only if a test needs it as the old-behaviour oracle (it is the pre-refactor wrapper); otherwise drop it. Drop the `m` parameter of `hcount_at` until a profile actually needs it.

### IN-04: `$DD` read path evaluates the gun check twice, and `ports.c` header claim no longer strictly holds

**File:** `src/io.c:421-437`, `src/io.c:355-358`, `src/ports.c:1-10`
**Issue:** On a gun-less run `chip_in` tests `cogmaster__gun_attached` and then `port_dd` calls `sensor_low(m, 3u)`, which re-tests both ports (two loads, two compares, two extra branches per `$DD` read). Cost is negligible (no Z80 memory hot path is touched) but is an instruction on a gun-less run that the "pay a compare and nothing else" comment promises not to have. Also `chip_in` now writes `m->ports[n].g_sense` on a `$DD` read, while the `ports.c` header still says a read of `$DC` or `$DD` cannot move a device; the bit is console state by the decision text but it lives in the device block and is saved there.
**Fix:** Optional: pass `0u` from `chip_in`'s gun-less branch (`port_dd(m, 0u)`) or have `port_dd` take the mask, and adjust the one comment sentence.

---

_Reviewed: 2026-11-11_
_Reviewer: Claude (gsd-code-reviewer)_
_Depth: standard_
