# Phase 9: Light Phaser - Adversarial pass

Attacked: `09-CONTEXT.md` at HEAD 54631cb on `phase/09-light-phaser`, against the code, the five lens reports and MAME at pin
1d7a454ed79e (BSD-3, `gh api` only: `lphaser.cpp`, `sms_m.cpp`, `315_5124.cpp`). Nothing GPL or LGPL was opened. Counts:
**2 BLOCKING, 7 SERIOUS, 9 MINOR.** Everything not listed below was checked and holds (see "Verified and holding").

## BLOCKING

### B1. Plan 1 is not inert: `fuzz.regress.pad` goes red at plan 1 (and again at plan 3)
- Evidence: `tests/fuzz/fuzz_pad.c:59-66` has `device_table[6] = COGMASTER_DEVICE_SPORTS_PAD + 1u` (that is 4, the new phaser);
  `fuzz_pad.c:239-241` models the call as `range = ... in.port1 <= COGMASTER_DEVICE_SPORTS_PAD ...` and expects
  `ERR_ARGUMENT` for 4; `tests/fuzz/seeds.c:63` seeds `pad_set(0u, 6u, ...)` "an unknown device is refused" (selector 6 is that 4).
  Once plan 1 raises `COGMASTER__DEVICE_LAST` to 4, `cogmaster_set_input` answers `ERR_PROFILE_REJECTED` for device 4
  (`io.c:84-89` range passes, `device_ok` refuses). The model disagrees, `op_set_input` calls `abort()`, and `fuzz.regress.pad`
  (registered for every lane, `tests/fuzz/tests.cmake:93-104`) fails. At plan 3, `device_ok` accepts the phaser on SMS and the
  independent `attachable()` (`fuzz_pad.c:135-147`, `default: return 0`) disagrees again. Plan 5 (the only plan that lists
  `fuzz_pad.c`) is two waves later. CONTEXT's "Existing tests and asserts that must change" table omits the file.
- Fix: add `tests/fuzz/fuzz_pad.c` and `tests/fuzz/seeds.c` (comment at 281-287; the seed stays valid as a model-checked
  call) to plan 1: `range` compares against `COGMASTER_DEVICE_LIGHT_PHASER`, `device_table[6]` becomes `LIGHT_PHASER + 1u`
  (5, still out of range). `attachable()` moves to plan 3 with `device_ok`. Plan 1 then has 11 files (the nine plus `fuzz_pad.c`
  and `seeds.c`); `docs.api_names` needs `embedding.md` in the same plan as the header, so split instead: plan 1 keeps the header,
  `internal.h`, `embedding.md`, `v12.c`, `v13.c`, `abi/tests.cmake` and the three inventories (9) and the fuzz edits go in as a tiny
  plan 1b (`fuzz_pad.c`, `seeds.c`), or fold `nofp.txt` into the same commit as `abi/tests.cmake` and count by plan, not by commit.

### B2. Plan 3 breaks the existing `ports.state.*` tests (independent checker, outside table)
- Evidence: `tests/ports/test_state.c:419-428` `available()` returns 0 for any device above 3; `model_ok()` (432-441) requires
  `active_* == 0` for every port unconditionally (`ok = ok && s->act[p][0] == 0 && ...`); the outside table at 604-605 has
  `{SPORTS, "device 4 pending", 1, ...}` and `{SPORTS, "device 4 in force", 1, ...}` expecting refusal, and 592-593
  `"active x1 not zero"` on a Sports Pad (still refused, fine). Plan 3 makes `device_ok` accept the phaser on every SMS
  profile and makes `active_*` legal for a held phaser, so `load_and_judge` reports "refused a state the checker holds" in the
  mutation, sweep and outside cases. CONTEXT's table names `tests/core/test_state.c` only; plan 3's file list has no
  `tests/ports/test_state.c`; plan 5 (wave 2) edits it. Plan 3's stated verification ("`ports.state` round trip", whole
  `ci` workflow before the wave closes) cannot pass.
- Fix: move the checker edits to plan 3: `available()` knows the phaser (not on a Game Gear), `model_ok` allows `act` for a held
  phaser in the canonical range and keeps 0 for every other device, outside rows become "device 5 pending/in force"; add them
  to the existing-tests table. Plan 3 then lists: `ports.c`, `io.c`, `internal.h`, `state.c`, `test_gun.c`, `ports/tests.cmake`,
  `test_state.c`, `fuzz_pad.c` (B1), `ci.txt`, `wasm32.txt` = 10 (at the cap; drop `ci.txt`/`wasm32.txt` by registering
  `ports.gun.hostile_aim` as the plan-4 test if a file must go).

## SERIOUS

