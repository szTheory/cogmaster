---
id: SEED-016
status: dormant
planted: 2026-11-12
planted_during: after v0.13.0, library run review
trigger_when: the next milestone is scoped; this is the recommended first item of it
scope: medium
---

# SEED-016: why a third of the checked cartridges fail the boots check

## Why This Matters

The first full library run published 396 default-profile passes and 210 default-profile `fail` rows (the run printed
fail=207, 603 judged). The page lists them honestly but nobody has looked at why. A fail here means the boots check
(a sampled display that is not blank, and no first fault within 600 frames) did not hold. Fixing a few causes should
flip many rows to `pass`; that is the biggest real improvement left in the core.

## What the ledger already shows (no ROMs needed)

- By mapper (ntsc default rows, fail / judged): `sega` 169 / 505, `none` 22 / 65, `zemina` 8 / 8, `korean_nb` 5 / 5,
  `korean` 5 / 7, `nemesis` 1 / 1, `codemasters` 0 / 4, `hicom` 0 / 9.
- 171 PAL-profile fails: every one is also an NTSC fail. PAL is not a cause; the same carts fail both ways.
- Well-known plain Sega-mapper titles fail (Dead Angle, Phantasy Star, Wimbledon, Vigilante), so one third of ordinary
  carts failing is more likely a few shared causes or a too-strict check than 169 separate emulation faults.
- Whole mapper families failing (`zemina`, `korean_nb`, most `korean`) point at a mapper behaviour gap.

## Where to start

1. Add a per-cart diagnostic to the library judge (count of frames to first non-blank, first fault kind and PC, mapper
   chosen) that prints counts and kinds only, never ROM bytes or names beyond catalog titles.
2. Group the 210 by the diagnostic and fix by group size: check strictness first, then mapper families, then
   individual faults.
3. Re-run the judge and merge the flipped rows by pull request.

## Open question for the discuss step

The agent has not read the owner's library (the secrets guard blocks it; the owner passes `COGMASTER_ROM_DIR` in a
shell). The diagnostic needs real dumps. Decide whether the owner runs the diagnostic and pastes the grouped counts, or
authorises the agent to run it in this shell against the extracted folder (counts only, nothing committed).

## Breadcrumbs

- Outcomes of the first fix phase: `.planning/phases/14-fix-the-causes/14-FIX-OUTCOMES.md` (library run_fail 205 to 49, not in ledger 0).
- Closed groups by leaf name: fault_static.unsupported_video_mode.vdp (40 rows) and fault_alive.unsupported_video_mode.vdp (6 rows), not emulated by design, reopened by SEED-006; dark_no_fault.never_draws (2 rows) and static_no_fault.frozen (1 row), no evidence, kept failing by the boots check; input_mismatch (0 rows). The boots check is recorded sound.
- Seed candidates: no new leaf under 5 rows printed. Plan 04 marked reg-zm (3 rows) and reg-cm (4 rows) of ranks 3 and 7 as below the 5-row threshold of the decision table (the owner can change that figure); both classes passed once the write fault was dropped.
- Rank 6 (6 rows, 5 at one program counter, all before the first picture) and dark_no_fault.never_draws (2 rows) are the first leaves to trace if the next fix phase wants more than the legacy-mode closure.
- Gap round entry: rank 3 fault_alive.rom_write.cart, open on D-04 (the board choice for the reg-0 carts needs new evidence; 0 rows fail at the head).
- The one pad press on the static_no_fault row was taken at the Phase 14 UAT (`.planning/phases/14-fix-the-causes/14-UAT.md`): one frozen candidate, no picture change under button 1 or button 2, so rank 10 stays closed.
- The Sega bank-shift trace (Phase 14 D-06) stays out. HEAD no longer raises that fault and the runner prints no write values, so the trace needs a baseline build and a manual read. Its residual risk is covered by the Phase 15 run: a pass row the change displaced would have been a refused ratchet row, and the run merged with `ratchet=ok` and no pass row lost. It reopens only if a library run refuses a row.
- Phase 15 library run and merge (`.planning/phases/15-re-run-merge-and-timeouts/15-RUN-EVIDENCE.md`): the page moved from pass 744 and fail 381 to pass 1029 and fail 96, 285 rows flipped from fail to pass, none lost; 50 ntsc fail rows are left (48 with fm off, 2 with fm on), against 49 at the Phase 14 head, and 46 pal rows fail.

