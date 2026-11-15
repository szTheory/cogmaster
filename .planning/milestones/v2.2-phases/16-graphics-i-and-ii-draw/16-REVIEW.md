---
phase: 16-graphics-i-and-ii-draw
reviewed: 2026-11-14T12:00:00Z
depth: standard
files_reviewed: 46
files_reviewed_list:
  - CMakeLists.txt
  - PROVENANCE.md
  - cmake/WlaDx.cmake
  - docs/audio.md
  - docs/conformance.md
  - docs/embedding.md
  - docs/hardware/video.md
  - docs/runner.md
  - docs/testing.md
  - include/cogmaster.h
  - scripts/fetch-pin.sh
  - scripts/library.sh
  - src/internal.h
  - src/state.c
  - src/vdp.c
  - src/vdp_legacy.c
  - src/vdp_render.c
  - tests/api/v14.c
  - tests/bios/test_bios.c
  - tests/built/expected.txt
  - tests/cmake/hashes.cmake
  - tests/core/test_core.c
  - tests/core/test_state.c
  - tests/core/tests.cmake
  - tests/diag/expected.csv
  - tests/fixtures/legacy.c
  - tests/fixtures/tests.cmake
  - tests/fuzz/seeds.c
  - tests/inventory/ci.txt
  - tests/inventory/wasm32.txt
  - tests/library/diagnose.cmake
  - tests/library/tests.cmake
  - tests/policy/repository-checks.txt
  - tests/policy/tests.cmake
  - tests/policy/tooling_cases.cmake
  - tests/ports/test_gun.c
  - tests/roms/manifest.txt
  - tests/roms/src/legacy_modes.asm
  - tests/roms/src/legacy_modes.link
  - tests/roms/src/legacy_sprites.asm
  - tests/roms/src/legacy_sprites.link
  - tests/runner/diagnose.cmake
  - tests/runner/tests.cmake
  - tests/vdp/test_background.c
  - tests/vdp/test_legacy.c
  - tests/vdp/tests.cmake
findings:
  critical: 0
  warning: 3
  info: 4
  total: 7
  necessity: 0
status: issues_found
---

# Phase 16: Code Review Report

**Reviewed:** 2026-11-14T12:00:00Z
**Depth:** standard
**Files Reviewed:** 46
**Status:** issues_found

## Summary

The core change (Graphics I and II draw, `src/vdp_legacy.c`, the render dispatch in `src/vdp_render.c`, the
status read in `src/vdp.c`, and state format 17 in `src/state.c`) is correct against its stated models. I found no
bug that changes emulated output, no unbounded call, no memory-unsafe access and no determinism hazard. The three
warnings are a false compatibility claim in a comment, a loader check weaker than the invariant it documents, and
missing timeouts in two download paths (tooling, so a seed).

What I checked beyond reading, so the absence of blockers rests on evidence:

- A scratch build with ASan and UBSan (`-fno-sanitize-recover=all`, `CMAKE_BUILD_TYPE=Debug`, fixtures on, the
  pinned WLA-DX reused). All of `vdp.legacy.*`, `vdp.unsupported.*`, `core.state.*`, `fixtures.legacy*`,
  `bios.*`, `policy.*`, `library.*` and `compat.*` pass. The `release.*` and `abi.*` failures in my out-of-tree
  build are environmental (no packaged zips, out-of-tree build directory) and do not touch this diff.
- Strict-warning syntax checks of `vdp_legacy.c`, `vdp_render.c`, `vdp.c` and `state.c` with `-Wconversion
  -Wsign-conversion -Wshadow -Wcast-qual -Wmissing-prototypes`: clean.
- A split-invariance fuzz of `cogmaster__vdp_render_until` in Graphics I and II (3000 random register sets, video
  memory, SATs and random cut points, all three profiles): the field, the status byte and the latched fifth index
  were identical to a single whole-field call every time.
- A state round-trip fuzz (random mid-frame writes to R0, R1, R3 to R6, the SAT and video memory, switching
  between Graphics I, II, mode 4 and text, saving at 21 600 points, loading into a fresh instance): no refusal
  of a state the core produced, byte-identical re-save, and 2 160 further runs of two frames after the load
  gave identical states on the original and the restored instance.
- `scripts/fetch-pin.sh` and `cmake/WlaDx.cmake` retry logic exercised against a refused port (three attempts,
  exit 1) and read for the digest check ordering.

Findings about test and tooling code are marked "seed" per project rule 7 and are not blockers.

