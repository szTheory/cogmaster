# Phase 16: Graphics I and II draw - Research

**Researched:** 2026-11-13
**Domain:** TMS9918 legacy video modes (Graphics I, Graphics II, legacy sprites) inside the deterministic C VDP core of a Master System and Game Gear emulator, plus three tooling riders (doc vocabulary, dirty-tree refusal, assembler fetch retry)
**Confidence:** MEDIUM overall. HIGH for repository mechanics and for what each source states; MEDIUM to LOW for hardware behaviour, which CONTEXT already labels "selected model" (no permissive TMS9918 test ROM exists and `mame` is not installed, so a fixture proves fidelity to the notes, not to silicon).

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions

### Mode selection and the shared "would this fault" test
- **D-01:** One classifier in `internal.h` returns MODE4, LEGACY or BLOCKED from the mode nibble and the profile. LEGACY is nibble 0 or 2 on every profile except native Game Gear (`cogmaster__native_gg`). Everything else that is not mode 4 today stays BLOCKED, including SMS1 nibbles 11, 14 and 15. `vdp_mode()` moves to `internal.h` beside `native_gg`.
- **D-02:** `cogmaster__vdp_render_blocked` becomes `display && class == BLOCKED`, the same expression `render_until` uses, so the Light Phaser check cannot drift from the renderer. `render_until` computes the class once per call, not per dot. Today `vdp_supported` runs three times per dot, so hoisting it makes mode 4 cheaper, not dearer.
- **D-03:** The fault is still recorded at the start of a call, first fault wins, and it is saved state. Transient legacy dots during mode 4 bring-up (R0 and R1 written separately with display on) now draw and record no fault; text, multicolour and mixes still fault. This can only improve ledger rows, never lose a pass.
- **D-04:** Mode switches are dot-exact to the register-write dot: `cogmaster__vdp_write` renders up to the write dot before the register changes (`vdp.c`), so registers are constant inside any one `render_until` call. The mode-switch fixture asserts the lines wholly before and after the write with the oracle and pins the switch line by hash only.

### Module shape and dispatch
- **D-05:** New module `src/vdp_legacy.c` (about 170 lines) exporting a per-line **span** renderer (`legacy_span(m, line, x0, x1)`), a legacy sprite scan and the palette lookup. The mode 4 per-dot loop stays as it is (its body only loses the in-loop `supported` tests). An exported `cogmaster__vdp_background` wrapper keeps its current checks because `tests/vdp/test_background.c` calls it directly.
- **D-06:** Legacy modes are 192 lines, 32 by 24 cells, with no scroll (R8, R9 and the R0 bit 6/7 locks are ignored) and no left-column mask (R0 bit 5 is ignored). Rows at or past 192 return the backdrop, so a mid-frame switch from a 224/240-line mode 4 cannot overrun the name table or the Graphics II thirds. R1 bit 6 (blank) gives the backdrop only. These are selected models; no fixture sets R0 bit 5.
- **D-07:** Legacy address helpers are their own, never the mode 4 helpers: SAT base `(R5 & 0x7F) << 7`, sprite pattern base `(R6 & 7) << 11`, every VRAM index masked `& 0x3FFF`, addresses in `uint32_t`, signed only for sprite Y and X deltas. The SMS1 `sat_address`, `sprite_address` and `pattern_address` masks are silently wrong for legacy and must not be reused.

