---
id: SEED-014
status: dormant
planted: 2026-11-11
planted_during: phase 11, plan 09
trigger_when: the next phase's compact page exists, or a reviewer reopens one of the items below, or a release run repeats a CI failure named here
scope: medium
audit_acknowledged:
  milestone: v2.0
  at: 2026-11-12
  status: dormant
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

## Phase 11 review leftovers

Left out of review fix round 1 on purpose; each is a seed, none moves a hash today.

Owner decision (admin settings, not agent work):

- The committed ruleset requires no approving review (`required_approving_review_count` is 0, no code owner review, no
  CODEOWNERS file) and its one required check is a job of the `ci.yml` a pull request can edit. `docs/testing.md` now says
  the protected comparison guards against mistakes and regressions, not against an author who can edit `ci.yml`. Whether to
  add CODEOWNERS for `.github/workflows/**`, `scripts/compat-*.sh` and `tests/compat/**` with required code owner review is
  the owner's call.

Blind spots of the fixtures, by design:

- `vdp_vtable` is rotation invariant: it sees the multiset, the descents, the maximum and the minimum of the V counter, so
  a table that is correct but shifted against the frame passes. Only the Pause V record and the `vdp_lines` boundary rows
  are phase evidence on PAL.
- `pause_nmi` reads V about 350 ticks after the NMI, so a V step tick error smaller than that, a sub-120-cycle NMI tick
  error, or which frame the NMI lands in are not seen.

Runner and fixture half, informational:

- The six `sms1` expectation rows in `tests/diag/expected.csv` cite the 224-line section for a 192-line fallback the core
  selects; cite the model-selection section and say "selected fallback, not measured".
- `--probe` rejects `--fm` but still accepts `--machine`, `--region`, `--video-mode`, `--port1` and `--port2`, which also
  change nothing under `--probe`; extend the list in one pass with a control each, or document it.

Ledger, library judge and CI half, informational:

- `library.sh run` with no `COGMASTER_ROM_DIR` aborts with an unbound variable instead of printing "skipped: not configured",
  and an empty value scans `/*`.
- A candidate's date is kept for any valid day, including a future one; pass today's date in and refuse a later day.
- `library.sh` copies the ledger and the page one after the other; copy both to temporaries and move them once both
  writes succeeded, and skip the facts of an unchanged status so a no-change run does not dirty the tree.
- The be32 job limit of 15 minutes rests on one apt stall; a `timeout-minutes: 3` on the apt step would fail a stall fast.
- The ledger header literal lives in five places (`compat-common.sh`, `library.sh`, `ci.yml` twice, the tests) and the
  `base_rev` line is duplicated between the bridge's two branches; delete the bridge in the first plan of the next phase.
- `hygiene.tree.header_offset` plants its marker at one of the three offsets the scan reads, and `hygiene.analyzer` has no
  control against a pipeline that swallows `xargs`'s status.
- The ledger migration test and the `compat-common.sh` comment claim an equivalence for a `sg` machine row that no core
  machine has; say it is inert.
- `rehearsal_delete` counts any `gh api` failure (a 5xx, an expired token) as "gone"; accepting only an error text with
  `HTTP 404` and retrying otherwise would close that.
- The timeout of `compat.public_update` and `library.synthetic` is dominated by about 55 `execute_process(sh ...)` calls
  that each run two `git rev-parse` and an awk; batching the single-row `update()` cases of `update.cmake` into the "all
  together" call, and one `git rev-parse` per `compat-update.sh` call, would cut it.

Closed in fix round 1 (recorded here because the seed above lists it): the rehearsal cleanup's read-after-delete race now
reads up to six times and re-issues the delete (`gone` in `scripts/publish.sh`, `policy.publish_delete`).