## Warnings

### WR-01: Comment claims video RAM and colour RAM offsets of a saved state are unchanged; they moved

**File:** `src/internal.h:122-129` (also `src/state.c:8-13`)
**Issue:** The new comment on the appended fields says "Appended after vram so the colour and video RAM offsets of
a saved state stay where they were." That is false. `struct cogmaster__vdp`'s `sprite[8]` gained a fourth byte
(`attr`) and sits before `cram` and `vram`, so both move by 8 bytes in the struct (the `_Static_assert` moves from
16510 to 16520, only +10 for 11 added bytes because of padding). In the state stream `vdp_io` writes `sprite_attr`
inside the slot loop (`state.c:291-300`), before `cram_latch`, `cram` and `vram`, so those offsets move by 8 as
well. Nothing in the repository reads the offsets (the sweep and the BIOS equivalence test go by field name,
`docs/embedding.md` calls the state opaque and says state format 17 widens the slots), so no code is wrong, but the
comment asserts a layout-compatibility property of a persisted format that does not hold. The file header in
`state.c` also lists only the three appended fields and leaves out the `attr` byte inserted in each slot.
**Fix:** Say what is true, in both places:
```c
/* sprite_legacy, sprite_size and fifth_index are appended after vram in the saved state. The slot
   records also grew by one byte (attr), so every later offset of a state moved: format 17. */
```

### WR-02: Loader accepts a Graphics I or II overflow record no scan can leave, and a test pins that acceptance

**File:** `src/state.c:319-321`; `tests/core/test_state.c` case `"fifth match 32 after five SAT reads"`
**Issue:** In a Graphics I or II scan the fifth match at SAT index `i` sets `sprite_overflow = 1 + i` after
`sprite_scanned` was incremented to `i + 1`, so a legacy overflow always has `sprite_overflow == sprite_scanned`
(`vdp_legacy.c:106,116`). The loader only requires `overflow <= 32`, `count == 4` and `scanned >= 5`. A CRC-valid
state with `sprite_overflow = 32`, `sprite_scanned = 5` therefore loads, and the next line's first dot latches
fifth index 31 for a scan that read five entries. I confirmed the invariant on 152 reachable legacy overflow states
from the fuzz run (no violation), so tightening cannot refuse a state the core produces. The bounds test header says
the loader "refuses a sprite field set no scan of the saved class can leave", yet the case above is registered as
`refused = 0`, which locks the gap in. No memory-safety or determinism consequence (the index is bounded to 0..31 by
the field range), but it is the one place the new loader check is weaker than the data it validates, and `eeprom_io`
in the same file shows the standard (reachable states only).
**Fix:**
```c
cogmaster__sio_check(
    io, !overflow || (legacy ? count == 4u && overflow == scanned : count == 8u && scanned >= 9u));
```
and change the test: `{"fifth match 32 after five SAT reads", 1, ...}` refused, with the accepted neighbour
`{"fifth match 32 after 32 SAT reads", 0, 2, {{"sprite_overflow", 0u, 32u}, {"sprite_scanned", 0u, 32u}}}`.

### WR-03: Seed. Neither download path bounds a stalled transfer, so the retry on a timeout can hardly fire