### S1. Two guns: edges are not merged in time order, so schedule independence fails for Gangster Town
- Evidence: Decision 5 keeps one cursor per port and `cogmaster__gun_sync` "brings every phaser port's cursor up to the beam"
  (a sequential loop). The H latch is one register (`vdp.c:260-263`). If port 1 has an edge at tick 400 and port 2 at tick 300
  of the same row and a sync arrives at 500, port 1 latches 400 and port 2 then overwrites with 300; a poll at every
  instruction processes 300 then 400. The saved `hcounter` and the later `$7F` value differ, and so do the hashes of a program
  that reads it. Decision 7 makes the Gangster Town row attach a phaser to both ports by default, so this is the shipped
  two-player case; `ports.gun.walk` and fuzz leg B as written use one gun.
- Fix: process by minimum next-edge tick across the two cursors (a two-way merge, a few lines), and add a two-gun case to
  `ports.gun.walk` and to leg B with aims that put both guns' edges in the same sync interval.

### S2. Fixture aim is off by one: `ax = SQ_X + 15` is dot `SQ_X + 14`, so the "row independent" claim is false
- Evidence: Decision 1 makes aims 1-based (dot = x - 1). Decision 10 aims "at its RIGHT edge dot (`ax = SQ_X + 15`, `ay = SQ_Y + 8`)".
  The right edge dot is `SQ_X + 15`, which is `ax = SQ_X + 16`. With the written value the centre dot is `SQ_X + 14`; the dy = +-6
  rows hold only that one dot (row widths 13,11,11,11,9,7,1, checked by enumeration: 113 dots), so those two rows end at
  `x_off = SQ_X + 15`, not `SQ_X + 16`, and the latched H differs on 2 of 13 rows (by 0 or 1 count). The text's own derived
  numbers assume the right dot: "the leading edge is 3.5 counts early on the centre row" is a 7 dot run (dots 9..15 of the square),
  while the written aim gives 8 dots (4 counts). The V range ("rows `SQ_Y + 2 .. SQ_Y + 14`") has the same 0-based/1-based mix
  (the written `ay` is row `SQ_Y + 7`).
- Fix: `ax = SQ_X + 16`, `ay = SQ_Y + 9` (dot row `SQ_Y + 8`), V rows `SQ_Y + 2 .. SQ_Y + 14`, and have the cmake oracle assert
  `x_off` per disc row equals `SQ_X + 16` for all 13 rows from the same half-width loop. State the 1-based conversion once.

### S3. `policy.no_title_table` as designed will fail on gun.c's own mandatory comments and is too narrow for the criterion
- Evidence: Decision 10 scans `src/gun.c` for `cart`, `sha1`, `crc`, `product`, `serial`, `title`. Decisions 3, 4 and the hardware
  comment rule require comments in `gun.c` that cite MAME, the Hang-On title screen, "13 flagged dumps", "titles"
  and "cartridge". An awk with no comment stripping fails on them; with stripping it needs more than the allowance of about 5
  lines. ROADMAP criterion 2 says "a check finds no per-title offset table in `src/`"; the sensor hooks also live in `io.c`,
  `vdp.c`, `ports.c`, `frame.c`, where a keyed branch would not be scanned.
- Fix: strip `/* */` and `//` before matching, match code tokens (`->cart`, `cartdb`, `sha1`, `crc32`, `product`), scan
  `gun.c`, `ports.c`, `io.c`, `vdp.c`, `vdp_render.c` (frame.c legitimately tests `m->cart.rom`), and budget about 10 counted
  lines (limit is 879, used 723; verified below). Keep the self-test (`static const uint8_t offs[] = {9, 19, 15};`).

### S4. The sticky "seen" bit has no expiry and is described as MAME's bit; MAME's is gated and cleared in three places
- Evidence: MAME `sms_m.cpp:309-317, 330-338`: a sticky TH-low shows only `if (m_vdp->hcount_latched())`, is cleared by the read,
  and `315_5124.cpp:447-450` clears `m_hcounter_latched` on every `$7F` read and `:684` at the end of the top border each
  frame. The lens (hardware 1.3) records this. CONTEXT Decision 3 says "MAME carries the same bit" and defers the `$7F`
  clear. In cogmaster `hheld` never clears, so porting the gate literally is a no-op, and `seen` persists until the next `$DD`
  read, possibly frames later: a poll after a long gap reads TH low for a pulse from an earlier picture.
