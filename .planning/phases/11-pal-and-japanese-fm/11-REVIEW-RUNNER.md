---
phase: 11-pal-and-japanese-fm
reviewed: 2026-11-11T23:30:00Z
depth: standard
files_reviewed: 26
files_reviewed_list:
  - runner/args.c
  - runner/args.h
  - runner/main.c
  - runner/expect.c
  - runner/expect.h
  - runner/audio.c
  - runner/audio.h
  - runner/input.c
  - runner/input.h
  - runner/exit_status.h
  - tests/roms/src/vdp_vtable.asm
  - tests/roms/src/pause_nmi.asm
  - tests/roms/src/header.inc
  - tests/fixtures/tests.cmake
  - tests/diag/expected.csv
  - tests/built/expected.txt
  - tests/roms/manifest.txt
  - tests/cmake/hashes.cmake
  - tests/audio/transitions.txt
  - tests/audio/pcm.txt
  - tests/runner/tests.cmake
  - tests/runner/expect.cmake
  - tests/compat/tests.cmake
  - tests/inventory/ci.txt
  - tests/inventory/wasm32.txt
  - tests/inventory/hygiene.txt
findings:
  critical: 0
  warning: 2
  info: 7
  total: 9
  necessity: 0
status: issues_found
---

# Phase 11: Code Review Report (runner and fixture half)

**Reviewed:** 2026-11-11
**Depth:** standard, with cross-file tracing into `src/synth.c`, `src/io.c`, `src/vdp.c`, `src/instance.c`
**Branch:** `phase/11-pal-and-japanese-fm` at `ab08de1` (base `origin/main`, v0.10.0)
**Status:** issues_found (0 critical, 2 warnings, 7 info)

## Summary

The runner half is sound. I found no determinism, overflow, aliasing or grammar defect in `--fm`, `fm=`, `audio@F=H` or the
exact sample count. The two warnings are a refusal built on a false premise and one fixture image whose digest nothing checks.

What I checked and could not break:

- **Exact samples.** `want = f*N/D - (f-1)*N/D` is evaluated in `uint64_t` on both operands (`rate_num`, `rate_den`
  are `uint64_t`, `frame` is `uint64_t`). The worst case, `--frames 4294967295` on PAL, is 4.29e9 x 6.85e8 = 2.9e18, under
  1.8e19. `N/D` equals the core's own `inc/mod` (`src/internal.h:292-295`: 9600 / 2,128,137 per tick = 685,094,400 /
  709,379 per cycle, and the core carries its phase across frames, `src/synth.c:251-254`), so the formula holds for every F.
  I ran `--region pal --frames 120000 --hash-audio` natively: exit 0, `samples 115891967`.
- **SHA copy.** `cogmaster__sha256` is plain data (`h[8]`, `block[64]`, `used`, `total`; no pointers); the struct copy
  carries 4 bytes of padding that `final` never reads. Records are serialised byte by byte (`audio.c:83-88`), so be32,
  wasm32 and MSVC hash the same bytes. Finalising the copy at F=60 and again at F=300 is exercised by
  `runner.expect.audio`.
- **Drain.** `--hash-audio` already drained the trace once per frame before the phase; `want_audio` only adds the same
  single drain for an audio case. No two consumers, same 32,768-record per-frame bound, a lost record is still exit 1.
  Runs with no audio case and no `--hash-audio` still leave `trace_records` 0.
- **`--fm` grammar.** Empty word, missing word, a repeat, `--fm --fm`, `ON`, `1`, `--probe --fm` and `--fm on --machine gg`
  (77) all behave as the tests and docs say. The `COGMASTER_FM_AUTO` value is 0, so the zero-initialised default is `auto`.
- **Pause band.** I re-derived every cycle count in `pause_nmi.asm` (body 78, handler 76, accept 13, longest wait 59,
  resume offsets 42/54/61, longest instruction 16) and `S_LO = 132`, `S_HI = 36`. They are the documented T-states, the
  CMake constants match the listing, and the five CSV bands (`C5-C6`, `67-68`, `15A-15B`, `FC-FD`, `CD-CF`) equal the
  formula. Observed values on the current build are `C5`, `68`, `15A`, `FD`, `CE`: edges and a centre, which is what a band
  derived from worst cases (not fitted to a run) looks like.
- **V table.** I re-summed all five tables by hand from the section 4.4 intervals (PAL 192 = 44,838; 224 = 44,982; 240 =
  43,390; NTSC 192 = 33,933; 224 = 34,029). They match the CSV and the closed-form test. The 228-cycle loop sums to 228
  (45 + 125 + 48 + 10). The summary code (sum, square-and-add, min/max, cyclic descent) is correct including the wrap.
- **No planning IDs** in any added code, test name or comment (the one grep hit is the hex `$D2`).

## Warnings

### WR-01: The reset-script rejection rests on a false premise and refuses a valid script

