---
phase: 08-sports-pad-paddle-and-3-d-glasses
reviewed: 2026-11-11T00:00:00Z
depth: standard
files_reviewed: 35
files_reviewed_list:
  - include/cogmaster.h
  - src/bus.c
  - src/cart.c
  - src/cartdb.inc
  - src/frame.c
  - src/instance.c
  - src/internal.h
  - src/io.c
  - src/ports.c
  - src/state.c
  - src/state.h
  - tests/api/v10.c
  - tests/api/v11.c
  - tests/api/v12.c
  - tests/api/v3.c
  - tests/api/v8.c
  - tests/api/v9.c
  - tests/cart/fixture_facts.txt
  - tests/cart/generator.cmake
  - tests/cart/test_cart_device.c
  - tests/cart/test_cartdb.c
  - tests/cart/tests.cmake
  - tests/fuzz/fuzz_pad.c
  - tests/fuzz/seeds.c
  - tests/fuzz/tests.cmake
  - tests/ports/test_devices.c
  - tests/ports/test_glasses.c
  - tests/ports/test_input.c
  - tests/ports/test_ports.c
  - tests/ports/test_state.c
  - tests/ports/tests.cmake
  - tests/roms/src/sports_pad.asm
  - tests/roms/src/paddle.asm
  - tests/roms/src/glasses.asm
findings:
  critical: 0
  warning: 1
  info: 5
  total: 6
  necessity: 1
status: issues_found
---

# Phase 8: Code Review Report (core half)

**Reviewed:** 2026-11-11
**Depth:** standard
**Files Reviewed:** 35
**Status:** issues_found

## Summary

The core half is in good shape. I traced, and found correct: the `bus.c` hook (the `$FFFC-$FFFF` mapper-or-fault
decision and the RAM-off fault are the same as before, and no instruction is added to the plain RAM-write path);
the glasses window arithmetic (unsigned, never negative, including the overshoot case and loaded states); the
`set_input` min(tag, ours) copy, validation order and pending-axis sums (no int32 overflow: both addends are
within +-32767); the biased 8/16/32-bit state helpers (decode and bounds are correct at every width); and the
struct layouts, offsetof asserts and version numbers. Every state field the new visitor loads is bounded and
cross-checked, and every state the core can reach passes its own validator. The pin refactor is equivalent to the
old `$DC`/`$DD` formulas. The Sports Pad nibble order, timeout constant (1,536 ticks = 512 cycles), the Paddle
TR/nibble order and clock, and the eye polarity all match `SMS-CARTRIDGES-AND-PERIPHERALS.md` 3.4 to 3.6. One
modelling inconsistency (WR-01) and a few maintainability points follow. No sanitizer or build was run for this
review.

## Warnings

### WR-01: The Sports Pad never returns to idle after a read, contrary to the code comment and `docs/controllers.md`

**File:** `src/ports.c:30-37` (`sports_nibble`), `src/ports.c:97-121` (`sports_fall`, `cogmaster__ports_th`)
**Issue:** `step` goes back to 0 only on reset or on a device change. After a read finishes at step 4, or after a
read abandoned at step 2, the pad keeps presenting the last nibble (the low nibble of Y, or of X) on D0-D3 for as
long as TH stays put. The 512-cycle abandon rule is applied only when the next TH fall arrives at step 2, and never
at step 4 or while TH is low. `docs/controllers.md:63` says "Idle it shows `$F`", and the comment above
`sports_nibble` says step 0 is idle and presents `$F`. In practice $F is shown only before the first read after
load or reset. The test confirms this: `tests/ports/test_devices.c` has the comment "Idle between reads, the pad
presents $F" directly above `CHECK_EQ_U64(m->ports[port].step, 4)`. Failure scenario: a game that reads `$DC` with
TH high outside a four-strobe sequence, for example to detect the pad or to poll idle D0-D3, sees a stale nibble
such as `$B` from the last read's Y byte rather than the idle level. Per the cited MAME model the sequence is
abandoned by a timer 512 cycles after TH last changed. The model also makes every poll one frame apart look
mid-sequence. I cannot confirm from here what MAME presents at idle, so the fix should be checked against the
cited source.
**Fix:** `ports_pins` already receives `tick`, so make the idle decision time-aware and keep it pure:
```c
case COGMASTER_DEVICE_SPORTS_PAD: {
    const struct cogmaster__port *p = &m->ports[port];
    int expired = (p->step == 2u || p->step == 4u) && tick - p->edge >= SPORTS_IDLE_TICKS;
    uint8_t nib = expired ? 0x0Fu : sports_nibble(p);
    return (uint8_t)(nib | (~held & (PIN_TL | PIN_TR)));
}
```
`sports_fall` already treats an expired step 2 and any step 4 as the start of a new read, so no other change is
needed. Otherwise, correct the doc and comment to say that the last nibble stays on the pins until the next fall.