- Fix: clear `seen` at the wrap (tick 600 of the last line, already a sync site; deterministic and cheap), say in
  `docs/controllers.md` that this approximates MAME's top-border clear, and make cross-check (d) require `seen == 0` when
  `g_scan == 0` at a wrap boundary or leave it free but test the clear. Reword Decision 3 ("a bit with the same purpose as MAME's
  `th_latch`, not gated by `hcount_latched`").

### S5. Decision 3 misstates MAME's release rule; the "plays the 13 dumps" rationale transfers only for solid-bright frames
- Evidence: `lphaser.cpp:213-221` (`bright_aim_area`): once the sensor is on it stays on "until out of the aim area" and the check
  is rescheduled at `aim_area.max_x + 1`, the chord end of the aim disc, not the end of the lit run. CONTEXT says MAME
  "releases it at the end of the lit run". The Lens-conflicts table does record "pixel-following" as the choice, but Decision 3
  still presents MAME as the support for the latch position. For a uniformly bright frame (what Phaser games draw) both rules
  give the chord end; for the fixture's square at its right edge they differ (MAME would release at the chord end, up to 6
  dots beyond the square). The fixture therefore pins a rule MAME does not follow.
- Fix: say so in Decision 3 and `docs/controllers.md`; keep the pixel-following rule (GUN-01 says "emitted pixel") but record
  that agreement with MAME is for bright frames only, and add one fixture row on a full-width bright band where the two rules
  coincide, so a local judge can separate "edge rule" from "shape rule".

### S6. GUN-02 amendment: the masked comparison must also mask device words and axes, and the invariant is thin where parked
- Evidence: Decision 6(a) masks "gun fields BY NAME" but a run with the phaser differs from the pad run in `input_port1/2`
  (pending and active records, `state.c:707-708`) and in `pending_*`/`active_*` when an aim is sent; only `port_gun_*` are
  named. With aim `0 0` the scanner is off by construction, so the "parked" half of the invariant is true by design and
  proves nothing about the scanner; only the "aim over dark dots" half exercises it.
- Fix: list the masked names exactly (`input_port1`, `input_port2` x2, `pending_*`, `active_*`, `port_gun_*`) and make the dark-aim
  case the one counted in the requirement evidence; keep the lit-square control.

### S7. `$DD` "seen" clear and `th_pins` hot-site wording hides a side effect on a read path that tests treat as pure
- Evidence: `src/ports.c:1-10` and `io.c:208-225` say reading `$DC`/`$DD` "cannot move a device"; `tests/ports/test_ports.c` and
  `ports.devices.hammer` rely on a pure read. Decision 5/GUN-01 add `gun_sync` plus a `seen` clear inside `chip_in`. The clear is
  idempotent on a second read, but `cogmaster_peek` and any tool that calls `port_dd` for observation would change state. `port_dd`
  is static and only `chip_in` calls it today, so the claim holds now; it needs a guard.
- Fix: put the clear in `chip_in` only (as written), add a one-line `ports.gun.peek` assertion (`cogmaster_peek` and
  `cogmaster__ports_pins` leave the block byte-identical) and keep the comment in `ports.c` accurate.

## MINOR

- M1. `ports.gun.edge` reads `$7F` at E+1 .. E+700; the next row's release edge falls at E+684, so any lit shape whose next row ends at a
  different dot changes the value inside the window. Use E+1 .. E+683 or state "square with a vertical right edge".
- M2. `# Timeout:` sizing after dispatch 1 names `sh scripts/ci-durations.sh ci.yml` but no plan lists `.github/workflows/ci.yml`, and the
  repo convention in `tests/ports/tests.cmake` is `TIMEOUT 20` floors, not 10. Either list the workflow in a plan or drop the sentence.
- M3. "no `const` arrays in the core (XFER-071)" is false at HEAD (`synth.c:17` kernel, `crc32.c:7`, `cart.c:34-39`, `io.c` `gg_reset`). It is a
  self-imposed rule for `gun.c`; word it so and do not cite XFER-071 as a core-wide rule in a comment.
- M4. Last-wins parks the aim on any later axis-free `set_input` in the same frame (a tag-32 or 35-byte call). Phase 8's summing is harmless
  on that; here it parks. Say so next to "send each increment once" in `docs/controllers.md`.
- M5. The frame-end sync must sit inside the `if (m->cart.rom != NULL)` branch of `cogmaster_run_frame` (`frame.c:60-62`); the test-card branch
  has no beam. State it so plan 4 does not call `gun_sync` on a card.
- M6. `docs/hardware/video.md` describes the stamped-mutation render model; the gun's early render (the neutrality lemma) belongs there, not only in
  `controllers.md`.
- M7. Gun behaviour is documented only in plan 12 although behaviour lands in plan 4 (rule 5, "tested and documented together"). Phase 8
  did the same inside one phase; acceptable, but name it as an exception or add the `docs/controllers.md` section to plan 4.
