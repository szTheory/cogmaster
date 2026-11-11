---
phase: 10-bios-boot
fixed_at: 2026-11-11T00:00:00Z
review_path: .planning/phases/10-bios-boot/10-REVIEW-CORE.md, .planning/phases/10-bios-boot/10-REVIEW-HOST.md
iteration: 1
findings_in_scope: 11
fixed: 11
skipped: 0
status: all_fixed
---

# Phase 10: Code Review Fix Report

**Fixed at:** 2026-11-11
**Source reviews:** `10-REVIEW-CORE.md` (core half), `10-REVIEW-HOST.md` (host half)
**Iteration:** 1 of at most 2

**Summary:**
- Findings in scope: 11 (core WR-01, WR-02; host CR-01, CR-02, WR-01, WR-02, WR-03, WR-04, IN-01 with its two parts)
- Fixed: 11
- Skipped: 0
- Out of scope, recorded as seeds in `.planning/seeds/SEED-011-bios-follow-ups.md` ("Phase 10 review leftovers"):
  host WR-05, WR-06, WR-07, IN-02, IN-03 and core IN-01 to IN-06. A tracked file name holding a newline, which the
  CR-02 fix leaves unscanned, was added to the same section.
- No BIOS bytes, library file or `.env.local` was opened. Every scan reproduction uses the self-authored dump the
  controls already build (byte i = (i * 37 + 11) mod 256).

## Fixed Issues

### Core WR-01: the BIOS-attached silent-slot guard had no test for work RAM off

**Files modified:** `tests/bios/test_bios.c`
**Commit:** eead405
**Applied fix:** `test_ram_off`, called at the end of `bios.arbitration`, runs all sixteen b7/b6/b5/b3 combinations
with `$3E` b4 set and a BIOS attached, on SMS 1 and SMS 2, and reads and peeks `$C000` and `$FFFF`. Each probe must
return the open-bus value ($00 on SMS 1, $FF on SMS 2), record exactly one fault and count one open-bus read.
**Negative control:** with `addr < 0xC000u &&` removed from `src/bus.c` (`git diff --stat` showed the edit and the
build succeeded), `bios.arbitration` fails; restored, all eight `bios.*` tests pass. The reviewer's mutant passed
all five core cases before this change.

### Core WR-02: the `mem_slow` invariant comment overclaimed

**Files modified:** `src/bus.c`, `src/internal.h`
**Commit:** eead405
**Applied fix:** the comment on `mem_slow_driven` now says that peek and the cold path are independent of the flag,
that the dispatch in `mem_read` does test it, and that every writer of `$3E` or the BIOS size (reset, power-on, the
port write, a state load) must call `bus_decode`. The comment on the `mem_slow` field says the same.
**Deviation (no setter):** the single setter was not added. A state load replaces the whole `io` block, so it cannot go
through one function, and a setter that covers three of four writers would suggest a guarantee it does not give. The
two test sites that write `io.mem_ctrl` already call `bus_decode` by hand and stay as they are. Recorded as a seed.
**Negative control:** none; a comment change has no behaviour. The hot path is unchanged (no code line in `src/`
changed other than comments).

### Host CR-01: an unreadable target silently truncated the scan

**Files modified:** `scripts/firmware-window.sh`, `tests/policy/firmware_window_cases.cmake`, `tests/inventory/hygiene.txt`
**Commit:** 2f11a3d
**Applied fix:** a read goes through `rd`, which sends `od`'s own message to nowhere (it would print a dump path) and
records the target under its name; the scan goes on to the later targets and then prints
`firmware window: cannot read NAME` for each and exits 2 (a hit still exits 1). A closing `Z` marker makes awk report
`scan incomplete` and exit 2 if the stream ended early for any other reason. An unreadable dump file is recorded as
`dump`, never by path.
**Negative control:** `policy.firmware_window.unreadable` plants `a_unreadable.txt` before a target holding a
64-byte slice, with an `od` wrapper that fails for that name so the control holds for a privileged user. It expects
exit 1, the hit line, then the cannot-read line. On the old script it fails.

### Host CR-02: non-ASCII file names were skipped

**Files modified:** same as CR-01
**Commit:** 2f11a3d
**Applied fix:** `git ls-files -z` through `tr '\0' '\n'`.
**Negative control:** `policy.firmware_window.nonascii` tracks a file named with two UTF-8 bytes holding a slice and
expects the hit on that name. Fails on the old script.

### Host WR-01: the libretro module glob never matched

**Files modified:** same as CR-01
**Commit:** 2f11a3d
**Applied fix:** `build/*/libretro/cogmaster_libretro.*`.
**Negative control:** `policy.firmware_window.module` plants a slice in `build/x/libretro/cogmaster_libretro.so` with
`build/` ignored, so only the glob can find it. Fails on the old script.

### Host WR-02: a missing `unzip` or failed extraction was swallowed

