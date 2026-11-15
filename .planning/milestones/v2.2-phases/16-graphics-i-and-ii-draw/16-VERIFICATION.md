---
phase: 16-graphics-i-and-ii-draw
verified: 2026-11-14T06:57:00Z
status: passed
score: 14/14 must-haves verified
covered_files:
  - .planning/phases/16-graphics-i-and-ii-draw/16-01-PLAN.md
  - .planning/phases/16-graphics-i-and-ii-draw/16-01-SUMMARY.md
  - .planning/phases/16-graphics-i-and-ii-draw/16-02-PLAN.md
  - .planning/phases/16-graphics-i-and-ii-draw/16-02-SUMMARY.md
  - .planning/phases/16-graphics-i-and-ii-draw/16-03-PLAN.md
  - .planning/phases/16-graphics-i-and-ii-draw/16-03-SUMMARY.md
  - .planning/phases/16-graphics-i-and-ii-draw/16-04-PLAN.md
  - .planning/phases/16-graphics-i-and-ii-draw/16-04-SUMMARY.md
  - .planning/phases/16-graphics-i-and-ii-draw/16-05-PLAN.md
  - .planning/phases/16-graphics-i-and-ii-draw/16-05-SUMMARY.md
  - .planning/phases/16-graphics-i-and-ii-draw/16-06-PLAN.md
  - .planning/phases/16-graphics-i-and-ii-draw/16-06-SUMMARY.md
  - .planning/phases/16-graphics-i-and-ii-draw/16-07-PLAN.md
  - .planning/phases/16-graphics-i-and-ii-draw/16-07-SUMMARY.md
  - .planning/phases/16-graphics-i-and-ii-draw/16-08-PLAN.md
  - .planning/phases/16-graphics-i-and-ii-draw/16-08-SUMMARY.md
  - .planning/phases/16-graphics-i-and-ii-draw/16-09-PLAN.md
  - .planning/phases/16-graphics-i-and-ii-draw/16-09-SUMMARY.md
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
covered_digest: "v4:sha256:7a759b978c745602e896b67e59af33684c7787de830363b7ef3c0fde6ab9a7d2"
behavior_unverified: 0
overrides_applied: 0
---

# Phase 16: Graphics I and II draw Verification Report

**Phase Goal:** With Graphics I or Graphics II selected, the Master System 1, Master System 2 and Game Gear in Master
System video mode draw the name, pattern and colour tables, the backdrop and the legacy sprites as the TMS9918 does,
instead of recording the unsupported-video-mode fault; every other legacy mode keeps its fault with the reason stated,
no shipped frame, audio or state pin moves, and the per-frame instruction gate stays green. The stale fault-vocabulary
doc lines, the dirty-tree stamp and the assembler fetch retry land first, inside this phase, so they ride a release.
**Verified:** 2026-11-14T06:57:00Z, on head `bfd1223` (branch `phase/16-graphics-i-and-ii-draw`)
**Status:** passed
**Re-verification:** No, initial verification

The starting hypothesis was that tasks were complete and the goal missed. It did not survive: every truth below was
checked against the code and against commands run in this session, not against SUMMARY claims. Evidence follows
CLAUDE.md rule 11 (a command, an exit status, a run URL); no log is committed.

## Goal Achievement

### Observable Truths

The roadmap's five success criteria are the contract; truths 6 to 14 are plan-level truths that carry the behaviour
and prohibition claims the criteria rest on.