- M8. "Dispatch 1/2/3" numbering reads like formal slots (rule 8). Say "dispatch `ci.yml` when the first wave is on the branch, after the
  fixture, and on the final head" without counts; the perf comparison is made by the pull request gate at ship as well
  (`docs/perf-fuzz.md` gate section), so the manual `delta_bp` calculation is a local reading, not a requirement.
- M9. The one-shot "remove the `port_gun_dot` bound and the test fails" control is a mutation run recorded once; fine under rule 6 as long as it
  stays a recorded command and exit status and is not listed as a gate.

## Verified and holding (no action)

- (a) No frame, audio or `tests/io*` pin moves: `th_pins` gets a mask that is 0 without a gun; `vdp_hcount_at(bus.t)` is the same
  formula; all guards are one branch at `$DD`/`$7F`/`$3F`/wrap/frame end. Boot_to_play is 2,146,896 Ir/frame, 1% is 21,469; the hooks
  are tens of instructions. `scripts/library.cmake` always passes `--port1/--port2` from the key, and a bare runner run of a table
  phaser dump has aim `0 0` (the script is all zeros), which equals a pad run by the GUN-02 invariant.
- (b) Early render is idempotent: `vdp_write` renders `< t/2` first (`vdp.c:371`), the status read `<= t/2` (`vdp.c:273`), the line event
  renders the whole line at 512, sprites are scanned at tick 600 of the previous line, `hscroll/display` latch at 600, `vscroll/height`
  at the wrap; a prefix render at T uses the same registers, VRAM and CRAM the next mutation would use. At the frame boundary (192,607+overshoot)
  line 192 of a 224/240 picture was already rendered by its 512 event, and a 192-line picture has `line >= height`, so the frame-end
  sync renders nothing. The unsupported-mode wait does not strand rows: the 512 events still render every row before the wrap.
- (c) Arithmetic: 13 dots = 26 ticks, 8.67 cycles; `in a,($DD)/bit 6/jr` is 31 cycles; `H = (x_off + 35) >> 1` for `x_off <= 256` and
  `550 + 32 < 592` (re-run for 0, 1, 100, 135, 136, 255, 256); luma weights light 32 of 64 colours and the four boundary values
  125, 126, 128, 129 are right; `th_wire` (Sports Pad) is separate from `th_pins`, `ports_th` is not called for sensor edges.
- (d)/(e) 24-byte block has no padding on any lane (8+4+2+1+1+8); `ports_frame` per-port zeroing, `set_input` canonicalisation, load zeroes
  and soft reset keeps pending (`instance.c:239`, `cogmaster_reset`), snapshot-at-wrap matches the window origin (window starts at line 192
  tick 607, the wrap at tick 600 of the last line), attach at a frame start is scanned in the same window.
- (f) Dropping the Game Gear is right: `is_gg` returns pins `0xC0` (no TH), port index 1 is mapped to NONE on a GG, MAME models GG TH only as a
  TODO, the Phaser has a DE-9 connector. The GUN-02/03 amendments follow the Phase 8 PAD-04 precedent and keep the owner's intent
  ("sensed from the emitted pixel", "crosshair outside the hashed frame"); the weakening in GUN-02 is the hardware's own behaviour
  (sensor ungated by the trigger), as the lens shows.
- (g) 13 `ctrl1 = 1` rows, all `machine = 1`, `region = 0`; Gangster Town at `cartdb.inc:578` has `ctrl2 = 1`; code 4 stays a pad;
  `test_cartdb.c` counts and `fixture_facts.txt` rows are untouched; no committed ROM is a phaser dump.
- (h) `budgets.cmake`: `723 check lines / 8795 src lines`, rule `counted * 10 < src`, limit 879 (880 * 10 = 8800 >= 8795); the "735" was
  Phase 8's allowance. `repository.cmake` is a generic dispatcher (the lens's "dispatch line" is not needed). Docs headroom
  (controllers 9,867, embedding 35,243, testing 34,159 of 38,912; header 9,708 of 24,576) holds. Plans 2, 4 to 12 keep within 10 files once B1/B2 are
  folded in. `tests/inventory/hygiene.txt` needs no edit (`policy.no_title_table` is not in the hygiene lane).
- (i) No owner verification is smuggled in; no new CI job; RetroArch stays under the 300 line cap; no planning ID appears in a code or doc
  instruction (the `DEC.15` mention is CONTEXT only; keep it out of comments).
- (j) Lens items dropped by the synthesis, each with a reason in CONTEXT: security F2 (tag-32 caller rejected, covered by the 1-based zero),
  security F4 clamp band (replaced by one parked state), hardware sprite and dark-grey layers (sprite layer deferred), devops
  "GG in SMS mode fixture" (GG refused). The one lens finding not carried is the hardware/MAME `$7F` clear and top-border clear of the
  sticky bit (S4 above).
