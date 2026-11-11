---
phase: 09-light-phaser
verified: 2026-11-11T00:00:00Z
status: passed
score: 4/4 must-haves verified
covered_files:
  - ".planning/phases/09-light-phaser/09-01-PLAN.md"
  - ".planning/phases/09-light-phaser/09-01-SUMMARY.md"
  - ".planning/phases/09-light-phaser/09-02-PLAN.md"
  - ".planning/phases/09-light-phaser/09-02-SUMMARY.md"
  - ".planning/phases/09-light-phaser/09-03-PLAN.md"
  - ".planning/phases/09-light-phaser/09-03-SUMMARY.md"
  - ".planning/phases/09-light-phaser/09-04-PLAN.md"
  - ".planning/phases/09-light-phaser/09-04-SUMMARY.md"
  - ".planning/phases/09-light-phaser/09-05-PLAN.md"
  - ".planning/phases/09-light-phaser/09-05-SUMMARY.md"
  - ".planning/phases/09-light-phaser/09-06-PLAN.md"
  - ".planning/phases/09-light-phaser/09-06-SUMMARY.md"
  - ".planning/phases/09-light-phaser/09-07-PLAN.md"
  - ".planning/phases/09-light-phaser/09-07-SUMMARY.md"
  - ".planning/phases/09-light-phaser/09-08-PLAN.md"
  - ".planning/phases/09-light-phaser/09-08-SUMMARY.md"
  - ".planning/phases/09-light-phaser/09-09-PLAN.md"
  - ".planning/phases/09-light-phaser/09-09-SUMMARY.md"
  - ".planning/phases/09-light-phaser/09-10-PLAN.md"
  - ".planning/phases/09-light-phaser/09-10-SUMMARY.md"
  - ".planning/phases/09-light-phaser/09-11-PLAN.md"
  - ".planning/phases/09-light-phaser/09-11-SUMMARY.md"
  - ".planning/phases/09-light-phaser/09-12-PLAN.md"
  - ".planning/phases/09-light-phaser/09-12-SUMMARY.md"
  - "CMakeLists.txt"
  - "README.md"
  - "docs/conformance.md"
  - "docs/controllers.md"
  - "docs/embedding.md"
  - "docs/hardware/video.md"
  - "docs/runner.md"
  - "docs/testing.md"
  - "host/aim.c"
  - "host/aim.h"
  - "include/cogmaster.h"
  - "libretro/libretro.c"
  - "libretro/options.h"
  - "quality.json"
  - "runner/args.c"
  - "runner/args.h"
  - "runner/input.c"
  - "runner/input.h"
  - "runner/main.c"
  - "runner/probe.c"
  - "scripts/compat-common.sh"
  - "scripts/library.cmake"
  - "src/cart.c"
  - "src/frame.c"
  - "src/gun.c"
  - "src/internal.h"
  - "src/io.c"
  - "src/ports.c"
  - "src/state.c"
  - "src/vdp.c"
  - "src/vdp_render.c"
  - "tests/abi/tests.cmake"
  - "tests/api/v12.c"
  - "tests/api/v13.c"
  - "tests/built/expected.txt"
  - "tests/cart/test_cart_device.c"
  - "tests/cmake/hashes.cmake"
  - "tests/cmake/repository.sh"
  - "tests/compat/ratchet.cmake"
  - "tests/compat/tests.cmake"
  - "tests/diag/expected.csv"
  - "tests/fixtures/tests.cmake"
  - "tests/fuzz/fuzz_pad.c"
  - "tests/fuzz/seeds.c"
  - "tests/host/test_aim.c"
  - "tests/host/tests.cmake"
  - "tests/inventory/ci.txt"
  - "tests/inventory/nofp.txt"
  - "tests/inventory/wasm32.txt"
  - "tests/libretro/controls.h"
  - "tests/libretro/gun.h"
  - "tests/libretro/host.c"
  - "tests/libretro/tests.cmake"
  - "tests/policy/tests.cmake"
  - "tests/ports/test_gun.c"
  - "tests/ports/test_state.c"
  - "tests/ports/tests.cmake"
  - "tests/roms/manifest.txt"
  - "tests/roms/src/light_phaser.asm"
  - "tests/roms/src/light_phaser.link"
  - "tests/runner/tests.cmake"
covered_digest: "v4:sha256:329b7cc6de9ee016b503a495b03fd7b3fabea773de5ca627378210927be4f6ca"
behavior_unverified: 0
overrides_applied: 0
---