**File:** `runner/main.c:412-417`, `runner/input.c:114-121`, `runner/input.h:11-13`, `docs/runner.md:390-391`, `tests/runner/tests.cmake` (`expect_audio`, reset block)
**Issue:** The runner now exits 2 when an `--input` script holds `reset=1` and the expect file holds an `audio@F` case, with
the reason "a reset restarts time" (`docs/runner.md:391`, `input.h:12`, and the decision text "Resets already fail the trace
merge (exit 1)"). Neither statement is true.

- `reset` in the script is the SMS1 Reset button bit read through port `$DD` bit 4 (`src/io.c:352-356`, SMS-SYSTEM section
  7: "not wired to the Z80"). The runner never calls `cogmaster_reset` (`grep cogmaster_reset runner/` is empty), so no
  tick, frame counter or trace log restarts.
- Reproduced on the current build with a three-record script whose second record has `reset=1`:
  `cogmaster-run psg_sweep.sms --machine sms1 --region ntsc-u --frames 3 --input S --hash-audio` exits 0 and prints an
  `audio-transitions` line; the same script with `--expect` holding `audio@3=<64 hex>` exits 2 with "an audio expectation
  cannot follow a reset". `fm_sweep.sms` and the test card behave the same. The per-source monotonic-tick check that the
  rationale relies on never fires.

Failure scenario: a user locks the sound of an SMS1 run in which the Reset button is pressed (the only machine on which the
bit does anything) with `audio@F=H`. They get a usage error for a script the same runner accepts under `--hash-audio`, and
the hash they would have pinned equals that `--hash-audio` line. The documentation tells them the cause is a restart that
does not exist. The change also threads a new out-parameter through a shared validator (`cogmaster__input_validate`) and
adds a message that prints twice (`status` is 2, so `main.c:425-426` adds "input file error (status 2)").
**Fix:** Delete the refusal and the plumbing: drop `reset_seen` from `cogmaster__input_validate` (`input.c`, `input.h`), the
`if (... reset_seen && ... cogmaster__expect_has_audio ...)` block in `run()`, the sentence at `docs/runner.md:390-391`, and the
reset block in the `expect_audio` case. If a soft restart of time is ever introduced (the runner calling
`cogmaster_reset`), add the guard then, with the same control. Keep one regression test that a `reset=1` script with
`audio@F` equals the `--hash-audio` line of the same run.

### WR-02: `fm_sweep_jp` is the only new image whose digest no test checks

**File:** `tests/built/expected.txt:23`, `tests/fixtures/tests.cmake:574-589`
**Issue:** `vdp_vtable_*` and `pause_nmi_*` are digest-checked by the preamble of the script when their mailbox tests run
(`-DNAME=... -DEXPECTED=...`, `tests.cmake:166-174`). Nothing passes `-DNAME=fm_sweep_jp`: the only users are
`tests/library/tests.cmake:62-66`, which copy and SHA-1 the image, and `scripts/library.cmake:32`, which reads the table.
The line `fm_sweep_jp 63aac9e9...` matches the image on this machine today (`shasum -a 256` of
`build/ci/fixtures/fm_sweep_jp.sms`), but a WLA-DX bump or an edit to `header.inc` (which this phase already changed) would
change the Japan image, leave the "admitted bytes" line stale, and no test would fail.
**Fix:** Add one digest check for it, for example a `fixtures.fm_sweep_jp.digest` test running the existing preamble with
`-DNAME=fm_sweep_jp -DIMAGE=${COGMASTER_FIXTURES_DIR}/fm_sweep_jp.sms -DEXPECTED=...`, and register it in the three inventories.

## Info

### IN-01: `sms1` expectation rows cite section 4.4, which has no first-model row

**File:** `tests/diag/expected.csv:115-120`
**Issue:** The six `ntsc-sms1` / `pal-sms1` rows say "SMS-VDP section 4.4 literal intervals recomputed (no 224 on the first
model: the 192 values)". Section 4.4 lists no 315-5124 rows; section 1.1 says M1 with M4 on that chip gives "the invalid text
mode", and the 192 fallback is the core's selected model (`src/vdp.c:123-124`, SMS-VDP section 2.5). The closed-form test
checks these rows against the 192 intervals, so they are consistent with each other and with the core, but they encode a core
choice and not a hardware fact.
**Fix:** Cite section 2.5 and say "selected 192-line fallback, not measured" in the source column, matching how the PAL
Pause rows say "derived".

### IN-02: `--fm on --machine gg` is refused with a message that does not name FM

**File:** `runner/main.c:268-271`
**Issue:** `PROFILE_REJECTED` prints "no such console was made for this machine and region". For `--fm on --machine gg` the
machine and region are fine; the unit is what the Game Gear lacks. `docs/runner.md:71` documents the 77 correctly, the
stderr text does not help the person who typed `--fm on`.
**Fix:** Keep exit 77 and add "or FM sound unit" to the text (or print a second fixed line when `opt->fm == COGMASTER_FM_ON`).