## Info

### IN-01: The `$FFFC-$FFFF` cartridge/RAM gating is now written twice, with literal bit masks

**File:** `src/io.c:176-185`
**Issue:** `cogmaster__io_glasses_write` repeats `mem_ctrl & 0x40u` and `& 0x10u` as bare literals. `src/bus.c:108-109`
keeps the same bits as `BUS_CART_OFF` and `BUS_RAM_OFF`, and `bus_write` still computes both itself. The mapper-or-fault
decision for `$FFFC` and up used to live in one place. If one copy changes, a write at `$FFFC` and one at `$FFF8` will
disagree about the RAM-off fault. Today the two agree.
**Fix:** Move the helper into `bus.c` beside the constants, or move the two constants to `internal.h` and use them in `io.c`.

### IN-02: A soft reset wipes the Paddle position and the Sports Pad's undelivered motion

**File:** `src/io.c:159-165` (`memset(m->ports, 0, ...)` in `cogmaster__io_reset`)
**Issue:** The controller's own state lives in the console-reset path, so pressing Reset snaps a Paddle to centre and
drops up to 1023 counts of Sports Pad motion. The axes are relative deltas, so the host cannot know the position
changed. The behaviour is deliberate and documented (`docs/controllers.md:52-53`) and it is deterministic, so this is
only a usability note. A reset that keeps `pos` and `acc` but zeroes `edge`, `step` and `val` would match the
hardware better and would still satisfy the state cross-checks.
**Fix:** Optional. Keep `pos` and `acc` across `cogmaster__io_reset` and clear only `edge`, `step` and `val`.

### IN-03: A paddle table row replaces a region the ROM header already named

**File:** `src/cart.c:304-307`
**Issue:** For a row with `region == 0` and a paddle controller, `region_guess` is forced to Japan even when
`read_header` (`cart.c:99-112`) has set Export from the dump's own header. The comment says "a row that names a region
keeps it", but a header-derived region is dropped. Today only one paddle row exists, so this is latent. A future
paddle row for an Export-header dump would load as Japan on `REGION_AUTO`, which also turns the FM unit on.
**Fix:** `else if (paddle && out->region_guess == 0u) out->region_guess = COGMASTER_REGION_JAPAN;`

### IN-04: Necessity: unreachable guards in the device code

**File:** `src/ports.c:35` (`step > 4u`), `src/ports.c:68` (`default:`)
**Category:** necessity
**Issue:** `step` is only ever set to 0 to 4 (`sports_fall`, `cogmaster__ports_th`, `cogmaster__ports_frame`) and the
loader bounds it with `cogmaster__sio_u8(..., 0u, 4u)` (`state.c`, `port_step`). A device is bounded to 0..3 by
`set_input` and by the `input_port1` and `input_port2` bounds in `state.c`. The two branches cannot be taken.
`default:` also hides a missing case if a device is added.
**Fix:** Drop the `step > 4u` test and let `default:` share the PAD arm only if you want a safe fallback. Otherwise keep as is; this is a judgment call.

### IN-05: The loader accepts a `step` that disagrees with the TH wire

**File:** `src/state.c:661-676` (`ports_io`)
**Issue:** The core can only reach step 1 or 3 with the TH wire low, and step 2 or 4 with it high. The loader checks
neither against `io_ctrl`, so a hand-built state with `step == 1` and TH high loads. It is safe: the next change is a
fall, which at step 1 runs the restart branch, so there is no out-of-bounds access and no hang. It is a missing
cross-check, not a defect.
**Fix:** Optional. After `io_io` has been read, check `(step & 1u) == 0u || wire == 0u` and `step == 0u || (step & 1u) != 0u || wire == 1u` per port using `th_wire`.

---

_Reviewed: 2026-11-11_
_Reviewer: Claude (gsd-code-reviewer)_
_Depth: standard_
