# Deferred items, phase 02

## Found during 02-09 (not caused by it)

- `ctest --test-dir build/hygiene` fails two checks on files that 02-09 did not touch:
  - `hygiene.format` flags `src/vdp.c`, `src/z80.c`, `src/z80_cb.c`, `src/z80_ed.c`, `src/z80_cycle.h`,
    `tests/bus/test_bus.c`, `tests/bus/test_vdp.c` and `tests/z80/int_tests.c`. Most of the flags are single-line
    `case X: return ...;` arms that clang-format 21 splits. `src/vdp.c` at the base commit `af64ae3` already gives 8
    errors, so the failure predates 02-09.
  - `hygiene.no_float` matches section numbers such as "4.2" in comments: four lines in `src/vdp.c` and two in `src/z80.c`.
  - Fix: run clang-format 21 on those files and rewrite the section numbers in those comments ("section 4, part 2"),
    or teach `no_float.cmake` to skip comments. Either way, do it before `/gsd-ship`, because the hygiene lane
    runs in CI.
  - Resolved by commits 3834dff (clang-format 21 and single section numbers) and 2cceac5 (opcodes 0xC0-0xFF moved to the new `src/z80_x3.c` so `src/z80.c` stays under 800 lines).
