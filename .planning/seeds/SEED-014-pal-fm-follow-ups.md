---
id: SEED-014
status: dormant
planted: 2026-11-11
planted_during: phase 11, plan 09
trigger_when: the next phase's compact page exists, or a reviewer reopens one of the items below, or a release run repeats a CI failure named here
scope: medium
---

# SEED-014: PAL and Japanese FM follow-ups left out of the phase

## Why This Matters

The phase made the ledger carry `tv`, `region` and `fm`, pinned PAL and FM behaviour with owned fixtures and judged
carts through the catalog. Each item below was left out on purpose: no evidence settles it, it touches the release path,
or it adds ceremony for no recurring catch. None of them moves a hash today.

## Per-title TV default and library counts

- A per-title TV default from judged public rows, decided after the next phase has the compact page, so the default
  comes from rows and not from a file name or a title.
- Counts in the library summary for `tv_sensitive` (a cart whose result differs between `ntsc` and `pal`) and
  `fm_audible` (a Japanese cart whose FM stream is not empty), printed as counts only.

## Parser hardening

| Item | What it would do | Cost |
|---|---|---|
| Evidence charset | refuse an evidence cell outside `^[A-Za-z0-9 ._/=:+-]{1,200}$` | a few counted lines |
| Title check | for a 40-hex id, require the title to equal the catalog's | one branch |
| Hostile rows | about twenty-five hostile CSV rows (embedded quotes, CR, tabs, empty cells, huge cells) through the parser | one table in `tests/compat`, 0 counted |
| Planning identifiers | a grep in `scripts/hygiene.sh` that refuses requirement, decision and plan numbers in code, test names and user docs | about 3 counted lines and a throwaway-repository control |

## CI follow-ups

- Parallelise the 54 serial `gh attestation verify` calls in `scripts/publish.sh`: the rehearsal would drop from about
  5.5 to about 2.5 minutes. It touches the release path, so it needs the owner's decision first.
- Fold the `retroarch` job into the `macos-arm64` build leg, so one macOS runner does both.
- Drop `build` from the `wasm32` job's `needs`. It loses the fail-fast economy and it is unknown whether the coupling is
  deliberate.
- The rehearsal's cleanup reads a deleted tag back once (`rehearsal_delete` in `scripts/publish.sh`) and failed once in
  ten pull request runs with "still exists"; a bounded read-after-delete retry would close it, with a control that
  fakes one stale answer. It is left because it touches the release path.
- Not recommended: caching `apt` packages as `.deb` files, and ordering tests by `COST`; neither buys wall time on the
  macOS critical path.

## Machines and standards

- A Mark III machine with the FM unit and its own port rules.
- PAL-M and PAL-N colour standards (SEED-003 holds the measured-accuracy plan).
- A differential against a released binary of another emulator, if one is ever fetched: binary only, its SHA-256
  recorded, run as a released program, and it can only locate wiring errors, not say what the hardware does.

## Open questions without evidence

- Whether a hardware reset clears the YM2413 (the core puts the chip back to its power-on values at a load and at both
  kinds of reset, as `docs/audio.md` says; no held source settles the hardware).
- Whether the `$F2` read counter should follow extra vertical-sync pulses (no known cartridge reads it).
- A real PAL Pause capture: the fixture's placement is derived from the interval, not measured on a console.
- Whether the rule against running two macOS workflows at once guards a runner quota or contention for the same runners;
  no run has shown which.
