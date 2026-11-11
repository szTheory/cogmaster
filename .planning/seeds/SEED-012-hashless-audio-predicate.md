---
id: SEED-012
status: dormant
planted: 2026-11-11
planted_during: phase 11, plan 09
trigger_when: the milestone-end run of the owner's library (SEED-013) shows enough Japanese FM carts with no pinned golden, or a reviewer asks for a sound-based row that needs no golden
scope: medium
---

# SEED-012: A sound row for an FM cartridge with no golden (hash-less `audio@F`)

## Why This Matters

`audio@F=H` (the transition stream of the chips, hashed) needs a golden hash, and a golden for a commercial cartridge
can only come from running that cartridge, which agents never do. A hash-less form would let the judge say "the FM unit
was driven" for a Japanese cartridge that has no pinned sound, and would catch a cartridge whose FM never starts. It was
left out of the phase on purpose; this note keeps its design so a decision can use real numbers.

## The design

- A predicate such as `audio@F` with no `=H`, judged on a `script none` run only, so a title that waits for Start is
  `unknown` anyway.
- The runner already records one transition per chip write with its source number (5 is the FM unit). The predicate
  passes when the record holds at least 8 source-5 records with a tick above 0, at least 2 distinct level values and a
  peak above 0 (source 5 is the FM unit's output sum, so a driven unit moves it); otherwise it fails.
- A pass claims only that the unit was driven, never that it sounds right or that the title is playable.

## Why it is out

- It is not in the text of the requirement the phase closed.
- The thresholds would be tuned blind: agents cannot read the library, and the only evidence is the self-authored
  `fm_sweep` fixture, which is built to be loud.
- It needs a new accumulator in the runner, a parse branch in the ledger parser, a per-kind cap on the page and a
  probe-only control fixture (an image that reads `$F2` and writes nothing, which must fail the predicate).
- A title that waits for a keypress at the start screen would be `unknown` under `script none`, so the row would add
  little for the carts that matter most.

## What number would decide it

The milestone-end run of the owner's library (SEED-013) prints, per Japanese or FM-capable cartridge, whether the run
produced any source-5 record. If the share of such carts that are `unknown` for a missing golden is large (for example
more than a third) and their record counts sit clearly apart from the control's (a cartridge that never writes the unit
shows 0), the thresholds above can be set from those counts instead of from one fixture, and the predicate is worth
its three counted lines and its fixture. If most carts show records on every run or none on any run, it adds nothing.
