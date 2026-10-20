---
phase: 02-the-cpu-matches-the-public-vectors
fixed_at: 2026-10-20T00:00:00Z
review_path: .planning/phases/02-the-cpu-matches-the-public-vectors/02-REVIEW.md
iteration: 1
findings_in_scope: 5
fixed: 5
skipped: 0
status: all_fixed
---

# Phase 2: Code Review Fix Report

**Fixed at:** 2026-10-20
**Source review:** .planning/phases/02-the-cpu-matches-the-public-vectors/02-REVIEW.md
**Iteration:** 1 (fix round 1 of at most 2)

**Summary:**
- Findings in scope: 5 (WR-01 to WR-05; IN-01 to IN-06 left untouched by scope)
- Fixed: 5
- Skipped: 0

## Fixed Issues

### WR-01: A loaded state may be halted with a prefix pending, which locks out every interrupt and NMI for good

**Files modified:** `src/state.c`, `tests/core/test_state.c`
**Commit:** 775b29c
**Applied fix:** `z80_io` now has two cross-field rules: a pending DD/FD prefix excludes `halted`, `ei` and `p`, and
`ei` requires `iff1`. All of these follow from z80.c: a prefix is a step of its own that clears the EI and LD A,I/R
marks, the step that consumes it clears it again, and EI sets IFF1 in the same step as its mark. The test
`core.state.rejects_header` (`reject_cpu_states`) checks that each impossible pair or triple is refused and leaves the
instance unchanged. It also checks that each flag on its own still loads into a fresh instance. Before the fix,
the test failed with 3 acceptances.

### WR-02: The beam position (line, t) is not tied to `bus.at`

**Files modified:** `src/bus.c`, `src/internal.h`, `src/state.c`, `tests/core/test_state.c`
**Commit:** 5b0ec0d
**Applied fix:** New `cogmaster__bus_beam_at(m, frame, at)` in bus.c, which keeps the frame origin constants
private. It returns the origin advanced by `at` modulo the frame and does the arithmetic in 32 bits. `bus_io`
requires `line * 684 + t` to equal it. The test refuses line ±1 and tick ±1 (each inside its own bounds) for an
image state, and failed with 4 acceptances before the fix. The existing round-trip and sweep controls (NTSC and
PAL, test card and image) still load.
**Status:** fixed: requires human verification (a logic invariant; the round-trip tests show that states the core
produces still satisfy it)

### WR-03: Writes ignore the `$3E` cartridge disable, and work-RAM-off writes are dropped without a fault

**Files modified:** `src/bus.c`, `docs/embedding.md`, `tests/mapper/test_mapper.c`, `tests/mapper/tests.cmake`,
`tests/inventory/ci.txt`, `tests/inventory/wasm32.txt`
**Commit:** 3ec02c1
**Applied fix:** `cogmaster__bus_write` now applies the same `$3E` decode as reads:
- With the cartridge off, a write below `$C000` records `unmapped_write` on the cartridge and reaches neither its
  RAM nor its registers (Sega or Codemasters).
- With work RAM off, a write that work RAM would have taken records `unmapped_write` on RAM.
- `$FFFC`-`$FFFF` go to work RAM when it is on and to the mapper only while the cartridge is on. With both off,
  such a write records `unmapped_write` on RAM.

**Decision:** SMS-SYSTEM sections 3 and 4 say `$3E` drives the slot enables, but they do not settle whether a
cartridge's mapper still latches `$FFFC`-`$FFFF` while the cartridge is disabled. Their only related note is that the
SMS1 SegaScope at `$FFF8` works with the card slot off, and that is a console-side device. As instructed, I chose the
symmetric behaviour (the disabled cartridge sees no write, mirroring the read side). The bus.c comment says this,
and the new test `mapper.slot_off` pins it. The fault table in docs/embedding.md now lists these cases. The new test
failed on the old bus.c.
**Status:** fixed: requires human verification (hardware-behaviour choice the docs leave open)

### WR-04: `--mailbox` reports "done" before the ROM has initialised the mailbox

**Files modified:** `runner/mailbox.c`, `runner/mailbox.h`, `runner/main.c`, `docs/runner.md`,
`tests/runner/test_runner.c`, `tests/runner/tests.cmake`, `tests/inventory/ci.txt`, `tests/inventory/wasm32.txt`
**Commit:** f27e619
**Applied fix:** A pure stop rule, `cogmaster__mailbox_stop(&running, head)`. It stops on status `$00` with the
magic present, but only after some earlier frame ended with `$80`. A missing magic resets the flag, so a mailbox
caught between its clear and its magic write cannot stop the run. `cogmaster__mailbox_done` peeks the 6 head bytes
and applies the rule.

**Tests:**
- `runner.mailbox.stop` drives the rule frame by frame (unit).
- It also runs `di-halt.sms --frames 3 --mailbox --hash` and requires `frame 3`. The old runner stopped at frame 1.

The fixture mailbox tests still pass. docs/runner.md now documents the rule, including that a ROM which finishes
within its first frame runs to the frame count.

### WR-05: An early stop from `--trace` or `--mailbox` skips a requested `--png` and still exits 0

**Files modified:** `runner/main.c`, `docs/runner.md`, `tests/runner/tests.cmake`, `tests/inventory/ci.txt`,
`tests/inventory/wasm32.txt`
**Commit:** 9f9076e
**Applied fix:** After the frame loop, each `--png` whose frame did not run gets a line on standard error:
`cogmaster-run: --png N:FILE not written: the run stopped after frame R`. The exit status stays 0, and standard
output is unchanged. This covers trace, mailbox and signal stops. docs/runner.md (the `--png` row and Stopping)
documents it. The new test `runner.png_unreached` runs `in-a-00.sms` with `--trace first-fault`, a frame 1 PNG and
a frame 10 PNG. It requires:
- exit 0;
- standard output holding only the fault line;
- standard error naming the frame 10 PNG;
- the frame 1 PNG written and the frame 10 PNG absent.

## Verification

All gates ran in the main checkout (`workflow.use_worktrees` is `false`), so these results can be reproduced from
the tree on this branch:

- `/opt/homebrew/opt/llvm@21/bin/clang-format -i` on every touched C file, before each commit
- `cmake --workflow --preset ci`: exit 0 (183 tests, plus 4 in the release step; warnings are errors, with
  `-Wconversion -Wsign-conversion`)
- `cmake --workflow --preset hygiene`: exit 0
- Every `src/*.c` file is under 800 lines (the largest is `src/z80.c` at 580; `src/state.c` is 400)

---

_Fixed: 2026-10-20_
_Fixer: Claude (gsd-code-fixer)_
_Iteration: 1_