| # | Truth | Status | Evidence |
|---|-------|--------|----------|
| 1 | SC1: owned Graphics I and II fixtures and a legacy-sprite fixture, run on the Master System 1, Master System 2 and Game Gear in Master System video mode, equal closed-form oracles committed before the renderer, including the sprite status reads (fifth sprite, coincidence, end-of-list Y, 8x8 and 16x16, magnification, early clock) | ✓ VERIFIED | `fixtures.legacy_modes.{g1,g2,g2_masked,g2_late,g2_switch}.{sms1,sms2,gg_sms}`, `fixtures.legacy_sprites.<profile>` and `fixtures.legacy_sprites.mailbox.<profile>` all pass (full `ctest` of `build/ci`, 742 of 742, exit 0). Oracle first added in `fcca930` and `4c4a6b0` (2026-11-13 22:29 and 22:50), renderer first added in `bbe5038` (23:19). The oracle in `tests/fixtures/legacy.c` is closed-form in the tile-index domain with its own per-line SAT walk; it shares no code with `src/vdp_legacy.c`. Note on wording: the eleven (R3, R4) mask pairs run in process under that oracle (`vdp.legacy.masks`) and two ROM images (`legacy_modes_g2`, `legacy_modes_g2_masked`) run through `cogmaster-run`, which is the mapping decision D-22 in `16-CONTEXT.md` set before planning and the SUMMARY states; the roadmap text "one per mask combination through `cogmaster-run`" is met in intent, not literally as eleven ROM images. See Notes. |
| 2 | SC2: text, multicolour, the mixed modes, the invalid text modes and native Game Gear video each still record the unsupported-video-mode fault in a test; the Light Phaser's "would this fault" check agrees with the supported-mode test; the video documentation names which modes render and which fault | ✓ VERIFIED | `vdp.unsupported.{text,multicolour,mixed,invalid_text,native_gg}` and `vdp.background.unsupported` pass. The last checks a 16-mode by 4-profile literal table (written as literals, not computed by the classifier) for both the fault and `cogmaster__vdp_render_blocked`, which `src/gun.c:137` calls. `cogmaster-run --trace first-fault` on all five legacy images on `--machine sms1`, `--machine sms` and `--machine gg --video-mode sms` prints `fault none`; on `--machine gg --video-mode gg` it prints `kind=unsupported_video_mode addr=$0000`. `docs/hardware/video.md` lists the nibbles that render and the ones that fault. |
| 3 | SC3: the base-versus-head hashes comparison is equal for every shipped pin; revision 19 and state format 17 recorded; legacy state round trip byte-equal; a state whose legacy sprite slots break the new bounds is refused; the enter-Graphics-II-after-frame-600 and mid-frame switch fixtures each have a pinned hash; mode 4 bring-up fixtures hash as before | ✓ VERIFIED | Independent check: `git archive origin/main` built with the `ci` preset in a scratch tree, its `runner.hashes_file` output (172 lines) compared with head's `build/ci/hashes.txt` (198 lines): `LC_ALL=C comm -23` of the sorted files is empty, exit 0. The recorded `build/p16-base-hashes.txt` (173 lines) gives the same empty result. `LC_ALL=C comm -13` lists 25 new lines (6 `fixture`, 19 `playing legacy`). The recorded base holds one more line than my fresh base (`fixture boot_to_play_gg.gg`) because the hashes script globs `build/ci/fixtures/` and that file exists only where the fixture tests have run; head holds it too. `include/cogmaster.h` has 19 and 17; `tests/api/v14.c` expects 17. `core.state.legacy_roundtrip` and `core.state.legacy_sprite_bounds` pass (row 16 with 16x16, row 8 with 8x8, count 5, colour byte bits 6..4, overflow without count 4, fifth index off 31 with bit 6 clear all refused, their neighbours load). `g2_switch` line pinned by literal `switch_line_hash` in `tests/fixtures/legacy.c`, frame 30; `g2_late` compared dot by dot with the oracle at frame 620 and its runner hash equals the oracle digest; image SHA-256s in `tests/built/expected.txt`. `vdp.legacy.bringup` passes. |
| 4 | SC4: the instructions-per-frame gate is green with no new allow line, first read from a draft pull request opened before completion; the legacy hardware notes are rewritten from sources with the MAME note corrected and the open model choices stated with reasons; permissive files consulted have provenance rows; the docs byte-cap check passes | ✓ VERIFIED | Draft PR #47 (`isDraft: true`) was created 2026-11-14T04:04:00Z; its first run (43554767220, head `d7da071`, before the renderer commit) printed `perf boot_to_play head=9893026 base=12288947 delta_bp=-1949 verdict=PASS`. Latest run https://github.com/szTheory/cogmaster/actions/runs/43564813364 on head `6e8984e`: conclusion success, `CI required` success, `perf boot_to_play head=10089912 base=12288947 delta_bp=-1789 verdict=PASS`. `git diff --name-only origin/main HEAD -- tests/perf/allow.txt .github/workflows/ci.yml cmake/packaging.cmake runner/diag_class.h` prints nothing. The two commits above `6e8984e` change only `.planning/` files (checked with `git diff --name-only 6e8984e HEAD`). `.planning/preparation/SMS-VDP.md` section 7 now carries a "(corrected)" Graphics II masks item (315_5124.cpp masks on every revision; the no-mask statement is msvdp's; tms9928a.cpp tests above $E0; R0 bit 3 note corrected). `docs/hardware/video.md` states each model with its reason and disagreeing sources (masks, Y threshold, colour-0 coincidence, fifth-sprite index, R0 bit 3, Game Gear CRAM 16+n, 192 lines, display-off backdrop). `PROVENANCE.md` sections 2, 4 and 10 name MAME `tms9928a.cpp/.h` and ares `component/video/tms9918`; `gh api repos/mamedev/mame/commits/1d7a454ed79e` and `gh api repos/ares-emulator/ares/commits/9408cb43d494` both answer with the sha, exit 0. `docs.budgets` passes; largest docs are `conformance.md` 38,034 and `runner.md` 37,973 bytes, under 38,912. |
| 5 | SC5: docs state the current fault vocabulary; the library run path refuses a tree with tracked changes outside `.planning` (dirty-tree control); the fetch scripts retry a transient DNS or TLS error and a permanent failure still fails; `cmake --workflow --preset ci` exits 0 on the head | ✓ VERIFIED | `docs/audio.md`, `docs/embedding.md` (kind 6 row, `CTRL` row, "six kinds and seven devices", faults paragraph, revision 19 sentence) and `docs/runner.md` (`ctrl`, `rom_write`, `where=port`, `wclass`, `pclass`) corrected; a grep for stale "unmapped port" lines finds only accurate ones. `policy.library_dirty.{refuses,planning,untracked,clean}`, `policy.wla_fetch.{transient,permanent,hash}`, `policy.pins.{retry,permanent}` all pass; verbose run shows the transient case ends `attempt 3 of 3`, the permanent and hash cases `attempt 1 of 3`, fake `curl` exit 6 gives `calls=3` and exit 22 gives `calls=1`. `cmake --workflow --preset ci` run locally on this head: exit 0 (31 s). Hosted: run 43564813364, `CI required` success, on a commit whose code tree equals head. `sh scripts/hygiene.sh --history origin/main..HEAD`: exit 0. |
| 6 | The oracles precede the renderer and are independent of it | ✓ VERIFIED | Commit order above. Twelve mutations of the renderer, state loader and status read, each applied to pristine source with a forced rebuild, were each caught by named tests (table under Behavioural Spot-Checks). |
| 7 | MODE-07: the legacy renderer is selected once per call and the mode 4 per-dot loop has no mode test; the gate stays green with no allow line | ✓ VERIFIED | `src/vdp_render.c:208` computes `cogmaster__vdp_class` once per `render_until` call; the legacy branch runs `cogmaster__legacy_span` once per line; the old per-dot `vdp_supported` calls are gone from the mode 4 loop. Hosted perf `delta_bp=-1789` (head cheaper than base by 17.89 percent) with `tests/perf/allow.txt` unchanged. |
| 8 | A register write takes effect at its own dot: the dot before draws under the old mode, the write dot under the new one, and two writes at one dot repaint nothing | ✓ VERIFIED (behavioural) | `vdp.legacy.switch` passes; `fixtures.legacy_modes.g2_switch.*` compares the lines before and after the switch with the oracle and pins the switch line. Behaviour-dependent truth, so it needs a passing behavioural test, which it has. |
| 9 | The fifth-sprite candidate survives a status read between the tick-600 scan and dot 0 of the next line; the index stays until read, first wins; bits 4..0 read $1F when bit 6 is clear | ✓ VERIFIED (behavioural) | `vdp.legacy.sprites.fifth_carry`, `.fifth`, `.status_low` pass, and the `tail_status` record of `legacy_sprites` (H counter inside ticks 607 to 683) passes on all three profiles. Removing the dot-0 assertion (mutation M2) fails 8 tests including these. |
| 10 | A sprite list scanned under one class is rescanned at the start of a render call that runs under the other, so a mid-line switch between mode 4 and a legacy mode draws the live class's sprites | ✓ VERIFIED (behavioural) | `vdp.legacy.sprites.class_switch` passes; removing the rescan (M6) fails it. |
| 11 | The Mode 4 sprite engine is untouched in behaviour | ✓ VERIFIED | `git diff origin/main HEAD -- src/vdp_render.c`: `vdp_sprite_pixel` is unchanged, `cogmaster__vdp_sprite_scan` gains only a class dispatch at its top, `sprite_legacy = 0` and `attr = 0` stores. Status read in mode 4 returns the old byte because `fifth_index` stays 31. The base-versus-head subset is empty. |
| 12 | Hostile legacy input stays bounded, including sprites | ✓ VERIFIED | `vdp.legacy.hostile` (fills video memory and the SAT with garbage, asserts `sprite_count <= 4`, `sprite_scanned <= 32`, `fifth_index <= 31` per frame) passes; the scan is capped at 32 entries and the span at 256 dots by 4 slots; a legacy-mode seed is in `tests/fuzz/seeds.c`. |
| 13 | No selected model is presented as a hardware measurement; no behaviour is keyed on a cartridge, title or digest | ✓ VERIFIED | `grep -ci model`: `docs/hardware/video.md` 14, `src/vdp_legacy.c` 6, `tests/fixtures/legacy.c` 10, `tests/vdp/test_legacy.c` 13 (plan thresholds 14, 6, 5, 5). `src/vdp_legacy.c` holds none of cart, title, crc32, sha1, serial. `policy.no_title_table` passes. I read `docs/hardware/video.md` against `src/vdp_legacy.c`: table and pattern address formulas, mask expressions, palette values (match the SMS Power legacy palette), Game Gear CRAM 16+n, quadrant order, plain doubling, $E0 threshold, early clock and coincidence rule agree with the code. |
| 14 | Prohibited edits did not happen: `ci.yml`, `cmake/packaging.cmake`, `tests/perf/allow.txt`, `runner/diag_class.h`, `header.inc`, `mailbox.inc`, `boot_to_play.asm` are unchanged; no new fixture joins the diagnose tests; the dirty-tree refusal prints no file name | ✓ VERIFIED | `git diff --name-only origin/main HEAD -- <those paths>` prints nothing (exit 0). `grep -c legacy_modes tests/runner/diagnose.cmake tests/library/diagnose.cmake` prints 0 and 0. The refusal text in `scripts/library.sh` names only the condition and `.planning`. |

