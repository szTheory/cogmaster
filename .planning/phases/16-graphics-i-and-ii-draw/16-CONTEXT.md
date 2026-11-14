# Phase 16: Graphics I and II draw - Context

**Gathered:** 2026-11-13
**Status:** Ready for planning

<domain>
## Phase Boundary

With Graphics I (mode nibble 0000) or Graphics II (0010) selected, the Master System 1, Master System 2 and Game Gear in
Master System video mode draw the name, pattern and colour tables, the backdrop and the legacy sprites, instead of
recording the unsupported-video-mode fault. Text (0001), multicolour (0100), the mixed modes, the invalid text modes and
native Game Gear video keep the fault. No shipped frame, audio or state pin moves. The stale fault-vocabulary doc lines,
the dirty-tree refusal and the assembler fetch retry land first, as the first plan, so they ride the release.

Scope is fixed by ROADMAP.md and REQUIREMENTS.md (TUNE-12..14, MODE-01..08). The SG-1000 machine, text and multicolour
modes, save states and the diagnose-vocabulary prune (Phase 17) are out.

Method: five parallel research lenses (Graphics I/II, legacy sprites, architecture, verification, riders and legal) and an
adversarial pass that re-checked disputed claims against the code. Every choice below that has no hardware evidence is a
**selected model**: the docs say "model", fixtures pin it, and nothing claims it is a measurement. No permissive TMS9918
test ROM exists and `mame` is not installed, so a fixture proves fidelity to the notes, not to silicon (CM-VIDEO-001).

</domain>

<decisions>
## Implementation Decisions

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
  - *Plan-time sequencing refinement (made during `/gsd-plan-phase 16`, for the owner to see):* step 5 runs as Graphics I and II (plans 04 and 05), then state format 17 and behaviour revision 19 (plan 06), then sprites (plans 07 and 08), then docs (plan 09). Reason: the sprite engine writes new saved fields (`sprite_legacy`, `sprite_size`, `fifth_index`, the widened slots), and rule 4 forbids instance state outside the saved record for even one commit, so the record and the revision bump land, round-trip tested, just before the engine that writes them. D-18 to D-20 and the push cadence (at the draft, after renderer and sprites, after state and docs) are unchanged. This is a planning-order refinement, not a new decision; the owner can reverse it by reordering plans 06 and 07.

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

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Scope and process
- `.planning/ROADMAP.md` Phase 16 - goal, success criteria, notes
- `.planning/REQUIREMENTS.md` - TUNE-12, TUNE-13, TUNE-14, MODE-01..MODE-08
- `.planning/research/NEXT-MILESTONE.md` - the verified-facts and open-model-choices list this discussion settles
- `.planning/LESSONS.md` - CM-VIDEO-001, CM-PERF-001, CM-DOCS-001, CM-CI-001, CM-GSD-002, XFER-025, XFER-012, XFER-013, XFER-021
- `AGENTS.md` (also `CLAUDE.md`) - rules 1, 3, 4, 5, 6, 7, 8, 10, 11, 12 and the Style section
- `.planning/preparation/DECISIONS.md` - settled choices; `.planning/preparation/README.md` - index

### Hardware sources (documents and permissive code only)
- `.planning/preparation/SMS-VDP.md` sections 2, 3, 6, 7, 8, 10 - prep notes, section 7 is rewritten in this phase
- `docs/hardware/video.md` - video notes, lines 60-100 and 73-77
- TI TMS9918A data manual (Nov 1982): `https://www.mirrorservice.org/sites/www.bitsavers.org/components/ti/TMS9900/TMS9918A_TMS9928A_TMS9929A_Video_Display_Processors_Data_Manual_Nov82.pdf` - 2.3.2, 2.3.3, 2.4.1, Table 3-2
- Sean Young, TMS9918A doc: `https://www.smspower.org/uploads/Development/tms9918a.txt` section 4
- Charles MacDonald `msvdp-20021112.txt`: `https://www.smspower.org/uploads/Development/msvdp-20021112.txt`
- MAME `src/devices/video/tms9928a.cpp` and `315_5124.cpp` (BSD-3), pin `1d7a454ed79e`; ares `ares/component/video/tms9918/*` and `ares/ms/vdp/*` (ISC), pin `9408cb43d494`
- `PROVENANCE.md` sections 2, 4, 10 - rows to add