### IN-03: `docs/runner.md` says a failing sample count "prints nothing to standard output"

**File:** `docs/runner.md:152-157`
**Issue:** Under `--expect`, `--png` or `--trace`, predicate lines for frames before the failing one are already on stdout when
the exit-1 count check fires (`audio_frame` runs after each frame, `expect_frame` prints per frame). The statement holds only
for `--hash-audio` alone, where the lines are printed at the end. The new test happens to fail at frame 1, so it cannot see this.
**Fix:** Say "prints no `audio-` line".

### IN-04: Unreachable `audio_hex == NULL` branch in the audio predicate

**File:** `runner/expect.c:184-188`
**Issue:** `main.c:521-526` passes a non-NULL `audio_hex` exactly when `cogmaster__expect_audio_at()` is true, and
`expect_frame` takes the kind-4 branch only for the same frames, so `audio_hex != NULL` is always true and `"none"` never prints.
Two tests of the same property at two sites also means `expect_audio_at` is called from `main` and the case match is repeated
in `expect_frame`.
**Fix:** Drop the NULL test and the `"none"` fallback, or assert it. Not harmful.

### IN-05: `--probe` rejects `--fm` but still accepts `--region`, `--machine`, `--port1/2` and `--video-mode`

**File:** `runner/args.c:217-221`
**Issue:** The new `|| o->fm_given` follows the rule "a run option is a usage error with `--probe`", but the older run options
that also change nothing under `--probe` (`--machine`, `--region`, `--video-mode`, `--port1`, `--port2`) are still silently
accepted. `docs/runner.md:71` states the rule only for `--fm`.
**Fix:** Either leave as is and say so, or extend the list in one pass with a control each.

### IN-06: `audio.h` comment on the new fields is split mid-sentence

**File:** `runner/audio.h:43-44`
**Issue:** `rate_num` carries "3,504,512 / 4,375, or on PAL" and `rate_den` carries "685,094,400 / 709,379"; each line alone is misleading.
**Fix:** One comment above both fields: "samples per frame as a fraction: 3,504,512 / 4,375 on NTSC, 685,094,400 / 709,379 on PAL".

### IN-07: New fixture tests mix `TIMEOUT 10` and `TIMEOUT 20` with one shared comment

**File:** `tests/fixtures/tests.cmake:1012-1060, 1862-1889`
**Issue:** `fixtures.vdp_vtable.control.wrong_profile` has `TIMEOUT 10` and no `Timeout:` comment; its siblings in the same block
have `TIMEOUT 20` under a single comment that names `closed_form`'s lane only. The Pause control uses 20. The rule recorded for
the phase is one measured `2 x S` per test comment.
**Fix:** Give each new test its own `# Timeout:` line from the hosted per-test table, as the runner tests got.

## Fixture assessment (not findings)

- **Rotation invariance is deliberate and has one blind spot.** `vdp_vtable` cannot see a PAL V table that is correct but
  phase-shifted against the frame (the multiset, descents, max and min are unchanged). The Pause V record (`$FF` at the NMI)
  and the existing `vdp_lines` boundary rows are the only phase evidence on PAL. A shift that moves the V step tick (604) by
  less than the handler's ~350-tick read delay, or moves frame placement of the NMI by whole frames, would pass both new
  fixtures. That is a property of the design, not a defect.
- **Plausible PAL core bugs and what catches them.** NMI line `lines-2` computed with the NTSC count: caught (band, about 150
  bodies off). NMI one line late: caught (about 3 bodies out of a 1-3 value band, V reads `$00`). A wrong PAL 224 or 240
  jump (258, 266 shifted by one): caught by sum and sum of squares. A 312-line PAL frame: caught (count, extra-read drift).
  A level-triggered NMI: caught (NMI count 5, not 1, at frame 12). An NMI on release: caught (count 2 at frame 12). Missed by
  design: sub-120-cycle NMI tick error, V step tick error, and which frame the NMI lands in.
- **Bands are derived, not tuned.** `pz_*` are typed constants in CMake that match the header's cycle sums; nothing links them to
  the assembly mechanically, so an edit to the loop body is caught only by the mailbox test failing, which is acceptable.
- **Hash rows.** `mailbox` rows judge the records against `expected.csv` in every lane before hashing, so lane equality cannot
  agree on a wrong answer. The device-row regex change keeps the BIOS row (empty third field) and the new `|pal` rows parsing.
- **Inventories.** The three inventories list exactly the registered names and labels I traced (`compat.migration`,
  `runner.expect.audio` as `fm,portable,runner`, the 25-odd new fixture names); the native `ctest -R` over the vtable, Pause,
  cadence, `--fm` and audio-expect tests passed (28 of 28).

---

_Reviewed: 2026-11-11_
_Reviewer: Claude (gsd-code-reviewer)_
_Depth: standard_