**Score:** 14/14 truths verified (0 present but behaviour-unverified)

### Required Artifacts

`node gsd-tools.cjs query verify.artifacts` and `verify.key-links` on each plan, all exit 0:

| Plan | Artifacts | Key links |
|------|-----------|-----------|
| 16-01 | 4/4 | 3/3 |
| 16-02 | 4/4 | 2/2 |
| 16-03 | 3/3 | 2/2 |
| 16-04 | 3/3 | 3/3 |
| 16-05 | 2/2 | 2/2 |
| 16-06 | 3/3 | 2/2 |
| 16-07 | 2/2 | 2/2 |
| 16-08 | 2/2 | 2/2 |
| 16-09 | 3/3 | 2/2 |

`src/vdp_legacy.c` (213 lines) is substantive (background, palette, sprite scan, per-dot sprite pixel, status
flags) and wired: `src/vdp_render.c` calls `cogmaster__legacy_span` and, through `cogmaster__vdp_sprite_scan`,
`cogmaster__legacy_sprite_scan`; it is in `CMakeLists.txt`. `src/gun.c` calls `cogmaster__vdp_render_blocked`, which
shares `cogmaster__vdp_class` with the fault.

### Key Link Verification

| From | To | Via | Status |
|------|----|-----|--------|
| `src/vdp_render.c` | `src/vdp_legacy.c` | `cogmaster__legacy_span` once per line when display and class LEGACY | WIRED |
| `src/vdp_render.c` | `src/internal.h` | `cogmaster__vdp_class` once per call | WIRED |
| `src/gun.c` | `src/vdp_render.c` | `cogmaster__vdp_render_blocked` equals the fault condition | WIRED (literal-table test) |
| `src/vdp.c` | `src/internal.h` | status read returns `(status & 0xE0) | latched index` and resets `fifth_index` | WIRED |
| `tests/bios/test_bios.c` | `src/state.c` | the four new visitor names classified, `EQ_EXCLUDED_NAMES` 69 to 73 | WIRED |
| `tests/policy/tests.cmake` | `tests/policy/tooling_cases.cmake` | `include()` so the nine rider controls run in the ci lane | WIRED |
| `tests/runner/tests.cmake` | `tests/cmake/hashes.cmake` | `hash_matrix` asserts the `legacy_modes` and `legacy_sprites` rows | WIRED |

