# Phase 8 adversarial review of 08-CONTEXT.md

Checked at HEAD bb7d158 against code, tests, MAME BSD-3 (pin 1d7a454ed79e, `sports.cpp`, `paddle.cpp`, `sms_m.cpp`, `sms.cpp`, `315_5124.h`), ares ISC (9408cb43d494, `paddle.cpp`, `sports-pad.cpp`) and SMS Power 3DGlasses. Counted-code budget re-run: `cmake -DROOT=$PWD -P tests/cmake/budgets.cmake` prints 723 check lines / 8279 src lines (agrees with the context). Nothing edited, nothing committed.

Counts: BLOCKING 4, SERIOUS 7, MINOR 9.

## BLOCKING

### B1. The "only exceptions to unchanged tests" list is wrong; plan 2 cannot pass `cmake --workflow --preset ci`

Decision 5 says `tests/api/v3.c:73` (`port2 = 2u`), `tests/api/v11.c` and the cartdb count pins are the only existing-test edits. Growing `cogmaster_input` (32 to 48), `cogmaster_run_result` (20 to 24), `cogmaster_info` (68 to 76) and `cogmaster_cart_info` (528 to 536) also breaks:

- `tests/api/v3.c:78-79`: `in.size--` expects `ERR_STRUCT_SIZE`. With `sizeof in == 48`, tag 47 is now a legal tag (33..47 copy whole fields) and returns OK. A second line of v3.c, not just :73.
- `tests/api/v8.c:22`: `CHECK_EQ_U64(sizeof(cogmaster_run_result), 20u)`.
- `tests/api/v9.c:57`: `CHECK_EQ_U64(sizeof(cogmaster_info), 68u)`.
- `tests/api/v10.c:44`: `CHECK_EQ_U64(sizeof(cogmaster_cart_info), V9_CART_INFO_SIZE + 6u * 4u + 128u)` (= 528).
- `tests/core/test_core.c:886`: `CHECK_EQ_U64(sizeof(cogmaster_info), 68u)` (inside `test_first_released_sizes`).
- `src/internal.h:413,432,433`: `_Static_assert` on `sizeof` of info, run_result and input (a compile failure, and `src/internal.h` is not in plan 2's file list).
- `tests/runner/tests.cmake:47-53`: `expect_probe_form` anchors the whole `--probe` text with `...unsupported_reason=[^\n]*\n$`. Appending `port1_default`/`port2_default` after `unsupported_reason` (decision 7/Defaults) fails `runner.probe` until that regex is edited. Plan 7 lists the file but the context calls the existing tests "stay".

Fix: list every one of these as a stated exception (or relax to `>=`/offset pins in the v8/v9/v10 style Phase 7 used for v10.c), edit `v3.c` `in.size--` to `in.size = 31u` as well as `2u`, add `src/internal.h` and `tests/core/test_core.c` to plan 2, and extend the probe form regex in plan 7.

### B2. The new pending-axis state fields would break an existing `tests/io` test

`tests/io/input_cases.h:109-119` takes every serialized field whose name starts `input_`, writes byte 255 at its first byte, reseals, and requires `ERR_ARGUMENT`. The axes are visited as signed 32-bit with the `sio_i64` XOR-bias trick (`state.h` encodes little endian, value 0 as `00 00 00 80`). Byte 0 = 255 decodes to +255, which is inside the `pending` bound of +-32767, so the load is accepted and the test fails. Decision 5/10 promise `tests/io` shows no diff, and plan 1's one-shot `git diff --quiet origin/main -- tests/io` proves nothing about this.

Fix: name the axis fields so they do not begin `input_` (for example `pending_x1`, `latched_x1`), or choose names and bounds that a first-byte 255 violates. Say so in the context, since the visitor naming is currently unspecified.

### B3. A Sports Pad cross-check refuses states the core itself produces

Decision "State bounds" says `edge == 0 implies step == 0 and val == 0`. By decision 3/Sports Pad machine, `edge = tick` is set only on a RISE. After a reset the wire is high, `edge` is 0, and the first TH fall runs the else branch: `val = -d`, `step = 1`, `edge` still 0. A state saved between that fall and the next rise has `step == 1`, `val != 0`, `edge == 0`, and the load refuses it. The acceptance tests "a state saved mid-sequence loads into a second instance and saves byte-equal" (at step 1) and the fuzz `check_pad()` / loader legs would fail or abort. The architecture lens had `edge` set on every wire change; the synthesis switched to MAME's rise-only edge but kept the lens's cross-check.

Fix: cross-check `step >= 2` implies `edge > 0` (rise sets it) and `edge == 0` implies `step <= 1`; or set `edge` on every wire change and keep the rule. Add the step-1 mid-sequence round trip to plan 3's tests.

### B4. Pending-axis accumulation contradicts its own state bound and can overflow int32

Decision 8 "Where deltas live": `set_input` "adds its axes with a saturating add (int32_t arithmetic...)" and "Hostile INT32 values are refused at range check, so the accumulate cannot overflow". Decision "State bounds": `pending` axes are visited within +-32767. Both cannot hold:

- Per-call refusal does not bound the sum across calls. Two calls of +20000 give 40000 (or after enough calls INT32_MAX with a saturating add), which `state_save` writes and `state_load` refuses. The decision-8 test "two calls before a frame equal one summed call" and the fuzz op table (adversarial deltas then save/load/save equality) would hit this.
- `ports_frame` does `acc[i] = clamp(acc[i] + pending, -1023, 1023)`. If pending can be near INT32_MAX this is signed overflow (UB) before the clamp. The same for `pos + x`.

Fix: accumulate with a clamp to +-32767 (`pending = clamp(pending + d, -32767, 32767)`; each addend is already within +-32767, so the sum fits int32 with no saturating intrinsic). The state bound is then true, `ports_frame` adds an already-bounded value, and the "sum rule" test is stated for |sum| <= 32767. This is the security lens's D4 (clamp, do not let the sum escape); the synthesis dropped the clamp.

## SERIOUS

### S1. Plan sizes exceed the stated 10-file cap, and plans omit files they must touch

- Plan 2: header, io.c, v3, v11, v12, embedding.md, ci/nofp/wasm32 inventories = 9 as listed, but also needs `src/internal.h` (B1), `tests/abi/tests.cmake` (registers `abi.api_v12`; today `tests/abi/tests.cmake:186-189` registers v11), v8.c, v9.c, v10.c, `tests/core/test_core.c`, plus the new poison/exact-malloc test and its registration. About 16 files. Split (struct growth and test relaxation; then the API delta and docs).
- Plan 8: each fixture is `name.asm` plus `name.link` (see `tests/roms/src/battery.link`; the loop at `tests/fixtures/tests.cmake:200-229` needs both). 3 asm + 3 link + manifest + expected.txt + fixtures/tests.cmake + diag/expected.csv + hashes.cmake + ci/wasm32 inventories = 13.
- Plan 10: adds `host/eyes.c` to the `cogmaster-host` target, which is defined in the root `CMakeLists.txt:123`, not `libretro/CMakeLists.txt`. 11 files.
- Plan 7: lists 9 files, but its new runner tests need inventory lines (`ci`, `wasm32`), so 11.
- Plan 9: `tests/cart/tests.cmake` (new fixture identity tests, as `cart.fixture_identity` is registered there for battery.sms only) and `tests/cart/generator.cmake` are not listed; `tests/fixtures/tests.cmake` needs new `ROW=3/4` identity branches beside `battery`/`eeprom`.

### S2. Always-on glasses plus a default `left` host view can distort non-3-D titles, and contradicts the documented design

`bus.c:126` mirrors RAM across `$E000-$FFFF`, so any title that writes to the mirror at `$FFF8-$FFFB` (a clear or fill loop, a variable placed via a mirror address) sets the eye with b0 = 0 or 1. The context claims "a non-3-D title is never touched" because the core reports 0 when nothing wrote it. That holds only for titles that never write there. A title that writes there once and leaves b0 = 0 reports RIGHT on every frame; `left` mode then holds a "left picture" that never arrives. Nothing in the plan measures this (no count over the owner's library of how many non-3-D titles report a non-zero eye). It also departs from `SMS-SYSTEM.md:105` ("SMS1 only") and PITFALLS 12 ("refused or inert rather than silently on"): MAME wires `$FFF8` only in `sms1_state` and only when its `SSCOPE` option is on (`sms.cpp:274`, `sms_m.cpp:506-518`), so MAME never has this false-positive path.

The amendment is defensible on the `$3E` point: SMS Power's "even though the slot itself is not [active]" is about the card slot, not the cartridge bit b6, and `SMS-SYSTEM.md:105` itself says "works with the card slot disabled". It is not justified by primary source on SMS2.

Fix (keeps the owner's "default must be playable" intent): the adapter treats a title as 3-D only after it has seen both eyes on consecutive frames (a pure adapter rule, no core state), and `none`/single-eye runs send the raw frame; and run `cogmaster-run --eyes` once over the local library (a recorded command and exit status) to count false positives. If wrong, who is hurt: users of non-3-D titles with the default libretro option; the hashes are untouched.

### S3. Adding `docs/controllers.md` to the `docs.api_names` DOC list makes the check reject the host-mapping prose

`tests/cmake/repository.sh:64-72` flags every `cogmaster_[a-z0-9_]+` or `COGMASTER_[A-Z0-9_]+` token in a DOC file that the header (or `cmake/version.h.in`, `CMakeLists.txt`) does not declare. The decision puts the libretro option keys (`cogmaster_port1`, `cogmaster_port2`, `cogmaster_3d_view`) and `cogmaster__eyes_anaglyph` in the host-mapping text. These match the regex (the double underscore too) and fail. Fix: document option keys and host functions in `docs/runner.md` (not a DOC file) or keep controllers.md free of such tokens, and add a check line to plan 12's acceptance.

### S4. Inventory lines named for the wrong lanes

- Plan 5 puts `fuzz.regress.pad` in `nofp.txt`. `tests/inventory/nofp.txt` is 23 abi-only lines; the lane registers no fuzz tests, so `cogmaster__check_inventory` fails configure on that lane. Correct set: `ci`, `wasm32` (`fuzz.regress.*` are there), `fuzz` for `fuzz.libfuzzer.pad`.
- Plan 10 puts the `tests/host` eyes test only in `ci`. `host.palette` is in both `ci.txt:191` and `wasm32.txt:193` because `host/` builds on the wasm32 runner; a new `host.eyes` registered the same way must be in both or the wasm32 lane fails configure.

### S5. The hot-path change in plan 4 is first seen by the perf gate four plans later

Dispatch 1 comes after plans 1-2; plan 4 (`bus.c` RAM-write branch, the only change on the hot path) is first measured at dispatch 2 after plan 8. The risks list says "the perf gate sees plan 1 and plan 4 first", which the schedule does not do. If `delta_bp` moves, plans 5-8 are stacked on it. Fix: move the glasses plan into wave 1 before dispatch 1, or add the dispatch after plan 4.

### S6. Motion for the 16 owned titles cannot be judged the way the context says

Decision "Defaults, runner and ledger" bullet 3 and the Risks say the 7 device dumps are "judged locally ... as ledger rows". `scripts/library.cmake:50` and `tests/compat/tests.cmake:50` accept only `script=none` with the empty-script SHA-256, so a ledger row can carry no delta script. A ledger run proves a boot or a frame hash, not tracking. Say how motion is judged (an agent-run `cogmaster-run --input <delta script>` with a recorded command and exit status, or a RetroArch session the agent runs per rule 6), or drop the claim.

### S7. Plan 6 test pins collide with plan 9's new rows

Plan 6's "every table row resolves its default" and any count it asserts (paddle 4, sports 3) change when plan 9 adds the sports and paddle fixture rows (5 and 4). Plan 9 lists `test_cartdb.c` only. Write plan 6's test count-agnostic, or list it in plan 9.

## MINOR

### M1. Glasses rule constants are not MAME's
MAME's cutoff is `vpos < m_main_scr->height() >> 1` (`sms_m.cpp:526`), where `height()` is the screen's total height (262 NTSC, 313 PAL; `sms.cpp:571-578`, `315_5124.h:37-38`) and the active area starts at vpos 43 (NTSC 192-line, `TBORDER_START 16 + NTSC_192_TBORDER_HEIGHT 27`). That puts the override threshold at active line 88 (NTSC), about 86 (PAL), not 96, and it varies by video mode. Also `bus.line >= 192` includes line 192 ticks 0..606, which is the END of the window (after the active area, so by the context's own argument it belongs to the next picture), and the up-to-68-tick overshoot past the deadline. Fix: state 96 as the core's own choice in `docs/controllers.md`, or compare the window-relative tick (`ticks - (deadline - frame_ticks)`) with no 64-bit divide.

### M2. The paddle "cannot phase-lock" claim is true only for exact lock
With a 299-tick half period (598 full), a loop of N cycles advances the phase 3N mod 598 per sample. N = 199 gives 597, i.e. -1 per sample: 256 samples cover 256 ticks (less than half a period), so the ones ratio can be 0 to 100 percent. The sweep list {29, 31, 47, 64, 97, 100, 101, 200} omits 199 (and 398), so "kept only if every swept length gives 30 to 70 percent" cannot detect it. N = 200 (+2/sample) is fine. No known title polls at 199 cycles. Fix: reword the claim, or add 199 as a documented near-lock case outside the pass criterion.

### M3. The "state refused by an export instance" test is vacuous
`state.c:112-117` fixes machine, region, tv and video_mode in the header, so a Paddle state from a Japan run is already refused by the header, not by the new availability cross-check. To exercise the cross-check, craft a same-profile state with `port1 = PADDLE` on an export instance (record plus reseal).

### M4. The "step 1 or 3 restart" attack in the Risks section is unreachable
Edges alternate, so a fall can occur only at steps 0, 2 or 4, never at 1 or 3. The only guard needed is step 2 (as written). State this and drop the attack, or the test will pass by construction.

### M5. Adapter overflow handling
`libretro.c:291` ignores `cogmaster_set_input`'s result. A libretro mouse delta can be -32768 (int16) and the summed stick, D-pad and mouse terms can exceed 32767; the whole call is then refused and the buttons stay stale. Add an adapter clamp to +-32767, or have the core clamp per call (security lens D4).

### M6. Pending axes survive a soft reset; document it
Decision says pending axes survive a soft reset and load zeroes them. A host that sent a delta just before reset applies it after; fine, but state it in `docs/controllers.md` and test it so a later refactor does not change replay behaviour.

### M7. Fixture SHA-1 rows lock the assembler output
Plan 9 pins each new fixture's SHA-1 in `fixture_facts.txt`/`cartdb.inc`; any later review-fix edit to plan 8's `.asm` invalidates the row and fails the build-time `identity_check`. `owned_fixture` in `tools/cartdb.c:429-436` also hard-codes `bytes == 65536`, `persistence == 2`, `geometry == 1`; the plan says the shapes are extended, but the 65536 bound is not mentioned. Order plan 9 after the last `.asm` edit and note the `bytes` rule.

### M8. Hygiene-duration recording is an invented ritual
"Record `ctest -R hygiene.tree` duration at start and end" (decision 10/Process) has no gate or consumer; it comes from the devops lens as a heads-up. Keep it as one line in the closeout summary or drop it (rule 8).

### M9. Fixtures still depend on the sweep constant
Plan 3 hard-codes `PADDLE_HALF_TICKS 299` and plan 5 fuzzes it; the constant is only judged in plan 8. If the sweep moves it, plans 3 and 5 tests that mention 299 or 598 change. Define the constant once in `internal.h` and have tests use the name.

## Checked and found sound

- (a) Pad-only frame and audio hashes: the test-card path and the PAD arm are unchanged by construction. `ports_pins` for PAD equals `~p & 0x3F` and port 2's bits 0-1/2-5 mapping matches `port_dc`/`port_dd` (`io.c:107-159`). `tests/io/test_io.c` and `input_cases.h` use `sizeof in` and keep passing apart from B2. The `bus_write` compare count is unchanged (`addr >= 0xFFF8` replaces `addr >= 0xFFFC`), so `delta_bp = 0` is plausible; per-read cost is one switch.
- (b) The th_changed split is byte for byte: `th_pins` depends on `ctrl` and `region` only (`io.c:117-130`), so evaluating `old = th_pins(m, old_ctrl)` after the assignment gives the same value as today. The WIRE function (direction input reads 1 else the level bit) does advance a Sports Pad on a Japan profile, where the echoed readback would not. Edge order matters only for Phase 9.
- (c) The 48-byte layout has no padding on any lane; `min(tag, ours) & ~3u` into a zeroed local, validate, then commit leaves no over-read for tags below 48; `check_size` still bounds the zero-tail scan by 4096. `cogmaster_load` zeroes `pending_input`/`input` (`instance.c:239-240`), `cogmaster_reset` does not (confirmed), and the context's `io_reset` placement of the edge clear covers `bus_origin` zeroing `ticks`.
- (d) Tick-stored edge: `edge <= time.ticks` with the header `ticks` cap 2^62; `io_reset` clears it on load and both reset kinds; `bus_origin` precedes `boot_handoff` in both callers.
- (e) MAME `sports.cpp` matches the step machine (fall latches, rise advances, timer armed on every rise, cancelled on fall, 1536 ticks = 512 Z80 cycles) and the negated delta; the `0x80` base and wraparound are MAME artefacts the context correctly leaves out. `paddle.cpp` half period is 300 ticks (`XTAL/3/100` toggles every tick) and D4 = button, D5 = clock, as the context says; ares toggles every 596.6 ticks as coded, as the context says.
- (f) The region rule touches one row (Woody Pop, 32768 bytes, `ctrl1 = 2`, region 0, SHA-1 beginning `b74078c4`); the other three paddle rows already carry region 2 and the three sports rows need no region. No ledger row, frame pin or audio pin references it (`tests/compat/ledger.csv` has five fixture rows only). The rule only runs on probe and AUTO resolution, so it is deterministic. The profile change turns the FM unit on for that dump (Japan AUTO), which the docs should say.
- (g) Budget: 723 counted / 8279 src lines, 104 spare; the four device-token edits are in-place regex changes. Doc caps are per file at 38,912: embedding.md has 4.9 KB, runner.md 10 KB, conformance.md 9 KB, testing.md 7 KB, so the stated additions fit. `tests/fuzz` total is far under 524,288. `quality.json` floors only rise.
- (h) No owner verification, no new CI job and no quota is introduced; the three dispatches plus one nightly are schedule, not ceremony, as long as M8 stays a single line.
- (i) Dropped lens items worth restoring: the security lens D4 clamp (B4) and the devops note that `tests/core/test_core.c` first-size expectations may need touching (B1); the hardware lens's SMS1-only gate was dropped deliberately (S2).
