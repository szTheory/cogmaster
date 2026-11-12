---
phase: 09-light-phaser
reviewed: 2026-11-11T00:00:00Z
depth: standard
files_reviewed: 38
files_reviewed_list:
  - CMakeLists.txt
  - README.md
  - docs/conformance.md
  - docs/controllers.md
  - docs/embedding.md
  - docs/hardware/video.md
  - docs/runner.md
  - docs/testing.md
  - host/aim.c
  - host/aim.h
  - include/cogmaster.h
  - libretro/libretro.c
  - libretro/options.h
  - quality.json
  - runner/args.c
  - runner/args.h
  - runner/input.c
  - runner/input.h
  - runner/main.c
  - runner/probe.c
  - scripts/compat-common.sh
  - scripts/library.cmake
  - tests/cmake/hashes.cmake
  - tests/cmake/repository.sh
  - tests/compat/ratchet.cmake
  - tests/compat/tests.cmake
  - tests/fixtures/tests.cmake
  - tests/host/test_aim.c
  - tests/host/tests.cmake
  - tests/libretro/gun.h
  - tests/libretro/host.c
  - tests/libretro/tests.cmake
  - tests/policy/tests.cmake
  - tests/roms/manifest.txt
  - tests/roms/src/light_phaser.asm
  - tests/roms/src/light_phaser.link
  - tests/runner/tests.cmake
  - tests/inventory/ci.txt
findings:
  critical: 0
  warning: 2
  info: 4
  total: 6
status: issues_found
---

# Phase 9: Code Review Report (host and tooling half)

**Reviewed:** 2026-11-11
**Depth:** standard
**Files Reviewed:** 38
**Status:** issues_found

## Summary

No Critical defects. The runner, `host/aim.{c,h}`, the libretro adapter, the ledger token widening and the docs hold up against the code.

What was checked beyond reading:
- `host/aim.c` was fuzzed 2 million times under ASan and UBSan with random rectangles, pointers, deltas and cursors. It found no overflow and no point outside the rectangle, and the cross never exceeded 17 points.
- The `ci` preset configures with the inventory exact and sorted, and the floors (588, 548) equal the inventory counts. The full suite ran in a scratch build tree. All new tests pass (`policy.no_title_table*`, `host.aim`, `runner.gun.*`, `libretro.host.gun.*`, `fixtures.light_phaser.*`, `abi.api_v13`, `compat.*`). Five unrelated tests fail only because the build tree sits outside the checkout (`compat.public_update`, `library.synthetic`, `release.*`); they are not defects.
- The `repository.sh` counted lines are 735 of a limit of 922 (9225 `src/` lines).
- The three ledger regex copies and the `conformance.md` copy are identical. No other copy exists in `scripts`, `tools`, `tests` or `.github`.
- Pre-existing tests and the diff outside the new rows are byte-equal.
- No planning IDs appear in user docs. Every constant, option name, exit code and reset rule in `controllers.md`, `runner.md` and `embedding.md` matches the code, apart from the stick figure in WR-02.
- The crosshair is written only into `pixels` after the conversion loop, and the colour layout matches `cogmaster__rgb444_to_rgb565`.
- Pointer and cursor sources, Pause and Reset on a gun at port 0, descriptor count 18, controller types 3 and option count 11 are consistent.

Two warnings and four info items follow.

## Warnings

### WR-01: `no_title_table` does not catch what it claims (string-initialised arrays; string literals defeat the comment stripper)

**File:** `tests/cmake/repository.sh:177-189`
**Issue:** The gate is described as failing on "an initialised array" in `gun.c` and as scanning code only. Verified with the awk body copied verbatim:
- `static const char offs[] = "\x09\x13\x0F";` passes. Only the literal text `] = {` is matched, so a string-initialised table (the compact form of a MAME-style offset table) is invisible. `]={` and `] =\n{` also pass, but clang-format in CI normalises those.
- The stripper is not string-aware. A line such as `const char *u = "a//b"; int x = m->cart.rom[0x7ff0];` drops everything after the `//`. A literal `"/*"` opens a block comment that blanks the rest of the file until a `*/`. Both returned exit 0 against a one-file tree. No current file triggers this, and the plan accepted it, but no self-test pins the limit, so the first debug string with `//` in `vdp.c` or `io.c` silently disables the gate for the rest of that file.
- A ROM header read through a helper (`cogmaster__cart_byte(m, 0x7ff0)`) matches none of the tokens, because `(->|[.])cart` needs a member access. This one is by design and is noted only for completeness.