# Phase 9: Light Phaser Verification Report

**Phase Goal:** Light Phaser titles hit where the player aims, sensed from the emitted pixel at the beam position with H latched at the edge tick, without changing any run that has no gun or fires no shot.
**Verified:** 2026-11-11
**Status:** passed
**Re-verification:** No, initial verification
**Branch / HEAD:** `phase/09-light-phaser` at 6a1df6a (55 commits over `origin/main`)

Human verification items: none. Every check below is a test, a command I ran, or a hosted run. One item is a
documented hand-off and is not a gap (see "Carried hand-offs"); `human_needed: false` for phase completion.

## Goal Achievement

### Observable Truths (ROADMAP success criteria, criteria 3 and 4 as amended)

| # | Truth | Status | Evidence |
|---|-------|--------|----------|
| 1 | A self-authored fixture lights a known square and polls TH at several loop lengths, and the latched X matches within the stated tolerance at every length, while the existing latch-position tests stay unchanged (GUN-04) | VERIFIED | `tests/roms/src/light_phaser.asm` (square at `SQ_X 96`, `SQ_Y 40`) is built from source; its oracle in `tests/fixtures/tests.cmake` derives every expected H from the `.DEFINE` positions and the sensor rule (`H = (dot + 35) >> 1`), not from a run, and checks each literal in `tests/diag/expected.csv` against it. Ten loop lengths (29 31 47 64 97 100 101 200 114 228 cycles), polled after TH high and after a poll, 20 rows, each judged within plus or minus one H count; `fixtures.light_phaser.mailbox.export/.japan/.pal/.sms1` and `.port2` pass. I ran it by hand: all 20 records read `0049` (closed form `(96 + 16 + 35) >> 1 = 73 = 0x49`, error 0); the line reads `0054` (`(128 + 6 + 35) >> 1 = 84`); V counter `002A` inside 0x28 to 0x39. Controls fail as required: `fixtures.light_phaser.control.left/.parked/.pad` (aim 16 dots left reads `0043`, 6 counts away; the others read `FFFF`). Existing latch positions: `git diff --quiet origin/main -- tests/io tests/vdp tests/bus tests/audio tests/testcard_pins.cmake` exits 0; `ports.latch`, `ports.latch_control`, `ports.gun.latch_at` (a differential of the new `vdp_hcount_at` against the old latch) pass; the built/diag/manifest data has 0 removed lines. |
| 2 | The sensor reads the emitted pixel at the beam position and latches H at the edge tick, and a check finds no per-title offset table in `src/` (GUN-01) | VERIFIED | `src/gun.c` (read in full): the lazy sensor reads `m->field[line][dot]` only once `render_line`/`render_x` has passed the dot (`emitted`), brightness is a weighted sum (`dot_lit`), an edge goes to `cogmaster__io_sensor_edge` at `2 * dot + GUN_DELAY_TICKS`; `io.c` (`io_th_changed`, `$DD` pins) and `ports.c` carry the TH level and the sticky bit. Tests `ports.gun.model` (oracle runs), `.edge`, `.luma`, `.order`, `.lag`, `.seen_expiry`, `.peek`, `.th_toggle`, `.walk` (including `walk_height`, the review fix) pass. No table: `policy.no_title_table` scans `gun.c`, `ports.c`, `io.c`, `vdp.c`, `vdp_render.c` for cart, hash, title and checksum tokens and for an initialised array in `gun.c`; its five self-tests (`array`, `strarray`, `hook`, `strings`, `comments`) each pass with a negative control; `ports.gun.identity` is the behavioural backstop (the same latch with different ROM header bytes). I grepped `src/gun.c` for `cart` and `title`: none. |
| 3 | A run with the gun plugged in, no shot and no light in its view has the same frame hash as one without it, and a poll-hammer test shows sensor work bounded per line however often the game polls (GUN-02, wording as amended) | VERIFIED | `ports.gun.noshot` runs four profiles (SMS1 export, SMS2 export, Japan, PAL) for the full frame count: a dark aim (scanner runs the whole disc, `samples > 100`, `edges == 0`) and a parked aim equal a pad run in frame hash, audio hash and the whole state record with the gun's own fields masked by name; controls: a lit aim and a pulled trigger must differ. `ports.gun.hammer`: a million of each of `$DD`, V/H counter and `$3F` at one tick leave the sensor, H counter and render cursor unchanged; a hammer at every tick of a 192-line picture stays within `2 R + 2` samples per row and `disc + 2 R + 1` per picture. `ports.gun.fault_neutral` (no fault recorded earlier or later than without the gun). Runner: `runner.gun.noshot` (parked and dark-aimed runs equal the pad's). By hand: parked and dark `lphaser` runs give the same frame hash and audio hashes (`fef552d9...`, `98f1274d...`) as the pad run. Gun-free runs are untouched: the three review-fix calls in `src/vdp.c` are inside `cogmaster__gun_attached`, perf `delta_bp` 0 (below). |
| 4 | Host tests map aim through the Master System and Game Gear frame rectangles to field-dot coordinates, a runner script aim at the fixture square latches the expected X, and the frame hash is the same with the crosshair drawn or not (GUN-03, wording as amended) | VERIFIED | `host/aim.c` is a pure helper; `host.aim` tests it with Master System rectangles (256 by 192, 224, 240 at 0,0) and Game Gear rectangles (160 by 144 at 48,24 and 48,40). Runner: a script aim IS the field dot (every machine that holds a gun crops at 0,0): `runner.gun.crop` judges the fixture row at the aim and rejects it 16 dots left (hand run above, closed form `0049`); `--port1 lphaser --machine gg` exits 77 with `unsupported: lphaser on port 1 is not available here; the Game Gear has no such controller` (`runner.gun.gg_refused`); an aim of `x 257` or `y 241` in a script is exit 2 naming the port and the controller (`runner.gun.args`). libretro: `libretro.host.gun.aim` (pointer through the crop), `.cursor` (stick step is exactly 4 counts both ways after the review fix), `.crosshair` (every presented picture with the option off equals the library's byte for byte; with it on the library's picture plus at most 17 cross points in the four colours; with no phaser on a port, no cross). The cross is drawn into the presented buffer only, so the library frame hash cannot change. |