**Files modified:** same as CR-01
**Commit:** 2f11a3d
**Applied fix:** the zip loop records the zip's repository-relative name when `mkdir`, `unzip` or `find` fails (a
missing `unzip` is exit 127 through the same branch), prints it and exits 2 after the rest of the scan.
**Negative control:** `policy.firmware_window.zip_bad` (a file that is not a zip) expects exit 2 and the cannot-read
line; it fails on the old script. `policy.firmware_window.zip` is the positive control that a window inside a
release zip is found; it passes on both scripts and guards the extraction path against being dropped.

### Host WR-03: a configured but unusable dump directory read as a clean pass

**Files modified:** same as CR-01, plus `docs/testing.md`
**Commit:** 2f11a3d
**Applied fix:** a configured value that is not a directory prints `the configured directory is not usable` and exits
2; a directory with no usable dump file prints `no usable dump file in the directory` and exits 2. Only an
unconfigured scan says `no dump configured, 0 compared` and exits 0. `docs/testing.md` lists a dump kept as a zip,
`xxd` and `hexdump -C` layouts as not covered (36,918 bytes, cap 38,912).
**Negative control:** `policy.firmware_window.quoted` (a quoted `.env.local` value, variable unset) and
`policy.firmware_window.dir_empty` both expect exit 2 and their line. Both fail on the old script.

### Host WR-04: no control reached the default `targets()` path

**Files modified:** `tests/policy/firmware_window_cases.cmake`, `tests/inventory/hygiene.txt`
**Commits:** 2f11a3d, 01bca37 (the dump variable name built in pieces, which `policy.bios_dir_reads` requires),
2fde639 (timeouts marked provisional)
**Applied fix:** seven controls in the hygiene lane, each in a throwaway repository with `build/` ignored and the scan
run with no target: `nonascii`, `module`, `zip`, `zip_bad`, `unreadable`, `quoted`, `dir_empty`. The `fwin_case`
helper takes `FAIL2` for an expected exit 2. Inventory lines added to `tests/inventory/hygiene.txt`, sorted.
**Negative control:** against the old script six of the seven fail (all but `zip`, the positive control above); with
the new script all 17 `policy.firmware_window.*` tests pass.

### Host IN-01: stale pointer and an unreachable branch

**Files modified:** `host/bios.h`, `runner/main.c`
**Commit:** 70ff6ed
**Applied fix:** `host/bios.h` points at `docs/bios.md`. `load_status` lost its `gg_bios` parameter and the Game Gear
branch: `bios_load` refuses the same case first, with the same message and exit 77, and `run` only gets a BIOS
after it succeeds. The runner's existing Game Gear refusal test still passes (111 of 111 `runner` and `bios` tests).
**Negative control:** none needed; the deletion removes a branch the existing tests prove unreachable.

## Verification

Run in the main checkout (`workflow.use_worktrees` is false), from the repository root.

| Command | Exit |
|---|---|
| `cmake --workflow --preset ci` | 0 |
| `cmake --workflow --preset hygiene` | 8: only `hygiene.tree` timed out at its 60 s limit; every other hygiene test passed |
| `sh scripts/hygiene.sh --tree` (what `hygiene.tree` runs, outside the limit; 51 s under load) | 0 |
| `cmake -DROOT=$PWD -P tests/cmake/budgets.cmake` | 0 (845 check lines, 9,402 src lines; limit 940) |
| `ctest --test-dir build/hygiene -R firmware_window` | 0 (17 of 17) |
| `ctest --test-dir build/ci -R '^bios\.'` | 0 (8 of 8) |
| gcc 13 container, `-std=c17 -Wall -Wextra -Wpedantic -Werror -Wtype-limits -Wconversion -fsyntax-only` over `tests/bios/test_bios.c`, `src/bus.c`, `runner/main.c` | 0 |
| `clang-format` 21 `--dry-run -Werror` over `src/bus.c`, `src/internal.h`, `tests/bios/test_bios.c`, `runner/main.c`, `host/bios.h` | 0 |
| `git push origin phase/10-bios-boot` | 0 |

A manual run of the new script over the real tree with the synthetic dump as the configured directory printed
`1 compared, 0 ignored` and exited 0 (56 s).

Deviations:
- One commit (2f11a3d) covers host CR-01, CR-02, WR-01, WR-02, WR-03 and WR-04: they are one rewrite of one script
  with one set of controls, and an earlier-finding commit would not have had a green control set on its own.
- No core setter for `io.mem_ctrl` (see core WR-02).
- Timeouts of the seven new controls are provisional on the existing 20 s floor (local slowest 0.43 s; no hosted time
  exists yet). The next hosted dispatch settles them.
- The new controls run only in the Linux hygiene lane, like the others in the file.

---

_Fixed: 2026-11-11_
_Fixer: Claude (gsd-code-fixer)_
_Iteration: 1_
