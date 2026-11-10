---
phase: 02-the-cpu-matches-the-public-vectors
reviewed: 2026-10-20T00:00:00Z
depth: standard
files_reviewed: 30
files_reviewed_list:
  - CMakeLists.txt
  - include/cogmaster.h
  - runner/args.c
  - runner/args.h
  - runner/exit_status.h
  - runner/mailbox.c
  - runner/mailbox.h
  - runner/main.c
  - runner/md5.c
  - runner/md5.h
  - runner/probe.c
  - runner/probe.h
  - runner/sha1.c
  - runner/sha1.h
  - src/boot.c
  - src/bus.c
  - src/cart.c
  - src/frame.c
  - src/instance.c
  - src/internal.h
  - src/io.c
  - src/mapper.c
  - src/state.c
  - src/vdp.c
  - src/z80.c
  - src/z80.h
  - src/z80_cb.c
  - src/z80_cycle.h
  - src/z80_ed.c
  - src/z80_x3.c
findings:
  critical: 0
  warning: 5
  info: 6
  total: 11
status: issues_found
---

# Phase 2: Code Review Report

**Reviewed:** 2026-10-20
**Depth:** standard
**Files Reviewed:** 30
**Status:** issues_found

## Narrative Findings (AI reviewer)

## Summary

The review covered the phase's diff (`f09ac97..HEAD`): the Z80 step and its bus glue, the VDP timing half, the
cartridge probe and copy, the mappers, the I/O ports, the save-state visitor, and the runner's new argument
parser, probe, mailbox and digests. Opcode flag math was not re-derived, because the vectors cover it.

The memory-safety review found nothing to fix:

- **Loader and probe on hostile images.** Every header read is bounds-checked against the body (`at + 16 > body`,
  `body >= 0x8000` for Codemasters, `i + 2 < end` in the opcode scan). Padding and allocation sizes come from a body
  of at most 1 MiB + 511 bytes, so they cannot overflow.
- **Indices from loaded state.** Every one is range-checked before APPLY runs: banks against `pages - 1`, the VDP
  address at 14 bits, CRAM masked, registers below 11, `idx` up to IY, the beam line and tick, and cartridge RAM
  sized by the image.
- **Bounded execution.** It holds. Every step adds at least 12 ticks, prefix chains and block repeats are one step
  each, and a loaded state cannot put the deadline more than one frame ahead of `ticks`.

The findings are about consistency, not memory safety:

- **The save-state validator accepts two impossible machine states (WR-01, WR-02).** One of them wedges the CPU
  permanently.
- **The bus applies the `$3E` cartridge disable to reads but not to writes (WR-03).**
- **Runner `--mailbox` can report "done" before the ROM has started (WR-04).**
- **Runner `--trace`/`--mailbox` can silently drop a requested `--png` and still exit 0 (WR-05).**

## Warnings

### WR-01: A loaded state may be halted with a prefix pending, which locks out every interrupt and NMI for good

**File:** `src/state.c:148-169` (with `src/z80.c:546-563`)
**Issue:** `z80_io` range-checks `halted` (0..1) and `idx` (0..2) independently and never relates them. Real
execution cannot reach `halted = 1` with `idx != NONE`: `DD 76` runs HALT through `exec`, and the step then
clears `idx`. In `cogmaster__z80_step`, interrupt and NMI acceptance are skipped whenever `idx != NONE`. The halted
branch returns before `idx` is cleared. So a state with both set passes VALIDATE, and the CPU then halts forever:
no `/INT`, no Pause NMI and no way out short of a reset. The run stays bounded, but a hostile or corrupt state puts
the instance into a condition no ROM can produce. The same gap admits `ei = 1` with `iff1 = 0`, and `ei`/`p` set
together with a pending prefix. Neither of those can be reached either.
**Fix:** Add cross-field rules in `z80_io`:
```c
uint8_t idx = cogmaster__sio_u8(io, "z80_idx", &z->idx, 0u, COGMASTER__Z80_IDX_IY);
uint8_t halted = cogmaster__sio_u8(io, "z80_halted", &z->halted, 0u, 1u);
uint8_t ei = /* the value returned for z80_ei */;
uint8_t iff1 = /* the value returned for z80_iff1 */;
cogmaster__sio_check(io, !(halted && idx != COGMASTER__Z80_IDX_NONE));
cogmaster__sio_check(io, !(ei && (iff1 == 0u || idx != COGMASTER__Z80_IDX_NONE)));
```
Add a matching rejection case to the state rejection sweep in `tests/core/test_state.c`.

### WR-02: The beam position (line, t) is not tied to `bus.at`, so a state can move the frame interrupt off the frame boundary

