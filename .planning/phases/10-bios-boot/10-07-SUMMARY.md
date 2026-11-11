---
phase: 10-bios-boot
plan: "07"
subsystem: test
tags:
  - bios
  - fixtures
  - equivalence
  - wla-dx
status: complete

requires:
  - 10-06
provides:
  - tests/roms/src/bios_shape.asm and its link file, four builds of one owned source: an 8 KiB BIOS-shaped image, a header-less probe cartridge and two bad variants
  - fixtures.bios_shape.pin.* digest pins for the four built images
  - bios.equivalence, the classification-complete field-by-field comparison of the fast-boot hand-off against the state the image reaches, with independence literals and controls
  - bios.equivalence.bad_ram and bios.equivalence.bad_sp, the two bad images run for real
affects:
  - 10-10
  - 10-11

actuals:
  tokens: 9000
  tasks: 2
  commits: 2

plan_head_before: 7ffbf2dc91c25554b38304629976064da47032ac
plan_head_after: a132db25fffe6599b5c4b183dedb9cbd29be0ae5
commits: 2

tech-stack:
  added: []
  patterns:
    - "One WLA-DX source with -D selectors builds a probe cartridge, a BIOS-shaped image and two single-fault variants, each with its own digest pin"
    - "A comparator over cogmaster__state_record in which every record name is in a compared or an excluded list, the lists are pinned by literal counts, and a name in neither fails naming it"
    - "A raw-stepped instance closes each frame through cogmaster_run_frame so the state it is walked in is one a host could have saved and a third instance loads"

key-files:
  created:
    - tests/roms/src/bios_shape.asm
    - tests/roms/src/bios_shape.link
  modified:
    - tests/bios/test_bios.c
    - tests/fixtures/tests.cmake
    - tests/built/expected.txt
    - tests/roms/manifest.txt
    - tests/inventory/ci.txt
    - tests/inventory/wasm32.txt

key-decisions:
  - "The probe cartridge is 64 KiB with sixteen unreachable stores to the top mapper register after its halt, not 16 KiB: a 16 KiB image wraps every power-on bank to 0, so the typed paging literals 0, 0, 1, 2 could not hold and the mapper restore in the image would test nothing"
  - "The comparison steps the BIOS instance with the Z80 inside a frame and closes each frame with cogmaster_run_frame, which runs no instruction once the deadline is passed, instead of normalising the frame count and the deadline; a raw step past the deadline leaves renderer bookkeeping that state load refuses"
  - "The bad-image controls are anchored to the comparator's own fixed line, differing fields [name], and the optional field-name argument is not used"

requirements-completed:
  - BIOS-03

coverage:
  - id: E1
    description: "One owned WLA-DX source builds the 8,192 byte BIOS-shaped image, a 64 KiB header-less probe cartridge, and the images that omit the store of AB at the top of work RAM or set SP DFF2; each is pinned by SHA-256 in tests/built/expected.txt, the image carries an owned marker and no cartridge header text, and nothing is committed as bytes"
    requirement: BIOS-03
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^fixtures\\.bios_shape\\.' exit 0 (4 tests); one marker byte changed in the source made the bios_shape pin exit 8 naming the SHA-256 difference; restored to exit 0; sh scripts/hygiene.sh --tree exit 0"
        status: pass
    human_judgment: false
  - id: E2
    description: "bios.equivalence steps instance B (probe cartridge plus the image) to address 0 with port 3E at AB, then compares it with instance A (fast boot) field by field on SMS1 and SMS2: 88 compared and 69 excluded record names, every name in exactly one list, every exclusion with a reason, and no difference in any compared field"
    requirement: BIOS-03
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci -R '^bios\\.equivalence$' exit 0; the same test exit 0 under cc -fsanitize=address,undefined"
        status: pass
    human_judgment: false
  - id: E3
    description: "Independence guard: both instances equal literals typed from the hand-off table (3E AB, C000 AB, 3F FF, SP DFF0, AF FFFF, paging 0, 0, 1, 2, PC 0) and the enable walk recorded in the scratch window equals literals typed from the memory map section (BIOS bytes with the cartridge off, open bus with neither, the cartridge's bytes with the BIOS off, mirror write then read)"
    requirement: BIOS-03
    verification:
      - kind: unit
        ref: "bios.equivalence exit 0; the typed SP literal changed to DFF2 made it exit 8 naming m->z80.sp; restored to exit 0"
        status: pass
    human_judgment: false
  - id: E4
    description: "Controls in the same executable: twenty-three in-memory mutations of the fast-boot instance (the required fields, the scratch window edges, the excluded fields) are each named exactly or not reported; a compared list missing vdp_reg names it unclassified; an excluded list extended by z80_sp, z80_sp moved to excluded and an excluded entry without a reason each fail the matching pin; a step bound of 1,000 is a named failure"
    requirement: BIOS-03
    verification:
      - kind: unit
        ref: "bios.equivalence exit 0; vdp_addr moved from excluded to compared made it exit 8 with the pinned counts and differing fields [vdp_addr]; restored to exit 0"
        status: pass
    human_judgment: false
  - id: E5
    description: "The two bad images run for real and fail naming the field: bios_shape_bad_ram.bin gives differing fields [ram] and bios_shape_bad_sp.bin gives differing fields [z80_sp], each through expect_fail with exit 1 and a pattern anchored to that fixed line"
    requirement: BIOS-03
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^bios\\.equivalence\\.bad_(ram|sp)$' exit 0 (2 tests)"
        status: pass
    human_judgment: false
  - id: E6
    description: "The four pins and the three equivalence tests are in the ci and wasm32 inventories with sorted labels, the ci and hygiene workflows stay green, and no existing pin, row, hash or seed moved"
    requirement: BIOS-03
    verification:
      - kind: integration
        ref: "cmake --workflow --preset ci exit 0 (616 tests) and cmake --workflow --preset hygiene exit 0 (63 tests); git diff --quiet against the merge base over the unchanged paths exit 0"
        status: pass
    human_judgment: false