**Score:** 4/4 truths verified, 0 present-but-behavior-unverified.

### Per-requirement table

| Requirement | Code | Test | Command run by me | Exit |
|-------------|------|------|-------------------|------|
| GUN-01 | `src/gun.c` (sensor, `GUN_RADIUS/LUMA/DELAY_TICKS`), `src/io.c` (`io_sensor_edge`, `io_th_changed`, `$DD` bit), `src/ports.c` (pins, `device_ok`), `src/vdp.c` (`vdp_hcount_at`, `gun_finish`/`gun_wrap`), `src/state.c` (bounds), `src/cart.c` (table code 1 to `LIGHT_PHASER`), `include/cogmaster.h` (device 4, API 13) | `ports.gun.latch_at/.aim/.pins/.hostile_aim/.model/.edge/.luma/.control/.order/.lag/.seen_expiry/.peek/.th_toggle/.walk/.identity`, `ports.state.*` (mutation, outside, sweep, midsequence, profile_device, gun), `cart.device.*`, `abi.api_v13`, `policy.no_title_table` and its five self-tests, `fuzz.regress.pad`, hosted `fuzz.libfuzzer.pad` | `ctest --test-dir build/ci -R '^(ports\.gun\|ports\.state\|fixtures\.light_phaser\|runner\.gun\|host\.aim\|libretro\.host\.gun\|policy\.no_title_table\|cart\.device\|compat\.\|fuzz\.regress\.pad\|abi\.api_v13\|ports\.latch)' -j8` (61 of 61 passed) | 0 |
| GUN-02 | `src/gun.c` (lazy, bounded sensor), `src/internal.h` (`cogmaster__gun_attached`), `src/state.c` | `ports.gun.noshot`, `.hammer`, `.fault_neutral`, `.walk`, `runner.gun.noshot`, `ports.state.gun`, `fuzz.regress.pad` | same ctest selection; full `ctest --test-dir build/ci -j8` (590 of 590); `cogmaster-run light_phaser.sms --frames 40 --port1 lphaser --input phaser-parked.input --hash` and `--hash-audio` against `--port1 pad` | 0; 0; hashes equal |
| GUN-03 | `host/aim.c`, `host/aim.h`, `runner/args.c`, `runner/input.c`, `runner/main.c` (exit 77), `runner/probe.c`, `libretro/libretro.c`, `libretro/options.h` | `host.aim`, `runner.gun.args/.gg_refused/.crop/.noshot`, `libretro.host.gun.aim/.cursor/.crosshair`, `compat.device_key`, `compat.device_rejected` | same ctest selection; runner by hand: `--machine gg --port1 lphaser` exits 77, `x1=+00257` and `y1=+00241` in a script exit 2, the same script replays to the same hash twice | 0; 77; 2; 2; equal twice |
| GUN-04 | `tests/roms/src/light_phaser.asm`, `tests/roms/src/light_phaser.link`, oracle in `tests/fixtures/tests.cmake`, `tests/diag/expected.csv` (rows of the fixture) | `fixtures.light_phaser.mailbox.export/.japan/.pal/.sms1`, `.port2`, `.control.left/.parked/.pad`, `runner.gun.crop`, `ports.latch`, `ports.latch_control`, unchanged `tests/io`, `tests/vdp`, `tests/bus`, `tests/audio` | same ctest selection; `cogmaster-run light_phaser.sms --frames 40 --port1 lphaser --input build/ci/tests/phaser.input --mailbox`; `git diff --quiet origin/main -- tests/io tests/vdp tests/bus tests/audio tests/testcard_pins.cmake` | 0; 0 |