### Code and tests the phase edits
- `src/vdp_render.c`, `src/vdp.c`, `src/internal.h`, `src/state.c` (sprite bounds near lines 277-288), `src/gun.c` (line 137), `src/fault.c`
- `tests/vdp/test_background.c` (line 272 calls `cogmaster__vdp_background`), `tests/ports/test_gun.c`, `tests/api/v14.c` (lines 14 and 21), `tests/fixtures/lines.c`, `tests/fixtures/tests.cmake`, `tests/roms/src/mailbox.inc`, `tests/diag/expected.csv`, `tests/cmake/budgets.cmake`, `tests/cmake/hashes.cmake`, `tests/policy/firmware_window_cases.cmake`
- `scripts/library.sh` (lines 31, 49-51, 225), `scripts/compat-common.sh` (line 88), `scripts/fetch-pin.sh`, `cmake/WlaDx.cmake` (line 15)
- `docs/audio.md`, `docs/embedding.md`, `docs/runner.md`, `docs/testing.md`, `docs/perf-fuzz.md`

## Prior-phase decisions carried forward
- v2.1 Phase 14: behaviour revision 18 is the previous bump; legacy-mode rows were closed as not emulated by design pending this phase (STATE 14-08)
- v2.1 Phase 13: phase branch cut from the milestone tip (STATE 13-01); diagnose line shape changes once (CM-CI-001)
- v2.1 Phase 15: timeouts are re-derived by the twice-the-slowest rule with run IDs

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `tests/fixtures/lines.c`: closed-form per-pixel oracle that prints the SHA-256 of expected pixels, matched by `cogmaster-run --hash` (`CASE=play`), with the `wrong_hash` negative control.
- Fixture families (`vdp_lines`, `vdp_vtable`, `pause_nmi`): one `.asm`, `-D` switches and a `file(GENERATE)` link per image, about 260 to 290 asm lines and 15 CMake lines each.
- RAM mailbox (`mailbox.inc`, 60 records) judged by `tests/diag/expected.csv` rows with `source=derived`.
- `hashes.cmake -DMODE=compare` and `runner.hashes_file`; the `fwin_repo` throwaway-repo test pattern; `vdp_colour` and the `c*5` lattice.

### Established Patterns
- `vdp_masks` (SMS1) gates address AND masks; SMS2 and Game Gear have none. Legacy follows the same split.
- Sprites are scanned lazily at tick 600 and per line; status events assert at their dot.
- Header `format_version` and `behaviour_revision` are `fixed()` fields; any bump refuses all older states.
- Dispatch with `switch`, not function-pointer tables (XFER-071); integer-only, state in the instance.

### Integration Points
- `cogmaster__vdp_render_until` (class once per call), `cogmaster__vdp_finish_field` (`completed.backdrop`), `cogmaster__vdp_backdrop`, the sprite scan entry, the VDP status read in `vdp.c`, `gun.c` render-ahead, `state.c` VDP record and size pin.

</code_context>

<specifics>
## Specific Ideas

- The fixtures' hazard is a self-fulfilling oracle: source the oracle's formulas from a different document than the renderer's comment, say "model" in the docs, and state that the fifth-sprite number and colour-0 collision are guesses.
- Check that "no pin moved" is not blind to a new row that depends on an old pin (g2_late and g2_switch are genuinely new lines).
- If a planner can read which R3 and R4 values the owner's 46 ledger ROMs write, that settles D-09 for good; the agent cannot read the dumps (secret-read guard), so it stays a selected model unless the owner runs a counts-only script, which is not required.

</specifics>

<deferred>
## Deferred Ideas

- Text and multicolour modes (MODE-09), only if the Phase 17 diagnose shows 5 or more rows landing on them.
- The SG-1000 machine (v3.1): revisit the colour-0 collision, fifth-sprite number and R0 bit 3 choices against SG-1000 software then.
- Pruning the dead diagnose vocabulary and the `ci.yml` edit (Phase 17); `runner.diagnose` trimming on emulated lanes; `quality.json` floor and `ir_per_frame` ceiling raises (CI-speed seeds).
- A MAME screenshot comparison (no firmware bytes, `mame` not installed): optional by-eye only, never a gate.
- A TI datasheet-derived oracle for the display-off backdrop (blank colour) if a title is found that needs it.

### Reviewed Todos (not folded)
- `[ci] Post-merge ci follow-ups` (`.planning/todos/pending/post-merge-ci-follow-ups.md`): not about video modes; stays with the CI-speed seeds.

</deferred>

---

*Phase: 16-Graphics I and II draw*
*Context gathered: 2026-11-13*