**File:** `cmake/WlaDx.cmake:29`; `scripts/fetch-pin.sh:42`
**Issue:** Both scripts now retry transient network errors including code 28 (operation timed out), but neither sets a
time limit. `file(DOWNLOAD ...)` is called with no `TIMEOUT` or `INACTIVITY_TIMEOUT` (CMake's defaults are none), and
the `curl` call has no `--connect-timeout` or `--max-time`. A connection that stalls after it is established hangs the
configure or the hosted job until the runner kills it, and code 28 can only come from curl's own 300 s connect
timeout. The retry list therefore does not cover the failure the hosted jobs most often meet (a stall, not a reset),
and a worst-case refused-and-slow path is several minutes per attempt. The attempt count 3 is also written five times
in `WlaDx.cmake` (loop bound, `attempt EQUAL 3`, three messages) and a fourth time in `fetch-pin.sh`.
**Fix:**
```cmake
file(DOWNLOAD "${url}" "${out}" STATUS status TIMEOUT 120 INACTIVITY_TIMEOUT 30)
```
```sh
curl -fsSL --retry 3 --connect-timeout 20 --max-time 300 -o "$file" "$url" || rc=$?
```
and `set(attempts 3)` once in the CMake function.

## Info

### IN-01: `cogmaster__vdp_background` is test-only now and describes the wrong picture for Graphics I and II

**File:** `src/vdp_render.c:103-111`; `src/vdp_legacy.c:182`
**Issue:** `render_until` no longer calls `cogmaster__vdp_background`; its only caller is
`tests/vdp/test_background.c:277-280`. It stays in the library as production code, and for the legacy classes it
returns `16 + (R7 & 15)`, the colour RAM entry, while the renderer draws the legacy palette entry while the display is
on. A test that used it for a legacy mode would assert the wrong colour. Separately, the `!v->display` arm of the first
test in `cogmaster__legacy_span` (`vdp_legacy.c:182`) is unreachable: its only caller (`vdp_render.c:231-241`) enters
only under `m->vdp.display`.
**Fix:** Make the mode 4 dot a static function of the test seam only, or move it under the test target; drop the
`!v->display` operand (keep `line >= LEGACY_LINES`).

### IN-02: Status flag literals and the "nothing latched" sentinel are repeated across four files

**File:** `src/state.c:310,322`; `src/vdp.c:49-55`; `src/vdp_render.c:185,266`; `src/vdp_legacy.c:14-15`
**Issue:** The coincidence and fifth sprite bits are `0x20u` and `0x40u` in `vdp_render.c` and `state.c`, named
`LEGACY_STATUS_*` in `vdp_legacy.c` and `VDP_STATUS_OVERFLOW` in `vdp.c`; the flag mask is `STATE_VDP_FLAGS` and
`VDP_STATUS_FLAGS`. The sentinel `VDP_FIFTH_NONE` (31) is private to `vdp.c`, while `state.c` hard-codes `31u` twice
(range and the `fifth == 31u` check) and the tests use `31u`. If the sentinel or a bit ever changes, the loader would
silently disagree with the renderer.
**Fix:** Define `COGMASTER__VDP_STATUS_COINCIDENCE`, `COGMASTER__VDP_STATUS_FIFTH`, `COGMASTER__VDP_STATUS_FLAGS` and
`COGMASTER__VDP_FIFTH_NONE` next to `struct cogmaster__vdp` in `internal.h` and use them in all four files.

### IN-03: Seed. The dirty-tree probe in `library.sh` treats any git failure as a dirty tree, and the docs omit the ledger condition

**File:** `scripts/library.sh:33-38`; `docs/testing.md:15-16`
**Issue:** `git diff --quiet HEAD -- . ':(exclude).planning' 2>/dev/null || dirty=+dirty` sets `dirty` for exit 1
(changes) and for exit 128 (unborn HEAD, damaged index), with stderr discarded, so `run` then prints "tracked
changes outside .planning" for a repository that has none (I confirmed exit 128 on an unborn HEAD). In practice the
next `git rev-parse HEAD` fails too, so the cost is a misleading message. `docs/testing.md` says the run path
"refuses ... a tree with tracked changes" without the condition the script and the tests apply: only against the
tracked ledger, so `COGMASTER_LEDGER=...` stand-ins run on a dirty tree.
**Fix:**
```sh
rc=0
git -C "$root" diff --quiet HEAD -- . ':(exclude).planning' 2>/dev/null || rc=$?
[ "$rc" -eq 0 ] || { [ "$rc" -eq 1 ] && dirty=+dirty || { echo 'library: git could not compare the tree' >&2; exit 2; }; }
```
and add "when it writes the tracked ledger" to the `docs/testing.md` sentence.

### IN-04: Seed. Timeout comments still say "provisional" and "no hosted run yet"

**File:** `tests/policy/tooling_cases.cmake:18-19,49-50,105`; `tests/vdp/tests.cmake:65`
**Issue:** The commit that sized the legacy timeouts from the hosted run (`runner.hashes_file`, the late image, the
sprite tests) left these comments claiming the new tests have not run on a hosted lane. My ASan run of all 39 `vdp`
tests took 4.4 s of process time and the three `policy.library_dirty.*` cases under a second each, so the 10, 30 and
60 s values are not in doubt; the stale words are. Rule 5 asks budgets to be justified from a measurement.
**Fix:** After the next hosted run, replace "provisional ... no hosted run yet" with the measured times and the run id,
as `tests/fixtures/tests.cmake` now does.

---

_Reviewed: 2026-11-14T12:00:00Z_
_Reviewer: Claude (gsd-code-reviewer)_
_Depth: standard_