**Fix:** Extend the array rule to `\] *=[ \t]*(\{|")`. Also add a `gun.c` self-test (a table in a string) and either teach the stripper to skip `"..."` (a small state flag) or add a test that fails when a scanned file contains `//` or `/*` inside a string:
```awk
code ~ /\] *= *[{"]/
```
Rule 7 normally makes tooling findings Info; this is a Warning only because the gate's stated guarantee ("any initialised array declaration") is not met.

### WR-02: Cursor stick speed is not the documented 4 counts and is asymmetric

**File:** `libretro/libretro.c:361-374` (`lr_axis`, used with `LR_STICK_PHASER 4`); `docs/controllers.md` (libretro paragraph: "4 counts a frame at full deflection"); `tests/libretro/gun.h` (comment "moves 4 counts at full deflection")
**Issue:** `stick * k / 32768` truncates toward zero. At +32767 that is 32767*4/32768 = 3, and at -32768 it is -4. So full deflection moves the cursor 3 dots a frame right or down and 4 left or up, not "4". The new doc, constant and test comment all state 4. The cursor test cannot catch it because its model (`dev_sum`) uses the same formula as the adapter. The existing Sports Pad and Paddle constants share the pattern, but the phaser cursor is the new, user-visible case, since the player steers a pointer at a target.
**Fix:** Either document "up to 3 or 4" or make the scale symmetric with rounding away from zero, and update the shared `dev_sum` model in `tests/libretro/controls.h` accordingly:
```c
int32_t scaled = (stick * k + (stick < 0 ? -16384 : 16384)) / 32768; /* symmetric */
```

## Info

### IN-01: Fixture rows cite sources whose latch edge they do not use

**File:** `tests/diag/expected.csv:261-289`
**Issue:** The `light_phaser` rows have source `doc` and cite "SMS Power Light Phaser and MacDonald msvdp section 11". The expected H values (`0048-004A`, `0053-0055`) are the closed form of the release-edge rule taken from MAME and ares. `controllers.md` itself says SMS Power describes the latch at TH going low and MacDonald either edge. The citation therefore attributes the pinned value to sources that disagree with it.
**Fix:** Cite the model ("closed form of the release-edge rule, MAME `lphaser.cpp`; unmeasured") instead of the two documents, or use a source word other than `doc` if the column is meant to separate measured from derived values.

### IN-02: `compat.device_key` never asserts an `lphaser` pair is present or distinct

**File:** `tests/compat/tests.cmake:100-120`
**Issue:** The 25-pair body is accepted (exit 0), which proves the token is in the regex. The two content assertions (`port1=sports;port2=paddle` and the reverse) and the duplicate-key control still use only the old devices. A normaliser that collapsed or dropped `lphaser` pairs would pass. `ratchet.cmake` does cover changed and adjacent `lphaser` keys.
**Fix:** Add `port1=lphaser;port2=pad` and `port1=pad;port2=lphaser` to the content `MATCHES`, and use an `lphaser` pair for the duplicate control.

### IN-03: Stale and ambiguous comments

**File:** `tests/fixtures/tests.cmake` (V counter comment above `lp_v_high`); `docs/runner.md:45`
**Issue:**
- The comment says "one more line per 228 cycles of loop (both periods here are shorter than a line)" while the code adds `+ 1` unconditionally and the fixture has ten periods, with only periods 29 and 64 used for V.
- `docs/runner.md` still says "the second case names the port and the controller". The sentence now lists three refusal cases, so "second" is ambiguous.

**Fix:** State the `+ 1` as the read-and-loop slack, and rewrite the runner.md sentence as "a refusal for a port's controller names the port and the controller".

### IN-04: The cross-lane hash row for the fixture covers only the fixed picture

**File:** `tests/cmake/hashes.cmake:87-93`
**Issue:** The `light_phaser.sms|phaser|...` device row prints the frame hash, and the fixture's picture is fixed whatever the gun reads. A lane-dependent latch error inside the plus or minus 1 tolerance would therefore not show in `hash-equality`. Core-level exactness is covered by the `ports.gun.*` tests, which run on every lane, so this is a gap in the stated claim, not in coverage.
**Fix:** Either say so in the row's comment, or append the mailbox records to the hashed device output for this row.

---

_Reviewed: 2026-11-11_
_Reviewer: Claude (gsd-code-reviewer)_
_Depth: standard_
