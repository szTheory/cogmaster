# Phase 13: Why carts fail - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md — this log preserves the alternatives considered.

**Date:** 2026-11-12
**Phase:** 13-why-carts-fail
**Areas discussed:** Diagnostic surface and plumbing, Leak-proof output contract, Cause taxonomy and the note, Run protocol and release

Selection note: the owner's pasted fan-out boilerplate asks for subagent fan-out over every decision point and one coherent
set of recommendations that is followed, so all four areas were researched and no per-area question was put to the owner.

---

## Diagnostic surface and plumbing

| Option | Description | Selected |
|--------|-------------|----------|
| A. No runner change | `library.sh diagnose` runs `--probe` and one `--expect` run with a ladder of `boots@F` records, parsed by regex | |
| B. New runner mode | One closed-vocabulary line with exact first-display and first-non-blank frames, picture count and an extended cap | ✓ |

**Choice:** B, reconciled by the orchestrator. The plumbing advisor recommended A and named B as its tripwire; the leak
advisor recommended a runner line; the taxonomy advisor's evidence (first non-blank frame, picture count, 1800-frame view)
cannot come from a ladder, and the roadmap itself asks for "first non-blank sample".
**Notes:** the `diagnose` branch of `library.sh` must exit before the merge block; `cmake --workflow --preset library` rewrites the tracked ledger and page.

---

## Leak-proof output contract

| Option | Description | Selected |
|--------|-------------|----------|
| A. Parse-and-reconstruct | Closed grammar, validator prints only validated fields, private scratch removed by a trap, canary test with negative controls | ✓ |
| B. Filter free text | Keep the judge's per-cart files and redact path, name and hex patterns | |

**Choice:** A. Blocklist redaction misses unforeseen formats and `.observation` already holds a frame digest.
**Notes:** the test is a separate `library.diagnose` (plumbing and leak advisors) rather than an extension of `library.synthetic` (run-protocol advisor), to keep the Windows ARM64 timeout margin.

---

## Cause taxonomy and the note

| Option | Description | Selected |
|--------|-------------|----------|
| A. Truth-table classes of the boots check, then fault kind x device, then coarse PC buckets | Disjoint and exhaustive by construction; fixed before the data | ✓ |
| B. Crash-state buckets (fault kind, device, exact PC) | ClusterFuzz style; the no-fault rows fall into one catch-all | |

**Choice:** A, with exact PC kept only as a sub-row when 5 or more rows share it.
**Notes:** counts-only note at `.planning/phases/13-why-carts-fail/13-CAUSE-GROUPS.md`; no cumulative share and no flip projection; leaves are never merged or re-split after seeing counts.

---

## Run protocol and release

| Option | Description | Selected |
|--------|-------------|----------|
| A. `diagnose` mode in existing scripts, proved in CI by a synthetic test | Counted check code unchanged; agent runs once on the real library | ✓ |
| B. New script plus new test | Isolates the timeout, adds classification and inventory rows | |

**Choice:** A for the scripts; the test is its own file (see above).
**Notes:** release title is a `feat`, because `docs`, `test` and `chore` cut no release; owner supplies `COGMASTER_ROM_DIR` in the launching shell.

---

## Claude's Discretion

Field order inside the grammar, whether RAM liveness ships, whether `library.diagnose` later folds into `library.synthetic`, leaf names beyond those listed, the runner flag's spelling.

## Deferred Ideas

Start-press scripted rerun (Phase 14), per-instruction trace (Phase 14), PAL and Japanese FM diagnostic, CI job for the real library, counted ledger-unchanged gate, library fingerprint, attempt quotas.
