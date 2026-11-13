---
status: complete
phase: 14-fix-the-causes
source: [14-VERIFICATION.md]
started: 2026-11-13T05:59:41Z
updated: 2026-11-13T15:53:58.786Z
---

## Current Test

[testing complete]

## Tests

### 1. Pad press for static_no_fault (and, optionally, the D-06 bank-shift trace)
expected: the picture does not change inside the frame cap; bits 1-0 of the $FFFC value are clear in the common case.
result: pass
source: automated
note: Pad press only. A scratch script run by the owner over their dump directory (counts-only output) replayed the one frozen no-fault cart (frozen_candidates=1, matching the ledger's single static_no_fault row) with a 40-frame press of button 1 and, separately, button 2, in a 1800-frame run, and compared each against an all-released input run of the same length (button1_changed_picture=0, button2_changed_picture=0), so rank 10 stays closed. A first run compared against a no-input run and read 6 of 6 changed; the control showed that a run with an input file differs from one without it, so that comparison was an artifact and is not used. The D-06 bank-shift trace is optional in the verification report and was not taken (it needs a baseline build and a manual read of the written value; the runner prints no write values and HEAD raises no fault there); its residual risk stays as plan 03's SUMMARY states it.

## Summary

total: 1
passed: 1
issues: 0
pending: 0
skipped: 0
blocked: 0

## Gaps