### Backgrounds, palette and backdrop
- **D-08:** Graphics I (TI manual 2.4.1; MAME and ares agree): name `(R2&15)<<10 | row<<5 | col`; pattern `(R4&7)<<11 | name<<3 | (y&7)`; colour `((R3<<6)&0x3FFF) | name>>3`; set bit gives the colour byte's high nibble, clear bit the low nibble; nibble 0 shows the R7 low-nibble backdrop.
- **D-09:** Graphics II unmasked (TI Table 3-2): with `idx = (y>>6)<<8 | name`, pattern `(R4&4)<<11 | idx<<3 | (y&7)`, colour `(R3&0x80)<<6 | idx<<3 | (y&7)`. **Masks, a selected model:** SMS1 (`vdp_masks`) ANDs `idx` with `((R4&3)<<8)|0xFF` for the pattern and `((R3&0x7F)<<3)|7` for the colour (MAME 315_5124 model, about 50 percent); Master System 2 and Game Gear in Master System mode do not mask (Charles MacDonald's `msvdp` note, about 60 percent). The gate is the existing `vdp_masks` flag, consistent with mode 4 where only the SMS1 has the AND gates. MAME's TMS9918A cross-coupling of R3 into the pattern index is not adopted (unverified on Sega silicon). All sources agree on the all-ones configurations (R3 $FF/R4 $03 and R3 $7F/R4 $07), which real software uses, so the choice is expected to be moot in practice; the docs say so and name the three-way source disagreement (msvdp, MAME `315_5124.cpp`, ares).
- **D-10:** Palette: Master System 1 and 2 use a fixed 16-entry table, the RGB444 words `000 000 0A0 0F0 500 F00 005 FF0 00A 00F 055 0FF 050 F0F 555 FFF` (6-bit values `00 00 08 0C 10 30 01 3C 02 03 05 0F 04 33 15 3F` through the existing `c*5` lattice), cited to the SMS Power palette page, not to MAME's table. Game Gear in Master System mode uses CRAM entry 16+n (msvdp, SMS Power, MAME; ares disagrees, about 75 percent), detected with `machine == COGMASTER_MACHINE_GG && !native_gg`. MAME's GG border fill without the +0x10 offset is a quirk not to copy.
- **D-11:** Backdrop is gated on `display`: `cogmaster__vdp_backdrop`, the early-return backdrop in the pixel path and `completed.backdrop` use the TMS colour only when `display && class == LEGACY`. Power-on registers are all zero (Graphics I, display off), so ungated TMS backdrop would move boot-frame pins on every mode 4 game. The deviation (a legacy game's display-off frames show the CRAM backdrop on SMS) is bounded, stated in `docs/hardware/video.md`, and checked by the base-versus-head compare. `completed.backdrop` stays within 0..0xFFF so its state check is unchanged.
- **D-12:** In the docs write modes as the mode nibble or as R0/R1 bits. MODE-02 says "M3=1" in TI naming (R0 bit 1) while `vdp_mode()` uses SMS Power naming where that bit is M2.

### Legacy sprites (model choices, each pinned by a fixture and named in the notes)
- **D-13:** Attribute table 32 entries by 4 bytes (Y, X, name, EC and colour). Size and magnification from R1 bits 1 and 0; 16x16 uses name `& 0xFC` (TL +0, BL +1, TR +2, BR +3). Colour 0 is transparent. Lower index wins; sprites are always over the background; there is no background collision.
- **D-14:** Y threshold: `Y >= $E0` is negative (ares and the TI manual, about 70 percent), expressed as `top = Y - 255`, otherwise `Y + 1`. MAME's TMS file (`> $E0`) and MAME's SMS legacy path (`>= $F0`) differ only for 32-line sprites with Y in $E0..$EF; one fixture row pins it (16x16 magnified, Y=$E8 shows pattern rows 11..15 on lines 0..8). `$D0` ends the list unconditionally in legacy modes (all sources agree; mode 4 keeps its 192-line gate).
- **D-15:** Early clock is the **attribute bit 7** (X minus 32). R0 bit 3 is ignored in legacy modes (MAME and ares; `msvdp` says it shifts by 8, an open item the notes record). The old note that R0 bit 3 shifts TMS sprites is corrected.
- **D-16:** Four sprites per line. The fifth sets status bit 6; the fifth sprite's index latches in bits 4..0 on the first overflow and stays until a status read (first wins, 70 percent); bits 4..0 read `$1F` when bit 6 is clear (consistent with the mode 4 choice). Five-sprite detection counts Y matches only, including transparent or off-screen sprites. No F-flag gate, as in mode 4; fixtures clear F first so both models agree.
- **D-17:** Coincidence (bit 5) follows the datasheet: pattern bits decide, so colour-0 sprites collide (TI 2.3.2, Young; about 55 percent against MAME-SMS's opaque-only rule). Sprites after the terminator and sprites dropped as fifth do not collide. Collision counts only x in 0..255. The horizontal zoom "slot i+4" quirk of mode 4 on SMS1 does **not** apply to legacy: plain doubling. The ares asymmetric stack rule is a bug not to copy. This choice is a likely revisit when the SG-1000 arrives (v3.1); the notes say so.

### State format 17, behaviour revision 19
- **D-18:** Share the `sprite[8]` array and counters and widen each slot to `{x, tile, row, attr}` (raw SAT colour byte, bit 7 is EC). Add saved `sprite_legacy` (the scan-time class, not the live mode, so a save after a mid-line switch still loads), `sprite_size` (legacy 8x8 or 16x16 latched at scan) and `fifth_index` (31 when unlatched). Append the new fields after `vram` so cram and vram offsets do not move; update the pinned struct size. A separate 4-slot struct was weighed and rejected (duplicate counters and validators); the planner may revisit only if the size pin makes it cleaner.
- **D-19:** Loader bounds. Legacy: `count <= 4`, `row <= (size ? 15 : 7)`, `attr & 0x70 == 0`, `scanned <= 32`, `overflow` implies `count == 4 && scanned >= 5`. Mode 4: `count <= 8`, `row <= 7`, `attr == 0`, existing checks. Both: `fifth_index <= 31`, flags in 0..1, and `fifth_index == 31` when status bit 6 is clear. A status read now returns `(status & 0xE0) | (bit6 ? fifth_index : 0x1F)` and resets `fifth_index` to 31.
- **D-20:** Bump the state format to 17 and the behaviour revision to 19 (header fields refuse every older state; revision 18 did this with format 16). Edit the two hard-coded 16s in `tests/api/v14.c`. No test hashes state bytes; "no shipped state pin moves" means the frame and audio hashes plus the ratchet and the state sweeps.

### Fixtures, oracles and test order
- **D-21:** Two assembly sources, one `-D` variant per image, six images: `legacy_modes` (g1, g2 all-ones, g2 masked, g2_late entering Graphics II after frame 600, g2_switch) and `legacy_sprites` (a scenario list over successive frames, status read into the mailbox after each). Profiles: ms1, ms2 and gg-in-SMS-mode on the NTSC-U profile; native Game Gear is not run. Do not touch `header.inc`, `mailbox.inc` or `boot_to_play.asm` (any byte change moves every fixture hash). One CMake function builds all families, not five pastes. New fixtures must not join `diagnose` (the line shape changes once, in Phase 17).
- **D-22:** The exhaustive cases run in process, not as ROMs: the eleven (R3, R4) pairs ($FF/$03, $7F/$07, $FF/$00, $FF/$01, $FF/$02, $80/$03, $9F/$03, $BF/$03, $DF/$03, $FE/$03 and $E0/$03 as the model-separating row) and the sprite grid, as `vdp.legacy.*` tests next to `tests/vdp/test_background.c`.
- **D-23:** Oracles come first, in their own commit before any renderer: `tests/fixtures/legacy.c` (modelled on `tests/fixtures/lines.c`: closed-form `pixel(x,y)`, prints the SHA-256 of the expected pixels, the runner hash must equal it, plus a wrong-hash negative control) and oracle functions in `tests/vdp/test_legacy.c`. The oracle works in the tile-index domain with a generated scene (for example pattern byte `(tile*7+row)&255`) while the renderer works in the address domain, plus 6 to 10 hand-computed literal cases and a literal table for the unsupported-mode tests. The sprite oracle is a per-line list walk, not the slot structure. The palette and masks cite a different document than the renderer comment.
- **D-24:** Pins: a one-time **subset** compare, not the byte compare, because new fixtures add lines by design: `comm -23` of the sorted base and head hashes files is empty, exit status 0, recorded as a command and an exit status (rule 6, not a new gate). Regenerate the base from an `origin/main` worktree (the old `build/p14-base-hashes.txt` is stale). Check that the g2_late and g2_switch rows are new lines, not renamed ones. New frame rows add about 2,300 frames to `runner.hashes_file`; re-derive its 107 s timeout from the first CI run, never guess.
- **D-25:** Tests that must change with the renderer commit: `vdp.background.unsupported` (extend to a 16-mode by 4-profile literal table), `fault_run` in `tests/ports/test_gun.c` (it uses R0=0 as its "rejected" mode, which is now Graphics I; switch it to the text nibble) and the Light Phaser agreement against the same literal table. Add `vdp.unsupported.{text,multicolour,mixed,invalid_text,native_gg}`, `core.state.legacy_roundtrip` (save, load, save equal) and `core.state.legacy_sprite_bounds` (row 16, count 5 and a colour byte above 15 each refused), extending the record sweep, and a legacy-mode seed in `tests/fuzz/seeds.c`. Test names carry no planning IDs.
- **D-26:** Order inside the phase: (1) riders, (2) oracles and fixtures, (3) once-per-call dispatch skeleton with mode 4 untouched and legacy still faulting, then run the local subset compare and `ctest -R '^(vdp|runner\.hashes_file|core)'`, (4) **open the draft pull request here** (the first commit that can move mode 4 instructions; the perf job runs on drafts and needs no `ci.yml` edit), (5) Graphics I and II, then sprites, then state 17 and revision 19, then docs. Push at the draft, after renderer and sprites, and after state and docs, not per commit. Cheap checks first: `cmake --build --preset ci --target cogmaster-fixtures`, `ctest --preset ci -R '^(vdp\.legacy|fixtures\.legacy)'`, and the agent views `cogmaster-run IMG --frames 30 --png 30:FILE` and `--diagnose IMG` (expects no fault). The perf gate sees only `boot_to_play`, so a slow legacy path is invisible; the only real instruction risk is the mode 4 dispatch edit.

### Riders (plan 1)
- **D-27 (TUNE-12):** Correct, tersely, `docs/audio.md:76-77` (a write latches nothing and is no fault; every channel reaches both outputs), `docs/embedding.md:323-325` (faults are a cartridge or RAM address that port `$3E` took off the bus, a mapper value out of range, an undefined opcode, or an unsupported video mode), `docs/embedding.md:355` (CTRL no longer recorded, the constant stays), `docs/embedding.md:480` (six kinds, seven devices), `docs/runner.md:232-233` (`ctrl`, `rom_write` no longer recorded) and `docs/runner.md:430,437,438` (`where=port`, `wclass`, `pclass` always `none` since revision 18). Annotate, never delete the diagnose keys: the diagnose line still prints them and Phase 17 owns the prune. Headroom to the 38,912-byte cap: `runner.md` about 1 KB, `embedding.md` about 2 KB; the MODE plans own the revision 19 sentence and the `embedding.md:346` row 6 rewrite (about 400 bytes).
- **D-28 (TUNE-13):** Refuse early. In `scripts/library.sh`, after `ledger=`, compute `dirty` from `git diff --quiet HEAD -- . ':(exclude).planning'` and, for `run` against the tracked ledger only, exit 2 with a message naming tracked changes outside `.planning`. Gate on the tracked ledger (as the `counts=` line does) because `library.synthetic` and `library.diagnose*` run `library.sh` in the real tree with stand-in ledgers; an unconditional refusal would fail `ctest` on every dirty dev tree. The diagnose path reuses the same variable. Test in a throwaway repo (pattern: `tests/policy/firmware_window_cases.cmake` `fwin_repo`, identity from the GitHub noreply form, `core.autocrlf=false`, at least one commit): modified tracked file refuses with the message; modified `.planning/x`, an untracked file and a clean tree do not (the three controls are what make the test mean something). One sentence in `docs/testing.md`.
- **D-29 (TUNE-14):** The assembler is fetched by `cmake/WlaDx.cmake` (`file(DOWNLOAD ... EXPECTED_HASH ... STATUS)`), not by `scripts/fetch-pin.sh`; with `EXPECTED_HASH` CMake raises FATAL_ERROR itself, so the `STATUS` branch is dead and a retry around it cannot work. Drop `EXPECTED_HASH`, loop up to 3 attempts of `file(DOWNLOAD ... STATUS)`, retry only curl codes 6, 7, 18, 28, 35, 52, 55, 56 (HTTP error 22 is permanent), sleep with `${CMAKE_COMMAND} -E sleep ${RETRY_DELAY}` (default 2), then `file(SHA256)` once after the loop and fail with a message containing "hash mismatch" (keeps `fixtures.wla_pin.self_test` green). Tests with `-DRETRY_DELAY=0`: `http://127.0.0.1:1/x` (code 7, no DNS) exits 1 and prints "attempt 3 of 3"; the existing tampered `file://` tarball prints only attempt 1 and still fails on the hash. Give `fetch-pin.sh` a POSIX retry loop on the same codes instead of `--retry-all-errors` (curl 7.71+, and it retries 404), tested with a fake `curl` on `PATH` that exits 6 twice then succeeds. Do not edit `ci.yml` or `cmake/packaging.cmake` (either sets `release_inputs=true` and triggers a release rehearsal; `ci.yml` is reserved for Phase 17's last push).
- **D-30:** Budget: the repository-check cap is 938 lines against 887 used (about 51 lines of headroom, per `tests/cmake/budgets.cmake`). Only `tests/cmake`, `scripts`, `cmake`, `.githooks` and `tests/compat` count; `library.sh` is classified `operation`, and `tests/fixtures`, `tests/vdp`, `tests/policy` and `tests/roms/src` are not counted. The riders land first and must stay under about 40 counted lines; the planner measures before writing and does not raise the cap.

### Hardware notes, provenance and ship
- **D-31 (MODE-08):** Rewrite the legacy paragraph at `docs/hardware/video.md:73-77` (it is outside the `docs/*.md` byte-cap glob; keep it terse and cite as lines 60-100 do) to list what renders, what faults and the selected models above. Rewrite `.planning/preparation/SMS-VDP.md` section 7 and the open-question line 232. Correct the "MAME contradicts" claim in the author's words, from a read of the BSD-3 file at the pin: `315_5124.cpp` applies the Graphics II masks on every revision (`color_mask = ((R3&0x7F)<<3)|7`, `pattern_mask = ((R4&3)<<8)|0xFF`), whereas the prep note attributed "SMS2 and GG do not mask" to the TMS file, which does not say it; and the sprite Y threshold differs (`tms9928a.cpp` uses `> $E0`). Also correct the TI-manual note: it has a usable text layer (`pdftotext` works), so it is not only a scan.
- **D-32:** `PROVENANCE.md` rows (section 2 at lines 46-47, section 10, section 4 "Read, never copied"): add MAME `src/devices/video/tms9928a.cpp/.h` (BSD-3, headers read at pin `1d7a454ed79e`, which `gh api` answers 200) and extend the ares row to `ares/component/video/tms9918/**` (ISC, pin `9408cb43d494`, 200; outside the currently listed scopes, so the scope grows). Re-pin VDP.21 in SMS-VDP.md from "master, accessed 2026-10-18" to `1d7a454ed79e` and add a row for ares tms9918, re-verified 2026-11-13. No seed row if the renderer is own code. GPL emulator source is never opened.
- **D-33:** Ship: branch `phase/16-graphics-i-and-ii-draw`, cut from the milestone-branch tip (the milestone docs commits are not on `origin/main`; Phase 13 did the same). Record this rule 10 deviation in STATE.md; the owner may override by asking for a `docs:` pull request of the milestone branch first. PR title `feat(vdp): draw Graphics I and II on the Master System video profiles` (release-please pre-1.0 gives 0.14.2 to 0.15.0), then `gh pr merge --auto --squash` from a shell and a CI run confirmed on the head commit.

### Claude's Discretion
- Exact file split and function names inside `vdp_legacy.c`, as long as the interface keeps one class decision per call and one span per line.
- Whether the new fixtures share one linked-image function or two, and the exact mask pairs beyond the eleven listed.
- Wording of the doc corrections, within the byte headroom above.
- Whether the sprite slot widening needs the separate-struct fallback in D-18.

### Deferred Ideas (OUT OF SCOPE)
- Text and multicolour modes (MODE-09), only if the Phase 17 diagnose shows 5 or more rows landing on them.
- The SG-1000 machine (v3.1): revisit the colour-0 collision, fifth-sprite number and R0 bit 3 choices against SG-1000 software then.
- Pruning the dead diagnose vocabulary and the `ci.yml` edit (Phase 17); `runner.diagnose` trimming on emulated lanes; `quality.json` floor and `ir_per_frame` ceiling raises (CI-speed seeds).
- A MAME screenshot comparison (no firmware bytes, `mame` not installed): optional by-eye only, never a gate.
- A TI datasheet-derived oracle for the display-off backdrop (blank colour) if a title is found that needs it.
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| TUNE-12 | Docs state the current fault vocabulary | Line-by-line check of the six doc locations done (see Code Examples, "Doc locations"); `COGMASTER__DEVICE_CTRL` and `ROM_WRITE` have no raiser in `src/` (grep of every `cogmaster__bus_fault(` caller); byte headroom measured (`embedding.md` 36,791, `runner.md` 37,844, cap 38,912). |
| TUNE-13 | Library run refuses to stamp `core=` from a dirty tree | `scripts/library.sh` already computes `+dirty` on the diagnose path (lines 49-51); the `run` path stamps the full 40-hex head and the ratchet demands 40 hex, so refusal (not a `+dirty` stamp) is the only consistent rule; the file is classified `operation`, so it adds nothing to the repository-check budget; test goes in the hygiene lane beside the throwaway-repo precedent. |
| TUNE-14 | Transient fetch failure no longer fails a hosted job | Probed this session: `file(DOWNLOAD)` returns `7;"Couldn't connect to server"` for a refused port and `6;"Couldn't resolve host name"` for DNS; `EXPECTED_HASH` mismatch is a FATAL_ERROR even with `STATUS`. `curl --retry` does not retry connect, DNS or TLS errors (local curl manual). |
| MODE-01 | Graphics I draws tables, backdrop, fixed palette | TI 2.4.1 and Table 3-2 read; MAME `315_5124.cpp` `draw_scanline_mode0` and ares `graphics1` agree with D-08; palette values confirmed in three sources. |
| MODE-02 | Graphics II draws three-section tables with masks | Mask behaviour in five sources compared (table below); SMS1 versus SMS2/GG gating follows `vdp_masks`. |
| MODE-03 | Legacy sprite rules | TI 2.3.2, 2.3.3, 2.4.5, Young section 4, MAME TMS and SMS paths, ares read; disagreements tabulated; hidden gap on carrying the fifth-sprite index across a status read (Pitfall 3). |
| MODE-04 | Other modes keep the fault; Light Phaser agrees | Classifier design; four existing tests plus two runner seeds that depend on R0=0 or R1=$40 faulting are listed (Pitfall 1), more than D-25 names. |
| MODE-05 | No pin moves; revision 19, state 17; round trip; bounds | The branch tip is byte-identical to `origin/main` in every code path, so the base hashes can be taken before the first edit with no worktree; hidden state-field pins in `tests/bios/test_bios.c` (Pitfall 2); struct size becomes 16520 in both D-18 layouts. |
| MODE-06 | Mode switches draw correctly | `render_until` registers are constant inside a call (verified in `vdp.c:393-405`); mid-line class change versus the sprite list scanned at tick 600 needs a rule (Pitfall 4). |
| MODE-07 | Once-per-line dispatch; Ir gate green; draft PR | Perf job runs on drafts (`ci.yml:573-575`); gate arithmetic and the hoisting plan are in Architecture Patterns. |
| MODE-08 | Hardware notes rewritten; provenance rows; byte caps | Both pins answer 200 and the four MAME and ares licence headers read BSD-3 or ISC at the pins; TI text layer is usable but OCR-noisy in tables; `docs/hardware/` is outside the byte-cap glob. |
</phase_requirements>

## Summary

The phase is well specified. CONTEXT's 33 decisions agree with the primary sources I could read this session (TI TMS9918A manual, Sean Young's TMS9918A notes, Charles MacDonald's `msvdp`, the SMS Power palette page through an archive capture, and the BSD-3 and ISC reference code at the two pins). What the planner still needs is mostly mechanical coupling that CONTEXT does not list: six existing tests and seeds go red when Graphics I and II stop faulting (CONTEXT names four of them), every new state field name must be classified in `tests/bios/test_bios.c`, every new test needs a line in `tests/inventory/ci.txt` and `wasm32.txt` (and `hygiene.txt` for hygiene-lane tests), and one real design hole: the fifth-sprite index must survive a status read that falls between the tick-600 scan and the line's first dot.

Hardware evidence is thin by nature and the sources disagree on exactly the choices CONTEXT already marks as models: the Graphics II masks (five different behaviours), the sprite Y threshold (the TI manual's own wording reads as `> $E0`, not `>= $E0`), what bits 4..0 read when no fifth sprite exists, whether colour-0 sprites collide, and what the Game Gear shows in these modes. None of the choices is wrong; each must be pinned by a fixture row and stated as a model. One fixture row is missing for the Y threshold: Y=$E8 does not separate `>= $E0` from `> $E0` (see Open Question 2).

Two findings reduce risk. The branch tip is byte-identical to `origin/main` in `src include runner scripts tests cmake CMakeLists.txt pins.txt`, so the base hashes file for the subset compare can be written from the unmodified tree in a few seconds with no worktree. And all three rider files (`library.sh`, `cmake/WlaDx.cmake`, `scripts/fetch-pin.sh`) are classified `operation` or `build`, so they do not consume the 51-line repository-check headroom; the cap even rises by one line per ten new `src/` lines.

**Primary recommendation:** Follow D-26's order. Take `build/p16-base-hashes.txt` from the unmodified branch tip first; land the oracles and fixtures; make the once-per-call classifier edit and open the draft pull request; then add the legacy renderer and sprites, with the fifth-sprite carry, the `tests/bios` field pins, the inventory lines and the two runner seeds handled in the same commits as the code that makes them necessary.

## Project Constraints (from CLAUDE.md)

Root `CLAUDE.md` (symlink to `AGENTS.md`) and `.claude/CLAUDE.md`. Treated as locked.

- Rule 1: reuse permissive code only (MIT, BSD, ISC, zlib, CC0); never open GPL, LGPL, non-commercial or unlicensed emulator source; reference emulators run only as released binaries. Notice kept per `PROVENANCE.md`.
- Rule 2: three deliverables (library, runner, libretro core); no window, audio-device or input-device code.
- Rule 3: no ROM, BIOS or save bytes except fixtures listed in `tests/roms/manifest.txt`; no personal paths, e-mail addresses or names; GitHub noreply identity; `scripts/hygiene.sh` checks every commit and push.
- Rule 4: deterministic, bounded, integer-only core; all state in the instance; same hashes on every platform; every call returns within bounded emulated time even for a hostile ROM; no wall clock.
- Rule 5: every behaviour change lands with a test that `cmake --workflow --preset ci` runs; README short; maintainer docs in `docs/`; public header terse.
- Rule 6: verification is automation-first; the owner verifies nothing by default; the agent reads docs against code, views images and screenshots, runs RetroArch.
- Rule 7: proportional gates; repository-check code stays under a tenth of `src/` lines; real-host e2e code under 300 lines; every gate names the fault it prevents; at most two code-review fix rounds and one gap round per phase.
- Rule 8: no invented ceremony (no attempt quotas, slots, lifetime counters).
- Rule 9: one GSD step per session; never run `/gsd-next`, `/gsd-autonomous`, `--auto` or `--chain` forms; never set `COGMASTER_ALLOW_CHAIN` or `COGMASTER_ALLOW_MAIN_COMMIT`; record runtime and model in STATE.md at every phase step.
- Rule 10: branch, then pull request; never commit on `main` once a remote exists; `gh pr merge --auto --squash` from a shell; confirm a CI run on the head commit; every phase ends in a release.
- Rule 11: evidence is a command, an exit status and a run URL, never a committed log; written from the repository root, no `cd` chains.
- Rule 12: pins are public (`gh api repos/<o>/<r>/commits/<sha>` answers 200); sibling code fetched with `gh api`, never a local checkout.
- Rule 13: inbound lessons are hypotheses until reproduced here.
- GSD summaries: every plan SUMMARY carries `requirements-completed:` and a block-style `coverage:` list (`id`, `description`, `requirement`, `verification` with kind in unit, integration, e2e, automated_ui, manual_procedural, other, plus `ref` and `status`, and `human_judgment`); no flow style `{}` and no inline `#` comments in that block; plan acceptance checks assert on exit status and tolerant regexes, never on a tool's exact output text.
- Style: small modules, plain control flow, fixed-width integer types; public names start `cogmaster_`, module-private names carry the module prefix; a hardware comment says what the hardware does and cites its source; no planning IDs in code, test names or user docs; follow the ponytail ladder (remove the need, reuse, standard library, platform, minimum new code); copy and paste beats a new dependency.

Additional repository checks the planner must respect, found by reading the scanners (see Pitfall 11): `scripts/hygiene.sh` (home-directory paths such as `/Users/<name>`, e-mail addresses, GPL text, and a "retyped bytes" tripwire for 256 or more consecutive hex-byte-only lines), `hygiene.format` (clang-format 21 on every tracked `.c` and `.h`), `hygiene.no_float`, `hygiene.analyzer` (clang-tidy analyzer on every `src/*.c`), and the single-version check that rejects the current and next release numbers in any tracked file outside `.planning/`, `CHANGELOG.md` and the manifest files.

## Architectural Responsibility Map

The usual browser, SSR, API and CDN tiers do not apply: this is an embeddable C library with a runner and a libretro adapter. The tiers below are this repository's.

| Capability | Primary Tier | Secondary Tier | Rationale |
|------------|-------------|----------------|-----------|
| Mode classification (MODE4, LEGACY, BLOCKED) | Library core, `src/internal.h` | Light Phaser sensor (`src/gun.c`) asks it | One expression shared by the renderer and the sensor so they cannot drift (D-01, D-02). |
| Graphics I and II background pixels | Library core, new `src/vdp_legacy.c` | `src/vdp_render.c` dispatches once per call | Own module keeps the mode 4 per-dot loop out of the legacy code (D-05). |
| Legacy sprite scan, per-dot sprite pixel, status flags 5S and C | Library core, `src/vdp_legacy.c` | `src/vdp.c` status read and tick-600 scan call | Flags assert at their dots through the shared cursor; the status read lives in `vdp.c`. |
| Fixed TMS palette versus CRAM 16+n | Library core | `vdp_colour` lattice in `src/vdp_render.c` | Profile decision (`machine == GG && !native_gg`) lives next to the colour expansion. |
| Backdrop and `completed.backdrop` metadata | Library core, `cogmaster__vdp_finish_field` | Frame descriptor read by hosts (`src/frame.c`) | Hosts draw borders from this value; it must stay within 0..0xFFF. |
| Saved state (format 17, bounds, field names) | Library core, `src/state.c` | State tests (`tests/core/test_state.c`, `tests/bios/test_bios.c`) | One visitor drives save, load and record passes; field names are pinned by tests. |
| Oracles and owned fixtures | Test harness (`tests/fixtures/`, `tests/vdp/`, `tests/roms/src/`) | CMake fixture family in `tests/fixtures/tests.cmake` | Fixtures never judge: closed-form oracles and CSV rows judge. |
| Cross-lane pin evidence | `tests/cmake/hashes.cmake`, `runner.hashes_file` | `.github/workflows/ci.yml` hash-equality job | Lanes must agree byte for byte; one-time base-versus-head subset compare is a command and exit status. |
| Dirty-tree refusal | `scripts/library.sh` (operation) | Hygiene-lane test with a throwaway git repository | Local operation script; the test proves refusal and three controls. |
| Assembler fetch retry | `cmake/WlaDx.cmake`, `scripts/fetch-pin.sh` (build) | Fixture and hygiene tests | Retries live where the downloads happen; hash is checked once, after the loop. |
| Hardware notes and provenance | `docs/hardware/video.md`, `.planning/preparation/SMS-VDP.md`, `PROVENANCE.md` | `docs/*.md` capped at the quality ceiling | `docs/hardware/` is outside the byte-cap glob; `docs/*.md` is inside it. |

## Standard Stack

No external package is installed by this phase. The "stack" is the repository's own toolchain.

### Core
| Tool | Version | Purpose | Why Standard |
|------|---------|---------|--------------|
| C17 | `CMAKE_C_STANDARD 17` (CMakeLists.txt:16) | The core | Project standard; integer-only, fixed-width types. |
| CMake | `cmake_minimum_required(VERSION 3.25)`; 4.4.3 installed locally | Build, CTest, script-mode checks | `cogmaster_add_test` and the inventory check are CMake. |
| WLA-DX | v10.7, `pins.txt` row `wla-dx 10.7 ... 38296a96bc20be873d17e0e88be0c5b20a15ef2ec4da5279600e56af30ad925a` (GPL-2.0-or-later, tool only) | Assembles owned Z80 fixtures at configure time | Pinned by SHA-256; source never copied into the tree. |
| clang-format | 21.1.8 (`/opt/homebrew/opt/llvm@21/bin/clang-format`) | `hygiene.format` | The hygiene lane runs it on every tracked C file. |

### Supporting
| Tool | Version | Purpose | When to Use |
|------|---------|---------|-------------|
| clang-tidy analyzer | llvm@21 | `hygiene.analyzer` over `src/*.c` | Run on the new `src/vdp_legacy.c` before pushing. |
| Cachegrind (valgrind) | CI only (not installed locally) | Ir per frame gate on `boot_to_play`, +1 percent allowed | First read comes from the draft pull request. |
| `runner/sha256.c` (`cogmaster__sha256`) | in repo | Oracle digest printing | Reuse as `tests/fixtures/lines.c` does; no new hash code. |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| New `src/vdp_legacy.c` | Grow `src/vdp_render.c` | Rejected in D-05; `policy.no_title_table` also scans `vdp_render.c` text, and the mode 4 loop stays easier to diff. |
| Shared widened `sprite[8]` | Separate 4-slot struct | Same saved size either way (16520, see Pitfall 6); CONTEXT keeps the shared array. |
| `curl --retry-all-errors` in `fetch-pin.sh` | POSIX loop on fixed exit codes | `--retry-all-errors` needs curl 7.71 and retries HTTP 404 (D-29). |

**Installation:** none. **Version verification:** `cmake --version` gave 4.4.3; `/opt/homebrew/opt/llvm@21/bin/clang-format --version` gave 21.1.8; the WLA-DX row is read from `pins.txt`.

## Package Legitimacy Audit

| Package | Registry | Age | Downloads | Source Repo | Verdict | Disposition |
|---------|----------|-----|-----------|-------------|---------|-------------|
| (none) | - | - | - | - | - | This phase installs no external package. WLA-DX stays the existing pinned tool; the permissive MAME and ares files are read for facts only and no code is copied. |

**Packages removed due to [SLOP] verdict:** none
**Packages flagged as suspicious [SUS]:** none

## Architecture Patterns

### System Architecture Diagram

```
 Z80 OUT ($BF) register write
        |
        v
 cogmaster__vdp_write -- renders every dot strictly before the write dot FIRST ---------+
        |                                                                               |
        v  (registers change only here, so they are constant inside one render_until)   |
 R0/R1 hold the mode nibble                                                             |
                                                                                        v
 cogmaster__vdp_render_until(line, x)  <---- also: status read (+1 dot), Light Phaser, vdp_event t=512
   |  class = classify(nibble, profile)        <- ONCE per call (D-02)
   |  fault if at<end && display && class==BLOCKED      (first fault wins, saved state)
   |
   +-- MODE4   -> existing per-dot loop (hot path; loses its three in-loop "supported" tests)
   |
   +-- LEGACY  -> per line: legacy_span(m, line, x0, x1)
   |                 background: Graphics I or II address math -> nibble -> palette
   |                 sprites: slot list from the scan -> first opaque wins, pattern bits make coincidence
   |                 asserts: status bit 5 at the dot, bit 6 (+ fifth index latch) at dot 0
   |
   +-- BLOCKED -> backdrop dots (today's behaviour), fault already recorded

 tick 600 of each line (vdp_event): latch display and R8; cogmaster__vdp_sprite_scan(next line)
        scan dispatches on the CLASS AT THAT TICK and saves it (sprite_legacy)  -> state

 status read ($BF): render +1 dot, return (status & 0xE0) | (bit6 ? fifth_index : 0x1F), clear flags, fifth_index = 31

 frame interrupt (t=607, line H): cogmaster__vdp_finish_field -> completed.pixels, completed.backdrop
```

### Recommended Project Structure
```
src/
├── vdp_render.c     # class once per call; mode 4 loop; finish_field; exported wrappers keep their checks
├── vdp_legacy.c     # NEW: legacy_span, legacy sprite scan, per-dot legacy sprite pixel, fixed palette
├── vdp.c            # status read gets fifth_index; reset sets the new fields
├── internal.h       # vdp_class enum + classifier (vdp_mode moves here), widened slot, new fields, prototypes
├── state.c          # slot attr, sprite_legacy, sprite_size, fifth_index, class-dependent bounds, size pin 16520
└── gun.c            # render_blocked caller unchanged
tests/
├── vdp/test_legacy.c        # NEW: oracle functions + vdp.legacy.* cases (masks, palette, backdrop, sprites, switch, bring-up, hostile)
├── vdp/test_background.c    # unsupported -> 16-mode by 4-profile literal table
├── fixtures/legacy.c        # NEW: closed-form pixel oracle, prints expected SHA-256, wrong-hash control
├── roms/src/legacy_modes.asm, legacy_modes.link, legacy_sprites.asm, legacy_sprites.link   # NEW
├── built/expected.txt       # six new "<name> <sha256>" lines
├── diag/expected.csv        # derived rows for legacy_sprites
├── inventory/ci.txt, wasm32.txt   # one line per new test, identical in both
└── core/test_state.c        # legacy_roundtrip, legacy_sprite_bounds
```

### Pattern 1: classify once per call, branch once per call
**What:** `render_until` computes `class` before its loop and chooses MODE4 loop or LEGACY span path. The three per-dot `vdp_supported` calls (inside `cogmaster__vdp_background`, the sprite gate and the dot-0 overflow gate) disappear from the hot loop.
**When to use:** the only hot-path edit; everything else in the phase is off the `boot_to_play` path.
**Why it is safe:** registers cannot change inside a call because `cogmaster__vdp_write` calls `render_until` up to the write dot before it mutates (`src/vdp.c:393-405`). The display latch changes only in `cogmaster__vdp_event` at tick 600, between calls.
**Keep exactly:** the exported `cogmaster__vdp_background` wrapper with its current `!v->display || !vdp_supported(m) || ...` test (called directly by `tests/vdp/test_background.c:272-275`), and the per-dot `display`, left-mask and fine-scroll tests inside the mode 4 body. Only the mode test is hoisted.
**Gate arithmetic:** 122,889 Ir per frame over 49,152 dots is about 2.5 Ir per dot of slack (+1 percent). Removing three calls to `vdp_supported` (each a handful of integer operations plus a profile read) should lower Ir; the draft pull request is the first time the gate can be read because there is no valgrind on the Mac.

**Existing code to preserve (quoted):**
```c
/* src/vdp_render.c:52-58 */
static uint32_t vdp_mode(const cogmaster *m)
{
    const uint8_t *r = m->vdp.reg;
    return ((uint32_t)(r[0] & 4u) << 1) | ((r[1] & 8u) >> 1) | (r[0] & 2u) | ((r[1] & 16u) >> 4);
}
/* src/vdp_render.c:62-69 */
static int vdp_supported(const cogmaster *m)
{
    uint32_t mode = vdp_mode(m);
    if (cogmaster__profile(m).vdp_masks) {
        return mode == 8u || mode == 10u || mode == 12u;
    }
    return mode >= 8u && mode != 9u && mode != 13u;
}
```
A classifier skeleton that keeps these facts (the enum names are a suggestion, not in-repo values): MODE4 when the existing `vdp_supported` is true; LEGACY when `mode == 0u || mode == 2u` and `!cogmaster__native_gg(m)`; BLOCKED otherwise. Note the nibble is computed once and reused for the fault address (`cogmaster__bus_fault(m, (uint16_t)vdp_mode(m), ...)` at `src/vdp_render.c:195`).

### Pattern 2: span renderer with explicit [x0, x1)
**What:** `legacy_span(m, line, x0, x1)` fills `m->field[line][x0..x1)` with RGB444 words. Background and sprite pixel are pure functions of (x, line, VRAM, registers, slot list), so spans split at any dot (CPU writes, status reads, Light Phaser) are trivially correct. Status flags assert at their dot inside the span: bit 5 at the second pattern hit, bit 6 only when the span starts at x0 == 0 (as `render_until` does today at `render_x == 0`).
**Alignment with the cursor:** advance `m->render_x`, `m->render_line` and `at` exactly as the dot loop does (`if (++m->render_x == COGMASTER__FIELD_W) { render_x = 0; render_line++; }`) so state round trips and `tests/core/test_core.c:849-851` (cursor does not move on an idempotent call) keep passing.
**Rows at or past 192:** write the backdrop (D-06). `m->vdp.height` can be 224 or 240 when a mode 4 frame latched it and a game then selects Graphics I mid-frame.

### Pattern 3: legacy address helpers are separate, 32-bit, masked
Use `uint32_t` throughout and mask every VRAM index with `0x3FFF` (D-07). The only signed values are the sprite Y and X deltas (`int32_t`), as `cogmaster__vdp_sprite_scan` does today (`src/vdp_render.c:133-135`). Never call `cogmaster__vdp_pattern_address`, `cogmaster__vdp_sat_address` or `cogmaster__vdp_sprite_address`; their SMS1 masks apply `reg[3]`, `reg[4]`, `reg[5]` and `reg[6]` bits that mean something else in legacy modes.

### Pattern 4: the legacy sprite scan is class-aware and records its class
Mode 4 scan today: `src/vdp_render.c:117-152`, `top = y >= 0xF0u ? (int32_t)y - 255 : (int32_t)y + 1;` and `$D0` ends the list only `if (y == 0xD0u && v->height == 192u)`. Legacy differs: 32 entries at `(R5 & 0x7F) << 7`, 4 bytes each, `Y >= 0xE0` negative, `$D0` terminates unconditionally, four slots, five-match overflow with the 5th's SAT index, EC from attribute bit 7, `size` from R1 bit 1, `zoom` from R1 bit 0, R0 bit 3 ignored. Save `sprite_legacy` and `sprite_size` at scan time (D-18).
**Mid-line class change:** the scan runs at tick 600 of the previous line under whatever class is live then. If a game enters or leaves a legacy mode after that tick, dots of the new line render under the other class with the other class's slot list. Recommended rule: at the start of each `render_until` call (not per dot), if `sprite_legacy` differs from the live class is-legacy, rescan the current line under the live class. This costs one compare per call, nothing per dot, and the loader bound in D-19 stays valid because the saved `sprite_legacy` always describes the saved slots.

### Pattern 5: fifth-sprite index must be carried until the flag asserts
The 5S flag asserts at the line's first dot (`render_x == 0`), up to 84 ticks after the scan at tick 600 of the previous line. A status read in that window clears `fifth_index` to 31 (D-19) while `sprite_overflow` is still 1. See Pitfall 3 for the recommended carry (store `1 + SAT index` in the legacy `sprite_overflow`; copy to `fifth_index` at assertion if `fifth_index` is still 31; bound 0..32 for legacy, 0..1 for mode 4).

### Pattern 6: oracle-first, tile-index domain
Follow D-23. Pattern bytes generated by a formula such as `(tile*7+row)&255`, name bytes by a formula in (column, row), colour bytes by a formula in colour group; the oracle computes `pixel(x, y)` from the tile index without any address arithmetic, while the VRAM loader places the generated bytes at the base addresses the registers select. The two share no formula. `tests/fixtures/lines.c` is the template: literal per-pixel function, per-pixel compare against `cogmaster_get_frame`, SHA-256 of the expected pixels printed. `tests/fixtures/play.c` plus `CASE=play` in `tests/fixtures/tests.cmake:176-209` show how the runner's `--hash` is compared with the printed digest and how the `WRONG` negative control is wired (`fixtures.boot_to_play.wrong_hash`). `CASE=play` hard-codes `--frames 14` and the Master System 2 machine; the new family needs `FRAMES`, `MACHINE` and `VIDEO` arguments.

### Pattern 7: one CMake function for the six images
`tests/fixtures/tests.cmake` today repeats a 12-line `file(READ link)`, `file(GENERATE)`, `add_custom_command` block for each family (lines 557-669). D-21 asks for one function. Dependencies to list: the `.asm`, the `.link`, `header.inc`, `mailbox.inc`. Add the six names to the `add_custom_target(cogmaster-fixtures ALL DEPENDS ${fixtures_images})` list; `hashes.cmake` globs `${FIXTURES}/*.sms` and `*.gg`, so each image gets its `fixture <file> sha256 <hex>` line automatically.

### Anti-Patterns to Avoid
- **Reusing the mode 4 helpers for legacy addresses.** Wrong masks on SMS1 (Pattern 3).
- **Calling `cogmaster__vdp_background` from the hot loop.** It keeps its per-call checks for tests; use an inline body without the mode test.
- **Per-dot class tests or per-dot rescan checks in the mode 4 loop.** The gate has about 2.5 Ir per dot; do class work per call.
- **Storing the raw SAT colour byte unmasked.** The loader bound `attr & 0x70 == 0` refuses a state a real game can produce (Pitfall 5).
- **Writing the fifth-sprite index at scan time only.** A status read in the tail of the previous line erases it (Pitfall 3).
- **Letting the fixtures judge.** The mailbox carries measurements; `tests/diag/expected.csv` judges.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| SHA-256 of expected pixels | A new hash | `runner/sha256.c`, linked as `lines.c` and `play.c` do | Already compiled for oracles. |
| Fixture result judging | A new judge | `tests/cmake/mailbox.cmake` plus rows in `tests/diag/expected.csv` (columns `rom,test,profile,expected,source,citation`; `source` is `doc` or `derived`) | Every row needs its record and every record a row. |
| Negative controls | Ad-hoc scripts | `tests/cmake/expect_fail.cmake` (merges stdout and stderr, compares exit code and regex) | Used by every control in the repository. |
| Cross-lane agreement | New comparison | `tests/cmake/hashes.cmake` MODE=write and MODE=compare | CMake wraps long messages, so regexes must tolerate a line break between words (`tests/runner/tests.cmake:1625`). |
| Throwaway git repository | New harness | `fwin_sh` and `fwin_repo` in `tests/policy/firmware_window_cases.cmake` (identity from the noreply form, `COGMASTER_SH`) | Same pattern the owner asked for in D-28. |
| 6-bit to RGB444 | A second lattice | The `c*5` expansion in `vdp_colour` (`src/vdp_render.c:6-12`); export it rather than copy it | One lattice keeps hashes consistent between modes. |
| Test registration | `add_test` | `cogmaster_add_test(NAME ... COMMAND ... LABELS ... TIMEOUT ...)` | Scales timeouts per lane and feeds the exact inventory check. |

**Key insight:** every piece of machinery this phase needs exists; the work is wiring and keeping the many pinned lists consistent.

## Hardware Findings and Source Conflicts

What each source states, read this session. These are statements about documents and emulators, not silicon measurements.

### Sources and their access
- TI TMS9918A/9928A/9929A Data Manual, Nov 1982: fetched from the bitsavers mirror URL in CONTEXT; `pdftotext -layout` gives a 6,391-line text layer. [VERIFIED: pdftotext run this session] It is usable for prose; tables are OCR-noisy (hex such as `1A00` appears as `lAOO`, `OAOO`), so numbers must be checked against the page image before they enter docs. This corrects the prep note that called the manual only a scan.
- Sean Young, "Texas Instruments TMS9918A VDP", v0.4.2, 2002-09, and Charles MacDonald `msvdp-20021112`: the direct SMS Power host answered 503 on 2026-11-13; both were read from web.archive.org captures. [CITED: smspower.org/uploads/Development/tms9918a.txt and msvdp-20021112.txt, archive capture]
- SMS Power "Palette" page: read from an archive capture; it holds the 16-entry table. [CITED: smspower.org/Development/Palette]
- MAME `tms9928a.cpp/.h` and `315_5124.cpp/.h` at `1d7a454ed79e` (commit date 2026-10-18) and ares at `9408cb43d494` (2026-10-06): `gh api repos/<o>/<r>/commits/<sha>` answered 200 for both; file headers read `// license:BSD-3-Clause` on all four MAME files (line 1) and ares `LICENSE` is the ISC text. [VERIFIED: gh api this session]

### Facts all sources agree on
- Graphics I: name `(R2&15)<<10 | row*32+col`; pattern `(R4&7)<<11 | name<<3 | row&7`; colour byte at `(R3<<6) | name>>3`, high nibble for set bits, low nibble for clear bits; nibble 0 shows the backdrop colour. [CITED: TI manual 2.4.1 and Table 3-2; VERIFIED: MAME `315_5124.cpp:1592-1620`, ares `ms/vdp/background.cpp:28-51`]
- Graphics II tables: `R4&4` and `R3&0x80` give bit 13; the two top bits of the row (y>>6) give address bits 12..11; name gives bits 10..3; row gives bits 2..0. [CITED: TI Table 3-2; VERIFIED: MAME `315_5124.cpp:1554-1569`]
- Sprites: 32 entries of Y, X, name, EC and colour; four per line; `$D0` in the Y byte ends processing and is tested before the negative adjustment; 16x16 uses `name & 0xFC` and quadrants TL, BL, TR, BR in 8-byte units; early clock is attribute bit 7 and moves 32 dots left; colour 0 is transparent; lower index wins. [CITED: TI 2.4.5 and Figure 2-21; Young section 4; VERIFIED: MAME `tms9928a.cpp:564,579,597`, ares `ms/vdp/sprite.cpp:11-23`]
- Fixed palette 6-bit values `$00 $00 $08 $0C $10 $30 $01 $3C $02 $03 $05 $0F $04 $33 $15 $3F` on the Master System. [CITED: SMS Power Palette page table; VERIFIED: ares `ms/vdp/dac.cpp:27-28`] Each converts through the existing lattice to exactly D-10's sixteen RGB444 words (checked entry by entry this session).
- Naming: TI numbers register bits from the most significant end. TI "M3" is R0 bit 1 (SMS Power calls it M2), TI "M2" is R1 bit 3 (SMS Power M3), TI "M1" is R1 bit 4. [VERIFIED: TI 2.2.1 and 2.2.2 text and Figure 2-3] This is D-12's reason for writing modes as the nibble or as register bits.

### Where the sources disagree (and what CONTEXT selects)

| Point | TI manual / Young | msvdp | MAME TMS (`tms9928a.cpp`) | MAME SMS (`315_5124.cpp`) | ares | CONTEXT selects |
|-------|-------------------|-------|---------------------------|---------------------------|------|-----------------|
| Sprite Y negative | TI 2.4.5: "values for vertical displacement between -31 and 0 (E1 to 0)" (OCR), `-1` butts at the top. Young: 255 is the top line, 254 hides the top line, 208 ends. Neither states `$E0`. | - | `if ( spr_y > 0xE0 )` (line 567) | `if (sprite_y >= 240)` (line 1211) | `if(y >= 0xe0)` (`tms_sprite.cpp:11`, `ms_sprite.cpp:12`) | `>= $E0` (D-14) |
| Bits 4..0 with no fifth sprite | TI 2.3.3: valid only when 5S is 1. Young: index of the last SAT entry processed, 31 or the first Y=208 entry | - | `m_FifthSprite = sprattr / 4` before the `$D0` break (561-564); 31 when sprites disabled (546) | mode 4 formula `sprite_count_overflow` | overflow index | `$1F` (D-16) |
| Fifth sprite stickiness | Young: first illegal sprite on the highest line wins | - | index copied only while bit 6 is clear (641-645) | derived from first overflow | last overflow written | first wins until a status read (D-16) |
| 5S gated by F | TI 2.3.3: set only while the frame flag is 0 | - | `fifth_encountered && ~m_StatusReg & 0x80` (644) | not gated by F in legacy | not gated | no gate (D-16) |
| Coincidence of colour-0 sprites | TI 2.3.2: transparent sprites are considered; Young: pattern decides, colour can be 0 | - | pattern bits decide (`spr_drawn |= 1` before the colour test, 613-618) | opaque only (`if (pen_selected && BIT(pattern, ...))`, 1514) | asymmetric: only when an earlier opaque sprite exists (`tms_sprite.cpp:57`) | pattern bits decide (D-17) |
| Collision off the 0..255 range | TI 2.3.2: sprites partly or wholly off the screen "are also considered" | - | only `colission_index` in 32..287 (613) | only 0..255 (1528) | only dots 0..255 | x in 0..255 only (D-17) |
| EC via R0 bit 3 in legacy | attribute bit 7 only | R0 bit 3 shifts by 8 as well (section 15) | not applicable | attribute bit 7 only (1499) | attribute bit 7 only | R0 bit 3 ignored (D-15) |
| Graphics II address masks | TI: low bits "must be set to all 1s". Young (TMS9918A): R4 bits 1..0 and R3 bits 6..0 AND the 10-bit character number (top bits) | SMS2 and GG: none (section 15) | `colourmask = ((R3&0x7f)<<3)|7`; `patternmask = ((R4&3)<<8) | (m_99 ? (colourmask&0xff) : 0xff)` (231-234): the "99" family also couples R3 into the pattern index | masks on every revision, no R3 coupling (`315_5124.cpp:1558-1560`; only one `draw_scanline_mode2` exists) | R4 bits applied to both pattern and colour through address bits 11, 12 (`ms/vdp/background.cpp:63-67`); `colorMask` computed and never used | SMS1 masks per MAME 315; SMS2 and GG none (D-09) |
| Palette on the Game Gear in Master System mode | - | CRAM 16-31 (section 14: "colors 16-31"; SMS2 uses a hardwired palette) | - | `m_palette_offset = 0x10` for the 315-5377, TMS palette for 5124 and 5246 (`update_palette`, 1855-1868) | fixed table everywhere (`dac.cpp:24-54`) | CRAM 16+n (D-10) |
| Colour nibble 0 in Graphics I and II | backdrop (TI, Young) | - | backdrop (`BackColour`) | `pen_selected = BACKDROP_COLOR` (1582, 1617) | colour 0 of the fixed table, black, not R7 (`background.cpp`) | R7 low nibble backdrop (D-08) |
| SMS1 horizontal zoom slot quirk in legacy | - | - | none | applies (`m_max_sprite_zoom_hcount`, 1497) | none | plain doubling (D-17) |
| Backdrop with display off | blank shows border colour (TI 2.2.2) | - | TMS colour | `BACKDROP_COLOR` is the TMS colour in non-mode-4 even when display is disabled | TMS palette | CRAM backdrop with display off, TMS colour only when display is on (D-11; a bounded deviation from MAME and ares) |

Read-outs the planner should carry into the docs and fixtures:
1. D-14's attribution "ares and the TI manual" is generous to the manual. TI's own range ends at `$E1`, which fits `> $E0` (MAME TMS) as well as `>= $E0`. The decision stands; the docs should say the three candidates differ only for a 32-line sprite at Y=$E0 (and `>= $F0` differs for 32-line sprites over `$E0..$EF`), and the fixture needs a Y=$E0 row to pin the choice (Open Question 2).
2. D-16's `$1F` is the repository's mode 4 choice; Young and MAME TMS would read the index of the last processed entry. State it as a deliberate departure.
3. D-17 restricts collision to x in 0..255, as MAME does and as the TI wording does not strictly require.
4. The prep note's "MAME contradicts" claim traced to a mis-attribution (D-31); the corrected statement is in the table: `315_5124.cpp` masks on every revision and the TMS file's R3 coupling is a separate behaviour.

## Common Pitfalls

### Pitfall 1: more tests go red than CONTEXT lists
**What goes wrong:** D-25 names `vdp.background.unsupported`, `fault_run` in `tests/ports/test_gun.c` and the Light Phaser table. Two runner tests also use "display on, R0 = 0" as their unsupported-mode seed and fail the moment Graphics I stops faulting.
**Where:** `tests/runner/diagnose.cmake:72-82` builds `mode0.sms` (`LD A,$40; OUT ($BF),A; LD A,$81; OUT ($BF),A`, R1=$40, R0=0) expecting `fault=unsupported_video_mode dev=vdp` and `vdpmode=graphics1`, and `mode2.sms` (R0=2) expecting `vdpmode=graphics2`. `tests/runner/tests.cmake:1022-1028` (`CASE trace_fault`) writes the same R1=$40 image and expects `kind=unsupported_video_mode`. `tests/ports/test_gun.c:889` sets `m->vdp.reg[0] = 0u` with display on.
**Fix (same commit as the renderer):** keep the images' byte counts and change the selected mode: R1=$50 gives the text nibble 1 (`vdpmode=text`); a multicolour seed is R1=$48 (nibble 4, `vdpmode=multicolor`); in `fault_run` also set the text bit in `reg[1]`. `diag(...)` count `want_checked 11` stays if both seeds stay. Canned diagnose lines in `tests/library/diagnose.cmake` do not run the core and are unaffected. Phase 17 owns the vocabulary prune; do not touch `runner/diag_class.h`.
**Correction (planning):** `tests/library/diagnose.cmake` `CASE=main` is not canned: it runs a real `mode0.sms` (R1=$40) and expects a Graphics I fault group, so `library.diagnose` goes red too; see Open Questions (RESOLVED), plan-level deviations.
**Warning signs:** `runner.diagnose`, `runner.trace.fault`, `ports.gun.fault_neutral` (name per its registration) fail; `vdp.background.unsupported` fails for modes 0 and 2 on every profile except native Game Gear.

### Pitfall 2: new state field names break the BIOS equivalence test
**What goes wrong:** `tests/bios/test_bios.c` classifies every state field name as compared or excluded and pins the counts.
**Quote:** `tests/bios/test_bios.c:977-978`: `#define EQ_COMPARED_NAMES 88u` and `#define EQ_EXCLUDED_NAMES 69u`; lines 1119-1189 hold `eq_excluded[]` with entries such as `{"sprite_row", why_out},`; lines 1038-1045 show the fault record names are in the compared list. A name in neither list fails "naming it" (`eq_class` returns -1).
**Fix:** each new visitor field name (for example `sprite_attr`, `sprite_legacy`, `sprite_size`, `fifth_index`) is added to `eq_excluded[]` with a reason (`why_out` for scratch, `why_beam` for the status-adjacent `fifth_index`), and `EQ_EXCLUDED_NAMES` goes from 69 to 69 plus the number of new names. Reusing existing names inside the loop (`sprite_x`, `sprite_tile`, `sprite_row`) adds none.
**Also noted:** this test compares `fault_*` at the hand-off and passes today, so the BIOS-shaped image does not currently record an unsupported-mode fault at hand-off.

### Pitfall 3: the fifth-sprite index is lost across a status read
**What goes wrong:** D-18 and D-19 give one saved `fifth_index` (31 when unlatched) that a status read resets. The flag asserts at dot 0 of the line (existing dot-0 gate in `render_until`, line 212). The scan that found the fifth match ran at tick 600 of the previous line. A status read between tick 600 and dot 0 resets `fifth_index` to 31 although `sprite_overflow` is still 1, so the flag asserts later with index 31.
**Recommended resolution (stays inside D-18's field list):** let the legacy scan store `1 + SAT index` of the fifth match in `sprite_overflow` (0 means none); at the dot-0 assertion, if `fifth_index` is still 31 set it to `sprite_overflow - 1`, then set status bit 6. Loader bounds per class: mode 4 `sprite_overflow` 0..1 as today (`src/state.c:283-284`: `uint8_t overflow = cogmaster__sio_u8(io, "sprite_overflow", &v->sprite_overflow, 0u, 1u);` and `cogmaster__sio_check(io, !overflow || (count == 8u && scanned >= 9u));`), legacy 0..32 with `count == 4 && scanned >= 5`. Alternative: a second saved byte; D-18 permits revisiting the layout.
**Fixture row:** read status once in the tail of the previous line (after tick 600) and check the next read still reports the index. Without it the hole is invisible, because every other row reads status at the frame end.

### Pitfall 4: a mid-line class change meets a stale sprite list
See Pattern 4. The mid-line fixture (`g2_switch`) should switch at a dot where sprites are on the line, so the rescan rule is exercised; assert the lines wholly before and after the switch with the oracle and pin the switch line by hash only (D-04).

### Pitfall 5: loader bound on the colour byte refuses real state
**What goes wrong:** D-19 requires `attr & 0x70 == 0` for legacy slots. Young notes colour-byte bits 6..4 "should be zero officially, but they are ignored", so games may write them. If the scan stores the raw byte, a saved legacy frame mid-line is refused on load.
**Fix:** store `attr & 0x8F` (EC and colour) at scan. The test `core.state.legacy_sprite_bounds` then sets `0x10` directly in a saved record and expects refusal.

### Pitfall 6: the struct layout claim in D-18
`sprite[8]` sits before `cram` and `vram` in `struct cogmaster__vdp` (`src/internal.h:111-118`), so widening each slot from three to four bytes moves `cram` and `vram` by 8 bytes even if the other new fields are appended after `vram`. Nothing depends on those offsets (no `offsetof` on the VDP struct in `src` or `tests`, and no test hashes state bytes), so it is harmless. Computed in a scratch copy of `internal.h` this session: current `sizeof(struct cogmaster__vdp)` is 16510 (matches `src/state.c:59`: `_Static_assert(sizeof(struct cogmaster__vdp) == 16510u, "vdp changed: update vdp_io");`); widening the slot and appending `sprite_legacy`, `sprite_size`, `fifth_index` gives 16520, and keeping three-byte slots with a separate appended `sprite_attr[8]` also gives 16520. The new pin is `16520u`. Locations that hold the format and revision numbers: `include/cogmaster.h:16-17` (`#define COGMASTER_BEHAVIOUR_REVISION 18`, `#define COGMASTER_STATE_FORMAT_VERSION 16`) and the two literals `CHECK_EQ_U64(COGMASTER_STATE_FORMAT_VERSION, 16u);` (line 14) and `CHECK_EQ_U64(v.state, 16u);` (line 21) in `tests/api/v14.c`. The other `tests/api/v*.c` files use `>=`.

### Pitfall 7: inventories are exact, per lane
**What goes wrong:** `cmake/CogmasterTests.cmake` defers a check that the registered test names and labels equal the sorted file `tests/inventory/<lane>.txt`; configure fails on any difference.
**Rule found by diffing the files:** `tests/inventory/wasm32.txt` equals `tests/inventory/ci.txt` minus the 42 `libretro.*` tests. So every new test outside `libretro.*` needs the identical line (name, TAB, labels) in both files, kept sorted. Tests registered from `tests/hygiene/tests.cmake` (the hygiene lane never downloads WLA-DX and registers only its own tests, per `tests/tests.cmake`) go only in `tests/inventory/hygiene.txt`. `tests/policy/firmware_window_cases.cmake` is included by the hygiene lane only (`tests/hygiene/tests.cmake:4`), which is the right home for the dirty-tree test and the fake-curl test; the WlaDx retry test sits beside `fixtures.wla_pin.self_test` (`tests/fixtures/tests.cmake:1916`), which is in ci and wasm32.
**Cheap check:** `cmake --preset ci` (configure only) fails with the registered and listed lists; add the lines until it passes. `quality.json` floors `tests.ci` 669 and `tests.wasm32` 627 are minimums and only rise.

### Pitfall 8: fixture images are pinned and exact-listed
Each new image needs a line `<name> <sha256>` in `tests/built/expected.txt` (33 lines today), read by `CASE` script mode (`tests/fixtures/tests.cmake:165-175`) and by `scripts/library.cmake:123`, which maps an image's SHA-256 to `fixture/<name>` for the judge. Adding lines adds no ledger row (rows come from the ledger). Take the digest from the first build, then pin. `tests/roms/manifest.txt` holds one comment line per owned image family; add one for the two sources. `docs/conformance.md` documents each fixture family (35,855 bytes, cap 38,912): add a short paragraph.

### Pitfall 9: `hashes.cmake` hard-codes which images get frame rows
`tests/cmake/hashes.cmake` runs `playing` rows only for filenames matching `^(boot_to_play[.]sms|boot_to_play[.]gg|vdp_lines_[0-9]+[.]sms)$` and always passes `--frames 14`. New images get only their `fixture` line unless a new branch is added (the file is classified `behavior`, so it adds nothing to the counted budget). `runner.hashes_file` (`tests/runner/tests.cmake:1612`, timeout 107 s from `2 x 53.07 s (be32 lane)`) asserts a fixed list of rows; add rows for the new frame hashes so a silent omission fails (the lesson behind the exact inventory).
**Cost:** a native 600-frame run of `boot_to_play` took 0.5 s here. D-24's estimate of 2,300 extra frames is dominated by `g2_late` (about 620 frames times three profiles). The be32 lane is the slowest (53 s for roughly 5,000 frames today, so about 10 ms per frame) and would grow by about 20 to 25 s. Running `g2_late` on one profile in the runner matrix (the in-process oracle already covers all profiles natively) would remove about 1,240 frames. Re-derive the timeout from the first hosted run as D-24 says.

### Pitfall 10: base-versus-head compare needs no worktree
`git diff --quiet origin/main HEAD -- src include runner scripts tests cmake CMakeLists.txt pins.txt` exited 0 this session, so the phase branch tip builds the same code as `origin/main`. Write the base file before the first code edit: `ctest --test-dir build/ci -R '^runner\.hashes_file$'` then copy `build/ci/hashes.txt` to `build/p16-base-hashes.txt` (Phase 14's method, 173 lines then and now). After the changes, `LC_ALL=C comm -23 <(sort base) <(sort head)` must print nothing. If edits to `scripts/` or `cmake/` come first (the riders do), the runner binary and fixture images are unchanged, so the base file can still be taken from the rider head. The new lines to expect in head: six `fixture legacy_*` lines plus the new frame rows; no old line should change.

### Pitfall 11: repository scanners reject some natural text
- `scripts/hygiene.sh` flags `(/Users/|/home/)<name>`, `<name>-Users-<name>-`-style strings, e-mail addresses, GPL licence text in code, and 256 or more consecutive bytes of hex-only lines (it strips `.db`, `.byte`, commas and spaces, then matches runs of `$XX` or `0xXX`). Keep `.DB` tables in fixtures short or generate patterns in code; C tables with a `u` suffix do not match. Do not write absolute paths into any tracked file (this document uses repository-relative paths for that reason).
- `tests/cmake/repository.sh` `single_version` fails when the current release number or either possible next number appears in any tracked file outside `.planning/`, `CHANGELOG.md` and the release manifest files. Do not write the release numbers into docs, tests or PROVENANCE rows.
- `-Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion -Wstrict-prototypes -Wmissing-prototypes -Wvla -Wundef -Wcast-qual -Wimplicit-fallthrough -Wmissing-variable-declarations` (CMakeLists.txt:38) with errors: every exported function needs a prototype in `src/internal.h`; narrowing stores need explicit casts as the existing code does.
- `policy.no_title_table` scans `src/gun.c`, `ports.c`, `io.c`, `vdp.c` and `vdp_render.c` (comments stripped) for `cart`, `cartdb`, `sha1`, `crc32`, `product`, `serial`, `title`, `checksum`; the new `vdp_legacy.c` is not in that list, but keep those words out of code in `vdp.c` and `vdp_render.c` edits.
- `hygiene.no_float`: no floating-point tokens in `src/`.

### Pitfall 12: mailbox capacity and CRC
The mailbox holds 60 records of four bytes (`mailbox.inc`: `MB_RECORDS_MAX 60`); `mb_record` silently drops the 61st. The `legacy_sprites` scenario list must fit in 60 records (one per scenario plus a few ids). The runner stops when the mailbox reports done (`--mailbox`), which `mailbox.cmake` demands with status `00` and a good CRC; a hang shows as status `80`.

### Pitfall 13: do not rely on exact tool output
Acceptance texts must use exit status and tolerant regexes (AGENTS "GSD summaries"). CMake wraps `FATAL_ERROR` lines, so "hash mismatch" in the WlaDx message should be matched as `hash[ \n]+mismatch` or kept short; `expect_fail.cmake` merges stdout and stderr, so "attempt 3 of 3" printed with `message(STATUS ...)` is visible to it.

## Code Examples

### Doc locations for TUNE-12 (read this session; replace wording, keep the diagnose keys)
```
docs/audio.md:76-77      "... a write latches nothing,
                          is recorded as the unmapped write a Japanese Master System records, and every channel reaches both outputs."
docs/embedding.md:323-325 "reset: an access to an unmapped address or port, a mapper value out of range, or an undefined
                           opcode. Only the first is kept; ..."
docs/embedding.md:346     "| 6 | `COGMASTER_FAULT_KIND_UNSUPPORTED_VIDEO_MODE` | an enabled display selects an unsupported TMS or invalid text mode; output is backdrop and frame delivery stays bounded |"
docs/embedding.md:355     "| 5 | `COGMASTER_FAULT_DEVICE_CTRL` | the memory and I/O control ports `$3E` and `$3F` |"
docs/embedding.md:480     "... the five `COGMASTER_FAULT_KIND_*` and six `COGMASTER_FAULT_DEVICE_*` values"
docs/runner.md:232-233    "| `dev` | `none`, `cpu`, `cart`, `ram`, `mapper`, `ctrl` (...) or `vdp` |" and "| `kind` | `unmapped_read`, `unmapped_write`, `rom_write`, ..."
docs/runner.md:430,437,438 `where` row names `port`; `wclass` row; `pclass` row
docs/embedding.md:61      the revision 18 sentence; revision 19 adds one sentence after it
```
Every `cogmaster__bus_fault(` caller in `src/` (`io.c:197,210`, `mapper_ext.c:34`, `bus.c:185,255,260`, `mapper.c:39`, `vdp_render.c:195`) raises RAM, CART, MAPPER or VDP faults; `COGMASTER__DEVICE_CTRL` is only declared (`src/internal.h:75`) and `COGMASTER__FAULT_ROM_WRITE` is only declared (`src/internal.h:65`). [VERIFIED: grep and Read this session] Byte arithmetic: `embedding.md` is 36,791 bytes and `runner.md` 37,844 against `"docs.max_bytes": 38912` (`quality.json:3`); the check is `size LESS cap`, so the file must stay strictly under. Row 6 rewrite and the revision 19 sentence are about 650 bytes in `embedding.md`; the doc wording for the removed faults should be net-neutral.

### TUNE-14 probe results (run this session, CMake 4.4.3)
```
file(DOWNLOAD "http://127.0.0.1:1/x" ... STATUS s)            -> 7;"Couldn't connect to server"
file(DOWNLOAD "file://.../payload.txt" ... STATUS t)          -> 0;"No error"
file(DOWNLOAD "http://nonexistent.invalid/x" ... STATUS u)    -> 6;"Couldn't resolve host name"
file(DOWNLOAD "file://..." EXPECTED_HASH SHA256=000... STATUS v)
   -> CMake Error: file DOWNLOAD HASH mismatch ... (script stops; the STATUS variable is never read)
```
So the retry loop must drop `EXPECTED_HASH`, test `list(GET status 0 code)` against the retry set, and verify with `file(SHA256 ...)` after the loop. Loop skeleton (codes from D-29):
```cmake
foreach(attempt RANGE 1 3)
  file(DOWNLOAD "${url}" "${out}" STATUS status)
  list(GET status 0 code)
  if(code EQUAL 0 OR NOT code IN_LIST retry_codes OR attempt EQUAL 3)
    break()
  endif()
  message(STATUS "wla-dx: attempt ${attempt} of 3 failed (${status}); retrying")
  execute_process(COMMAND ${CMAKE_COMMAND} -E sleep ${RETRY_DELAY})
endforeach()
```
(`retry_codes` is `6;7;18;28;35;52;55;56`; the message text must read "attempt 3 of 3" on the last try, so print the attempt number before the break test, not after.) The existing `fixtures.wla_pin.self_test` regex is `[Hh]ash|EXPECTED_HASH|mismatch` (`tests/fixtures/tests.cmake:1916-1920`).

`curl` semantics, local manual (curl 8.7.1): `--retry` treats as transient only "a timeout, an FTP 4xx response code or an HTTP 408, 429, 500, 502, 503 or 504"; connect failures need `--retry-connrefused` and everything else `--retry-all-errors`. So `scripts/fetch-pin.sh` line `curl -fsSL --retry 3 -o "$file" "$url"` never retries DNS (6), connect (7) or TLS (35). Keep `--retry 3` and wrap the call:
```sh
rc=0; curl -fsSL --retry 3 -o "$file" "$url" || rc=$?
case $rc in 6|7|18|28|35|52|55|56) ... retry up to 3 times ...;; 0) ;; *) exit 1 ;; esac
```
(`set -eu` is on in that script, so the `|| rc=$?` form is required.) A fake `curl` earlier on `PATH` must parse `-o FILE`, count calls in a file and exit 6 twice, then copy a payload to FILE and exit 0; a second variant exits 22 and must fail after exactly one call.

### TUNE-13 insertion point (read this session)
`scripts/library.sh` computes `core` on the diagnose path with `git -C "$root" diff --quiet HEAD -- . ':(exclude).planning' || core=$core+dirty` (lines 49-51) and stamps the run path with `COGMASTER_LIBRARY_REV=$(git rev-parse HEAD)` (full 40 hex; `compat-common.sh` update demands `length(ENVIRON["COMPAT_REV"])!=40` to refuse). A `+dirty` stamp would be rejected anyway, so refuse early. Gate on `[ "$ledger" = "$root/tests/compat/ledger.csv" ]`, the same equality the `counts=` line uses (`[ "$COGMASTER_CATALOG" != "$root/tests/compat/catalog.tsv" ] || counts=`), because `library.synthetic` runs the script in the real tree with `COGMASTER_LEDGER` stand-ins (`tests/library/tests.cmake`). Exit status 2 matches the script's other refusals.

### Fixture assembly sketch (WLA-DX, patterns from `tests/roms/src/vdp_lines.asm`)
Header and structure to copy: `.INCLUDE "header.inc"`, `.IFNDEF`/`.DEFINE` for the `-D` variant, `.BANK 0 SLOT 0`, `.ORG 0` `di` `ld sp,$DFF0` `jp main`, `.ORG $66` `retn`, `ld a,<rom id>` `call mb_init`, control-port writes `out ($BF),a` low then high byte, data port `$BE`. The graphics-mode switch is a pair of control-port writes (`ld a,<value>` `out ($BF),a` `ld a,$80` `out ($BF),a` for R0; `$81` for R1). Generate pattern and name data in loops instead of `.DB` tables (Pitfall 11).

### Cheap checks to run before the long suite
| Check | Command | Cost here |
|-------|---------|-----------|
| Docs caps, `src` to check-line ratio | `cmake -DROOT=$PWD -P tests/cmake/budgets.cmake` | 0.07 s; printed `repository: 887 check lines / 9386 src lines, strict one tenth` |
| clang-format on every tracked C file | `git ls-files -z '*.c' '*.h' ':!:libretro/libretro.h' \| xargs -0 /opt/homebrew/opt/llvm@21/bin/clang-format --dry-run --Werror` | about 0.1 s for five files; clean on the current tree |
| Inventory versus registered tests | `cmake --preset ci` (configure only) | seconds with WLA-DX already built |
| Fixture assembly and oracles | `cmake --build --preset ci --target cogmaster-fixtures` then `ctest --preset ci -R '^(vdp\.legacy\|fixtures\.legacy)'` | seconds |
| Behaviour of the picture | `cogmaster-run IMG --frames 30 --png 30:FILE` and `--diagnose IMG` (expects `fault=none`) | under a second |
| Pins | base file, then `LC_ALL=C comm -23` subset compare | seconds plus one `runner.hashes_file` run |
| Text scanner | `sh scripts/hygiene.sh --tree` | 34 s here (CI hygiene job takes 138 s); `--staged` is what the hook runs |
| Single-version source | `ctest --preset ci -R '^release\.single_version_source'` | fast |

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Graphics I and II record the unsupported-video-mode fault and draw backdrop | Draw tables, backdrop and legacy sprites on SMS1, SMS2, GG in Master System mode | This phase | 46 default-profile `fail` rows become candidates; acceptance rests on owned fixtures, not on the row count (CM-VIDEO-001). |
| Prep note: "SMS2 and GG do not mask" attributed to MAME's TMS file | The TMS file does not say it; `315_5124.cpp` masks on every revision; `msvdp` says SMS2 and GG do not | D-31 | Docs name the three-way disagreement. |
| `vdp_supported` called three times per dot | Class computed once per call | This phase | Lower Ir on the mode 4 path. |
| TI manual treated as a scanned PDF | Usable text layer, OCR-noisy tables | D-31 | Prep note corrected. |

**Deprecated or stale:** the old note that R0 bit 3 shifts TMS sprites (D-15); `docs/hardware/video.md:73-77` ("Legacy TMS modes and invalid text modes render deterministic backdrop and record `COGMASTER_FAULT_KIND_UNSUPPORTED_VIDEO_MODE`"); `src/fault.c` header comment "nothing in this release records a fault" (not required to change). `docs/perf-fuzz.md` still shows a stale Ir example per `.planning/research/NEXT-MILESTONE.md`; optional.

## Runtime State Inventory

Not a rename or migration phase. The one persisted-state change is state format 17: every older saved state is refused by the fixed header fields (revision 18 did the same with format 16). No datastore, service config, OS registration, secret or installed artifact carries a renamed string.

| Category | Items Found | Action Required |
|----------|-------------|------------------|
| Stored data | Saved states only; none shipped (states arrive in v3.0) | none |
| Live service config | None | none |
| OS-registered state | None | none |
| Secrets and env vars | None | none |
| Build artifacts | `build/` trees are ignored and rebuilt; `build/p14-base-hashes.txt` is stale (taken at `81a9cc9`) | write a fresh `build/p16-base-hashes.txt` |

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| cmake, ninja | build, tests | yes | 4.4.3 | - |
| WLA-DX build tree | fixtures | yes, `build/ci/wla-dx` and fixtures exist | v10.7 | rebuilt at configure |
| `cogmaster-run` (ci preset) | timing, `--png`, `--diagnose` | yes, `build/ci/cogmaster-run` | current tree | rebuild |
| clang-format, clang-tidy | hygiene lane | yes, llvm@21 | 21.1.8 | - |
| gh (authenticated) | pin checks, reading MAME and ares at pins | yes | - | - |
| pdftotext | TI manual text layer | yes | - | - |
| wasmtime | wasm32 lane | yes | - | CI |
| valgrind (Cachegrind) | Ir gate | no (CI only) | - | draft pull request |
| qemu big-endian runner | be32 lane | no (CI only) | - | hosted lane |
| `mame` | by-eye comparison | no, by design | - | none (never a gate) |
| SMS Power direct host | Young, msvdp, palette page | 503 on 2026-11-13 | - | web.archive.org captures (curl works; WebFetch refuses that host) |

**Missing dependencies with no fallback:** none for planning.
**Missing dependencies with fallback:** valgrind and qemu (hosted lanes), SMS Power (archive captures).

## Validation Architecture

(`workflow.nyquist_validation` is `false` in `.planning/config.json`; this section is included because the invocation asked for it.)

### Test Framework
| Property | Value |
|----------|-------|
| Framework | CTest through `cogmaster_add_test`; per-case executables take the case name as argument (`cogmaster-test-background tiles`, `cogmaster-test-state <case>`) |
| Config file | `tests/vdp/tests.cmake`, `tests/fixtures/tests.cmake`, `tests/core/tests.cmake`, inventories in `tests/inventory/*.txt` |
| Quick run command | `ctest --preset ci -R '^(vdp\.legacy\|vdp\.unsupported\|vdp\.background\|fixtures\.legacy)'` |
| Full suite command | `cmake --workflow --preset ci` |

### Phase Requirements to Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| MODE-01 | Graphics I pixels against closed form, three profiles | unit + fixture | `ctest --preset ci -R '^(vdp\.legacy\.g1\|fixtures\.legacy_modes)'` | Wave 0 |
| MODE-02 | Graphics II, eleven (R3,R4) pairs, SMS1 versus SMS2/GG | unit | `ctest --preset ci -R '^vdp\.legacy\.g2'` | Wave 0 |
| MODE-03 | sprite grid and status register (Y rows incl. $FF, $E8, $E0, $D0; EC; 8x8/16x16/zoom; fifth index; colour-0 collision; read in the line tail) | unit + fixture | `ctest --preset ci -R '^(vdp\.legacy\.sprites\|fixtures\.legacy_sprites)'` | Wave 0 |
| MODE-04 | text, multicolour, mixes, invalid text, native GG fault; Light Phaser agrees | unit | `ctest --preset ci -R '^(vdp\.unsupported\|vdp\.background\.unsupported\|ports\.gun)'` | partly exists; extend |
| MODE-05 | base-versus-head subset, revision 19, format 17, round trip, bounds | script + unit | subset compare command; `ctest --preset ci -R '^(core\.state\|abi\.api_v14\|bios)'` | exists; extend |
| MODE-06 | enter Graphics II after frame 600; mid-frame switch; bring-up transient | fixture + unit | `ctest --preset ci -R '^(fixtures\.legacy_modes\|vdp\.legacy\.(switch\|bringup))'` | Wave 0 |
| MODE-07 | hosted Ir gate green, no allow line | CI | the `perf` job on the draft pull request; `tests/perf/allow.txt` stays unchanged | exists |
| MODE-08 | notes rewritten, provenance rows, byte caps | docs check | `cmake -DROOT=$PWD -P tests/cmake/budgets.cmake` and `ctest --preset ci -R '^docs\.'` | exists |
| TUNE-12 | docs match the fault vocabulary | docs check | `ctest --preset ci -R '^docs\.'` | exists |
| TUNE-13 | dirty tree refuses; three controls pass | hygiene lane | `ctest --preset hygiene -R '^policy\.library_dirty'` (name to choose) | Wave 0 |
| TUNE-14 | retry then fail; permanent failure fails; fake curl | cmake + hygiene lane | `ctest --preset ci -R '^fixtures\.wla_'`; `ctest --preset hygiene -R '^policy\.pins'` | partly exists |

### Sampling Rate
- **Per task commit:** the quick run command plus `cmake -DROOT=$PWD -P tests/cmake/budgets.cmake` and the clang-format check.
- **Per wave merge:** `ctest --preset ci -R '^(vdp\|core\|runner\|fixtures\|ports\|bios\|library\|docs)'`.
- **Phase gate:** `cmake --workflow --preset ci` exits 0 on the phase head, a hosted CI run on the head commit, and the draft pull request's `perf` job green.

### Wave 0 Gaps
- [ ] `tests/vdp/test_legacy.c` and its registration in `tests/vdp/tests.cmake` (cases for masks, palette, backdrop, sprites, switch, bring-up, hostile loop).
- [ ] `tests/fixtures/legacy.c` plus `tests/roms/src/legacy_modes.{asm,link}` and `legacy_sprites.{asm,link}`, the one CMake family function, `tests/built/expected.txt` lines, CSV rows in `tests/diag/expected.csv`.
- [ ] A frames, machine and video parameter for the `CASE=play`-style check, and `hashes.cmake` rows with matching `runner.hashes_file` assertions.
- [ ] Inventory lines in `tests/inventory/ci.txt` and `wasm32.txt` (and `hygiene.txt` for hygiene-lane tests) for every new test.
- [ ] `tests/bios/test_bios.c` pins (`EQ_EXCLUDED_NAMES`, `eq_excluded[]`) for new state field names.
- [ ] A hostile-input case: `tests/core/test_core.c:822-846` loops `i | 4u` into R0 (always mode 4); a variant with R0 values that select legacy modes proves the bounded call for the new path.

## Security Domain

`security_enforcement` is `false` in `.planning/config.json`, so no ASVS mapping is produced. The relevant hostile-input controls are the ones AGENTS rule 4 already requires: every legacy VRAM index masked `& 0x3FFF`, bounded loops (32 SAT entries, four slots, 256 dots), and the loader bounds in D-19 so a crafted state cannot index outside the slot array or the field buffer.

## Assumptions Log

| # | Claim | Section | Risk if Wrong |
|---|-------|---------|---------------|
| A1 | be32 `runner.hashes_file` grows by about 20 to 25 s for 2,300 extra frames (extrapolated from 53 s for roughly 5,000 frames) | Pitfall 9 | Timeout needs a larger value than guessed; D-24 already says re-derive from the first hosted run. |
| A2 | `$E0` versus `$E1` reading of the TI manual's "-31 and 0 (E1 to 0)" is correct despite OCR noise in that line | Hardware Findings | Docs would misstate TI's range; check the page image before quoting. |
| A3 | The SMS Power Game Gear sentence ("maps the TMS9918a colours to the corresponding palette indices") means CRAM 16+n; `msvdp` and MAME say 16-31, the page does not give the offset | Hardware Findings | If the offset were 0, GG-in-SMS-mode colours differ; D-10 already records about 75 percent. |
| A4 | No current fixture hashes a frame that contains a mode 4 bring-up transient; the compare will confirm | Pitfall 10 | A shipped pin would move; D-24's compare catches it. |
| A5 | Hosted curl behaves like local curl 8.7.1 for `--retry` semantics and exit codes 6, 7, 18, 28, 35, 52, 55, 56 | TUNE-14 | A runner whose curl returns another code for a transient failure would still fail first try; D-29's list is fixed. |
| A6 | Registering the dirty-tree test in the hygiene lane (rather than ci) is acceptable to the owner's lane layout | Pitfall 7 | Only the inventory file changes; either placement works. |

## Open Questions (RESOLVED)

1. **Where does the fifth-sprite candidate live between the scan and dot 0?** RESOLVED
   - What we know: D-18 lists `fifth_index` only; the status read resets it; the flag asserts at dot 0 (Pitfall 3).
   - What is unclear: whether the owner wants a second byte or the `1 + index` encoding.
   - Recommendation: encode `1 + SAT index` in the legacy `sprite_overflow`, bound 0..32 for legacy, and add the tail-read fixture row.
   - Resolution: adopted as recommended. The legacy scan stores `1 + index` in `sprite_overflow` (loader bound 0..32 for legacy, plan 16-06), the dot-0 assertion copies it into `fifth_index` (plan 16-07, `vdp.legacy.sprites.fifth_carry`), and the tail-read fixture row is mandatory: the line-interrupt delay is calibrated until the recorded H counter lands in ticks 600 to 683 (plan 16-02, judged in plan 16-08). No second saved byte.

2. **Does the fixture pin `>= $E0` against `> $E0`?** RESOLVED
   - What we know: the Y=$E8 row separates `$E0..$EF` negative from MAME-SMS's `>= $F0` only; `>= $E0` and `> $E0` differ at exactly Y=$E0 for a 32-line sprite (16x16 magnified, line 0 shows pattern row 15 if Y=$E0 is negative).
   - Recommendation: add that one row to the sprite scenario list and say in the docs that the TI manual's range reads like `> $E0`.
   - Resolution: adopted. The Y=$E0 row is in the scenario list (plan 16-02) and in `vdp.legacy.sprites.y_wrap` (plan 16-07); the docs quote the TI range only after checking the page image (A2), otherwise they say it fits both readings (plan 16-09).

3. **Which fixture is MODE-06's "mode 4 bring-up dots that wrote registers 0 and 1 transiently"?** RESOLVED
   - What we know: no fixture here writes R1 before R0 with display on in a hashed frame (the compare will show); `bios_shape.asm:147-154` writes R0=0 then R1=0 with display latched on, yet the BIOS equivalence test passes today with the fault record compared.
   - Recommendation: add an in-process `vdp.legacy.bringup` case (R1 display on while the nibble is 0, then R0 gets M4) asserting no fault, transient dots equal to the Graphics I oracle and later dots mode 4; treat the subset compare as the "still hash as before" evidence for shipped fixtures.
   - Resolution: adopted. `vdp.legacy.bringup` lands with the renderer (plan 16-04) and the subset compare runs after the skeleton, the renderer, the legacy_modes fixtures, the state change, the sprites and on the phase head (plans 16-03 to 16-09).

4. **Is D-30's budget guard needed?** RESOLVED
   - What we know: the three rider files are classified `operation` or `build`, not `check` (`tests/policy/repository-checks.txt:21,28,53`); only new files in `tests/cmake`, `scripts`, `cmake`, `.githooks`, `tests/compat` need a classification and count only if classed `check`.
   - Recommendation: measure with `cmake -DROOT=$PWD -P tests/cmake/budgets.cmake` after each rider; the cap rises by one line per ten `src/` lines the legacy module adds.
   - Resolution: not needed. Plan 16-01 measures before and after each rider with `budgets.cmake`, adds no `check`-classified file and does not raise the cap.

5. **Should the two shell-only rider tests run in every lane?** RESOLVED (plan-level deviation)
   - What we know: lane placement decides where CI minutes are spent (hygiene lane is one Linux job; fixtures tests run in every build leg).
   - Recommendation: dirty-tree and fake-curl tests in the hygiene lane (precedent `policy.pins.self_test`); the WlaDx retry test beside `fixtures.wla_pin.self_test`.
   - Resolution: all nine rider tests, the WlaDx retry tests included, go in the hygiene lane (plan 16-01). They download nothing real, and keeping them out of `tests/fixtures/tests.cmake` keeps the rider plan's files disjoint from the fixture plan's. `docs/testing.md` records why (plan 16-01).

**Plan-level deviations from this research, recorded here so nothing stays open:**
- Pitfall 1, `mode2.sms`: R1=$48 alone with R0=2 selects nibble 6 (`mixed`), not multicolour. The plans change R0 $02 to $00 and R1 $40 to $48 (nibble 4, `vdpmode=multicolor`).
- Pitfall 1, `tests/library/diagnose.cmake`: the research said its lines are canned and unaffected. `CASE=main` (`library.diagnose`, ci and wasm32 lanes) builds a real `mode0.sms` (R1=$40, R0=0) and runs it through the runner, expecting a Graphics I fault group. Plan 16-03 retargets that seed to R1=$50 (text) and its group expectation with the other seeds.
- Pitfall 1, timing: the runner and library seeds are retargeted to the text and multicolour nibbles in plan 16-03, before the renderer, because those nibbles fault both before and after it; `vdp.background.unsupported` and `fault_run` still change in the renderer commit, as D-25 says.
- Pitfall 10: the base hashes file is written from the branch after plan 16-01 (riders change no code that builds the runner or the images), not from an `origin/main` worktree.

## Sources

### Primary (HIGH confidence for what the source states)
- Repository files read this session: `include/cogmaster.h`, `src/vdp_render.c`, `src/vdp.c`, `src/internal.h`, `src/state.c`, `src/gun.c`, `src/fault.c`, `tests/vdp/test_background.c`, `tests/vdp/tests.cmake`, `tests/fixtures/{lines.c,play.c,tests.cmake}`, `tests/cmake/{budgets,hashes,expect_fail,mailbox}.cmake`, `tests/runner/{diagnose.cmake,tests.cmake}`, `tests/bios/test_bios.c`, `tests/api/v14.c`, `tests/ports/test_gun.c`, `tests/core/test_state.c`, `tests/inventory/*.txt`, `tests/policy/{repository-checks.txt,firmware_window_cases.cmake}`, `scripts/{library.sh,fetch-pin.sh,hygiene.sh}`, `cmake/{WlaDx.cmake,CogmasterTests.cmake}`, `pins.txt`, `quality.json`, `CMakeLists.txt`, `.github/workflows/ci.yml`, `docs/*.md` lines named above, `docs/hardware/video.md`, `.planning/preparation/SMS-VDP.md`, `PROVENANCE.md`.
- TI TMS9918A/9928A/9929A Data Manual (Nov 1982), bitsavers mirror URL from CONTEXT: sections 2.2.1, 2.2.2, 2.3.2, 2.3.3, 2.4.1, 2.4.2, 2.4.5, Table 3-2 (pdftotext).
- MAME `src/devices/video/tms9928a.cpp` and `315_5124.cpp` at `1d7a454ed79e` (BSD-3-Clause, `gh api`).
- ares `ares/component/video/tms9918/*` and `ares/ms/vdp/*` at `9408cb43d494` (ISC, `gh api`).

### Secondary (MEDIUM confidence)
- Sean Young, TMS9918A v0.4.2 (archive capture of `smspower.org/uploads/Development/tms9918a.txt`): sections 3.1, 3.3, 4.1 to 4.3.
- Charles MacDonald `msvdp-20021112` (archive capture): sections 6, 14, 15.
- SMS Power "Palette" page (archive capture): the legacy palette table and Game Gear sentence.
- Local `curl --manual` (8.7.1): `--retry` semantics.

### Tertiary (LOW confidence)
- Timing extrapolations (A1) and the inference that no shipped hashed frame contains a bring-up transient (A4).

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH, no new dependency.
- Architecture: HIGH for the dispatch and state mechanics read in the code; MEDIUM for the span and sprite-carry designs, which are recommendations.
- Pitfalls: HIGH for the coupling findings (each read in a file this session); MEDIUM for cost estimates.
- Hardware behaviour: MEDIUM for source statements, LOW for any claim that a selected model matches silicon.

**Research date:** 2026-11-13
**Valid until:** 2026-12-13 (repository mechanics may move with Phase 17 edits; the permissive pins are fixed).