### Data-Flow Trace (Level 4)

| Artifact | Data | Source | Produces real data | Status |
|----------|------|--------|--------------------|--------|
| `cogmaster__legacy_span` | pattern and colour bytes, sprite slots | `m->vdp.vram`, registers R2 to R7, scanned `sprite[]` | Yes: frames viewed (below) and equal to the oracle dot for dot | FLOWING |
| status read | bits 7..5 and latched fifth index | `status`, `fifth_index` set by `legacy_span` and the scan | Yes: mailbox records equal oracle rows | FLOWING |
| `completed.backdrop` | backdrop colour | `cogmaster__vdp_backdrop`, gated on display and class | Yes: `vdp.legacy.backdrop` | FLOWING |

I rendered `legacy_modes_g1`, `legacy_modes_g2` and `legacy_sprites` (frame 47) with `cogmaster-run --png` and viewed the
images: Graphics I shows eight-name colour bands, Graphics II a dense per-row coloured pattern, the sprite frame a
backdrop with five sprites on one line and the fifth dimmer. Each is real drawn content, not a flat backdrop.

### Behavioural Spot-Checks

| Behaviour | Command | Result | Status |
|-----------|---------|--------|--------|
| Phase test subset on head | `ctest --test-dir build/ci -j8 -R '^(vdp\.\|core\.state\|core\.fault\|bios\.\|ports\.gun\|policy\.(wla_fetch\|pins\|library_dirty\|no_title)\|fixtures\.legacy\|runner\.(hashes_file\|diagnose\|trace\.fault\|probe\|gun)\|library\.diagnose\|docs\.)'` | 197 of 197 pass, exit 0 | ✓ PASS |
| Whole ci tree | `ctest --test-dir build/ci -j8` (run once) | 742 of 742 pass, exit 0 | ✓ PASS |
| Workflow | `cmake --workflow --preset ci` | exit 0 | ✓ PASS |
| Hygiene over the phase's commits | `sh scripts/hygiene.sh --history origin/main..HEAD` | exit 0 | ✓ PASS |
| Independent base compare | `cmake --preset ci` on `git archive origin/main`, `ctest -R '^runner\.hashes_file$'`, then `comm -23` against head | empty, exit 0 | ✓ PASS |
| Faults through the runner | `cogmaster-run <profile> --frames 60 --trace first-fault legacy_*.sms` on three profiles | `fault none` for all 15 combinations; native Game Gear prints `kind=unsupported_video_mode` | ✓ PASS |

