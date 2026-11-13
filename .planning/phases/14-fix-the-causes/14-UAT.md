---
status: testing
phase: 14-fix-the-causes
source: [14-VERIFICATION.md]
started: 2026-11-13T05:59:41Z
updated: 2026-11-13T05:59:41Z
---

## Current Test

number: 1
name: Pad press for static_no_fault (and, optionally, the D-06 bank-shift trace)
expected: |
  Pad press: the picture does not change inside the frame cap (rank 10 stays closed).
  Trace: bits 1-0 of the written $FFFC value are clear in the common case (shift left as it is).
  Or record an override accepting both as deferred to Phase 15's confirmation batch.
awaiting: user response

## Tests

### 1. Pad press for static_no_fault (and, optionally, the D-06 bank-shift trace)
expected: the picture does not change inside the frame cap; bits 1-0 of the $FFFC value are clear in the common case. Both need the owner's dump directory, which the agent may not read.
result: [pending]

## Summary

total: 1
passed: 0
issues: 0
pending: 1
skipped: 0
blocked: 0

## Gaps