### Runs I performed (all from the repository root)

| Command | Result | Exit |
|---------|--------|------|
| `cmake --preset ci && cmake --build --preset ci` | configured, `ninja: no work to do` (build current at HEAD) | 0 |
| `ctest --test-dir build/ci -R '^(ports\.gun\|ports\.state\|fixtures\.light_phaser\|runner\.gun\|host\.aim\|libretro\.host\.gun\|policy\.no_title_table\|cart\.device\|compat\.\|fuzz\.regress\.pad\|abi\.api_v13\|ports\.latch)' -j8` | 61 of 61 passed | 0 |
| `ctest --test-dir build/ci -j8` (everything) | 590 of 590 passed (the `ci` inventory and floor; floor is 588) | 0 |
| `cmake -DROOT=$PWD -P tests/cmake/budgets.cmake` | `repository: 737 check lines / 9237 src lines, strict one tenth` | 0 |
| `git diff --quiet origin/main -- tests/io tests/vdp tests/bus tests/audio tests/testcard_pins.cmake` | no diff | 0 |
| `git diff origin/main -- tests/built/expected.txt tests/diag/expected.csv tests/roms/manifest.txt \| grep -c '^-[^-]'` | 0 removed lines | 0 |
| `cogmaster-run build/ci/fixtures/light_phaser.sms --frames 40 --port1 lphaser --input build/ci/tests/phaser.input --mailbox` | ten periods after TH high (`02` to `0B`) and ten after the poll (`0C` to `15`) all `0049`; `1B`, `1C` (line) `0054`; `17` lit `000D`; `1A` TH bit `0040`; `18`, `19` V counter `002A`; `01` parked `0000`, `16` dark `0000`; magic, crc and status ok | 0 |
| the same with `phaser-parked.input` (lphaser), `phaser-dark.input` (lphaser) and `phaser-parked.input` (`--port1 pad`) | every period record `FFFF`, `1B` `FFFF`, `17` `0000`, `1A` `0000` | 0 / 0 / 0 |
| the same with `phaser-left.input` (aim 16 dots left, the control) | periods `0043`, `1B` `004C`, outside the rows | 0 |
| `--frames 1 --port1 lphaser --machine gg` (and with the script) | `unsupported: lphaser on port 1 is not available here; the Game Gear has no such controller` | 77 |
| script copy with `x1=+00257` on the aim line, `--port1 lphaser` | `input file error: port 1 is lphaser, which takes an aim of x 1 to 256 and y 1 to 240, or 0 0` | 2 |
| script copy with `y1=+00241` | the same message | 2 |
| `--frames 40 --port1 lphaser --input phaser.input --hash` twice | both `0043ed635241b36f1a0a5864dba62b8268c15d95c581c47cf082426dcef44258` | 0 / 0 |
| `--hash-audio` with `phaser-parked.input`, lphaser against pad | both `audio-transitions ... fef552d9...` and `audio-pcm ... 98f1274d...` | 0 / 0 |
| `./scripts/hygiene.sh --tree` and `--history origin/main..HEAD` | clean | 0 / 0 |
| `git ls-files '*.srm' '*.gg' '*.sms'` | empty | 0 |
| `git ls-files` and `git diff --name-only origin/main...HEAD` filtered for `srm gg sms rom sav bin bios` | only the pre-existing manifest-listed `tests/vectors/z80/sst-sample.bin`; nothing changed on the branch | n/a |
| scan of every non-planning file changed on the branch for `/Users/`, `/home/<name>/`, mail domains | no match; the only `szTheory` hits are the public release URLs in `README.md` that predate the phase (the phase changes one README line, the controllers link) | no personal path |
| grep for `TBD`, `FIXME`, `XXX` in changed non-planning files | no match | no output |
| `git log --format='%an <%ae>' origin/main..HEAD` and the committer form | 55 commits, all `28652+szTheory@users.noreply.github.com` | n/a |