Mutation checks (rule: do not trust a green test that cannot go red). Each mutation was applied to a scratch copy of
head, the sources were touched and rebuilt pristine before each one, and the named tests were run. Every mutation made
named tests fail:

| Mutation | Tests that went red (selection) |
|----------|---------------------------------|
| Graphics II colour mask removed | `vdp.legacy.masks`, `fixtures.legacy_modes.g2_masked.sms1` |
| No fifth-sprite assertion at dot 0 | `vdp.legacy.sprites.fifth_carry`, `.fifth`, `.status_low`, `.collision`, `vdp.legacy.hostile`, the three `legacy_sprites.mailbox` tests |
| Colour-0 sprites do not collide | `vdp.legacy.sprites.collision`, three mailbox tests |
| Y threshold $E0 changed to $F0 | `vdp.legacy.sprites.y_wrap`, three `legacy_sprites` image tests |
| Early clock removed | `vdp.legacy.sprites.early_clock`, `.collision`, six `legacy_sprites` tests |
| No class-change rescan | `vdp.legacy.sprites.class_switch` |
| Status read does not reset the latched index | `core.state.*`, `vdp.legacy.sprites.fifth*`, `.status_low` |
| `cogmaster__vdp_render_blocked` also blocks legacy | `vdp.background.unsupported`, `vdp.unsupported.native_gg` |
| TMS backdrop used with the display off | `vdp.legacy.backdrop` |
| Graphics I colour table address off by one bit | `vdp.legacy.g1`, `.palette`, `.rows`, `fixtures.legacy_modes.g1.*`, `fixtures.legacy_modes.wrong_hash` |
| Game Gear uses the fixed palette | 23 tests across `vdp.legacy.*` and every `gg_sms` fixture |
| 16x16 name not masked with $FC | `vdp.legacy.sprites.sizes`, `.zoom` |
| Loader: row, overflow precondition or fifth-index bounds relaxed (three variants) | `core.state.legacy_sprite_bounds` each time |