duration: about 2 hours
completed: 2026-11-11
---

# Phase 10 Plan 07: The BIOS-shaped image and the hand-off equivalence Summary

CI now runs a self-authored BIOS-shaped image with no skip and shows, field by field with every name classified, that the state it hands a cartridge equals the state fast boot sets, with controls that fail when either side, the comparator or its lists are wrong.

## Accomplishments

- `tests/roms/src/bios_shape.asm`: one WLA-DX source, four builds. `bios_shape.bin` (8,192 bytes) powers up at `$3E = $E0`, gives the VDP a visible picture and backdrop (CRAM entries 0 and 17, R7, mode 4 and the display on), copies a 191 byte program to work RAM inside `$C100-$C1FF` and jumps to it. The program walks the enable combinations (BIOS on and cartridge off, neither, cartridge on and BIOS off), recording reads of `$0000` and `$0001` and a write then read of `$FFFC-$FFFF` at `$C1D0`; restores the mapper (0, 0, 1, 2) and the mirror bytes; waits about two frames; restores R0, R1, R7 and CRAM entries 0 and 17; zeroes every register pair; writes `$3F = $FF` and `$3E = $AB`; stores `$AB` at `$C000`; loads AF `$FFFF` by push and pop with SP inside the window; sets SP `$DFF0` and jumps to `$0000`. It never sets IM. The probe is header-less `di; halt`; the bad builds omit the `$C000` store or set SP `$DFF2`.
- `tests/fixtures/tests.cmake`: a foreach over the four images with `file(GENERATE)` link files and `-D` selectors as `vdp_lines` does; `fixtures.bios_shape.pin.<image>` through a two-line `pin` case in the script-mode head (the digest check that already opens every case is the whole check); `bios.equivalence` and its two bad-image controls with the unsettled timeout comments.
- `tests/bios/test_bios.c`: case `equivalence` with the classification-complete comparator, the pins, the independence literals, the mutation loop, the rule controls and the step-bound control, on SMS1 and SMS2.
- `tests/built/expected.txt` gains four rows and `tests/roms/manifest.txt` one comment line; the inventories gain seven names each on ci and wasm32.

## Equivalence

- Pinned list lengths: 88 compared names and 69 excluded names (157 distinct names in the 203,664 records of the probe-cartridge state); 16 required names, all compared. The pinned literals are `EQ_COMPARED_NAMES`, `EQ_EXCLUDED_NAMES` and `EQ_REQUIRED_NAMES` in `tests/bios/test_bios.c`.
- Step bound: instance B reaches address 0 with `$3E = $AB` after 18,819 steps and two closed frames on both machines; the bound is 40,000 steps. A bound of 1,000 is the "never reached" control.
- First-run classification: the compared names are the header identity fields, the Z80 registers, flip-flops and interrupt mode, the VDP registers, CRAM, VRAM, scroll and latch state, work RAM (everything but bytes `$0100` to `$01FF`), cartridge RAM, the mapper, ports, the PSG attenuation, period, noise and latch, the input and device state, and the whole fault record (so the walk provably raised no fault). The excluded names, each with a reason, are the master clock and frame count, the bus stamps, the body checksum, the boot source and BIOS digest fields, ten diagnostic counters, the beam and interrupt flags (`vdp_vline`, `vdp_hcounter`, `vdp_hheld`, `vdp_line_counter`, `vdp_line_pending`, `vdp_status`), the guest-visible VDP port state (`vdp_addr`, `vdp_code`, `vdp_first`, `vdp_buffer`), the hidden Z80 state (`z80_r`, `z80_wz`, `z80_p`, `z80_q`) and the picture, sample and renderer scratch (`render_*`, `sprite_*`, `synth_*`, `completed_*`, `field_pixel`, `lcd_*`, and the PSG counters). The differences a first run found were exactly the excluded names and the work RAM window; the comparator also named `cart_ram`, a name the first lists lacked, which proves the completeness check.
- Honest limit, one paragraph: the image writes the constants fast boot sets, so this check proves the mechanism (the BIOS window, the `$3E` walk, the hand-off through the mapper) and the single source of constants in `src/boot.c`, and that the comparator and its lists cannot silently ignore a field; it does not prove what Sega's BIOS leaves behind, which is checked only on a dump its owner holds, in the local library preset, as predicates. The limit is stated at the top of the test's equivalence section; the guide that states it for users is written in plan 10.