**File:** `src/state.c:173-189`
**Issue:** The beam always equals the origin (line 192, tick 607) advanced by `bus.at` modulo the frame length.
`bus_origin` sets it there, `bus_sync` advances it by `ticks - at`, and the test card adds exactly one frame to
both. `bus_io` checks `line` against `frame_lines - 1` and `t` against 683, but never checks that they match `at`.
A state with any (line, t) therefore loads. After that, the frame-interrupt event (line 192, tick 607) no longer
falls on the frame deadline. The V counter and H counter read values that disagree with `ticks`, and the
documented invariant that every frame is exactly one frame long from interrupt to interrupt no longer holds.
**Fix:** Derive the expected beam from `at` and require equality. The arithmetic below uses a 32-bit value, so it
does not divide a 64-bit count:
```c
/* at is at most 69 ticks before frame * ft, so reduce the in-frame offset modulo ft. */
uint32_t ft = cogmaster__frame_ticks(m);
uint64_t start = time.frame * ft;
uint32_t into = at >= start ? (uint32_t)(at - start) : ft - (uint32_t)(start - at);
uint32_t pos = (192u * 684u + 607u + into) % ft;
cogmaster__sio_check(io, b_line == pos / 684u && b_t == pos % 684u);
```
Here `b_line` and `b_t` are the values that `bus_line` and `bus_t` return. Alternatively, stop
saving `line` and `t` and rebuild them from `at` on APPLY.

### WR-03: Writes ignore the `$3E` cartridge disable, and work-RAM-off writes are dropped without a fault

**File:** `src/bus.c:161-173`
**Issue:** `mem_read` treats the slot area as undriven when `$3E` bit 6 is set, records `UNMAPPED_READ` and returns
open bus. `cogmaster__bus_write`, however, always passes addresses below `$C000` to `cogmaster__mapper_write`, so
with the cartridge disabled:
- a Sega board with RAM mapped still stores into cartridge RAM at `$8000-$BFFF`;
- a Codemasters board still latches bank registers at `$0000/$4000/$8000` and still writes its RAM at `$A000`;
- any other write records `ROM_WRITE` against a cartridge that is off the bus, where `UNMAPPED_WRITE` would be
  expected.

The cartridge's chip enable comes from the slot select that `$3E` gates (SMS-SYSTEM section 4), so these writes
should not reach it. In the other direction, a write to `$C000-$FFFB` with work RAM off (`$3E` bit 4) is dropped
silently. The matching read records `UNMAPPED_READ`, so the first-fault record misses that half of the condition.
**Fix:** Apply the same decode to writes:
```c
void cogmaster__bus_write(cogmaster *m, uint16_t addr, uint8_t v)
{
    cogmaster__bus_sync(m);
    if (addr >= 0xC000u) {
        if ((m->io.mem_ctrl & BUS_RAM_OFF) == 0u) {
            m->ram[addr & (COGMASTER__RAM_SIZE - 1u)] = v;
        } else if (addr < 0xFFFCu) {
            cogmaster__bus_fault(m, addr, COGMASTER__DEVICE_RAM, COGMASTER__FAULT_UNMAPPED_WRITE);
        }
        if (addr < 0xFFFCu) {
            return;
        }
    } else if ((m->io.mem_ctrl & BUS_CART_OFF) != 0u) {
        cogmaster__bus_fault(m, addr, COGMASTER__DEVICE_CART, COGMASTER__FAULT_UNMAPPED_WRITE);
        return;
    }
    cogmaster__mapper_write(m, addr, v);
}
```
Decide, with a citation, whether the Sega mapper's `$FFFC-$FFFF` latches still see writes while the slot is
disabled. Pin that decision with a bus test.

### WR-04: `--mailbox` reports "done" before the ROM has initialised the mailbox

**File:** `runner/mailbox.c:38-41` (with `runner/main.c:319`)
**Issue:** `cogmaster__mailbox_done` returns true whenever `$C105` reads `$00`, but power-on load clears work RAM
(`boot_handoff(m, 1)`), so the status byte reads `$00` before the ROM writes anything. The runner then stops after
frame 1 and prints `status=00` (with `magic=bad`) in two cases:
- the ROM has not reached `mb_init` within its first frame;
- the image is not a diagnostic ROM at all.

The same false "done" happens if a frame boundary falls inside `mb_init`. That routine clears `$C100-$C1FF`, then
writes the magic, then sets `$80`, so a frame can end with the magic already present and the status still `$00`.
The runner "never judges", but its stop condition decides which frame the measurement comes from, and here it
stops early.
**Fix:** Stop only on a running-to-done transition seen by the runner:
```c
/* in run(): */
int mailbox_seen_running = 0;
...
if (opt->mailbox && status == COGMASTER__EXIT_OK) {
    uint32_t s = cogmaster_peek(m, COGMASTER_SPACE_BUS, 0xC105u);
    if (s == 0x80u && mailbox_magic_ok(m)) mailbox_seen_running = 1;
    else if (s == 0x00u && mailbox_seen_running && mailbox_magic_ok(m)) break;
}
```
Document the rule in docs/runner.md.