### Probe Execution

Step 7c: SKIPPED. The phase declares no `probe-*.sh`, and `find scripts -path '*/tests/probe-*.sh'` finds none.

### Requirements Coverage

Every ID in the plans' `requirements:` fields appears in `REQUIREMENTS.md` and is mapped to Phase 16 there; no
requirement is mapped to Phase 16 without a plan claiming it (no orphans). The union of the nine plans' `requirements:`
and of the SUMMARYs' `requirements-completed:` is exactly the eleven IDs.

| Requirement | Source plans | Description | Status | Evidence |
|-------------|--------------|-------------|--------|----------|
| TUNE-12 | 16-01 | Fault vocabulary docs corrected | ✓ SATISFIED | Doc diffs above; `docs.budgets` and docs checks pass |
| TUNE-13 | 16-01 | Library run refuses a dirty tree, with a control | ✓ SATISFIED | `scripts/library.sh` exit 2; `policy.library_dirty.*` four cases |
| TUNE-14 | 16-01 | Transient fetch errors retried, permanent still fails | ✓ SATISFIED | `cmake/WlaDx.cmake`, `scripts/fetch-pin.sh`; `policy.wla_fetch.*`, `policy.pins.*` |
| MODE-01 | 16-02, 16-04, 16-05 | Graphics I draws, equal to an oracle written first | ✓ SATISFIED | Truth 1 |
| MODE-02 | 16-02, 16-04, 16-05, 16-09 | Graphics II masks, fixture per mask combination, disputed masks named | ✓ SATISFIED | Truth 1 (eleven pairs in process plus two ROM images, per D-22); `video.md` masks paragraph |
| MODE-03 | 16-02, 16-07, 16-08 | Legacy sprite rules, Mode 4 engine untouched | ✓ SATISFIED | Truths 9 to 11; mutation table |
| MODE-04 | 16-03, 16-04, 16-09 | Other modes keep the fault; Light Phaser agrees; docs say which | ✓ SATISFIED | Truth 2 |
| MODE-05 | 16-05, 16-06, 16-08, 16-09 | No pin moves; revision 19, format 17; round trip; loader bounds | ✓ SATISFIED | Truth 3 |
| MODE-06 | 16-02, 16-04, 16-05, 16-07, 16-08 | Mode switches draw correctly; bring-up unchanged | ✓ SATISFIED | Truths 3, 8 |
| MODE-07 | 16-03, 16-08, 16-09 | Selected once, gate green, draft PR first read | ✓ SATISFIED | Truths 4, 7 |
| MODE-08 | 16-09 | Notes rewritten, provenance rows, docs inside caps | ✓ SATISFIED | Truth 4 |

### Anti-Patterns Found

| File | Line | Pattern | Severity | Impact |
|------|------|---------|----------|--------|
| (added lines of the phase diff, outside `.planning`) | - | `TBD`, `FIXME`, `XXX`, `TODO`, `HACK`, `PLACEHOLDER`, "not yet implemented" | none found | No debt marker, no stub in `src/vdp_legacy.c` |