## Task Commits

1. Task 1, the image source, its four builds, the pins, the manifest line: `d8539c7`
2. Task 2, `bios.equivalence`, its controls, the registrations and inventories: `a132db2`

## Evidence

All commands from the repository root; exit statuses as observed.

- `cmake --preset ci` exit 0 and `cmake --build --preset ci` exit 0 after each task; `ctest --test-dir build/ci -R '^fixtures\.bios_shape\.' --output-on-failure` exit 0 (4 tests) and `ctest --test-dir build/ci -R '^fixtures\.' --output-on-failure` exit 0; `ctest --test-dir build/ci -N -R '^fixtures\.bios_shape\.pin\.'` prints `Total Tests: 4`; `ctest -N -R '^bios\.'` prints `Total Tests: 8`; `ctest --test-dir build/ci -R '^(bios\.|fixtures\.bios_shape\.)'` exit 0 (12 tests; `bios.equivalence` 0.20 s, each bad image 0.04 s).
- `wc -c < build/ci/fixtures/bios_shape.bin` prints 8192; the last eight bytes at offset 8176 are not the cartridge-header signature (comparison exit 0); `grep -c '\.dsb' tests/roms/src/bios_shape.asm` prints 1; `git ls-files | grep -c 'bios_shape.*\.\(bin\|sms\)'` prints 0.
- Negative controls, each a python edit confirmed by `cmp` (exit 1, the file differs) and a successful rebuild (exit 0), then restored and re-run green: (1) one marker byte of the source changed, `ctest -R '^fixtures\.bios_shape\.pin\.bios_shape$'` exit 8 naming the SHA-256 difference, restored exit 0; (2) `vdp_addr` moved from the excluded to the compared list, `bios.equivalence` exit 8 printing the pinned counts (89 and 68 against 88 and 69) and `differing fields [vdp_addr]`, restored exit 0; (3) the typed SP literal changed to `0xDFF2`, exit 8 printing `m->z80.sp == 0xDFF2u failed`, restored exit 0 (the file is byte-identical to the saved copy). The plan named `vdp_buffer` for the second control; this image leaves it equal (its last CRAM write stores 0), so `vdp_addr`, which differs, is the field the control names.
- `docker run --rm -v "$PWD":/w -w /w gcc:13 gcc -std=c17 -Wall -Wextra -Wpedantic -Werror -Wtype-limits -Wconversion -fsyntax-only -I include -I build/ci/generated -I src tests/bios/test_bios.c` exit 0; `clang-format --dry-run -Werror` (Homebrew `llvm@21`) exit 0; `cc -g -fsanitize=address,undefined -fno-sanitize-recover=undefined src/*.c tests/bios/test_bios.c` built (exit 0) and `equivalence` run with the built paths exit 0. The asan preset was not run (clang-21 is not installed here); the hosted asan job decides at the next dispatch.
- `cmake --workflow --preset ci` exit 0 (616 tests plus 5 release tests) after each task. `cmake --workflow --preset hygiene` exit 0 (63 tests) after Task 1; after Task 2 its first run exit 8 with only `hygiene.tree` at its 60 s limit while the `ci` workflow had just run, then `sh scripts/hygiene.sh --tree` exit 0 and the workflow rerun alone exit 0 (`hygiene.tree` 50.81 s).
- `ctest --test-dir build/ci -R '^(core\.|ports\.state|bios\.)' --output-on-failure` exit 0.
- No hosted dispatch in this plan: `bios.equivalence`, its two controls and the four pins (and plan 06's `bios.hostile` and `fuzz.regress.bios`) carry the unsettled timeout comment (20 s floor, provisional) until their first hosted run at the dispatch after plan 10; plan 11 settles their timeouts from that run.

## Unchanged proof

- `git diff --quiet $(git merge-base origin/main HEAD) -- tests/io tests/vdp tests/bus tests/audio tests/ports tests/testcard_pins.cmake tests/fuzz/fuzz_run.c tests/fuzz/fuzz_loader.c` exit 0.
- `! git diff $(git merge-base origin/main HEAD) -- tests/built/expected.txt tests/diag/expected.csv tests/roms/manifest.txt | grep -q '^-[^-]'` exit 0 (rows and comment lines only added).

## Budgets

`cmake -DROOT=$PWD -P tests/cmake/budgets.cmake` exit 0: counted check code 833 (phase cap 847, limit 939), src lines 9,399. This plan added no counted check code and no src line; the `pin` case lives in `tests/fixtures/tests.cmake`, which is not counted.

## Deviations from Plan

### Interpretations and measured corrections (no behaviour lost)

- **Probe cartridge is 64 KiB, not 16 KiB.** The independence guard types paging 0, 0, 1, 2 from the hand-off table. On a 16 KiB image the loader wraps every power-on bank to 0, so the literal could not hold, and the image's mapper restore would test nothing. The probe is `di; halt` followed by sixteen stores to `$FFFF` that never run, so the load takes the Sega mapper (the plan 05 test cartridge uses the same device); it adds a cartridge RAM record, `cart_ram`, which is compared. The pin and the file size differ from the plan's text; the build, the manifest line and the hygiene scans are unchanged in kind.
- **Registers and colours the image restores.** The plan lists R1, R7 and CRAM entry 17. The image also sets and restores R0 (mode 4) and CRAM entry 0. Measured with the display on and R7 only, the frame's pixels were identical to the black fast-boot frame (the backdrop is only in the border, outside the crop); with entry 0 coloured the two frames before the hand-off differ and the third equals fast boot. The extra writes are restored to the reset values, so the compared registers are unaffected.
- **No frame normalisation.** The plan allowed normalising `frame` and `bus.deadline` or stepping with `cogmaster_run_frame`. Measured: a raw Z80 step past the frame deadline leaves renderer bookkeeping that `cogmaster__state_load` refuses even after `frame` and the deadline are normalised (the first refusal is at the first step past 179,208 ticks). The test therefore steps with `cogmaster__z80_step` inside a frame and calls `cogmaster_run_frame` at the deadline, which runs no instruction and closes the frame; the stepped state loads into a third instance, which the test asserts.
- **Write pattern `$20` to `$23`.** The first pattern, 1 to 4, wrote `$01` to `$FFFC`, which the Sega mapper counts as an unused bit; with the cartridge on that recorded a mapper range fault and the comparator named `fault_present`. The values keep the unused bits clear and the fault record is now compared, so the walk provably raises none.
- **Results at `$C1D0`.** The program is 191 bytes, so the scratch results moved from `$C1A0` to `$C1D0` to stay clear of the code; everything the image writes still lies in `$C100-$C1FF`, plus `$C000` and the mirror bytes it restores.
- **Bad-image regexes, no field argument.** The comparator prints one fixed line per run, `equivalence: differing fields [name]`; the controls anchor to it, so the optional field-name argument the plan describes was not needed.
- **Second control field.** See the evidence: `vdp_addr` where the plan names `vdp_buffer`.
- **The `pin` case.** The script-mode head of `tests/fixtures/tests.cmake` verifies the digest and then dispatches on `CASE`; a two-line `pin` branch (a comment only) lets the pins reuse it, as the plan asks.

**Total deviations:** 0 rule-driven fixes; 7 interpretations recorded above. **Impact:** none on the requirement; the 64 KiB probe makes the paging literals meaningful instead of vacuous.

## Known Stubs

None. Every image is assembled from the owned source; no real firmware byte was read, copied, logged or compared, and no BIOS-shaped file is committed.

## Threat Flags

None. The test reads only the two built fixture paths it is given.

## Phase notes

The equivalence proves the mechanism and the single source of the hand-off constants. What a real BIOS leaves behind is verified only on the owner's own dump in the local library preset and recorded as predicates (the hand-off list is in the seed written in plan 11). The first hosted view of the plan 06 and plan 07 tests is the dispatch after plan 10; plan 11 settles their timeouts.

## Self-Check: PASSED

- `tests/roms/src/bios_shape.asm`, `tests/roms/src/bios_shape.link` and this SUMMARY exist.
- Commits `d8539c7` and `a132db2` are ancestors of HEAD (`git merge-base --is-ancestor` exit 0 for each).