### WR-05: An early stop from `--trace` or `--mailbox` skips a requested `--png` and still exits 0

**File:** `runner/main.c:305-327`
**Issue:** `check_combination` accepts `--png N:FILE` for any N up to `--frames`. When `--trace first-fault`
(line 314) or `--mailbox` (line 319) breaks out of the loop before frame N, that PNG is never written and the run
exits 0 with no message. A script that asked for `--png 600:out.png --trace first-fault --frames 600` and then
read `out.png` finds either nothing or a stale file from an earlier run. The signal stop is documented to behave
this way, but these two stops are not.
**Fix:** After the loop, report each `--png` whose frame was not reached. Either print a one-line notice on stderr
and keep status 0, as the signal stop does, or make the stop conditions and later `--png` frames a usage error.
Document whichever you choose in docs/runner.md.

## Info

### IN-01: Fault records name the wrong instruction for prefixed undefined ED opcodes and for interrupt acceptance

**File:** `src/bus.c:251-256`, `src/z80.c:494-531`, `src/z80.c:566-568`
**Issue:** `cogmaster__bus_ed_undefined` stamps `pc = z80.pc - 2`, which is wrong for `DD ED xx` / `FD ED xx`,
where the instruction started 3 bytes back. `m->bus.op_pc` already holds the correct start. NMI and `/INT`
acceptance, and halted M1 cycles, never update `bus.op_pc`/`bus.op`. A fault raised by the acceptance push, for
example with SP pointing into ROM or RAM disabled, therefore names the previous instruction.
**Fix:** Use `m->bus.op_pc` in `bus_ed_undefined`. Set `op_pc = z->pc` and a sentinel `op` at the start of `nmi()`
and `irq()`.

### IN-02: `bus.vdp_at` is saved, validated and never read

**File:** `src/bus.c:207`, `src/bus.c:219`, `src/state.c:180`, `src/state.c:188`
**Issue:** Nothing reads the field except tests. It costs 8 state bytes and a validation rule, and stays dead until
a consumer exists.
**Fix:** Leave it if a planned consumer (fast-access counting) is close. Otherwise drop it from the struct and the
state format.

### IN-03: Images of 1 MiB + 1 to 1 MiB + 511 bytes without a copier header load as 65 pages

**File:** `src/cart.c:12-13`, `src/cart.c:162`
**Issue:** `CART_MAX` is 1 MiB + 512 so that a 1 MiB image can carry a copier header. But a file in
`(1 MiB, 1 MiB + 512)` has no header by the `n & 0x3FFF == 512` rule, so its whole body is kept: 65 pages, which
exceeds the stated 1 MiB limit. It is memory-safe, since a bank is `uint8_t` and pages are 65 at most, but it is
outside the documented bounds.
**Fix:** After the strip, also refuse `body > 1048576u` with `ERR_UNSUPPORTED_SIZE`.

### IN-04: The `(int8_t)` conversion of a displacement byte is implementation-defined before C23

**File:** `src/z80.c:39`, `src/z80.c:162`, `src/z80_cb.c` (`indexed`)
**Issue:** Converting a `uint8_t` above 127 to `int8_t` is implementation-defined in C11/C17. Every supported
compiler wraps it, but rule 4 asks for identical results on every platform without relying on that.
**Fix:** Use `(uint16_t)(d ^ 0x80u) - 0x80u`, or `d >= 0x80u ? (uint16_t)(d | 0xFF00u) : d`.

### IN-05: The `--probe` report and options are loose at the edges

**File:** `runner/probe.c:164`, `runner/args.c:160-167`
**Issue:** `region_guess` prints `ntsc-u` when the probe reports 0 (unknown or no header), so "no guess" cannot be
told apart from "export". Separately, `--probe` silently accepts `--machine` and `--region`, though the usage text
says it "takes no run option".
**Fix:** Print `region_guess=none` when `info.region_guess == 0`. Reject `--machine`/`--region` together with
`--probe`, or document that they are ignored.

### IN-06: The state fault record accepts values the core never records

**File:** `src/state.c:129-145`
**Issue:** `fault_kind` and `fault_device` accept 0..255, `fault_width` any value (the core only writes 8), and
`fault_frame` may exceed the header's frame count. None of this is unsafe, since the runner maps unknown codes to
`unknown`, but a refused-means-impossible validator should reject them.
**Fix:** Bound kind to `COGMASTER__FAULT_ED_UNDEFINED`, device to `COGMASTER__DEVICE_CTRL`, and width to `{0, 8}`.
Check `frame <= time.frame` when `present`. To do that, pass `time` into `fault_io`.

---

_Reviewed: 2026-10-20_
_Reviewer: Claude (gsd-code-reviewer)_
_Depth: standard_