No blocker anti-pattern. The existing tests the renderer commit had to change (`vdp.background.unsupported`,
`ports.gun.fault_neutral`, the `runner.diagnose` and `library.diagnose` fault seeds) were retargeted to text and
multicolour nibbles that still fault, not weakened; D-25 names each.

### Code Review Findings Weighed Against Must-Haves

`16-REVIEW.md` (0 critical, 3 warning, 4 info; dispositions all `open`, no fix round has run). None changes a truth:

| ID | Effect on a must-have | Classification |
|----|-----------------------|----------------|
| WR-01 comment in `src/internal.h` and `src/state.c` says colour and video RAM offsets of a saved state stay put; they moved by 8 | No must-have depends on offsets; truth 3 asks for format 17 and the 16520 pin, both true. I confirmed the struct layout makes the comment false. | WARNING: a false statement about a persisted layout in two comments; worth the cheap fix round |
| WR-02 loader accepts a legacy overflow record no scan can leave (`overflow == scanned` always in a real scan), and a test pins the acceptance | MODE-05 asks the loader to refuse slots that break the stated bounds; the bounds as specified (count 4, five or more scanned, at most 32) are enforced and mutation-tested. The tighter reachable invariant is not a stated bound. | WARNING: hardening, memory-safe and deterministic as is |
| WR-03 (seed) no `TIMEOUT` or `--max-time` on the downloads, so a stalled transfer is not retried | TUNE-14 names transient DNS and TLS failures, which are curl codes 6, 7, 35 and 52 to 56 and are retried and tested; a stalled established transfer is a different failure | WARNING, tooling seed under rule 7 |
| IN-01 to IN-04 | None | INFO |

### Human Verification Required

None. Under CLAUDE.md rule 6 the judgement items (docs against code, viewing the rendered images, reading the SMS-VDP
note) were done in this session, no SUMMARY `coverage` entry has `human_judgment: true`, and no plan carries a
`<human-check>` block. Nothing here needs the owner's identity or hardware.

### Gaps Summary

No gaps. The goal is achieved in the code: Graphics I and II draw tables, backdrop and legacy sprites on the three
profiles and no longer fault there, every other legacy mode and native Game Gear video still fault with the
unsupported-video-mode record, no shipped pin moved (checked against an independent origin/main build), revision 19
and state format 17 are in place with a round trip and bounds refusals, the perf gate is green and improved with no
allow line, and the riders landed first.

Notes for the orchestrator, none blocking:

1. **Roadmap wording versus D-22.** SC1 and MODE-02 say a fixture per register 3 and 4 mask combination. The eleven pairs
   are exercised in process under the closed-form oracle, with two ROM images through `cogmaster-run`, by decision D-22
   in `16-CONTEXT.md`. The pixels of every combination are proven against the oracle, so the outcome holds. If the
   literal wording matters, amend the roadmap line to say "in process" rather than add ten ROM images.
2. **Open review findings.** WR-01 and WR-02 are core-code items and fit the one cheap fix round rule 7 allows before ship;
   WR-03 and IN-03, IN-04 are seeds. Applying WR-02 means changing the accepted-neighbour row of
   `core.state.legacy_sprite_bounds` as the review shows.
3. **Branch base.** `STATE.md` records that the branch was cut from the milestone tip rather than `origin/main` (seven
   milestone docs commits ride the pull request, named in the PR body), a rule 10 deviation recorded with "owner may
   override", as in an earlier phase.
4. **Unpushed commits.** Two docs-only commits (SUMMARY/STATE and the review artifacts) sit above the pushed head
   `6e8984e`; `16-VERIFICATION.md` is uncommitted. Hosted evidence (run 43564813364) covers a code tree identical to head.
5. **Carried deferred items** (`deferred-items.md`): `hygiene.tree` runs near its 60 s timeout when the machine is loaded,
   and `release.firmware_scan` can mistake a static library of exactly 262,656 bytes for a firmware image. Neither touches
   this phase's goal; both belong to the next phase's timeout and tooling work.
6. **Process nit.** The STATE.md runtime log has no separate rows for plans 16-04 and 16-07.

---

_Verified: 2026-11-14T06:57:00Z_
_Verifier: Claude (gsd-verifier)_
