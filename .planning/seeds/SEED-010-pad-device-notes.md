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