Note on the hand-run hashes: the fixture draws one picture whatever the aim, so frame hashes of the lit, parked and
left runs are equal to each other by design; the proof of aim is the mailbox, and the proof of the no-shot invariant
is the whole-state comparison in `ports.gun.noshot`, not the frame hash alone.

### Hosted evidence

| Run | Workflow | Head | Conclusion | Notes |
|-----|----------|------|------------|-------|
| https://github.com/szTheory/cogmaster/actions/runs/43015673654 | CI (workflow_dispatch, dispatch 3) | 8ed0a52 | success | read with `gh run view --json jobs`: hygiene, perf, asan, be32, nofp x2, build x6, retroarch, wasm32, hash-equality and `CI required` all success; title, rehearsal and cleanup skipped as for any dispatch |
| https://github.com/szTheory/cogmaster/actions/runs/43015678841 | Nightly (dispatch) | 8ed0a52 | success | vectors, bench x3, bench-wasm32, `fuzz` (the pad target with the gun ops) and report all success |
| https://github.com/szTheory/cogmaster/actions/runs/43015683127 | CI on `main` (perf base) | 54631cb | success | the control line for the delta |

Perf: from the job logs, branch `perf control boot_to_play runs=12303209,12303209,12303209 spread=0` and main
`runs=12303147,12303147,12303147 spread=0`; `(12303209 - 12303147) * 10000 / 12303147 = 0` basis points (limit 100).

`8ed0a52` is an ancestor of HEAD. The commits since (a739450, 925c4a7, a918e80, bbc9abc and the planning commits) change
`src/gun.c`, `src/vdp.c`, `src/internal.h` (the finish-before-height fix, called only when a gun is attached),
`libretro/libretro.c` (stick rounding for the phaser cursor only), `tests/cmake/repository.sh` (the policy stripper) and
test and doc files. They are covered by my local full ctest (590 of 590), by the fixer's local `cmake --workflow --preset ci`
and `hygiene` runs in `09-REVIEW-FIX.md` with recorded negative controls, and the pull request run at ship is the hosted
confirmation for them, a merge-time check by design (rule 6), not a verification item.

### Specific checks requested

