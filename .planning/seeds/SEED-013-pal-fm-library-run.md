---
id: SEED-013
status: done
completed: 2026-11-12 (v0.13.0)
planted: 2026-11-11
planted_during: phase 11, plan 08
revised_during: phase 12, plan 06
trigger_when: the release carrying the whole-list ledger is out and the owner can run the library with their carts
scope: small
audit_acknowledged:
  milestone: v2.0
  at: 2026-11-12
  status: dormant
---

# SEED-013: the owner's own-library run for PAL and Japanese FM rows

## Why This Matters

Agents cannot read the owner's library, so the library judge that flips the seeded rows and adds the PAL and Japanese
FM rows is proven in CI on synthetic images only. Only the owner's carts and consoles can show real PAL, FM, pad,
phaser and BIOS behaviour. This is one batched hand-off after the release, never a gate and never a user acceptance
step; nothing waits for it.

## The request

Run `sh scripts/library.sh run` once with your library folder configured and paste the printed `library-judge` block
and the `exit:` block. Why: only your carts and consoles can show real PAL and FM behaviour and flip the seeded rows; the
command prints counts only and changes only public ledger rows for catalog SHA-1s. It exercises no BIOS: `library.sh run`
and its CMake script never pass `--boot` or `--bios`, and only the window scan (`scripts/hygiene.sh`,
`scripts/firmware-window.sh`) reads the BIOS folder. Optionally run the commands that stay in SEED-010 (pad, Light Phaser
and BIOS judges) in the same sitting.

## What the run does and prints

The ledger holds one default-profile row for every catalog cart, `unknown` until judged. For each cart whose public
SHA-1 is in the catalog the judge runs the default row (and any other row of that id) with its own configuration and
turns it into `pass` or `fail`, then adds the profiles that are missing, derived from the catalog machine and the
cartridge header region, never from a file name or title: export PAL for a Master System export header, Japan NTSC
with FM on for a Japan header, one row for a Game Gear with its table region. A profile the ledger already holds as a
row of that cart is not run twice. A cart with a light phaser, paddle or sports port is never given a pad row, and a
`skipped` or `unsupported` row keeps its status and reason whatever the run observes. Each added profile runs
`boots@600` under the default configuration. A `pal` row means only that the cart ran under PAL timing. The block holds
`library-judge 1`, a `pal:` line, an `fm:` line and a `merge:` line, each with counts only; a section with no input
prints `skipped: not configured`.

## Why the merge can go through now

The page is bounded by construction (a fixed number of listed rows per section, the rest counted), so the rows the run
adds fit it and the run merges them into the tracked ledger and page. `merge: held` is only a runaway guard: it prints
when the page would pass the docs cap anyway or the ratchet refuses the rows, and it leaves both files untouched. The
run is the owner's one batched hand-off after the release. It is never a gate, never a release hold and never a user
acceptance step.

## The flag leaves with the rows

The owner's 2026-11-10 rule stands: `is_experimental` leaves the libretro core after this run, and nobody has to
remember it. The `exit:` block ends with a `judged_cart_floors=` verdict (20 judged cart passes, one PAL row and one
Japanese FM row). When it says `judged_cart_floors=met`, the pull request that merges the run's rows must also:

- delete `is_experimental = "true"` from the libretro `.info` template;
- drop "Experimental. " from the `.info` description and from the generated page intro, and delete the page's second
  sentence "Experimental leaves only on the own-library evidence ..." (both live in `scripts/compat-common.sh`;
  regenerate `docs/compatibility.md` with `sh scripts/compat-common.sh docs tests/compat/ledger.csv`);
- rewrite the README Status paragraph without "experimental" and without "most" (a title with no checked result is
  listed unknown);
- delete the experimental sentences in `docs/conformance.md` (the lines naming `is_experimental`, including the
  exit-command paragraph), `docs/saves.md` and `docs/embedding.md`, and reword the `policy.claims_scoped` table row in
  `docs/testing.md`, which also contains the word: in the flag-absent state the claims test reads the README, every
  `docs/*.md`, the `.info` template and the page, and fails on any "experimental".

Only then do the exit command and the claims test pass; the repository's exemption for the flag in the `.info` claims
check is not edited. When the block says `unmet`, the flag stays and the block shows the short floor. A red exit
command on that pull request is the tie or a triage item (an `infrastructure_error` row, an empty reason), never a
release hold.

## What stays elsewhere

The pad, Light Phaser and BIOS judges stay in SEED-010, which holds their commands; SEED-011 holds BIOS follow-ups, not
commands. None of them is part of this command. The optional `scan:` counts of SEED-009 are not part of it either.

## What the agent reads

Only the pasted blocks and the ledger and page diff, never `build/library/results/*.path`, `.probe` or `.error`. A row
for a catalog SHA-1 is the identity the asset policy already lets leave the machine. No digest, path or title of the
library is requested or recorded.

## Where it links

SEED-010 batches its pad and device measurements with this run. The exit command and its tie to the flag are built:
`compat.exit` reads the committed ledger and the `.info` template and fails in both directions, so the merge pull
request is red until the flag and its wording leave together with the rows.
