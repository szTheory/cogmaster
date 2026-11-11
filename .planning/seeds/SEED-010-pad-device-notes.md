---
id: SEED-010
status: dormant
planted: 2026-11-11
planted_during: phase 8, code review
trigger_when: a private library is configured (COGMASTER_ROM_DIR names a directory) and the Sports Pad, Paddle or 3-D titles are judged, or a pad-device change touches src/ports.c, src/io.c or libretro/libretro.c
scope: small
---

# SEED-010: pad device measurements and review leftovers

## Why This Matters

The pad devices follow MAME and ares and are unverified on hardware. The phase's own fixtures prove the protocol
and replay determinism; only real dumps can show a sign error, an idle-timing mismatch or a 3-D picture that
reads wrong. A local library run is the only judge.

## The local library judge (hand-off, not a requirement)

Run once with the owner's library path set: for each of the 7 Sports Pad and Paddle dumps, the hash of a run with
motion versus an all-zero script (they must differ while the dump reads the device); `--eyes` on the 9 3-D titles
(flip counts); the count of non-3-D titles whose glasses register moves. Outcomes can change at most one default
(`cogmaster_3d_view`), the delta sign and one doc sentence; never a hash. Batch it with the phase 11 and phase 12
library runs.

## The Light Phaser local judge (hand-off, not a requirement)

With the owner's library path set, for each library dump the table defaults to the gun (both ports for the two-port
row), run `cogmaster-run <dump> --frames N --input <script> --hash` with a scripted aim at a known on-screen target
and the trigger pressed, and again with an all-zero script of equal length. The hashes, a RAM change or a `--png`
pair must show the target hit. Run the 3-D gun title with `--eyes`, and read a title that polls TH with no trigger
(a title screen) for what it shows. Batch it with the phase 11 and 12 library runs.

What a run can change, and nothing else: `GUN_DELAY_TICKS` (38 ticks, 19 dots, MAME's mode), the sticky bit (set at
each fall, cleared by a `$DD` read and at the wrap) and the edge predicate (the one test in `th_edge`'s call site,
named in `docs/controllers.md`: the latch fires where the sensor releases TH). Radius 6 and luma 127 with weights 77,
150 and 29 are constants of the same kind. The edge rule is unmeasured: MAME and ares latch on the release, SMS Power
on the fall, MacDonald on either. A run of the 13 dumps can only move those constants or the predicate; it can never
change a hash of an existing run, because no pad-only, no-shot or committed-fixture run reads them. A disagreement on a
bright frame points at the edge rule; one on a lit shape smaller than the disc points at the shape rule.

## The BIOS local judge (hand-off, not a requirement)

With the owner's BIOS directory variable and library path set, per dump: the runner's recognised label on stderr (or
the printed SHA-1 fact); that `cogmaster-run <cartridge> --boot bios --bios <dump> --frames N` exits 0 for a dump the
table runs and 77 with a Game Gear cartridge; and that the local window scan prints `K compared` and exits 0.

What a real BIOS leaves that fast boot lacks, and a game can see: logo tiles in VRAM and CRAM, the VDP R1, R5, R6 and
R0 values, BIOS routines and variables in work RAM above `$C000`, the Z80 registers and R, IM and IFF at hand-off, PSG
attenuation after the jingle, the beam phase, and paging after a 64 KiB or larger checksum. The CI image writes the
same constants fast boot sets, so it proves the mechanism and not this residue.

Outcomes are predicates only: a pass or fail line and the SHA-1 fact. Never frames, RAM or states, never a committed
log, `--png`, `--state-out` or `--expect` value. If a title needs the residue, the fast-boot table grows; the BIOS
bytes are never copied. Batch it with the phase 11 and 12 library runs.

## A core sync at the start of `run_frame` (review leftover)

The fuzz leg that cuts a frame into pieces had to bring the sensor up to the beam before each `run_frame`, because a
device change is applied at the start of the call and drops the port's scan block while dots of the picture are
still unscanned. A host never calls `run_frame` mid-window, so this is unreachable through the API: hosts call at
frame boundaries, after the end-of-frame sync. A sync at the start of `run_frame` as well would remove the harness
workaround; it is a change to the five sync sites and is not worth it unless a host can reach the case.

## Review leftovers (rule 7: seeds, not requirements)

- **Gating written twice (core IN-01).** `cogmaster__io_glasses_write` in `src/io.c` repeats the `$3E` cart and RAM
  bits as literals that `src/bus.c` names; move the helper beside the constants or share them in `internal.h`.
- **Soft reset wipes device position (core IN-02).** The Paddle position and a Sports Pad's undelivered motion are
  zeroed by `io_reset`; decide whether a soft reset should keep them and pin it with a test.
- **Paddle region override (core IN-03).** A paddle table row replaces a region the ROM header already named; only
  Woody Pop is affected today (region 0 in the row).
- **Unreachable guards (core IN-04).** `step > 4` and a `default:` in the device code cannot be reached; keep as
  defence in depth or delete with a test of the loader bound.
- **Loader step versus TH wire (core IN-05).** The state visitor does not cross-check `step` against the TH wire;
  add the check if a hostile state can reach a different frame hash through it.
- **`--probe` and ports (host IN-01).** `--probe` silently takes `--port1/--port2` but refuses `--eyes`; pick one
  rule and add it to `runner.usage.*`.
- **Stale stereo picture (host IN-02).** The libretro stereo rule can hold a stale picture up to four frames on a
  title that stops alternating; shorten or document.
- **`hashes.cmake` argument parsing (host IN-04).** Device arguments go through `separate_arguments`; a name with a
  space would split. No such name exists.

## Light Phaser review leftovers (rule 7: seeds, not requirements)

- **Orphaned `th_pins` comment (core IN-02).** The doc comment in `src/io.c` sits above `sensor_low`; move it back
  beside `th_pins`.
- **Dead helper and unused parameter (core IN-03).** `cogmaster__vdp_latch_h` has no caller in `src/` and
  `cogmaster__vdp_hcount_at` carries an unused parameter; delete or use them when `vdp.c` is next touched.
- **Gun check twice on `$DD` (core IN-04).** A gun-less read evaluates the gun test twice and the `ports.c` header
  claim no longer holds exactly; fold the checks when the perf gate next has headroom to spare.
- **Fixture row citations (host IN-01).** `tests/diag/expected.csv` evidence for the light_phaser rows cites sources
  whose latch edge differs from the pinned release edge; reword the citation to name the rule actually pinned.
- **`compat.device_key` and the phaser pair (host IN-02).** The case never asserts that an `lphaser` pair is present
  and distinct from a pad pair; add one row each way.
- **Stale comments (host IN-03).** Two comments in the new gun code and tests are stale or ambiguous; reword.
- **Cross-lane hash row (host IN-04).** The `light_phaser` row in `hashes.cmake` covers only the fixed picture;
  add a lit-aim script row if a lane-specific divergence ever shows in the mailbox but not the hash.
- **Fuzz op for a height change (core IN-01 follow-up).** `ports.gun.walk` covers a height change between pictures,
  but `fuzz_pad.c` has no op for it: adding one changes `OP_COUNT` and reinterprets every stored seed byte, so add
  it together with regenerated seeds and a hosted libFuzzer run.
- **Stick asymmetry on Sports Pad and Paddle.** Full deflection at K=6 gives +5 and -6 counts because those
  devices truncate; `libretro.host.devices.*` pins the truncation. Round both devices and update the pins if a
  symmetric feel matters.