- **Amended wording is consistent.** `REQUIREMENTS.md` GUN-02 (the Light Phaser plugged in, trigger never pulled, no
  light in the sensor's view, same frame and audio hashes and machine state apart from the gun's fields, as a pad with
  nothing pressed; work bounded per picture and per line) and GUN-03 (a pure host helper through the rectangle of the
  last frame, Master System and Game Gear rectangles, libretro aims through it, the runner's script aim is the field
  dot because a Game Gear holds no Light Phaser, the crosshair is drawn only into the libretro output buffer) agree with
  `docs/controllers.md` ("A Game Gear is exit 77", "a script aim is the field dot", "never in the hashed frame",
  "bounded per picture", the no-shot exception stated) and with ROADMAP criteria 3 ("no shot and no light in its view")
  and 4 ("Host tests map aim through the Master System and Game Gear frame rectangles ... a runner script aim at the
  fixture square latches the expected X ... crosshair drawn or not"). The two are not worded identically: the ROADMAP
  criterion 3 keeps the shorter "frame hash ... bounded per line", the requirement states the strengthened form (audio
  hash, state, per picture). The requirement is a superset of the criterion and nothing contradicts; I treat it as
  consistent. The superseded wording ("map aim through the crop in the runner and libretro", "the gun plugged in and no
  shot") is gone from all three files. The git diff against `origin/main` shows only these amendments, the box ticks and
  the traceability rows.
- **Game Gear.** A Game Gear has no TH pins in this core and MAME models none, so the runner refuses with exit 77 and
  libretro degrades to a pad; this is why GUN-03 no longer promises a Game Gear crop run, and the helper is tested
  with Game Gear rectangles instead (CONTEXT decision 2).
- **Table default.** Table code 1 reaches the hosts as the `lphaser` default (`src/cart.c`, `cart.device.defaults`);
  the fixture is deliberately not a table row, so its `--probe` prints `port1_default=pad` and a bare run needs
  `--port1 lphaser` (a bare run with the aim script is exit 2, `port 1 is pad, which takes no motion`, the documented
  strictness).
- **Clean tree.** No ROM, BIOS or save file is tracked other than the manifest-listed vector. The fixture is built from
  `tests/roms/src/light_phaser.asm` and `.link`; the manifest has no removed line. `hygiene.sh --tree` and `--history`
  pass.
- **Review findings.** `09-REVIEW-CORE.md` WR-01 (the wrap finished the old picture's scan with the new height) and
  `09-REVIEW-HOST.md` WR-01 (no-title-table blind spots) and WR-02 (cursor stick step +3 and -4) are fixed in a739450,
  a918e80 and 925c4a7, each with a recorded negative control; the info findings are seeds (rule 7).

### Required artifacts and key links

| Artifact / link | Status | Details |
|-----------------|--------|---------|
| `src/gun.c` | VERIFIED | exists, substantive (sensor, arm, finish, wrap), wired: `bus`/`io` call `cogmaster__gun_sync`, `vdp_event` calls `gun_finish` then `gun_wrap`, the sensor edge reaches the H latch through `io_sensor_edge` |
| aim -> sensor | WIRED | `cogmaster_set_input` canonicalises the pair, `gun.c arm` reads `input.x1/y1` or `x2/y2`; mailbox rows move with the aim |
| `host/aim.c` -> `libretro/libretro.c` | WIRED | `libretro.host.gun.aim`, `.cursor` exercise the adapter through the helper |
| `runner/input.c` -> `cogmaster_set_input` -> exit 77 / exit 2 | WIRED | by hand and by `runner.gun.*` |
| `src/cart.c` code 1 -> `cogmaster_info.default_port1/2` -> hosts | WIRED | `cart.device.defaults`, `compat.device_key` |
| `src/state.c` bounds -> `ports.state.gun`, `fuzz_pad.c` gun ops | WIRED | in the inventories and passing |
| `tests/inventory/*.txt` and `quality.json` floors | WIRED | 590 tests run locally; policy tests pass in the full run |

### Anti-patterns

None blocking. No debt markers in any non-planning file changed by the phase. Residual, documented in
`09-REVIEW-FIX.md`: the `no_title_table` stripper would still be blinded by a character literal holding a double
quote (none exists), and a ROM header read through a helper matches none of its tokens by design, with
`ports.gun.identity` as the behavioural backstop.

## Carried hand-offs (not gaps, `human_needed: false`)

1. **Local-library judge of the 13 phaser dumps.** The edge rule, the delay constant (`GUN_DELAY_TICKS`, 38 ticks), the
   sticky bit, and the radius and luma are model-derived and unmeasured; the judge needs the owner's own game library
   (`COGMASTER_ROM_DIR`), which the agent must not read. It is in `.planning/seeds/SEED-010-pad-device-notes.md`
   ("The Light Phaser local judge (hand-off, not a requirement)"), batched with the phase 11 and 12 library runs, and
   can change constants only, never a hash of a pad-only, no-shot or committed-fixture run. Identity-bound to the
   owner's library, so correctly not automated here (rule 6).

## Advisory notes (no gap, nothing for the owner)

1. **Hosted run predates the review-fix commits.** See "Hosted evidence": local 590 of 590 and the fixer's workflow and
   hygiene runs cover them; the pull request run at ship is the hosted check (hash-equality, perf gate, nightly fuzz).
2. **ASan not run locally.** The asan preset needs clang-21, which is not installed here; the hosted asan job on 8ed0a52
   succeeded. The review-fix commits add no allocation or buffer code on the core path (a loop bound and a call order).
3. **STATE.md runtime log.** The verify step's runtime and model row (rule 9) is for the orchestrator to add; this
   verifier does not edit `STATE.md`.

## Gaps Summary

No gaps. All four roadmap criteria and all four GUN requirements are backed by code that exists, is wired and is
exercised by tests that I ran and that pass (590 of 590 locally), by hand runs of the runner (closed-form latched X at
all ten loop lengths, FFFF for the parked, dark and pad runs, exit 77 on a Game Gear, exit 2 for an out-of-field aim,
identical replay hash), and by hosted runs on the last functional head before the review fixes (ci dispatch 3, the
nightly with the pad fuzz target, perf delta 0 basis points). The only carried item is the owner-library judge, a
documented hand-off that cannot change a hash.

---

_Verified: 2026-11-11_
_Verifier: Claude (gsd-verifier)_
