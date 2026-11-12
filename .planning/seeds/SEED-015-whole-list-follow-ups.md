---
id: SEED-015
status: dormant
planted: 2026-11-12
planted_during: phase 12, plan 09
trigger_when: the next ledger work, the next CI-speed work, a pin bump that changes a catalog count, or a reviewer reopens one of the items below
scope: medium
---

# SEED-015: whole-list ledger and CI-speed follow-ups left out of the phase

## Why This Matters

The phase gave every Master System and Game Gear catalog identity one default-profile row, a closed reason for every
`unsupported` and `skipped` row, a bounded page and an exit command tied to the libretro flag. Each item below was left
out on purpose: no evidence settles it, it needs a measurement only the owner's hardware can make, or it edits the
release path, and `ci.yml` is edited at most once per phase, in the last push. None of them changes a hash or a row
today. Each says why it is not in the phase and what evidence would decide it.

## Ledger rows

- **PAL and Japanese FM `unknown` rows for every Master System identity.** About 2,100 rows would fit the 8,192 row
  cap, but nothing is behind them: a row with no run says only what the default row already says. The owner's library
  run adds the profiles a real cart reaches. Decided by: a reason to list a profile no run has judged.
- **A catalog `video` column for the 50 Game Gear entries MAME runs in Master System mode.** They are seeded with the
  Game Gear video and may judge `fail`, a profile fact. Needs a measurement of what a real Game Gear shows in Master
  System mode; until then the catalog stays without the column.
- **Real rows for the two `baddump` entries and the three short dumps.** They carry a derived id and a `skipped` reason.
  A real row needs a padding rule, because the loader fills with `0xFF` and a padded run would claim a result for bytes
  nobody dumped. Decided by: a source that says what the missing bytes are.
- **Profile rows from the 16 MAME `usage` hints.** A hint names a controller or mode a title needs. Turning a hint into
  a profile row needs the hint to be trusted per title; decided by the owner's run showing which hints a real cart
  confirms.
- **Per-alias `supported`.** `supported` is read per record but a shared image keeps the worst verdict, so the Game
  Gear twin that MAME marks working is judged `skipped` because its Master System twin is not. Per-alias support would
  let the twin be judged. Decided by: a pinned example where the verdicts differ and the twin runs.
- **A reason for `partial` entries.** Three Game Gear entries are `partial`, runnable and `unknown`. A reason needs
  evidence of what is partial; MAME's attribute does not say.
- **A promotion path for `skipped` rows other than a pin bump.** Today a row leaves `skipped` only when the pin stops
  saying so. A second path (a measured pass on an owned dump) would let the library judge lift one; it conflicts with
  the rule that a status never moves without evidence in the catalog, so it needs its own design.
- **Mark III, PAL-M and PAL-N.** Machine and timing profiles that no row can name yet; they wait on the measured
  accuracy work.
- **Hash-less audio.** An audio predicate that needs no committed digest of our output stays in its own seed.

## Procedures that stay in their own seeds

- The pad and device measurements and the BIOS follow-ups stay in the seeds that hold their commands; the owner runs
  them in the same sitting as the library run when convenient.
- A `scan:` counts section for the library run stays in the threshold seed beyond its counts line.

## Nightly freshness

A nightly step that fetches the three lists and compares the catalog, the pin counts and the cartridge table with the
committed files would catch a hand edit of `skipped` status, which only the pin counts tie today. It edits `ci.yml`, so
it waits for a phase that does not already edit it. Decided by: a recurring hand edit, or a pin bump that went stale.

## CI items

Measured on the phase's hosted run (dispatch of `ci.yml` on the phase branch, run 43153144670, head with the whole-list
ledger): build job on windows-arm64 214 s, be32 293 s, wasm32 55 s, the hygiene lane 38 to 45 s. The build timeout
widened from 6 to 8 minutes by the mechanical rule (2 x 214 s, `sh scripts/ci-durations.sh ci.yml 40`); that was the
only `ci.yml` line the phase edited. Earlier measurements of pull request runs: 627 to 637 s with the release
rehearsal, 302 to 321 s without; be32 and windows-arm64 are the critical path.

The owner's standing instruction: whenever CI runs or finishes, evaluate the low-hanging fruit to make it faster
without hurting quality. Read `sh scripts/ci-durations.sh tests <run-id>` and the job seconds, record any test above
half its timeout, and write the finding here or in a phase note, never only in a chat.

- **The `docs/maintaining.md` "CI speed" rewrite**, with three rules: a release-input edit doubles the run (the
  rehearsal is about six extra minutes, so a pull request that edits `ci.yml`, a release script or a release config
  pays it once more per push); open the pull request once, at the end, because every push to an open pull request
  reruns the matrix; a dispatch compares the ledger with itself (the protected comparison takes its base from the
  dispatched commit), so the base-judge property is proven locally against `origin/main` and recorded as a command and
  an exit status.
- **The duplicate attestation in the publish verify step** (about 120 s of release time). Edits the release path, so it
  waits for a phase that edits `ci.yml` anyway.
- **The dead branch from the first to the second ledger format** in the protected comparison step, made dead by the
  format 2 migration. Removing it edits `ci.yml`; same trigger.
- **Rejected with data, do not reopen without new numbers:** a cost ordering of the be32 tests, dropping the libretro
  host tests, caching, and path-filtering the `edited` filter. The phase's CI review measured each and found no saving
  that kept the checks it removes.

## Where it links

Plan 06 of the phase holds the library judge and its hand-off in SEED-013; SEED-009, SEED-010, SEED-011 and SEED-012
hold the scan, pad, BIOS and audio procedures named above; SEED-003 holds the machine and timing profiles.
