---
id: SEED-013
status: dormant
planted: 2026-11-11
planted_during: phase 11, plan 08
trigger_when: the compact compatibility page exists (the next phase) and the owner can run the library with their carts
scope: small
---

# SEED-013: the owner's own-library run for PAL and Japanese FM rows

## Why This Matters

Agents cannot read the owner's library, so the library judge that adds PAL and Japanese FM rows is proven in CI on
synthetic images only. Only the owner's carts and consoles can show real PAL, FM, pad, phaser and BIOS behaviour.
This is one batched milestone-end request, never a gate and never a user acceptance step.

## The request

Run `sh scripts/library.sh run` once with your library and BIOS folders configured and paste the printed
`library-judge` block. Why: only your carts and consoles can show real PAL, FM, pad, phaser and BIOS behaviour; the
command prints counts only and changes only public ledger rows.

## What the run does and prints

For each cart whose public SHA-1 is in the catalog and has no ledger row, the judge derives its profiles from the
catalog machine and the cartridge header region, never from a file name or title: export NTSC and export PAL for a
Master System export header, Japan NTSC with FM on for a Japan header, one row for a Game Gear with its table
region. Each profile runs `boots@600` under the default configuration and adds a `pass` or `fail` row. A `pal`
row means only that the cart ran under PAL timing. The block holds `library-judge 1`, a `pal:` line, an `fm:` line and
a `merge:` line, each with counts only; a section with no input prints `skipped: not configured`.

## Why the real merge waits

A real library adds about two rows per export cart, more than the cap of today's full listing, and a larger page fails
`policy.compat_doc_fresh` and the docs cap. The run therefore holds the merge (`merge: held`) and leaves the tracked
ledger and page untouched until the page can take the rows. The next phase's page budget and generator make room; its
closing plan appends the sections of the other library runs (SEED-009, SEED-010 and SEED-011) to the same command.

## What the agent reads

Only the pasted block and the ledger and page diff, never `build/library/results/*.path`, `.probe` or `.error`. A row
for a catalog SHA-1 is the identity the asset policy already lets leave the machine. No digest, path or title of the
library is requested or recorded.

## Where it links

SEED-010 batches its pad and device measurements with this run. The next phase's closing criterion for the exit
command includes the PAL and Japanese FM rows of the run recorded here.
